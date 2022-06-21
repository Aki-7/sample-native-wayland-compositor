#ifndef ZIPPO_OUTPUT_DRM_H
#define ZIPPO_OUTPUT_DRM_H

#include <wayland-server.h>

#include "output.h"

struct zippo_output_drm_state {
  struct zippo_output_drm *output;

  struct wl_list plane_list;
};

struct zippo_output_drm {
  struct zippo_output base;
};

#endif  //  ZIPPO_OUTPUT_DRM_H
