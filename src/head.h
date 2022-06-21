#ifndef ZIPPO_HEAD_H
#define ZIPPO_HEAD_H

#include "compositor.h"

struct zippo_head {
  struct zippo_compositor *compositor;
  char *name;

  uint32_t subpixel;
  bool connected;

  char *make;
  char *model;
  char *serial_number;
};

void zippo_head_set_monitor_string(struct zippo_head *self, const char *make,
    const char *model, const char *serialno);

void zippo_head_set_subpixel(struct zippo_head *self, uint32_t subpixel);

void zippo_head_set_connection_status(struct zippo_head *self, bool connected);

void zippo_head_init(struct zippo_head *self, const char *name);

void zippo_head_fini(struct zippo_head *self);

#endif  //  ZIPPO_HEAD_H
