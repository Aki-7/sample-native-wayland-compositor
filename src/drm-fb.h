#ifndef ZIPPO_DRM_FB_H
#define ZIPPO_DRM_FB_H

enum zippo_drm_fb_type {
  ZIPPO_DRM_BUFFER_INVALID = 0, /**< never used */
  ZIPPO_DRM_BUFFER_CLIENT,      /**< directly sourced from client */
  ZIPPO_DRM_BUFFER_DMABUF,      /**< imported from linux_dmabuf client */
  ZIPPO_DRM_BUFFER_PIXMAN_DUMB, /**< internal Pixman rendering */
  ZIPPO_DRM_BUFFER_GBM_SURFACE, /**< internal EGL rendering */
  ZIPPO_DRM_BUFFER_CURSOR,      /**< internal cursor buffer */
};

struct zippo_drm_fb {
  enum zippo_drm_fb_type type;
};

void zippo_drm_fb_unref(struct zippo_drm_fb *fb);

#endif  //  ZIPPO_DRM_FB_H
