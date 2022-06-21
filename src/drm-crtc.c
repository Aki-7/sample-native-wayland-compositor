#include "drm-crtc.h"

#include <stdio.h>
#include <stdlib.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "backend-drm.h"
#include "kms.h"

const struct zippo_drm_property_info crtc_props[] = {
    [ZIPPO_DRM_CRTC_MODE_ID] =
        {
            .name = "MODE_ID",
        },
    [ZIPPO_DRM_CRTC_ACTIVE] =
        {
            .name = "ACTIVE",
        },
};

struct zippo_drm_crtc*
zippo_drm_crtc_create(
    struct zippo_backend_drm* backend, uint32_t crtc_id, uint32_t pipe)
{
  struct zippo_drm_crtc* self;
  drmModeObjectPropertiesPtr props;

  props = drmModeObjectGetProperties(
      backend->drm.fd, crtc_id, DRM_MODE_OBJECT_CRTC);

  if (!props) {
    fprintf(stderr, "Failed to get CRTC properties\n");
    return NULL;
  }

  self = calloc(1, sizeof *self);
  if (self == NULL) goto out;

  drm_property_info_populate(
      backend, crtc_props, self->props_crtc, ZIPPO_DRM_CRTC__COUNT, props);

  self->backend = backend;
  self->id = crtc_id;
  self->pipe = pipe;

  // fprintf(stderr, "[DEBUG]: crtc: id: %d\n", crtc_id);
  // for (int i = 0; i < ZIPPO_DRM_CRTC__COUNT; i++) {
  //   fprintf(stderr, "\t %s: \n", crtc_props[i].name);
  //   fprintf(stderr, "\t\t range: [");
  //   for (unsigned j = 0; j < crtc_props[i].num_range_values; j++)
  //     fprintf(stderr, "%ld, ", crtc_props[i].range_values[j]);
  //   fprintf(stderr, "]\n");
  //   fprintf(stderr, "\t\t enum:\n");
  //   for (unsigned j = 0; j < crtc_props[i].num_enum_values; j++) {
  //     struct zippo_drm_property_enum_info info =
  //     crtc_props[i].enum_values[j]; fprintf(stderr, "\n\n\n%s: %ld (%s)\n",
  //     info.name, info.value,
  //         info.valid ? "valid" : "invalid");
  //   }
  // }

out:
  drmModeFreeObjectProperties(props);
  return self;
}

void
zippo_drm_crtc_destroy(struct zippo_drm_crtc* self)
{
  drm_property_info_free(self->props_crtc, ZIPPO_DRM_CRTC__COUNT);
  free(self);
}
