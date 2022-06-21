#ifndef ZIPPO_PLANE_H
#define ZIPPO_PLANE_H

#include <pixman.h>
#include <wayland-server.h>

struct zippo_plane {
  struct zippo_compositor *compositor;

  pixman_region32_t damage;
  pixman_region32_t clip;
  int32_t x, y;

  struct wl_list link;
};

void zippo_plane_init(
    struct zippo_plane *self, struct zippo_compositor *compositor);

void zippo_plane_fini(struct zippo_plane *self);

#endif  //  ZIPPO_PLANE_H
