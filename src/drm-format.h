#ifndef ZIPPO_DRM_FORMAT_H
#define ZIPPO_DRM_FORMAT_H

#include <wayland-server.h>

struct zippo_drm_format {
  uint32_t format;
  struct wl_array modifiers;
};

struct zippo_drm_format_array {
  struct wl_array arr;
};

void zippo_drm_format_array_init(struct zippo_drm_format_array *formats);

void zippo_drm_format_array_fini(struct zippo_drm_format_array *formats);

struct zippo_drm_format *zippo_drm_format_array_add_format(
    struct zippo_drm_format_array *formats, uint32_t format);

int zippo_drm_format_add_modifier(
    struct zippo_drm_format *format, uint64_t modifier);

void zippo_drm_format_array_remove_latest_format(
    struct zippo_drm_format_array *formats);

#endif  //  ZIPPO_DRM_FORMAT_H
