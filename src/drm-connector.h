#ifndef ZIPPO_DRM_CONNECTOR_H
#define ZIPPO_DRM_CONNECTOR_H

#include <stdio.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "backend-drm.h"
#include "kms.h"

enum zippo_drm_connector_property {
  ZIPPO_DRM_CONNECTOR_EDID = 0,
  ZIPPO_DRM_CONNECTOR_DPMS,
  ZIPPO_DRM_CONNECTOR_CRTC_ID,
  ZIPPO_DRM_CONNECTOR_NON_DESKTOP,
  ZIPPO_DRM_CONNECTOR_CONTENT_PROTECTION,
  ZIPPO_DRM_CONNECTOR_HDCP_CONTENT_TYPE,
  ZIPPO_DRM_CONNECTOR_PANEL_ORIENTATION,
  ZIPPO_DRM_CONNECTOR__COUNT,
};

enum zippo_drm_content_protection_state {
  ZIPPO_DRM_CONTENT_PROTECTION_UNDESIRED = 0,
  ZIPPO_DRM_CONTENT_PROTECTION_DESIRED,
  ZIPPO_DRM_CONTENT_PROTECTION_ENABLED,
  ZIPPO_DRM_CONTENT_PROTECTION__COUNT
};

enum zippo_drm_hdcp_content_type {
  ZIPPO_DRM_HDCP_CONTENT_TYPE0 = 0,
  ZIPPO_DRM_HDCP_CONTENT_TYPE1,
  ZIPPO_DRM_HDCP_CONTENT_TYPE__COUNT
};

enum zippo_drm_dpms_state {
  ZIPPO_DRM_DPMS_STATE_OFF = 0,
  ZIPPO_DRM_DPMS_STATE_ON,
  ZIPPO_DRM_DPMS_STATE_STANDBY, /* unused */
  ZIPPO_DRM_DPMS_STATE_SUSPEND, /* unused */
  ZIPPO_DRM_DPMS_STATE__COUNT,
};

enum zippo_drm_panel_orientation {
  ZIPPO_DRM_PANEL_ORIENTATION_NORMAL = 0,
  ZIPPO_DRM_PANEL_ORIENTATION_UPSIDE_DOWN,
  ZIPPO_DRM_PANEL_ORIENTATION_LEFT_SIDE_UP,
  ZIPPO_DRM_PANEL_ORIENTATION_RIGHT_SIDE_UP,
  ZIPPO_DRM_PANEL_ORIENTATION__COUNT
};

struct zippo_drm_connector {
  struct zippo_backend_drm *backend;

  drmModeConnector *conn;
  uint32_t connector_id;

  drmModeObjectProperties *props_drm;

  struct zippo_drm_property_info props[ZIPPO_DRM_CONNECTOR__COUNT];
};

int zippo_drm_connector_assign_connector_info(
    struct zippo_drm_connector *self, drmModeConnector *conn);

void zippo_drm_connector_init(struct zippo_drm_connector *self,
    struct zippo_backend_drm *backend, uint32_t connector_id);

void zippo_drm_connector_fini(struct zippo_drm_connector *self);

#endif  //  ZIPPO_DRM_CONNECTOR_H
