#include "output-drm.h"

#include <output.h>
#include <stdlib.h>

struct zippo_output *
zippo_output_create()
{
  struct zippo_output_drm *self;

  self = calloc(1, sizeof *self);
  if (self == NULL) {
    goto err;
  }

err:

  return &self->base;
}

void
zippo_output_destroy(struct zippo_output *self)
{
  free(self);
}
