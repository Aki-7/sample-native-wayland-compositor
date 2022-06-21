#include "kms.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "drm-crtc.h"

uint64_t
drm_property_get_value(struct zippo_drm_property_info *info,
    const drmModeObjectProperties *props, uint64_t def)
{
  unsigned int i;

  if (info->prop_id == 0) return def;

  for (i = 0; i < props->count_props; i++) {
    unsigned int j;

    if (props->props[i] != info->prop_id) {
      continue;
    }

    if (info->num_enum_values == 0) return props->prop_values[i];

    for (j = 0; j < info->num_enum_values; j++) {
      if (!info->enum_values[j].valid) continue;

      if (info->enum_values[j].valid != props->prop_values[i]) continue;

      return j;
    }

    break;
  }

  return def;
}

uint64_t *
drm_property_get_range_values(
    struct zippo_drm_property_info *info, const drmModeObjectProperties *props)
{
  unsigned int i;

  if (info->prop_id == 0) return NULL;

  for (i = 0; i < props->count_props; i++) {
    if (props->props[i] != info->prop_id) continue;

    if (!(info->flags & DRM_MODE_PROP_RANGE) &&
        !(info->flags & DRM_MODE_PROP_SIGNED_RANGE))
      continue;

    return info->range_values;
  }

  return NULL;
}

void
drm_property_info_populate(struct zippo_backend_drm *backend,
    const struct zippo_drm_property_info *src,
    struct zippo_drm_property_info *info, unsigned int num_infos,
    drmModeObjectProperties *props)
{
  drmModePropertyRes *prop;
  unsigned int i, j;

  for (i = 0; i < num_infos; i++) {
    unsigned int k;

    info[i].name = src[i].name;
    info[i].prop_id = 0;
    info[i].num_enum_values = src[i].num_enum_values;

    if (src[i].num_enum_values == 0) continue;

    info[i].enum_values =
        malloc(src[i].num_enum_values * sizeof(*info[i].enum_values));

    for (k = 0; k < info[i].num_enum_values; k++) {
      info[i].enum_values[k].name = src[i].enum_values[k].name;
      info[i].enum_values[k].valid = false;
    }
  }

  for (i = 0; i < props->count_props; i++) {
    unsigned int k;

    prop = drmModeGetProperty(backend->drm.fd, props->props[i]);
    if (!prop) continue;

    for (j = 0; j < num_infos; j++) {
      if (!strcmp(prop->name, info[j].name)) break;
    }

    // not found, we don't know/care about this property;
    if (j == num_infos) {
      fprintf(stderr, "DRM warning:  unrecognized property %u '%s'\n",
          prop->prop_id, prop->name);
      drmModeFreeProperty(prop);
      continue;
    }

    info[j].prop_id = props->props[i];
    info[j].flags = prop->flags;

    if (prop->flags & DRM_MODE_PROP_RANGE ||
        prop->flags & DRM_MODE_PROP_SIGNED_RANGE) {
      info[j].num_range_values = prop->count_values;
      for (int i = 0; i < prop->count_values; i++)
        info[j].range_values[i] = prop->values[i];
    }

    if (info[j].num_enum_values == 0) {
      drmModeFreeProperty(prop);
      continue;
    }

    if (!(prop->flags & DRM_MODE_PROP_ENUM)) {
      fprintf(stderr,
          "DRM: expected property %s to be an enum, but it is not; ignoring\n",
          prop->name);
      drmModeFreeProperty(prop);
      continue;
    }

    for (k = 0; k < info[j].num_enum_values; k++) {
      int l;
      for (l = 0; l < prop->count_enums; l++) {
        if (!strcmp(prop->enums[l].name, info[j].enum_values[k].name)) break;
      }

      if (l == prop->count_enums) continue;

      info[j].enum_values[k].valid = true;
      info[j].enum_values[k].value = prop->enums[l].value;
    }

    drmModeFreeProperty(prop);
  }

  for (i = 0; i < num_infos; i++) {
    if (info[i].prop_id == 0)
      fprintf(stderr, "DRM warning: property '%s' missing\n", info[i].name);
  }
}

void
drm_property_info_free(struct zippo_drm_property_info *info, int num_props)
{
  int i;

  for (i = 0; i < num_props; i++) {
    free(info[i].enum_values);
  }

  memset(info, 0, sizeof(*info) * num_props);
}

int
init_kms_caps(struct zippo_backend_drm *backend)
{
  uint64_t cap;
  int ret;

  fprintf(stderr, "using %s\n", backend->drm.filename);

  ret = drmGetCap(backend->drm.fd, DRM_CAP_TIMESTAMP_MONOTONIC, &cap);
  if (ret != 0 || cap != 1) {
    fprintf(stderr,
        "Error: kernel DRM KMS does not support DRM_CAP_TIMESTAMP_MONOTONIC\n");
    return -1;
  }

  // set presentation clock

  ret = drmGetCap(backend->drm.fd, DRM_CAP_CURSOR_WIDTH, &cap);
  if (ret == 0)
    backend->cursor_width = cap;
  else
    backend->cursor_width = 64;

  ret = drmGetCap(backend->drm.fd, DRM_CAP_CURSOR_HEIGHT, &cap);
  if (ret == 0)
    backend->cursor_height = cap;
  else
    backend->cursor_height = 64;

  ret = drmSetClientCap(backend->drm.fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
  if (ret) {
    fprintf(stderr, "Error: drm card doesn't support universal planes!\n");
    return -1;
  }

  ret = drmGetCap(backend->drm.fd, DRM_CAP_CRTC_IN_VBLANK_EVENT, &cap);
  if (ret != 0) cap = 0;
  ret = drmSetClientCap(backend->drm.fd, DRM_CLIENT_CAP_ATOMIC, 1);
  backend->atomic_modeset = ((ret == 0) && (cap == 1));

  fprintf(stderr, "DRM: %s atomic modesetting\n",
      backend->atomic_modeset ? "supports" : "does not support");

  ret = drmGetCap(backend->drm.fd, DRM_CAP_ADDFB2_MODIFIERS, &cap);
  if (ret == 0) backend->fb_modifiers = cap;
  fprintf(stderr, "DRM: %s GBM modifiers\n",
      backend->fb_modifiers ? "supports" : "does not support");

  drmSetClientCap(backend->drm.fd, DRM_CLIENT_CAP_WRITEBACK_CONNECTORS, 1);

  if (!backend->atomic_modeset) backend->sprites_are_broken = true;

  ret = drmSetClientCap(backend->drm.fd, DRM_CLIENT_CAP_ASPECT_RATIO, 1);
  backend->aspect_ratio_supported = (ret == 0);
  fprintf(stderr, "DRM: %s picture aspect ratio\n",
      backend->aspect_ratio_supported ? "supports" : "does not support");

  return 0;
}
