project(
  'sample-native-wayland-compositor', 'c',
  version: '0.0.1',
  license: 'Apache-2.0',
  meson_version: '>= 0.52.1',
  default_options: [ 'warning_level=3', 'werror=true', 'optimization=2', 'c_std=gnu11' ],
)

cc = meson.get_compiler('c')

# generic version requirements

req_dbus = '>= 1.6'
req_libdrm = '>= 2.4.95'
req_pixman = '>= 0.25.2'
req_systemd = '>= 209'
req_wayland_server = '>= 1.18.0'
req_wayland_client = '>= 1.18.0'
req_wayland_scanner = '>= 1.14.91'

# dependencies

dep_dbus = dependency('dbus-1', version: req_dbus)
dep_libdrm = dependency('libdrm', version: req_libdrm)
dep_pam = cc.find_library('pam')
dep_pixman = dependency('pixman-1', version: req_pixman)
dep_systemd = dependency('libsystemd', version: req_systemd)
dep_udev = dependency('libudev')
dep_wayland_server = dependency('wayland-server', version: req_wayland_server)
dep_wayland_client = dependency('wayland-client', version: req_wayland_client)
dep_wayland_scanner = dependency('wayland-scanner', version: req_wayland_scanner)

# config.h

cdata = configuration_data()
cdata.set_quoted('VERSION', meson.project_version())

subdir('playground')
subdir('src')
subdir('launcher')
