#ifndef ZIPPO_OUTPUT_H
#define ZIPPO_OUTPUT_H

struct zippo_output;

struct zippo_output_interface {
  int (*repaint)(struct zippo_output *output);  // FIXME:
};

struct zippo_output {
  struct zippo_output_interface const *impl;
};

struct zippo_output *zippo_output_create();

void zippo_output_destroy(struct zippo_output *self);

#endif  //  ZIPPO_OUTPUT_H
