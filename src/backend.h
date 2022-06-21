#ifndef ZIPPO_BACKEND_H
#define ZIPPO_BACKEND_H

#include <linux/dma-buf.h>
#include <wayland-server.h>

#include "compositor.h"
#include "output.h"

struct zippo_backend;

struct zippo_backend_interface {
  int (*init)(struct zippo_backend* self);
  void (*repaint_begin)(struct zippo_backend* self);
  void (*repaint_cancel)(struct zippo_backend* self, void* repaint_data);
  int (*repaint_flush)(struct zippo_backend* self, void* repaint_data);
  struct zippo_output* (*create_output)(
      struct zippo_backend* self, const char* name);
  void (*device_changed)(struct zippo_backend* self, dev_t device, bool added);
};

struct zippo_backend {
  struct zippo_backend_interface const* impl;

  struct zippo_compositor* compositor;
};

struct zippo_backend* zippo_backend_create(struct zippo_compositor* compositor);

void zippo_backend_destroy(struct zippo_backend* self);

#endif  //  ZIPPO_BACKEND_H
