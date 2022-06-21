#include "head.h"

#include <stdlib.h>
#include <string.h>

void
zippo_head_set_monitor_string(struct zippo_head *self, const char *make,
    const char *model, const char *serialno)
{
  free(self->make);
  free(self->model);
  free(self->serial_number);

  self->make = make ? strdup(make) : NULL;
  self->model = model ? strdup(model) : NULL;
  self->serial_number = serialno ? strdup(serialno) : NULL;
}

void
zippo_head_set_connection_status(struct zippo_head *self, bool connected)
{
  self->connected = connected;
}

void
zippo_head_set_subpixel(struct zippo_head *self, uint32_t subpixel)
{
  self->subpixel = subpixel;
}

void
zippo_head_init(struct zippo_head *self, const char *name)
{
  memset(self, 0, sizeof *self);

  self->name = strdup(name);
}

void
zippo_head_fini(struct zippo_head *self)
{
  free(self->name);
}
