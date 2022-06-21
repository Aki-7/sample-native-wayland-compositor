#ifndef ZIPPO_LAUNCHER_H
#define ZIPPO_LAUNCHER_H

#include <wayland-server.h>

struct zippo_launcher;

struct zippo_launcher_interface {
  int (*connect)(struct zippo_launcher *self);
  int (*open)(struct zippo_launcher *self, const char *path, int flags);
  void (*close)(struct zippo_launcher *parent, int fd);
};

struct zippo_launcher {
  struct zippo_launcher_interface const *impl;

  struct wl_display *wl_display;

  // signals
  struct wl_signal session_signal;
};

struct zippo_launcher *zippo_launcher_create(struct wl_display *wl_display);

void zippo_launcher_destroy(struct zippo_launcher *self);

#endif  //  ZIPPO_LAUNCHER_H
