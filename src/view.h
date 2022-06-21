#ifndef ZIPPO_VIEW_H
#define ZIPPO_VIEW_H

struct zippo_view {
  int i;
};

struct zippo_view *zippo_view_create();

void zippo_view_destroy(struct zippo_view *self);

#endif  //  ZIPPO_VIEW_H
