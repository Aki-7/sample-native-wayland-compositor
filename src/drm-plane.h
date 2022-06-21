#ifndef ZIPPO_DRM_PLANE_H
#define ZIPPO_DRM_PLANE_H

#include <pixman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "backend-drm.h"
#include "drm-fb.h"
#include "drm-format.h"
#include "kms.h"
#include "output-drm.h"
#include "plane.h"
#include "view.h"

#ifndef DRM_PLANE_ZPOS_INVALID_PLANE
#define DRM_PLANE_ZPOS_INVALID_PLANE 0xffffffffffffffffULL
#endif

enum wdrm_plane_property {
  ZIPPO_DRM_PLANE_TYPE = 0,
  ZIPPO_DRM_PLANE_SRC_X,
  ZIPPO_DRM_PLANE_SRC_Y,
  ZIPPO_DRM_PLANE_SRC_W,
  ZIPPO_DRM_PLANE_SRC_H,
  ZIPPO_DRM_PLANE_CRTC_X,
  ZIPPO_DRM_PLANE_CRTC_Y,
  ZIPPO_DRM_PLANE_CRTC_W,
  ZIPPO_DRM_PLANE_CRTC_H,
  ZIPPO_DRM_PLANE_FB_ID,
  ZIPPO_DRM_PLANE_CRTC_ID,
  ZIPPO_DRM_PLANE_IN_FORMATS,
  ZIPPO_DRM_PLANE_IN_FENCE_FD,
  ZIPPO_DRM_PLANE_FB_DAMAGE_CLIPS,
  ZIPPO_DRM_PLANE_ZPOS,
  ZIPPO_DRM_PLANE__COUNT
};

enum zippo_drm_plane_type {
  ZIPPO_DRM_PLANE_TYPE_PRIMARY = 0,
  ZIPPO_DRM_PLANE_TYPE_CURSOR,
  ZIPPO_DRM_PLANE_TYPE_OVERLAY,
  ZIPPO_DRM_PLANE_TYPE__COUNT,
};

struct zippo_drm_plane_state {
  struct zippo_drm_plane *plane;
  struct zippo_output_drm *output;
  struct zippo_output_drm_state *output_state;

  struct zippo_drm_fb *fb;

  struct zippo_view *view;

  int32_t src_x, src_y;
  uint32_t src_w, src_h;
  int32_t dest_x, dest_y;
  uint32_t dest_w, dest_h;

  uint64_t zpos;

  bool complete;

  int in_fence_fd;

  uint32_t damage_blob_id;

  struct wl_list link;
};

struct zippo_drm_plane {
  struct zippo_plane base;

  struct zippo_backend_drm *backend;
  enum zippo_drm_plane_type type;
  uint32_t possible_crtcs;
  uint32_t plane_id;
  struct zippo_drm_property_info props[ZIPPO_DRM_PLANE__COUNT];

  struct zippo_drm_plane_state *state_cur;

  uint64_t zpos_min;
  uint64_t zpos_max;

  struct zippo_drm_format_array formats;

  struct wl_list link;
};

struct zippo_drm_plane *zippo_drm_plane_create(
    struct zippo_backend_drm *backend, drmModePlane *plane);

void zippo_drm_plane_destroy(struct zippo_drm_plane *self);

#endif  //  ZIPPO_DRM_PLANE_H
