config_h = configure_file(
  input: 'config.h.meson',
  output: 'config.h',
  configuration: cdata,
)

deps_zippo = [
  dep_dbus,
  dep_libdrm,
  dep_pixman,
  dep_systemd,
  dep_udev,
  dep_wayland_client,
  dep_wayland_scanner,
  dep_wayland_server,
]

srcs_zippo = [
  'backend-drm.c',
  'compositor.c',
  'dbus.c',
  'drm-connector.c',
  'drm-crtc.c',
  'drm-format.c',
  'drm-fb.c',
  'drm-head.c',
  'drm-plane.c',
  'drm-writeback.c',
  'head.c',
  'kms.c',
  'launcher-logind.c',
  'main.c',
  'output-drm.c',
  'plane.c',
  'pixman-renderer.c',
  'view.c',
  config_h,
]

executable(
  'zippo',
  srcs_zippo,
  install: false,
  dependencies: deps_zippo,
)
