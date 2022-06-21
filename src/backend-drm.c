#define _GNU_SOURCE

#include "backend-drm.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "backend.h"
#include "drm-crtc.h"
#include "drm-plane.h"
#include "kms.h"
#include "launcher.h"
#include "output.h"

static void
zippo_backend_drm_repaint_begin(struct zippo_backend* parent)
{
  (void)parent;
  fprintf(stderr, "repaint begin! (not implemented)\n");
}

static void
zippo_backend_drm_repaint_cancel(
    struct zippo_backend* parent, void* repaint_data)
{
  (void)parent;
  (void)repaint_data;
  fprintf(stderr, "repaint cancel! (not implemented)\n");
}

static int
zippo_backend_drm_repaint_flush(
    struct zippo_backend* parent, void* repaint_data)
{
  (void)parent;
  (void)repaint_data;
  fprintf(stderr, "repaint flush! (not implemented)\n");
  return 0;
}

static struct zippo_output*
zippo_backend_drm_create_output(struct zippo_backend* parent, const char* name)
{
  (void)parent;
  (void)name;
  return zippo_output_create();
}

static void
zippo_backend_drm_device_changed(
    struct zippo_backend* parent, dev_t device, bool added)
{
  (void)parent;
  (void)device;
  (void)added;
  fprintf(stderr, "device changed! (not implemented)\n");
}

static void
zippo_backend_drm_create_sprites(struct zippo_backend_drm* self)
{
  drmModePlaneRes* plane_resources;
  drmModePlane* plane;
  struct zippo_drm_plane* drm_plane;
  uint32_t i;

  plane_resources = drmModeGetPlaneResources(self->drm.fd);
  if (!plane_resources) {
    fprintf(stderr, "Failed to get plane resources: %s\n", strerror(errno));
    return;
  }

  for (i = 0; i < plane_resources->count_planes; i++) {
    plane = drmModeGetPlane(self->drm.fd, plane_resources->planes[i]);
    if (!plane) continue;

    drm_plane = zippo_drm_plane_create(self, plane);
    drmModeFreePlane(plane);
    if (!drm_plane) continue;

    if (drm_plane->type == ZIPPO_DRM_PLANE_TYPE_OVERLAY) {
      // Next to do
    }
  }

  drmModeFreePlaneResources(plane_resources);
}

static int
zippo_backend_drm_create_crtc_list(
    struct zippo_backend_drm* self, drmModeRes* resources)
{
  struct zippo_drm_crtc *crtc, *crtc_tmp;
  int i = 0;

  for (i = 0; i < resources->count_crtcs; i++) {
    crtc = zippo_drm_crtc_create(self, resources->crtcs[i], i);
    wl_list_insert(self->crtc_list.prev, &crtc->link);
    if (!crtc) goto err;
  }

  return 0;

err:
  wl_list_for_each_safe(crtc, crtc_tmp, &self->crtc_list, link)
  {
    wl_list_remove(&crtc->link);
    zippo_drm_crtc_destroy(crtc);
  }

  return -1;
}

static void
zippo_backend_drm_destroy_crtc_list(struct zippo_backend_drm* self)
{
  struct zippo_drm_crtc *crtc, *crtc_tmp;

  wl_list_for_each_safe(crtc, crtc_tmp, &self->crtc_list, link)
  {
    wl_list_remove(&crtc->link);
    zippo_drm_crtc_destroy(crtc);
  }
}

static bool
zippo_backend_drm_device_is_kms(
    struct zippo_backend_drm* self, struct udev_device* device)
{
  struct zippo_launcher* launcher = self->launcher;
  const char* filename = udev_device_get_devnode(device);
  const char* sysnum = udev_device_get_sysnum(device);
  dev_t devnum = udev_device_get_devnum(device);
  drmModeRes* resource;
  int id = -1, fd;

  if (!filename) return false;

  fd = launcher->impl->open(launcher, filename, O_RDWR);
  if (fd < 0) return false;

  resource = drmModeGetResources(fd);
  if (!resource) goto err_fd;

  if (resource->count_crtcs <= 0 || resource->count_connectors <= 0 ||
      resource->count_encoders <= 0)
    goto err_resource;

  if (sysnum) id = atoi(sysnum);
  if (!sysnum || id < 0) {
    fprintf(stderr, "couldn't get sysnum for device %s\n", filename);
    goto err_resource;
  }

  // for multiple call
  if (self->drm.fd >= 0) launcher->impl->close(launcher, self->drm.fd);
  free(self->drm.filename);

  self->drm.fd = fd;
  self->drm.id = id;
  self->drm.filename = strdup(filename);
  self->drm.devnum = devnum;

  drmModeFreeResources(resource);

  return true;

err_resource:
  drmModeFreeResources(resource);

err_fd:

  return false;
}

static struct udev_device*
zippo_backend_drm_find_primary_gpu(struct zippo_backend_drm* self)
{
  struct udev* udev = self->udev;
  struct udev_enumerate* e;
  struct udev_list_entry* entry;
  const char *path, *id;
  struct udev_device *device, *drm_device, *pci;

  e = udev_enumerate_new(udev);
  udev_enumerate_add_match_subsystem(e, "drm");
  udev_enumerate_add_match_sysname(e, "card[0-9]*");

  udev_enumerate_scan_devices(e);
  drm_device = NULL;

  udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e))
  {
    bool is_boot_vga = false;  // trueだとkmsがoffになるのとか関係あるか?

    path = udev_list_entry_get_name(entry);
    device = udev_device_new_from_syspath(udev, path);
    if (!device) continue;

    // weston checks devices' "ID_SEAT" property

    pci = udev_device_get_parent_with_subsystem_devtype(device, "pci", NULL);

    if (pci) {
      id = udev_device_get_sysattr_value(pci, "boot_vga");
      if (id && strcmp(id, "1") == 0) is_boot_vga = true;
    }

    if (!is_boot_vga && drm_device) {
      udev_device_unref(device);
      continue;
    }

    if (!zippo_backend_drm_device_is_kms(self, device)) {
      udev_device_unref(device);
      continue;
    }

    if (is_boot_vga) {
      if (drm_device) udev_device_unref(drm_device);
      drm_device = device;
      break;
    }

    drm_device = device;
  }

  assert(!!drm_device == (self->drm.fd >= 0));

  udev_enumerate_unref(e);

  return drm_device;
}

static void
zippo_backend_drm_session_handler(struct wl_listener* listener, void* data)
{
  (void)data;
  struct zippo_backend_drm* self =
      wl_container_of(listener, self, session_listener);
  (void)self;
}

static int
zippo_backend_drm_init(struct zippo_backend* parent)
{
  struct zippo_backend_drm* self = (struct zippo_backend_drm*)parent;
  struct udev_device* drm_device;
  drmModeRes* resources;
  int ret;

  ret = self->launcher->impl->connect(self->launcher);
  if (ret < 0) goto err;

  self->udev = udev_new();
  if (self->udev == NULL) {
    fprintf(stderr, "Failed to initialize udev context\n");
    goto err;
  }

  drm_device = zippo_backend_drm_find_primary_gpu(self);
  if (drm_device == NULL) {
    fprintf(stderr, "No drm device found\n");
    goto err_udev;
  }

  if (init_kms_caps(self) < 0) {
    fprintf(stderr, "failed to initialize kms\n");
    goto err_udev_dev;
  }

  // init pixman or gel

  // setup vt switch key bindings: no keyboard configuration; skip

  resources = drmModeGetResources(self->drm.fd);
  if (!resources) {
    fprintf(stderr, "Failed to get drmModeRes\n");
    goto err_udev_dev;
  }

  wl_list_init(&self->crtc_list);
  if (zippo_backend_drm_create_crtc_list(self, resources) == -1) {
    fprintf(stderr, "Failed to create CRTC list for DRM-backend\n");
    goto err_crtc;
  }

  wl_list_init(&self->plane_list);
  zippo_backend_drm_create_sprites(self);

  // udev input init; skip

  wl_list_init(&self->writaback_connector_list);

  drmModeFreeResources(resources);

  return 0;

err_crtc:
  drmModeFreeResources(resources);

err_udev_dev:
  udev_device_unref(drm_device);

err_udev:
  udev_unref(self->udev);
  self->udev = NULL;

err:
  return ret;
}

static struct zippo_backend_interface const backend_drm_impl = {
    .init = zippo_backend_drm_init,
    .repaint_begin = zippo_backend_drm_repaint_begin,
    .repaint_cancel = zippo_backend_drm_repaint_cancel,
    .repaint_flush = zippo_backend_drm_repaint_flush,
    .create_output = zippo_backend_drm_create_output,
    .device_changed = zippo_backend_drm_device_changed,
};

struct zippo_backend*
zippo_backend_create(struct zippo_compositor* compositor)
{
  struct zippo_backend_drm* self;
  struct zippo_launcher* launcher;

  self = calloc(1, sizeof *self);
  if (self == NULL) {
    fprintf(stderr, "Failed to allocate memory\n");
    goto err;
  }

  launcher = zippo_launcher_create(compositor->wl_display);
  if (launcher == NULL) {
    fprintf(stderr, "Failed to create launcher\n");
    goto err_launcher;
  }

  self->session_listener.notify = zippo_backend_drm_session_handler;
  wl_signal_add(&launcher->session_signal, &self->session_listener);

  self->base.impl = &backend_drm_impl;
  self->base.compositor = compositor;
  self->launcher = launcher;

  return &self->base;

err_launcher:
  free(self);

err:
  return NULL;
}

void
zippo_backend_destroy(struct zippo_backend* parent)
{
  struct zippo_backend_drm* self = (struct zippo_backend_drm*)parent;

  zippo_backend_drm_destroy_crtc_list(self);

  if (self->udev) udev_unref(self->udev);
  zippo_launcher_destroy(self->launcher);
  close(self->drm.fd);
  free(self->drm.filename);
  free(self);
}
