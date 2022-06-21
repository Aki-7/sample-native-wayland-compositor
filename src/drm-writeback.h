#ifndef ZIPPO_DRM_WRITEBACK_H
#define ZIPPO_DRM_WRITEBACK_H

#include <wayland-server.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "backend-drm.h"
#include "drm-connector.h"

struct zippo_drm_writeback {
  struct wl_list link;

  struct zippo_backend_drm *backend;
  struct zippo_drm_connector connector;
};

int zippo_drm_writeback_create(
    struct zippo_backend_drm *backend, drmModeConnector *conn);

void zippo_drm_writeback_destroy(struct zippo_drm_writeback *self);

#endif  //  ZIPPO_DRM_WRITEBACK_H
