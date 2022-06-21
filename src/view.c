#include "view.h"

#include <stdio.h>
#include <stdlib.h>

struct zippo_view *
zippo_view_create()
{
  struct zippo_view *self;

  self = calloc(1, sizeof *self);
  if (self == NULL) {
    fprintf(stderr, "%s: no memory\n", __func__);
    goto err;
  }

  return self;

err:
  return NULL;
}

void
zippo_view_destroy(struct zippo_view *self)
{
  free(self);
}
