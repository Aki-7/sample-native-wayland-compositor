#include "plane.h"

#include <wayland-server.h>

void
zippo_plane_init(struct zippo_plane *self, struct zippo_compositor *compositor)
{
  pixman_region32_init(&self->damage);
  pixman_region32_init(&self->clip);
  self->x = 0;
  self->y = 0;
  self->compositor = compositor;

  wl_list_init(&self->link);
}

void
zippo_plane_fini(struct zippo_plane *self)
{
  pixman_region32_fini(&self->damage);
  pixman_region32_fini(&self->clip);

  // set NULL on correspoinding view's view->plane

  wl_list_remove(&self->link);
}
