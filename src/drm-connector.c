#include "drm-connector.h"

// clang-format off
struct zippo_drm_property_enum_info dpms_state_enums[] = {
  [ZIPPO_DRM_DPMS_STATE_OFF] = { .name = "Off" },
  [ZIPPO_DRM_DPMS_STATE_ON] = { .name = "On" },
  [ZIPPO_DRM_DPMS_STATE_STANDBY] = { .name = "Standby" },
  [ZIPPO_DRM_DPMS_STATE_SUSPEND] = { .name = "Suspend" },
};

struct zippo_drm_property_enum_info content_protection_enums[] = {
  [ZIPPO_DRM_CONTENT_PROTECTION_UNDESIRED] = { .name = "Undesired" },
  [ZIPPO_DRM_CONTENT_PROTECTION_DESIRED] = { .name = "Desired" },
  [ZIPPO_DRM_CONTENT_PROTECTION_ENABLED] = { .name = "Enabled" },
};

struct zippo_drm_property_enum_info hdcp_content_type_enums[] = {
  [ZIPPO_DRM_HDCP_CONTENT_TYPE0] = { .name = "HDCP Type0" },
  [ZIPPO_DRM_HDCP_CONTENT_TYPE1] = { .name = "HDCP Type1" },
};

struct zippo_drm_property_enum_info panel_orientation_enums[] = {
  [ZIPPO_DRM_PANEL_ORIENTATION_NORMAL] = { .name = "Normal", },
  [ZIPPO_DRM_PANEL_ORIENTATION_UPSIDE_DOWN] = { .name = "Upside Down", },
  [ZIPPO_DRM_PANEL_ORIENTATION_LEFT_SIDE_UP] = { .name = "Left Side Up", },
  [ZIPPO_DRM_PANEL_ORIENTATION_RIGHT_SIDE_UP] = { .name = "Right Side Up", },
};

const struct zippo_drm_property_info connector_props[] = {
  [ZIPPO_DRM_CONNECTOR_EDID] = { .name = "EDID" },
  [ZIPPO_DRM_CONNECTOR_DPMS] = {
    .name = "DPMS",
    .enum_values = dpms_state_enums,
    .num_enum_values = ZIPPO_DRM_DPMS_STATE__COUNT,
  },
  [ZIPPO_DRM_CONNECTOR_CRTC_ID] = { .name = "CRTC_ID", },
  [ZIPPO_DRM_CONNECTOR_NON_DESKTOP] = { .name = "non-desktop", },
  [ZIPPO_DRM_CONNECTOR_CONTENT_PROTECTION] = {
    .name = "Content Protection",
    .enum_values = content_protection_enums,
    .num_enum_values = ZIPPO_DRM_CONTENT_PROTECTION__COUNT,
  },
  [ZIPPO_DRM_CONNECTOR_HDCP_CONTENT_TYPE] = {
    .name = "HDCP Content Type",
    .enum_values = hdcp_content_type_enums,
    .num_enum_values = ZIPPO_DRM_HDCP_CONTENT_TYPE__COUNT,
  },
  [ZIPPO_DRM_CONNECTOR_PANEL_ORIENTATION] = {
    .name = "panel orientation",
    .enum_values = panel_orientation_enums,
    .num_enum_values = ZIPPO_DRM_PANEL_ORIENTATION__COUNT,
  },
};
// clang-format on

static int
zippo_drm_connector_update_properties(struct zippo_drm_connector *self)
{
  drmModeObjectProperties *props;

  props = drmModeObjectGetProperties(
      self->backend->drm.fd, self->connector_id, DRM_MODE_OBJECT_CONNECTOR);

  if (!props) {
    fprintf(stderr, "Error: Failed to get connector properties\n");
    return -1;
  }

  if (self->props_drm) drmModeFreeObjectProperties(self->props_drm);

  self->props_drm = props;

  return 0;
}

int
zippo_drm_connector_assign_connector_info(
    struct zippo_drm_connector *self, drmModeConnector *conn)
{
  if (zippo_drm_connector_update_properties(self) < 0) return -1;

  if (self->conn) drmModeFreeConnector(self->conn);

  self->conn = conn;

  drm_property_info_free(self->props, ZIPPO_DRM_CONNECTOR__COUNT);
  drm_property_info_populate(self->backend, connector_props, self->props,
      ZIPPO_DRM_CONNECTOR__COUNT, self->props_drm);

  return 0;
}

void
zippo_drm_connector_init(struct zippo_drm_connector *self,
    struct zippo_backend_drm *backend, uint32_t connector_id)
{
  self->backend = backend;
  self->connector_id = connector_id;
  self->conn = NULL;
  self->props_drm = NULL;
}

void
zippo_drm_connector_fini(struct zippo_drm_connector *self)
{
  drmModeFreeConnector(self->conn);
  drmModeFreeObjectProperties(self->props_drm);
  drm_property_info_free(self->props, ZIPPO_DRM_CONNECTOR__COUNT);
}
