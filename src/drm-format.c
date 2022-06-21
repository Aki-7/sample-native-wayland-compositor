#include "drm-format.h"

#include <assert.h>
#include <stdio.h>

void
zippo_drm_format_array_init(struct zippo_drm_format_array *formats)
{
  wl_array_init(&formats->arr);
}

void
zippo_drm_format_array_fini(struct zippo_drm_format_array *formats)
{
  struct zippo_drm_format *format;

  wl_array_for_each(format, &formats->arr) wl_array_release(&format->modifiers);

  wl_array_release(&formats->arr);
}

struct zippo_drm_format *
zippo_drm_format_array_add_format(
    struct zippo_drm_format_array *formats, uint32_t format)
{
  struct zippo_drm_format *fmt;

  fmt = wl_array_add(&formats->arr, sizeof *fmt);

  if (!fmt) {
    fprintf(stderr, "%s: out of memory\n", __func__);
    return NULL;
  }

  fmt->format = format;
  wl_array_init(&fmt->modifiers);

  return fmt;
}

int
zippo_drm_format_add_modifier(
    struct zippo_drm_format *format, uint64_t modifier)
{
  uint64_t *mod;

  mod = wl_array_add(&format->modifiers, sizeof(*mod));
  if (!mod) {
    fprintf(stderr, "%s: out of memory\n", __func__);
    return -1;
  }

  *mod = modifier;

  return 0;
}

void
zippo_drm_format_array_remove_latest_format(
    struct zippo_drm_format_array *formats)
{
  struct wl_array *array = &formats->arr;
  struct zippo_drm_format *fmt;

  array->size -= sizeof(*fmt);

  fmt = (void *)((char *)array->data + array->size);
  wl_array_release(&fmt->modifiers);
}
