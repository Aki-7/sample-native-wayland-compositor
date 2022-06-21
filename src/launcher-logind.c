#define _GNU_SOURCE

#include <dbus/dbus.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <systemd/sd-login.h>
#include <unistd.h>

#include "dbus.h"
#include "launcher.h"

struct zippo_launcher_logind {
  struct zippo_launcher base;
  char* sid;           // set when connected
  char* seat;          // set when connected
  unsigned int vt_nr;  // set when connected
  struct zippo_dbus* dbus;
  char* session_path;
};

static int
zippo_launcher_logind_take_control(struct zippo_launcher_logind* self)
{
  DBusError err;
  DBusMessage *message, *reply;
  dbus_bool_t force;
  bool res_b;
  int res;

  dbus_error_init(&err);

  message = dbus_message_new_method_call("org.freedesktop.login1",
      self->session_path, "org.freedesktop.login1.Session", "TakeControl");

  if (!message) {
    res = -ENOMEM;
    goto err_message;
  }

  force = false;

  res_b = dbus_message_append_args(
      message, DBUS_TYPE_BOOLEAN, &force, DBUS_TYPE_INVALID);

  if (!res_b) {
    res = -ENOMEM;
    goto err_append_args;
  }

  reply = self->dbus->impl->send_with_reply_and_block(
      self->dbus, message, -1, &err);

  if (!reply) {
    if (dbus_error_has_name(&err, DBUS_ERROR_UNKNOWN_METHOD))
      fprintf(stderr, "logind: old systemd version detected\n");
    else
      fprintf(
          stderr, "logind: cannot take control over session %s\n", self->sid);

    dbus_error_free(&err);
    res = -EIO;
    goto err_append_args;
  }

  dbus_message_unref(reply);
  dbus_message_unref(message);

  return 0;

err_append_args:
  dbus_message_unref(message);

err_message:
  return res;
}

static void
zippo_launcher_logind_release_control(struct zippo_launcher_logind* self)
{
  DBusMessage* message;

  message = dbus_message_new_method_call("org.freedesktop.login1",
      self->session_path, "org.freedesktop.login1.Session", "ReleaseControl");

  if (message) {
    self->dbus->impl->send(self->dbus, message, NULL);
    dbus_message_unref(message);
  }
}

static DBusHandlerResult
zippo_launcher_logind_filter_dbus(
    DBusConnection* c, DBusMessage* m, void* user_data)
{
  struct zippo_launcher_logind* self = (struct zippo_launcher_logind*)user_data;
  (void)self;
  (void)c;

  if (dbus_message_is_signal(m, DBUS_INTERFACE_LOCAL, "Disconnected")) {
    fprintf(stderr, "DBus: disconnected\n");
  } else if (dbus_message_is_signal(
                 m, "org.freedesktop.login1.Manager", "SessionRemoved")) {
    fprintf(stderr, "DBus: session removed\n");
  } else if (dbus_message_is_signal(
                 m, "org.freedesktop.DBus.Properties", "PropertiesChanged")) {
    fprintf(stderr, "DBus: properties changed\n");
  } else if (dbus_message_is_signal(
                 m, "org.freedesktop.login1.Session", "PauseDevice")) {
    fprintf(stderr, "DBus: pause Device\n");
  } else if (dbus_message_is_signal(
                 m, "org.freedesktop.login1.Session", "ResumeDevice")) {
    fprintf(stderr, "DBus: resume Device\n");
  }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static int
zippo_launcher_logind_setup_dbus(struct zippo_launcher_logind* self)
{
  int res;
  bool res_b;

  res = asprintf(
      &self->session_path, "/org/freedesktop/login1/session/%s", self->sid);
  if (res < 0) return -ENOMEM;

  res_b = self->dbus->impl->add_filter(
      self->dbus, zippo_launcher_logind_filter_dbus, self, NULL);

  if (!res_b) {
    fprintf(stderr, "logind: cannot add dbus filters\n");
    res = -ENOMEM;
    goto err_filter;
  }

  res = self->dbus->impl->add_much_signal(self->dbus, "org.freedesktop.login1",
      "org.freedesktop.login1.Manager", "SessionRemoved",
      "/org/freedesktop/login1");

  if (res < 0) goto err_add_match;

  res = self->dbus->impl->add_much_signal(self->dbus, "org.freedesktop.login1",
      "org.freedesktop.DBus.Properties", "PropertiesChanged",
      self->session_path);

  if (res < 0) goto err_add_match;

  res = self->dbus->impl->add_much_signal(self->dbus, "org.freedesktop.login1",
      "org.freedesktop.login1.Session", "PauseDevice", self->session_path);

  if (res < 0) goto err_add_match;

  res = self->dbus->impl->add_much_signal(self->dbus, "org.freedesktop.login1",
      "org.freedesktop.login1.Session", "ResumeDevice", self->session_path);

  if (res < 0) goto err_add_match;

  return 0;

err_add_match:
  fprintf(stderr, "logind: cannot add dbus match\n");

err_filter:
  free(self->session_path);
  self->session_path = NULL;
  return res;
}

static int
zippo_launcher_logind_activate(struct zippo_launcher_logind* self)
{
  DBusMessage* message;

  message = dbus_message_new_method_call("org.freedesktop.login1",
      self->session_path, "org.freedesktop.login1.Session", "Activate");

  if (!message) return -ENOMEM;

  self->dbus->impl->send(self->dbus, message, NULL);
  return 0;
}

static int
zippo_launcher_logind_connect(struct zippo_launcher* parent)
{
  struct zippo_launcher_logind* self = (struct zippo_launcher_logind*)parent;
  int res;

  res = sd_pid_get_session(getpid(), &self->sid);
  if (res < 0) {
    if (-res == ENODATA) {
      fprintf(stderr, "logind: not running in a systemd session\n");
    } else {
      fprintf(
          stderr, "logind: failed to get pid session: %s\n", strerror(-res));
    }

    goto err_session;
  }

  res = sd_session_get_seat(self->sid, &self->seat);
  if (res < 0) {
    fprintf(stderr, "logind: failed to get session seat\n");
    goto err_seat;
  }

  res = sd_seat_can_tty(self->seat);
  if (res > 0) {  // yes
    res = sd_session_get_vt(self->sid, &self->vt_nr);
    if (res < 0) {
      fprintf(stderr, "logind: session not running on a VT\n");
      goto err_vt;
    }
  } else if (res < 0) {  // failure
    fprintf(stderr, "logind: could not determine if seat %s has ttys or not",
        self->seat);
    goto err_vt;
  }

  fprintf(stderr, "vt: %d\n", self->vt_nr);

  if (self->dbus->impl->open(self->dbus, DBUS_BUS_SYSTEM) != 0) {
    fprintf(stderr, "logind: cannot connect to system dbus\n");
    goto err_vt;
  }

  res = zippo_launcher_logind_setup_dbus(self);
  if (res < 0) goto err_setup_dbus;

  res = zippo_launcher_logind_take_control(self);
  if (res < 0) goto err_take_control;

  res = zippo_launcher_logind_activate(self);
  if (res < 0) goto err_take_control;

  return 0;

err_take_control:
  free(self->session_path);

err_setup_dbus:
  self->dbus->impl->close(self->dbus);

err_vt:
  free(self->seat);
  self->seat = NULL;

err_seat:
  free(self->sid);
  self->sid = NULL;

err_session:
  return -1;
}

static int
zippo_launcher_logind_take_device(struct zippo_launcher_logind* self,
    uint32_t major, uint32_t minor, bool* paused_out)
{
  DBusMessage *message, *reply;
  bool res_b;
  int res, fd;
  dbus_bool_t paused;

  message = dbus_message_new_method_call("org.freedesktop.login1",
      self->session_path, "org.freedesktop.login1.Session", "TakeDevice");

  if (!message) return -ENOMEM;

  res_b = dbus_message_append_args(message, DBUS_TYPE_UINT32, &major,
      DBUS_TYPE_UINT32, &minor, DBUS_TYPE_INVALID);

  if (!res_b) {
    res = -ENOMEM;
    goto err_unref;
  }

  reply = self->dbus->impl->send_with_reply_and_block(
      self->dbus, message, -1, NULL);

  if (!reply) {
    fprintf(stderr, "logind: TakeDevice on %d:%d failed.\n", major, minor);
    res = -ENODEV;
    goto err_unref;
  }

  res_b = dbus_message_get_args(reply, NULL, DBUS_TYPE_UNIX_FD, &fd,
      DBUS_TYPE_BOOLEAN, &paused, DBUS_TYPE_INVALID);

  if (!res_b) {
    fprintf(stderr, "logind: Error parsing reply to TakeDevice.\n");
    res = -ENODEV;
    goto err_reply;
  }

  res = fd;
  if (paused_out) *paused_out = paused;

err_reply:
  dbus_message_unref(reply);

err_unref:
  dbus_message_unref(message);
  return res;
}

static void
zippo_launcher_logind_release_device(
    struct zippo_launcher_logind* self, uint32_t major, uint32_t minor)
{
  DBusMessage* message;
  bool res_b;

  message = dbus_message_new_method_call("org.freedesktop.login1",
      self->session_path, "org.freedesktop.login1.Session", "ReleaseDevice");

  if (message) {
    res_b = dbus_message_append_args(message, DBUS_TYPE_UINT32, &major,
        DBUS_TYPE_UINT32, &minor, DBUS_TYPE_INVALID);

    if (res_b) self->dbus->impl->send(self->dbus, message, NULL);
    dbus_message_unref(message);
  }
}

static int
zippo_launcher_logind_open(
    struct zippo_launcher* parent, const char* path, int flags)
{
  struct zippo_launcher_logind* self = wl_container_of(parent, self, base);
  struct stat st;
  int fl, res, fd;

  res = stat(path, &st);
  if (res < 0) {
    fprintf(
        stderr, "logind: connot stat: %s! error=%s\n", path, strerror(errno));
    return -1;
  }

  if (!S_ISCHR(st.st_mode)) {
    fprintf(stderr, "logind: %s is not a character special file\n", path);
    errno = ENODEV;
    return -1;
  }

  fd = zippo_launcher_logind_take_device(
      self, major(st.st_rdev), minor(st.st_rdev), NULL);
  if (fd < 0) {
    fprintf(stderr, "TakeDevice on %s failed, error=%s\n", path, strerror(-fd));
    errno = -fd;
    return -1;
  }

  fl = fcntl(fd, F_GETFL);
  if (fl < 0) {
    res = -errno;
    fprintf(stderr, "logind: cannot get file flags: %s\n", strerror(errno));
    goto err_close;
  }

  if (flags & O_NONBLOCK) fl |= O_NONBLOCK;

  res = fcntl(fd, F_SETFL, fl);
  if (res < 0) {
    res = -errno;
    fprintf(stderr, "logind: cannot set O_NONBLOCK: %s\n", strerror(errno));
    goto err_close;
  }

  return fd;

err_close:
  close(fd);
  zippo_launcher_logind_release_device(
      self, major(st.st_rdev), minor(st.st_rdev));
  errno = -res;
  return -1;
}

static void
zippo_launcher_logind_close(struct zippo_launcher* parent, int fd)
{
  struct zippo_launcher_logind* self = wl_container_of(parent, self, base);
  struct stat st;
  int res;

  res = fstat(fd, &st);
  close(fd);

  if (res < 0) {
    fprintf(stderr, "logind: cannot fstat fd: %s\n", strerror(errno));
    return;
  }

  if (!S_ISCHR(st.st_mode)) {
    fprintf(stderr, "logind: invalid device passed\n");
    return;
  }

  zippo_launcher_logind_release_device(
      self, major(st.st_rdev), minor(st.st_rdev));
}

static struct zippo_launcher_interface const launcher_logind_impl = {
    .connect = zippo_launcher_logind_connect,
    .open = zippo_launcher_logind_open,
    .close = zippo_launcher_logind_close,
};

struct zippo_launcher*
zippo_launcher_create(struct wl_display* wl_display)
{
  struct zippo_launcher_logind* self;
  struct zippo_dbus* dbus;

  self = calloc(1, sizeof *self);
  if (self == NULL) goto err;

  dbus = zippo_dbus_create(wl_display);
  if (dbus == NULL) goto err_dbus;

  self->base.impl = &launcher_logind_impl;
  self->base.wl_display = wl_display;
  self->dbus = dbus;
  wl_signal_init(&self->base.session_signal);

  return &self->base;

err_dbus:
  free(dbus);

err:
  return NULL;
}

void
zippo_launcher_destroy(struct zippo_launcher* parent)
{
  struct zippo_launcher_logind* self = (struct zippo_launcher_logind*)parent;

  zippo_launcher_logind_release_control(self);
  self->dbus->impl->close(self->dbus);

  free(self->session_path);
  free(self->seat);
  free(self->sid);
  zippo_dbus_destroy(self->dbus);
  free(self);
}
