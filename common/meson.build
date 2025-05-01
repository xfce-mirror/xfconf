xfconf_common_sources = [
  'xfconf-errors.c',
]

xfconf_common_sources += gnome.genmarshal(
  'xfconf-marshal',
  sources: 'xfconf-marshal.list',
  prefix: '_xfconf_marshal',
  internal: true,
  install_header: false,
)

xfconf_visibility_sources = []
xfconf_visibility_sources += configure_file(
  input: '..' / 'xfconf' / 'xfconf.symbols',
  output: 'xfconf-visibility.h',
  command: [xdt_gen_visibility, '--kind=header', '@INPUT@', '@OUTPUT@'],
)
xfconf_visibility_sources += configure_file(
  input: '..' / 'xfconf' / 'xfconf.symbols',
  output: 'xfconf-visibility.c',
  command: [xdt_gen_visibility, '--kind=source', '@INPUT@', '@OUTPUT@'],
)

xfconf_common = static_library(
  'xfconf-common',
  xfconf_common_sources,
  sources: xfconf_visibility_sources,
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfconf-common'),
    '-DLIBXFCONF_COMPILATION=1',
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    glib,
  ],
  gnu_symbol_visibility: gnu_symbol_visibility,
  install: false,
)

xfconf_gvaluefuncs = static_library(
  'xfconf-gvaluefuncs',
  [
    'xfconf-gvaluefuncs.c',
    'xfconf-gvaluefuncs.h',
  ],
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfconf-common'),
    '-DLIBXFCONF_COMPILATION=1',
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    glib,
    gio_unix,
  ],
  gnu_symbol_visibility: gnu_symbol_visibility,
  install: false,
)

xfconf_dbus_xml = configure_file(
  configuration: configuration_data({
    'XFCONF_SERVICE_NAME_PREFIX': xfconf_service_name_prefix,
    'XFCONF_SERVICE_PATH_PREFIX': xfconf_service_path_prefix,
  }),
  input: 'xfconf-dbus.xml.in',
  output: 'xfconf-dbus.xml',
)

xfconf_dbus = static_library(
  'xfconf-dbus',
  gnome.gdbus_codegen(
    'xfconf-gdbus-bindings',
    sources: xfconf_dbus_xml,
    interface_prefix: '@0@.Xfconf'.format(xfconf_service_name_prefix),
    namespace: 'Xfconf',
    annotations: ['@0@.Xfconf'.format(xfconf_service_name_prefix), 'org.gtk.GDBus.C.Name', 'Exported'],
    install_header: false,
  ),
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfconf-common'),
  ],
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [
    gio_unix,
  ],
  gnu_symbol_visibility: gnu_symbol_visibility,
  install: false,
)
