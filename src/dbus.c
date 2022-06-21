#define _GNU_SOURCE

#include "dbus.h"

#include <dbus/dbus.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <wayland-server.h>

struct zippo_dbus_default {
  struct zippo_dbus base;
  DBusConnection* connection;            // set when open
  struct wl_event_source* event_source;  // set when open
};

static DBusMessage*
zippo_dbus_default_send_with_reply_and_block(struct zippo_dbus* parent,
    DBusMessage* message, int timeout_milliseconds, DBusError* error)
{
  struct zippo_dbus_default* self = (struct zippo_dbus_default*)parent;

  return dbus_connection_send_with_reply_and_block(
      self->connection, message, timeout_milliseconds, error);
}

static bool
zippo_dbus_default_send(struct zippo_dbus* parent, DBusMessage* message,
    dbus_uint32_t* client_serial)
{
  struct zippo_dbus_default* self = (struct zippo_dbus_default*)parent;

  return dbus_connection_send(self->connection, message, client_serial);
}

static int
zippo_dbus_default_add_match(
    struct zippo_dbus_default* self, const char* format, ...)
{
  DBusError err;
  int res;
  va_list list;
  char* str;

  va_start(list, format);
  res = vasprintf(&str, format, list);
  va_end(list);

  if (res < 0) return -ENOMEM;

  dbus_error_init(&err);
  dbus_bus_add_match(self->connection, str, &err);
  free(str);
  if (dbus_error_is_set(&err)) {
    dbus_error_free(&err);
    return -EIO;
  }

  return 0;
}

static int
zippo_dbus_default_add_match_signal(struct zippo_dbus* parent,
    const char* sender, const char* iface, const char* member, const char* path)
{
  struct zippo_dbus_default* self = (struct zippo_dbus_default*)parent;
  return zippo_dbus_default_add_match(self,
      "type='signal',"
      "sender='%s',"
      "interface='%s',"
      "member='%s',"
      "path='%s'",
      sender, iface, member, path);
}

static bool
zippo_dbus_default_add_filter(struct zippo_dbus* parent,
    DBusHandleMessageFunction function, void* user_data,
    DBusFreeFunction free_data_function)
{
  struct zippo_dbus_default* self = (struct zippo_dbus_default*)parent;
  return dbus_connection_add_filter(
      self->connection, function, user_data, free_data_function);
}

static int
dbus_dispatch_watch(int fd, uint32_t mask, void* user_data)
{
  DBusWatch* watch = user_data;
  uint32_t flags = 0;
  (void)fd;

  if (dbus_watch_get_enabled(watch)) {
    if (mask & WL_EVENT_READABLE) flags |= DBUS_WATCH_READABLE;
    if (mask & WL_EVENT_WRITABLE) flags |= DBUS_WATCH_WRITABLE;
    if (mask & WL_EVENT_HANGUP) flags |= DBUS_WATCH_HANGUP;
    if (mask & WL_EVENT_ERROR) flags |= DBUS_WATCH_ERROR;

    dbus_watch_handle(watch, flags);
  }

  return 0;
}

static dbus_bool_t
zippo_dbus_default_add_watch(DBusWatch* watch, void* data)
{
  struct zippo_dbus_default* self = data;
  struct wl_event_loop* loop;
  struct wl_event_source* source;
  int fd;
  uint32_t mask = 0, flags;

  if (dbus_watch_get_enabled(watch)) {
    flags = dbus_watch_get_flags(watch);
    if (flags & DBUS_WATCH_READABLE) mask |= WL_EVENT_READABLE;
    if (flags & DBUS_WATCH_WRITABLE) mask |= WL_EVENT_WRITABLE;
  }

  loop = wl_display_get_event_loop(self->base.wl_display);
  fd = dbus_watch_get_unix_fd(watch);
  source = wl_event_loop_add_fd(loop, fd, mask, dbus_dispatch_watch, watch);

  if (!source) return FALSE;

  dbus_watch_set_data(watch, source, NULL);

  return TRUE;
}

static void
zippo_dbus_default_remove_watch(DBusWatch* watch, void* data)
{
  (void)data;
  struct wl_event_source* source;

  source = dbus_watch_get_data(watch);
  if (!source) return;

  wl_event_source_remove(source);
}

static void
zippo_dbus_default_toggle_watch(DBusWatch* watch, void* data)
{
  (void)data;
  struct wl_event_source* source;
  uint32_t mask = 0, flags;

  source = dbus_watch_get_data(watch);
  if (!source) return;

  if (dbus_watch_get_enabled(watch)) {
    flags = dbus_watch_get_flags(watch);
    if (flags & DBUS_WATCH_READABLE) mask |= WL_EVENT_READABLE;
    if (flags & DBUS_WATCH_WRITABLE) mask |= WL_EVENT_WRITABLE;
  }

  wl_event_source_fd_update(source, mask);
}

static int
dbus_dispatch_timeout(void* user_data)
{
  DBusTimeout* timeout = user_data;

  if (dbus_timeout_get_enabled(timeout)) dbus_timeout_handle(timeout);

  return 0;
}

static int
dbus_adjust_timeout(DBusTimeout* timeout, struct wl_event_source* source)
{
  int64_t interval = 0;
  if (dbus_timeout_get_enabled(timeout))
    interval = dbus_timeout_get_interval(timeout);

  return wl_event_source_timer_update(source, interval);
}

static dbus_bool_t
zippo_dbus_default_add_timeout(DBusTimeout* timeout, void* data)
{
  struct zippo_dbus_default* self = data;
  struct wl_event_loop* loop;
  struct wl_event_source* source;
  int res;

  loop = wl_display_get_event_loop(self->base.wl_display);

  source = wl_event_loop_add_timer(loop, dbus_dispatch_timeout, timeout);

  if (!source) return FALSE;

  res = dbus_adjust_timeout(timeout, source);
  if (res < 0) {
    wl_event_source_remove(source);
    return FALSE;
  }

  dbus_timeout_set_data(timeout, source, NULL);

  return TRUE;
}

static void
zippo_dbus_default_remove_timeout(DBusTimeout* timeout, void* data)
{
  struct wl_event_source* source;
  (void)data;

  source = dbus_timeout_get_data(timeout);
  if (!source) return;

  wl_event_source_remove(source);
}

static void
zippo_dbus_default_toggle_timeout(DBusTimeout* timeout, void* data)
{
  struct wl_event_source* source;
  (void)data;

  source = dbus_timeout_get_data(timeout);
  if (!source) return;

  dbus_adjust_timeout(timeout, source);
}

static int
zippo_dbus_default_dispatch(int fd, uint32_t mask, void* data)
{
  struct zippo_dbus_default* self = data;
  int res;
  (void)fd;
  (void)mask;

  do {
    res = dbus_connection_dispatch(self->connection);
    if (res == DBUS_DISPATCH_COMPLETE)
      res = 0;
    else if (res == DBUS_DISPATCH_DATA_REMAINS)
      res = -EAGAIN;
    else if (res == DBUS_DISPATCH_NEED_MEMORY)
      res = -ENOMEM;
    else
      res = -EIO;
  } while (res == -EAGAIN);

  if (res) fprintf(stderr, "cannot dispatch dbus events: %s\n", strerror(res));

  return 0;
}

// DBUSのイベントをDispatchする処理をwayland
// のイベントループの中に入れ込むためのしょり。
static int
zippo_dbus_default_bind(struct zippo_dbus_default* self)
{
  int status, fd;
  bool res;
  struct wl_event_loop* loop;

  loop = wl_display_get_event_loop(self->base.wl_display);

  fd = eventfd(0, EFD_CLOEXEC);
  if (fd < 0) return -errno;

  self->event_source =
      wl_event_loop_add_fd(loop, fd, 0, zippo_dbus_default_dispatch, self);
  close(fd);

  if (self->event_source == NULL) return -ENOMEM;

  wl_event_source_check(self->event_source);

  res = dbus_connection_set_watch_functions(self->connection,
      zippo_dbus_default_add_watch, zippo_dbus_default_remove_watch,
      zippo_dbus_default_toggle_watch, self, NULL);

  if (!res) {
    status = -ENOMEM;
    goto err;
  }

  res = dbus_connection_set_timeout_functions(self->connection,
      zippo_dbus_default_add_timeout, zippo_dbus_default_remove_timeout,
      zippo_dbus_default_toggle_timeout, self, NULL);

  if (!res) {
    status = -ENOMEM;
    goto err;
  }

  dbus_connection_ref(self->connection);
  return 0;

err:
  dbus_connection_set_timeout_functions(
      self->connection, NULL, NULL, NULL, NULL, NULL);

  dbus_connection_set_watch_functions(
      self->connection, NULL, NULL, NULL, NULL, NULL);

  wl_event_source_remove(self->event_source);
  self->event_source = NULL;
  return status;
}

static void
zippo_dbus_default_unbind(struct zippo_dbus_default* self)
{
  dbus_connection_set_timeout_functions(
      self->connection, NULL, NULL, NULL, NULL, NULL);

  dbus_connection_set_watch_functions(
      self->connection, NULL, NULL, NULL, NULL, NULL);

  dbus_connection_unref(self->connection);

  wl_event_source_remove(self->event_source);
  self->event_source = NULL;
}

static int
zippo_dbus_default_open(struct zippo_dbus* parent, DBusBusType bus)
{
  struct zippo_dbus_default* self = (struct zippo_dbus_default*)parent;
  int res;

  dbus_connection_set_change_sigpipe(FALSE);

  self->connection = dbus_bus_get_private(bus, NULL);
  if (!self->connection) return -EIO;

  dbus_connection_set_exit_on_disconnect(self->connection, FALSE);

  res = zippo_dbus_default_bind(self);
  if (res < 0) goto err;

  return 0;

err:
  dbus_connection_close(self->connection);
  dbus_connection_unref(self->connection);
  self->connection = NULL;
  return res;
}

static void
zippo_dbus_default_close(struct zippo_dbus* parent)
{
  struct zippo_dbus_default* self = (struct zippo_dbus_default*)parent;
  if (self->connection == NULL) return;

  zippo_dbus_default_unbind(self);
  dbus_connection_close(self->connection);
  dbus_connection_unref(self->connection);
  self->connection = NULL;
}

static struct zippo_dbus_interface dbus_default_impl = {
    .open = zippo_dbus_default_open,
    .close = zippo_dbus_default_close,
    .add_filter = zippo_dbus_default_add_filter,
    .add_much_signal = zippo_dbus_default_add_match_signal,
    .send_with_reply_and_block = zippo_dbus_default_send_with_reply_and_block,
    .send = zippo_dbus_default_send,
};

struct zippo_dbus*
zippo_dbus_create(struct wl_display* wl_display)
{
  struct zippo_dbus_default* self;

  self = calloc(1, sizeof *self);
  if (self == NULL) goto err;

  self->base.impl = &dbus_default_impl;
  self->base.wl_display = wl_display;

  return &self->base;

err:
  return NULL;
}

void
zippo_dbus_destroy(struct zippo_dbus* parent)
{
  struct zippo_dbus_default* self = (struct zippo_dbus_default*)parent;

  if (self->connection) self->base.impl->close(&self->base);
  free(self);
}
