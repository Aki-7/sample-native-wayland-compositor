#ifndef ZIPPO_DRM_HEAD_H
#define ZIPPO_DRM_HEAD_H

#include <libudev.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "backend-drm.h"
#include "drm-connector.h"
#include "head.h"

struct zippo_drm_edid {
  char eisa_id[13];
  char monitor_name[13];
  char pnp_id[13];
  char serial_number[13];
};

struct zippo_drm_head {
  struct zippo_head base;

  struct zippo_drm_edid edid;
  struct zippo_backend_drm *backend;
  struct zippo_drm_connector connector;
};

int zippo_drm_head_create(struct zippo_backend_drm *backend,
    drmModeConnector *conn, struct udev_device *drm_device);

void zippo_drm_head_destroy(struct zippo_drm_head *self);

#endif  //  ZIPPO_DRM_HEAD_H
