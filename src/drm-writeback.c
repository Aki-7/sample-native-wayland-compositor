#include "drm-writeback.h"

#include <stdlib.h>

#include "drm-connector.h"

static int
zippo_drm_writeback_update_info(
    struct zippo_drm_writeback *self, drmModeConnector *conn)
{
  return zippo_drm_connector_assign_connector_info(&self->connector, conn);
}

int
zippo_drm_writeback_create(
    struct zippo_backend_drm *backend, drmModeConnector *conn)
{
  struct zippo_drm_writeback *self;
  int ret;

  self = calloc(1, sizeof *self);
  self->backend = backend;

  zippo_drm_connector_init(&self->connector, backend, conn->connector_id);

  ret = zippo_drm_writeback_update_info(self, conn);
  if (ret < 0) goto err;

  wl_list_insert(&backend->writaback_connector_list, &self->link);

  return 0;

err:
  zippo_drm_connector_fini(&self->connector);
  return -1;
}

void
zippo_drm_writeback_destroy(struct zippo_drm_writeback *self)
{
  zippo_drm_connector_fini(&self->connector);
  wl_list_remove(&self->link);

  free(self);
}
