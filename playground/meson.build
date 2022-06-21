playground_single_src_executables = [
  'drm_devices',
  'vt',
]

playground_deps = [
  dep_udev,
]

foreach name : playground_single_src_executables
  executable(
    name,
    ['@0@.c'.format(name)],
    install: false,
    dependencies: playground_deps,
  )
endforeach
