#include "compositor.h"

#include <stdio.h>
#include <stdlib.h>

#include "backend.h"

struct zippo_compositor_default {
  struct zippo_compositor base;
};

static int
zippo_compositor_init(struct zippo_compositor* self)
{
  return self->backend->impl->init(self->backend);
}

static struct zippo_compositor_interface compositor_default_impl = {
    .init = zippo_compositor_init,
};

struct zippo_compositor*
zippo_compositor_create(struct wl_display* wl_display)
{
  struct zippo_compositor_default* self;

  self = calloc(1, sizeof *self);

  if (self == NULL) {
    fprintf(stderr, "failed to allocate memory\n");
    goto err;
  }

  self->base.impl = &compositor_default_impl;
  self->base.wl_display = wl_display;

  return &self->base;

err:
  return NULL;
}

void
zippo_compositor_destroy(struct zippo_compositor* self)
{
  free(self);
}
