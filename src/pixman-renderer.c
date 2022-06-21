#include "pixman-renderer.h"

#include <stdlib.h>

static int
read_pixels(struct zippo_output *output, pixman_format_code_t format,
    void *pixels, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
  (void)output;
  (void)format;
  (void)pixels;
  (void)x;
  (void)y;
  (void)width;
  (void)height;
  return 0;
}

static void
repaint_output(struct zippo_output *output, pixman_region32_t *output_damage)
{
  (void)output;
  (void)output_damage;
}

int
zippo_pixman_renderer_init(struct zippo_compositor *compositor)
{
  struct zippo_pixman_renderer *self;

  self = calloc(1, sizeof *self);
  if (self == NULL) return -1;

  self->imple.read_pixcels = read_pixels;
  self->imple.repaint_output = repaint_output;
  compositor->renderer = self;

  return 0;
}
