#ifndef ZIPPO_DBUS_H
#define ZIPPO_DBUS_H

#include <dbus/dbus.h>
#include <stdbool.h>
#include <wayland-server.h>

struct zippo_dbus;

struct zippo_dbus_interface {
  int (*open)(struct zippo_dbus* self, DBusBusType bus);
  void (*close)(struct zippo_dbus* self);
  bool (*add_filter)(struct zippo_dbus* self,
      DBusHandleMessageFunction function, void* user_data,
      DBusFreeFunction free_data_function);
  int (*add_much_signal)(struct zippo_dbus* self, const char* sender,
      const char* iface, const char* member, const char* path);
  DBusMessage* (*send_with_reply_and_block)(struct zippo_dbus* parent,
      DBusMessage* message, int timeout_milliseconds, DBusError* error);
  bool (*send)(struct zippo_dbus* parent, DBusMessage* message,
      dbus_uint32_t* client_serial);
};

struct zippo_dbus {
  struct zippo_dbus_interface const* impl;

  struct wl_display* wl_display;
};

struct zippo_dbus* zippo_dbus_create(struct wl_display* wl_display);

void zippo_dbus_destroy(struct zippo_dbus* self);

#endif  //  ZIPPO_DBUS_H
