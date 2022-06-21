#ifndef ZIPPO_KMS_H
#define ZIPPO_KMS_H

#include <xf86drm.h>
#include <xf86drmMode.h>

#include "backend-drm.h"

struct zippo_drm_property_enum_info {
  const char *name;
  bool valid;
  uint64_t value;
};

struct zippo_drm_property_info {
  const char *name;
  uint32_t prop_id;
  uint32_t flags;
  unsigned int num_enum_values;
  struct zippo_drm_property_enum_info *enum_values;
  unsigned int num_range_values;
  uint64_t range_values[2];
};

uint64_t drm_property_get_value(struct zippo_drm_property_info *info,
    const drmModeObjectProperties *props, uint64_t def);

uint64_t *drm_property_get_range_values(
    struct zippo_drm_property_info *info, const drmModeObjectProperties *props);

void drm_property_info_populate(struct zippo_backend_drm *backend,
    const struct zippo_drm_property_info *src,
    struct zippo_drm_property_info *info, unsigned int num_infos,
    drmModeObjectPropertiesPtr props);

void drm_property_info_free(
    struct zippo_drm_property_info *info, int num_props);

int init_kms_caps(struct zippo_backend_drm *backend);

#endif  //  ZIPPO_KMS_H
