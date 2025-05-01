xfconf_libname = 'xfconf'
xfconf_pkgname = '@0@-@1@'.format(xfconf_libname, libxfconf_version_api)

xfconf_install_subdir = 'xfce4' / xfconf_pkgname / xfconf_libname
xfconf_install_dir = get_option('prefix') / get_option('includedir') / xfconf_install_subdir

xfconf_headers = [
  'xfconf-binding.h',
  'xfconf-channel.h',
  'xfconf-errors.h',
  'xfconf-types.h',
  'xfconf.h',
]

xfconf_sources_gir = [
  '..' / 'common' / 'xfconf-types.c',
  'xfconf-binding.c',
  'xfconf-channel.c',
  'xfconf.c',
]

xfconf_sources = xfconf_sources_gir + [
  'xfconf-cache.c',
  'xfconf-cache.h',
  'xfconf-private.h',
]

xfconf_deps = [
  glib,
  gio,
  gio_unix,
  gthread,
  libxfce4util,
]

xfconf_so_version = '3'
xfconf_version = '@0@.0.0'.format(xfconf_so_version)

xfconf = library(
  xfconf_pkgname,
  xfconf_sources,
  sources: xfconf_visibility_sources,
  version: xfconf_version,
  soversion: xfconf_so_version,
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format(xfconf_libname),
    '-DLIBXFCONF_COMPILATION=1',
  ],
  include_directories: [
    '..',
    '..' / 'common',
  ],
  gnu_symbol_visibility: gnu_symbol_visibility,
  dependencies: xfconf_deps,
  link_with: [
    xfconf_common,
    xfconf_gvaluefuncs,
    xfconf_dbus,
  ],
  install: true,
)

install_headers(
  xfconf_headers,
  subdir: xfconf_install_subdir,
)

pkgconfig.generate(
  libraries: [
    xfconf,
  ],
  version: meson.project_version(),
  name: xfconf_libname,
  filebase: 'lib@0@'.format(xfconf_pkgname),
  description: 'Configuration library for Xfce',
  subdirs: [
    'xfce4' / xfconf_pkgname,
  ],
  variables: [
    'libxfconf_api_version=@0@'.format(libxfconf_version_api),
  ],
  install_dir: get_option('prefix') / get_option('libdir') / 'pkgconfig',
)

if get_option('introspection')
  xfconf_gir = gnome.generate_gir(
    xfconf,
    sources: xfconf_sources_gir + xfconf_headers,
    namespace: 'Xfconf',
    nsversion: libxfconf_version_api,
    identifier_prefix: [
      'Xfconf',
    ],
    symbol_prefix: [
      'xfconf_get',
      'xfconf_g',
      'xfconf',
    ],
    export_packages: 'lib@0@'.format(xfconf_pkgname),
    includes: [
      'GLib-2.0',
      'Gio-2.0',
      'Libxfce4util-1.0',
    ],
    header: 'xfconf' / 'xfconf.h',
    install: true,
    extra_args: [
      '-DLIBXFCONF_COMPILATION=1',
    ],
  )

  if generate_vala
    gnome.generate_vapi(
      'lib@0@'.format(xfconf_pkgname),
      install: true,
      packages: [
        'glib-2.0',
        'gio-2.0',
        'libxfce4util-1.0',
      ],
      sources: xfconf_gir[0],
    )
  endif
endif

xfconf_dep = declare_dependency(
  dependencies: xfconf_deps,
  link_with: xfconf,
)
