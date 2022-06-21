#ifndef ZIPPO_DRM_CRTC_H
#define ZIPPO_DRM_CRTC_H

#include <wayland-server.h>

#include "backend-drm.h"
#include "kms.h"

enum zippo_drm_crtc_property {
  ZIPPO_DRM_CRTC_MODE_ID = 0,
  ZIPPO_DRM_CRTC_ACTIVE,
  ZIPPO_DRM_CRTC__COUNT
};

struct zippo_drm_crtc {
  struct wl_list link;

  struct zippo_backend_drm* backend;
  uint32_t id;
  int pipe;

  struct zippo_drm_property_info props_crtc[ZIPPO_DRM_CRTC__COUNT];
};

struct zippo_drm_crtc* zippo_drm_crtc_create(
    struct zippo_backend_drm* backend, uint32_t crtc_id, uint32_t pipe);

void zippo_drm_crtc_destroy(struct zippo_drm_crtc* self);

#endif  //  ZIPPO_DRM_CRTC_H
