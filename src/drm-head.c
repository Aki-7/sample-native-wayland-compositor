#define _GNU_SOURCE
#include "drm-head.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "drm-connector.h"

static const char *const connector_type_names[] = {
    [DRM_MODE_CONNECTOR_Unknown] = "Unknown",
    [DRM_MODE_CONNECTOR_VGA] = "VGA",
    [DRM_MODE_CONNECTOR_DVII] = "DVI-I",
    [DRM_MODE_CONNECTOR_DVID] = "DVI-D",
    [DRM_MODE_CONNECTOR_DVIA] = "DVI-A",
    [DRM_MODE_CONNECTOR_Composite] = "Composite",
    [DRM_MODE_CONNECTOR_SVIDEO] = "SVIDEO",
    [DRM_MODE_CONNECTOR_LVDS] = "LVDS",
    [DRM_MODE_CONNECTOR_Component] = "Component",
    [DRM_MODE_CONNECTOR_9PinDIN] = "DIN",
    [DRM_MODE_CONNECTOR_DisplayPort] = "DP",
    [DRM_MODE_CONNECTOR_HDMIA] = "HDMI-A",
    [DRM_MODE_CONNECTOR_HDMIB] = "HDMI-B",
    [DRM_MODE_CONNECTOR_TV] = "TV",
    [DRM_MODE_CONNECTOR_eDP] = "eDP",
    [DRM_MODE_CONNECTOR_VIRTUAL] = "Virtual",
    [DRM_MODE_CONNECTOR_DSI] = "DSI",
    [DRM_MODE_CONNECTOR_DPI] = "DPI",
};

#define EDID_DESCRIPTOR_ALPHANUMERIC_DATA_STRING 0xfe
#define EDID_DESCRIPTOR_DISPLAY_PRODUCT_NAME 0xfc
#define EDID_DESCRIPTOR_DISPLAY_PRODUCT_SERIAL_NUMBER 0xff
#define EDID_OFFSET_DATA_BLOCKS 0x36
#define EDID_OFFSET_LAST_BLOCK 0x6c
#define EDID_OFFSET_PNPID 0x08
#define EDID_OFFSET_SERIAL 0x0c

static void
edid_parse_string(const uint8_t *data, char text[])
{
  int i;
  int replaced = 0;

  /* this is always 12 bytes, but we can't guarantee it's null
   * terminated or not junk. */
  strncpy(text, (const char *)data, 12);

  /* guarantee our new string is null-terminated */
  text[12] = '\0';

  /* remove insane chars */
  for (i = 0; text[i] != '\0'; i++) {
    if (text[i] == '\n' || text[i] == '\r') {
      text[i] = '\0';
      break;
    }
  }

  /* ensure string is printable */
  for (i = 0; text[i] != '\0'; i++) {
    if (!isprint(text[i])) {
      text[i] = '-';
      replaced++;
    }
  }

  /* if the string is random junk, ignore the string */
  if (replaced > 4) text[0] = '\0';
}

static int
edid_parse(struct zippo_drm_edid *edid, const uint8_t *data, size_t length)
{
  int i;
  uint32_t serial_number;

  if (length < 128 || data[0] != 0x00 || data[1] != 0xff) return -1;

  /* decode the PNP ID from three 5 bit words packed into 2 bytes
   * /--08--\/--09--\
   * 7654321076543210
   * |\---/\---/\---/
   * R  C1   C2   C3 */
  edid->pnp_id[0] = 'A' + ((data[EDID_OFFSET_PNPID + 0] & 0x7c) / 4) - 1;
  edid->pnp_id[1] = 'A' + ((data[EDID_OFFSET_PNPID + 0] & 0x3) * 8) +
                    ((data[EDID_OFFSET_PNPID + 1] & 0xe0) / 32) - 1;
  edid->pnp_id[2] = 'A' + (data[EDID_OFFSET_PNPID + 1] & 0x1f) - 1;
  edid->pnp_id[3] = '\0';

  /* maybe there isn't a ASCII serial number descriptor, so use this instead */
  serial_number = (uint32_t)data[EDID_OFFSET_SERIAL + 0];
  serial_number += (uint32_t)data[EDID_OFFSET_SERIAL + 1] * 0x100;
  serial_number += (uint32_t)data[EDID_OFFSET_SERIAL + 2] * 0x10000;
  serial_number += (uint32_t)data[EDID_OFFSET_SERIAL + 3] * 0x1000000;
  if (serial_number > 0)
    sprintf(edid->serial_number, "%lu", (unsigned long)serial_number);

  /* parse EDID data */
  for (i = EDID_OFFSET_DATA_BLOCKS; i <= EDID_OFFSET_LAST_BLOCK; i += 18) {
    /* ignore pixel clock data */
    if (data[i] != 0) continue;
    if (data[i + 2] != 0) continue;

    /* any useful blocks? */
    if (data[i + 3] == EDID_DESCRIPTOR_DISPLAY_PRODUCT_NAME) {
      edid_parse_string(&data[i + 5], edid->monitor_name);
    } else if (data[i + 3] == EDID_DESCRIPTOR_DISPLAY_PRODUCT_SERIAL_NUMBER) {
      edid_parse_string(&data[i + 5], edid->serial_number);
    } else if (data[i + 3] == EDID_DESCRIPTOR_ALPHANUMERIC_DATA_STRING) {
      edid_parse_string(&data[i + 5], edid->eisa_id);
    }
  }
  return 0;
}

static void
zippo_drm_head_find_and_parse_output_edid(struct zippo_drm_head *self,
    drmModeObjectProperties *props, const char **make, const char **model,
    const char **serial_number)
{
  drmModePropertyBlobPtr edid_blob = NULL;
  uint32_t blob_id;
  int rc;

  blob_id = drm_property_get_value(
      &self->connector.props[ZIPPO_DRM_CONNECTOR_EDID], props, 0);

  if (!blob_id) return;

  edid_blob = drmModeGetPropertyBlob(self->backend->drm.fd, blob_id);
  if (!edid_blob) return;

  rc = edid_parse(&self->edid, edid_blob->data, edid_blob->length);

  if (!rc) {
    if (self->edid.pnp_id[0] != '\0') *make = self->edid.pnp_id;
    if (self->edid.monitor_name[0] != '\0') *model = self->edid.monitor_name;
    if (self->edid.serial_number[0] != '\0')
      *serial_number = self->edid.serial_number;
  }
  drmModeFreePropertyBlob(edid_blob);
}

static bool
check_non_desktop(
    struct zippo_drm_connector *connector, drmModeObjectPropertiesPtr props)
{
  struct zippo_drm_property_info *non_desktop_info =
      &connector->props[ZIPPO_DRM_CONNECTOR_NON_DESKTOP];

  return drm_property_get_value(non_desktop_info, props, 0);
}

static int
drm_subpixel_to_wayland(int drm_value)
{
  return drm_value;  // FIXME:
}

static void
zippo_drm_head_update_head_from_connector(struct zippo_drm_head *self)
{
  struct zippo_drm_connector *connector = &self->connector;
  drmModeObjectProperties *props = connector->props_drm;
  drmModeConnector *conn = connector->conn;
  const char *make = "unknown";
  const char *model = "unknown";
  const char *serial_number = "unknown";

  zippo_drm_head_find_and_parse_output_edid(
      self, props, &make, &model, &serial_number);

  zippo_head_set_monitor_string(&self->base, make, model, serial_number);
  zippo_head_set_subpixel(&self->base, drm_subpixel_to_wayland(conn->subpixel));

  zippo_head_set_connection_status(
      &self->base, conn->connection == DRM_MODE_CONNECTED);
}

static char *
make_connector_name(const drmModeConnector *conn)
{
  char *name;
  const char *type_name = NULL;
  int ret;

  if (conn->connector_type <
      sizeof(connector_type_names) / sizeof(connector_type_names[0]))
    type_name = connector_type_names[conn->connector_type];

  if (!type_name) type_name = "UNNAMED";

  ret = asprintf(&name, "%s-%d", type_name, conn->connector_type_id);
  if (ret < 0) return NULL;

  return name;
}

static int
zippo_drm_head_update_info(struct zippo_drm_head *self, drmModeConnector *conn)
{
  int ret;

  ret = zippo_drm_connector_assign_connector_info(&self->connector, conn);

  zippo_drm_head_update_head_from_connector(self);

  return ret;
}

int
zippo_drm_head_create(struct zippo_backend_drm *backend, drmModeConnector *conn,
    struct udev_device *drm_device)
{
  struct zippo_drm_head *self;
  char *name;
  int ret;
  (void)drm_device;

  self = calloc(1, sizeof *self);
  if (self == NULL) return -1;

  self->backend = backend;

  zippo_drm_connector_init(&self->connector, backend, conn->connector_id);

  name = make_connector_name(conn);
  if (!name) goto err;

  zippo_head_init(&self->base, name);
  free(name);

  ret = zippo_drm_head_update_info(self, conn);
  if (ret < 0) goto err_update;

  fprintf(stderr, "%s: %s, %s, %s, (%s - %s)\n", self->base.name,
      self->base.make, self->base.model, self->base.serial_number,
      check_non_desktop(&self->connector, self->connector.props_drm)
          ? "non desktop"
          : "dekstop",
      self->base.connected ? "connected" : "non connected");

  // Next to do: start from here!!!
  // backlight can be skipped

  return 0;

err_update:
  zippo_head_fini(&self->base);

err:
  zippo_drm_connector_fini(&self->connector);
  free(self);
  return -1;
}

void
zippo_drm_head_destroy(struct zippo_drm_head *self)
{
  zippo_head_fini(&self->base);
  zippo_drm_connector_fini(&self->connector);
  free(self);
}
