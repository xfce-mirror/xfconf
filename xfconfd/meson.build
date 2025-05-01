xfconfd_sources = [
  '..' / 'common' / 'xfconf-types.c',
  'main.c',
  'xfconf-backend-factory.c',
  'xfconf-backend-factory.h',
  'xfconf-backend.c',
  'xfconf-backend.h',
  'xfconf-daemon.c',
  'xfconf-daemon.h',
  'xfconf-lifecycle-manager.c',
  'xfconf-lifecycle-manager.h',
  'xfconf-locking-utils.c',
  'xfconf-locking-utils.h',
]

if build_xfconf_backend_perchannel_xml
xfconfd_sources += [
  'xfconf-backend-perchannel-xml.c',
  'xfconf-backend-perchannel-xml.h',
]
endif

configure_file(
  configuration: configuration_data({
    'HELPERDIR': helper_path_prefix,
    'XFCONF_SERVICE_NAME_PREFIX': xfconf_service_name_prefix,
  }),
  input: 'xfconfd.service.in',
  output: 'xfconfd.service',
  install: true,
  install_dir: get_option('prefix') / 'lib' / 'systemd' / 'user'
)

configure_file(
  configuration: configuration_data({
    'HELPERDIR': helper_path_prefix,
    'XFCONF_SERVICE_NAME_PREFIX': xfconf_service_name_prefix,
  }),
  input: 'org.xfce.Xfconf.service.in',
  output: '@0@.Xfconf.service'.format(xfconf_service_name_prefix),
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / 'dbus-1' / 'services'
)

c_args = [
  '-DG_LOG_DOMAIN="@0@"'.format('xfconfd'),
  '-DLIBXFCONF_COMPILATION=1',
]
if get_option('profiling')
  c_args += '-pg'
endif

executable(
  'xfconfd',
  xfconfd_sources,
  sources: [
    xfce_revision_h,
    xfconf_visibility_sources,
  ],
  c_args: c_args,
  include_directories: [
    '..',
    '..' / 'common',
  ],
  dependencies: [
    glib,
    gio_unix,
    gthread,
    libxfce4util,
  ],
  link_with: [
    xfconf_common,
    xfconf_gvaluefuncs,
    xfconf_dbus,
  ],
  install: true,
  install_dir: helper_path_prefix / 'xfce4' / 'xfconf',
)
