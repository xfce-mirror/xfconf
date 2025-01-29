executable(
  'xfconf-query',
  [
    'main.c'
  ],
  sources: xfce_revision_h,
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfconf-query'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    glib,
    gio_unix,
    libxfce4util,
  ],
  link_with: [
    xfconf_gvaluefuncs,
    xfconf,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)
