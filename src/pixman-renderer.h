#ifndef ZIPPO_PIXMAN_RENDERER_H
#define ZIPPO_PIXMAN_RENDERER_H

#include <pixman.h>

#include "compositor.h"
#include "output.h"

struct zippo_pixman_renderer_interface {
  int (*read_pixcels)(struct zippo_output *output, pixman_format_code_t format,
      void *pixcels, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
  void (*repaint_output)(
      struct zippo_output *output, pixman_region32_t *output_damage);
};

struct zippo_pixman_renderer {
  struct zippo_pixman_renderer_interface imple;
};

int zippo_pixman_renderer_init(struct zippo_compositor *compositor);

#endif  //  ZIPPO_PIXMAN_RENDERER_H
