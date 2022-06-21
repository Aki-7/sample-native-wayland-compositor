#include "drm-plane.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "drm-fb.h"
#include "kms.h"
#include "output-drm.h"

// clang-format off
static struct zippo_drm_property_enum_info plane_type_enums[] = {
  [ZIPPO_DRM_PLANE_TYPE_PRIMARY] = { .name = "Primary" },
  [ZIPPO_DRM_PLANE_TYPE_OVERLAY] = { .name = "Overlay" },
  [ZIPPO_DRM_PLANE_TYPE_CURSOR] = { .name = "Cursor" },
};

static const struct zippo_drm_property_info plane_props[] = {
  [ZIPPO_DRM_PLANE_TYPE] = {
    .name = "type",
    .enum_values = plane_type_enums,
    .num_enum_values = ZIPPO_DRM_PLANE_TYPE__COUNT,
  },
  [ZIPPO_DRM_PLANE_SRC_X] = { .name = "SRC_X", },
  [ZIPPO_DRM_PLANE_SRC_Y] = { .name = "SRC_Y", },
  [ZIPPO_DRM_PLANE_SRC_W] = { .name = "SRC_W", },
  [ZIPPO_DRM_PLANE_SRC_H] = { .name = "SRC_H", },
  [ZIPPO_DRM_PLANE_CRTC_X] = { .name = "CRTC_X", },
  [ZIPPO_DRM_PLANE_CRTC_Y] = { .name = "CRTC_Y", },
  [ZIPPO_DRM_PLANE_CRTC_W] = { .name = "CRTC_W", },
  [ZIPPO_DRM_PLANE_CRTC_H] = { .name = "CRTC_H", },
  [ZIPPO_DRM_PLANE_FB_ID] = { .name = "FB_ID", },
  [ZIPPO_DRM_PLANE_CRTC_ID] = { .name = "CRTC_ID", },
  [ZIPPO_DRM_PLANE_IN_FORMATS] = { .name = "IN_FORMATS" },
  [ZIPPO_DRM_PLANE_IN_FENCE_FD] = { .name = "IN_FENCE_FD" },
  [ZIPPO_DRM_PLANE_FB_DAMAGE_CLIPS] = { .name = "FB_DAMAGE_CLIPS" },
  [ZIPPO_DRM_PLANE_ZPOS] = { .name = "zpos" },
};
// clang-format on

static inline uint32_t *
formats_ptr(struct drm_format_modifier_blob *blob)
{
  return (uint32_t *)(((char *)blob) + blob->formats_offset);
}

static inline struct drm_format_modifier *
modifiers_ptr(struct drm_format_modifier_blob *blob)
{
  return (
      struct drm_format_modifier *)(((char *)blob) + blob->modifiers_offset);
}

static int
zippo_drm_plane_populate_formats(struct zippo_drm_plane *self,
    const drmModePlane *plane, const drmModeObjectProperties *props,
    const bool use_modifiers)
{
  unsigned i, j;
  drmModePropertyBlobRes *blob = NULL;
  struct drm_format_modifier_blob *fmt_mod_blob;
  struct drm_format_modifier *blob_modifiers;
  uint32_t *blob_formats;
  uint32_t blob_id;
  struct zippo_drm_format *fmt;
  int ret = 0;

  if (!use_modifiers) goto fallback;

  blob_id = drm_property_get_value(
      &self->props[ZIPPO_DRM_PLANE_IN_FORMATS], props, 0);

  if (blob_id == 0) goto fallback;

  blob = drmModeGetPropertyBlob(self->backend->drm.fd, blob_id);
  if (!blob) goto fallback;

  fmt_mod_blob = blob->data;
  blob_formats = formats_ptr(fmt_mod_blob);
  blob_modifiers = modifiers_ptr(fmt_mod_blob);

  assert(plane->count_formats == fmt_mod_blob->count_formats);

  for (i = 0; i < fmt_mod_blob->count_formats; i++) {
    fmt = zippo_drm_format_array_add_format(&self->formats, blob_formats[i]);

    if (!fmt) {
      ret = -1;
      goto out;
    }

    for (j = 0; j < fmt_mod_blob->count_modifiers; j++) {
      struct drm_format_modifier *mod = &blob_modifiers[j];

      if ((i < mod->offset) || (i > mod->offset + 63)) continue;
      if (!(mod->formats) & (1 << (i - mod->offset))) continue;
      // ここの処理が、それぞれのformatに対して付けられているmodifierを紐づけようとしているのなら、
      // (i - mod->offset) ではなく、(blob_formats[i] - mod->offset) なのでは ?

      ret = zippo_drm_format_add_modifier(fmt, mod->modifier);
      if (ret < 0) goto out;
    }

    if (fmt->modifiers.size == 0)
      zippo_drm_format_array_remove_latest_format(&self->formats);
  }

out:
  drmModeFreePropertyBlob(blob);

  return ret;

fallback:

  return 0;
}

static struct zippo_drm_plane_state *
drm_plane_state_alloc(
    struct zippo_output_drm_state *state_output, struct zippo_drm_plane *plane)
{
  struct zippo_drm_plane_state *state = calloc(1, sizeof *state);

  if (state == NULL) {
    fprintf(stderr, "%s: no memory\n", __func__);
    return NULL;
  }

  state->output_state = state_output;
  state->plane = plane;
  state->in_fence_fd = -1;
  state->zpos = DRM_PLANE_ZPOS_INVALID_PLANE;

  if (state_output)
    wl_list_insert(&state_output->plane_list, &state->link);
  else
    wl_list_init(&state->link);

  return state;
}

static void
drm_plane_state_free(struct zippo_drm_plane_state *state, bool force)
{
  if (!state) return;

  wl_list_remove(&state->link);
  wl_list_init(&state->link);
  state->output_state = NULL;
  state->in_fence_fd = -1;
  state->zpos = DRM_PLANE_ZPOS_INVALID_PLANE;

  if (state->damage_blob_id != 0) {
    drmModeDestroyPropertyBlob(
        state->plane->backend->drm.fd, state->damage_blob_id);
    state->damage_blob_id = 0;
  }

  if (force || state != state->plane->state_cur) {
    zippo_drm_fb_unref(state->fb);
    free(state);
  }
}

struct zippo_drm_plane *
zippo_drm_plane_create(struct zippo_backend_drm *backend, drmModePlane *plane)
{
  struct zippo_drm_plane *self;
  drmModeObjectProperties *props;
  uint64_t *zpos_range_values;

  self = calloc(1, sizeof *self);
  if (!self) {
    fprintf(stderr, "%s: out of memory\n", __func__);
    return NULL;
  }

  self->backend = backend;
  self->state_cur = drm_plane_state_alloc(NULL, self);
  if (self->state_cur == NULL) {
    fprintf(stderr, "%s: Failed to allocate plane state\n", __func__);
    goto err;
  }

  self->state_cur->complete = true;
  self->possible_crtcs = plane->possible_crtcs;
  self->plane_id = plane->plane_id;

  zippo_drm_format_array_init(&plane->formats);

  props = drmModeObjectGetProperties(
      backend->drm.fd, plane->plane_id, DRM_MODE_OBJECT_PLANE);
  if (!props) {
    fprintf(stderr, "%s: couldn't get plane properties\n", __func__);
    goto err_props;
  }

  drm_property_info_populate(
      backend, plane_props, self->props, ZIPPO_DRM_PLANE__COUNT, props);

  self->type = drm_property_get_value(
      &self->props[ZIPPO_DRM_PLANE_TYPE], props, ZIPPO_DRM_PLANE_TYPE__COUNT);

  zpos_range_values =
      drm_property_get_range_values(&self->props[ZIPPO_DRM_PLANE_ZPOS], props);

  if (zpos_range_values) {
    self->zpos_min = zpos_range_values[0];
    self->zpos_max = zpos_range_values[1];
  } else {
    self->zpos_min = DRM_PLANE_ZPOS_INVALID_PLANE;
    self->zpos_max = DRM_PLANE_ZPOS_INVALID_PLANE;
  }

  if (zippo_drm_plane_populate_formats(
          self, plane, props, backend->fb_modifiers) < 0) {
    drmModeFreeObjectProperties(props);
    goto err_props;
  }

  drmModeFreeObjectProperties(props);

  if (self->type == ZIPPO_DRM_PLANE_TYPE__COUNT) goto err_type;

  pixman_region32_init(&self->damage);
  pixman_region32_init(&self->clip);
  self->x = 0;
  self->y = 0;
  wl_list_insert(&backend->plane_list, &self->link);

  return self;

err_type:
  drm_property_info_free(self->props, ZIPPO_DRM_PLANE__COUNT);

err_props:
  zippo_drm_format_array_fini(&self->formats);
  drm_plane_state_free(self->state_cur, true);

err:
  free(self);
  return NULL;
}

void
zippo_drm_plane_destroy(struct zippo_drm_plane *self)
{
  if (self->type == ZIPPO_DRM_PLANE_TYPE_OVERLAY)
    drmModeSetPlane(
        self->backend->drm.fd, self->plane_id, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  drm_plane_state_free(self->state_cur, true);
  drm_property_info_free(self->props, ZIPPO_DRM_PLANE__COUNT);
  pixman_region32_fini(&self->damage);
  pixman_region32_fini(&self->clip);

  // TODO: ? set compositor.view_list -> view.plane

  wl_list_remove(&self->link);
}
