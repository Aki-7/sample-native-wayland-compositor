#ifndef ZIPPO_COMPOSITOR_H
#define ZIPPO_COMPOSITOR_H

#include <wayland-server.h>

#include "backend.h"

struct zippo_compositor;

struct zippo_compositor_interface {
  int (*init)(struct zippo_compositor* self);
};

struct zippo_compositor {
  struct zippo_compositor_interface const* impl;

  struct zippo_backend* backend;  // injected
  struct wl_display* wl_display;
};

struct zippo_compositor* zippo_compositor_create(struct wl_display* wl_display);

void zippo_compositor_destroy(struct zippo_compositor* self);

#endif  //  ZIPPO_COMPOSITOR_H
