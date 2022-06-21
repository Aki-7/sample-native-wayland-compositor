#ifndef ZIPPO_BACKEND_DRM_H
#define ZIPPO_BACKEND_DRM_H

#include <libudev.h>
#include <wayland-server.h>

#include "backend.h"
#include "launcher.h"

struct zippo_backend_drm {
  struct zippo_backend base;
  struct zippo_launcher* launcher;
  struct udev* udev;

  struct {
    int id;
    int fd;
    char* filename;
    dev_t devnum;
  } drm;

  int32_t cursor_width;
  int32_t cursor_height;

  bool atomic_modeset;
  bool fb_modifiers;
  bool sprites_are_broken;
  bool aspect_ratio_supported;

  struct wl_list crtc_list;
  struct wl_list writaback_connector_list;
  struct wl_list plane_list;  // zippo_drm_plane

  int min_width, max_width;
  int min_height, max_heigth;

  // listener
  struct wl_listener session_listener;
};

#endif  //  ZIPPO_BACKEND_DRM_H
