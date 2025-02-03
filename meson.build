project(
  'xfconf',
  'c',
version : '4.20.0-dev',
  license : 'GPL-2',
  meson_version : '>= 0.54.0',
  default_options : ['c_std=gnu11', 'buildtype=debugoptimized', 'warning_level=2']
)

project_namespace = 'xfce'
pkgdatadir = get_option('prefix') / get_option('datadir') / meson.project_name()
libxfconf_version_api = '0'

cc = meson.get_compiler('c')
pkgconfig = import('pkgconfig')
gnome = import('gnome')
i18n = import('i18n')
fs = import('fs')

dependency_versions = {
  'glib': '>= 2.72.0',
  'libxfce4util': '>= 4.17.3',

  'gir': '>= 1.72.0',
}

glib = dependency('glib-2.0', version: dependency_versions['glib'])
gio = dependency('gio-2.0', version: dependency_versions['glib'])
gio_unix = dependency('gio-unix-2.0', version: dependency_versions['glib'])
gmodule = dependency('gmodule-2.0', version: dependency_versions['glib'])
gthread = dependency('gthread-2.0', version: dependency_versions['glib'])
libxfce4util = dependency('libxfce4util-1.0', version: dependency_versions['libxfce4util'])

xdt_gen_visibility = find_program('xdt-gen-visibility', required: true)

gir = dependency('gobject-introspection-1.0', version: dependency_versions['gir'], required: get_option('introspection'))

vapigen = find_program('vapigen', required: get_option('vala'))
if get_option('vala').enabled()
  assert(get_option('introspection'), 'Building Vala bindings requires building GObject introspection data')
endif
generate_vala = not get_option('vala').disabled() and vapigen.found()

feature_cflags = []

gnu_symbol_visibility = 'default'
if get_option('visibility')
  gnu_symbol_visibility = 'hidden'
  feature_cflags += '-DENABLE_SYMBOL_VISIBILITY=1'
  feature_cflags += '-DXFCONF_EXPORT=__attribute__((visibility("default")))'
else
  feature_cflags += '-DXFCONF_EXPORT='
endif

helper_path_prefix = get_option('helper-path-prefix')
if helper_path_prefix == ''
  helper_path_prefix = get_option('prefix') / get_option('libdir')
endif

xfconf_service_name_prefix = get_option('service-name-prefix')
if xfconf_service_name_prefix == ''
  xfconf_service_name_prefix = 'org.xfce'
endif
xfconf_service_path_prefix = '/@0@'.format('/'.join(xfconf_service_name_prefix.split('.')))
feature_cflags += '-DXFCONF_SERVICE_NAME_PREFIX="@0@"'.format(xfconf_service_name_prefix)
feature_cflags += '-DXFCONF_SERVICE_PATH_PREFIX="@0@"'.format(xfconf_service_path_prefix)

bash_completion_dir = get_option('bash-completion-dir')
if bash_completion_dir == ''
  bash_completion = dependency('bash-completion', required: false)
  if bash_completion.found()
    bash_completion_dir = bash_completion.get_variable(
      pkgconfig: 'completionsdir',
      pkgconfig_define: ['datadir', get_option('prefix') / get_option('datadir')]
    )
  else
    bash_completion_dir = get_option('prefix') / get_option('datadir') / 'bash-completion' / 'completions'
  endif
endif

# could be an option some day, or something else: see comment in xfconfd/xfconf-backend-factory.c
build_xfconf_backend_perchannel_xml = true
feature_cflags += '-DBUILD_XFCONF_BACKEND_PERCHANNEL_XML=1'

if get_option('runtime-checks')
  feature_cflags += '-DXFCONF_ENABLE_CHECKS=1'
endif

types = [
  'guchar',
  'gchar',
  'guint16',
  'gint16',
  'guint32',
  'gint32',
  'guint64',
  'gint64',
  'gfloat',
  'gdouble',
  'gboolean',
  'gpointer',
]
foreach type : types
  feature_cflags += '-DALIGNOF_@0@=@1@'.format(
    type.to_upper(),
    cc.alignment(
      type,
      dependencies: glib,
      prefix: '#include <glib.h>'
    )
  )
endforeach

headers = [
  'sys/stat.h',
  'sys/types.h',
  'sys/wait.h',
  'errno.h',
  'fcntl.h',
  'grp.h',
  'signal.h',
  'stdlib.h',
  'string.h',
  'unistd.h',
]
foreach header : headers
  if cc.check_header(header)
    feature_cflags += '-DHAVE_@0@=1'.format(header.underscorify().to_upper())
  endif
endforeach

extra_cflags = []
extra_cflags_check = [
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wold-style-definition',
  '-Wredundant-decls',
  '-Wpointer-arith',
  '-Wcast-align',
  '-Winit-self',
  '-Wshadow',
  '-Wmissing-include-dirs',
  '-Wundef',
  '-Wformat',
  '-Wformat-security',
  '-Wformat-y2k',
  '-Wnested-externs',
  '-Wno-unused-parameter',
  '-Wno-declaration-after-statement',
  '-Wno-missing-field-initializers',
  '-Werror=implicit-function-declaration',
  '-Wno-error=deprecated-declarations',
]

optimization = get_option('optimization')
if get_option('debug') and optimization in ['0', 'g']
  extra_cflags_check += '-fstack-protector-strong'
  extra_cflags += [
    '-DDEBUG=1',
    '-DDEBUG_TRACE=1',
    '-DG_ENABLE_DEBUG',
  ]
elif optimization in ['3', 'minsize']
  extra_cflags += [
    '-DNDEBUG',
    '-DG_DISABLE_CAST_CHECKS',
    '-DG_DISABLE_ASSERT',
  ]
endif

if dependency_versions.has_key('glib')
  glib_version_parts = dependency_versions['glib'].split(' ')
  glib_min_version_parts = glib_version_parts[1].split('.')
  glib_min_version_define = 'GLIB_VERSION_@0@_@1@'.format(glib_min_version_parts[0], glib_min_version_parts[1])
  extra_cflags += [
    '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_min_version_define),
    '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_min_version_define),
    '-DG_LOG_USE_STRUCTURED=1',
  ]
endif

version_parts = meson.project_version().split('-dev')[0].split('.')
version_short = '@0@.@1@'.format(version_parts[0], version_parts[1])

extra_cflags += [
  '-DPACKAGE="@0@"'.format(meson.project_name()),
  '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
  '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
  '-DVERSION="@0@"'.format(meson.project_version()),
  '-DVERSION_SHORT="@0@"'.format(version_short),
  '-DPACKAGE_STRING="@0@ @1@"'.format(meson.project_name(), meson.project_version()),
  '-DPACKAGE_DATADIR="@0@"'.format(pkgdatadir),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DPACKAGE_BUGREPORT="https://gitlab.xfce.org/@0@/@1@/-/issues"'.format(project_namespace, meson.project_name()),
  '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
  '-DPREFIX="@0@"'.format(get_option('prefix')),
  '-DBINDIR="@0@"'.format(get_option('prefix') / get_option('bindir')),
  '-DDATADIR="@0@"'.format(get_option('prefix') / get_option('datadir')),
  '-DINCLUDEDIR="@0@"'.format(get_option('prefix') / get_option('includedir')),
  '-DLIBDIR="@0@"'.format(get_option('prefix') / get_option('libdir')),
  '-DLIBEXECDIR="@0@"'.format(get_option('prefix') / get_option('libexecdir')),
  '-DLOCALEDIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DLOCALSTATEDIR="@0@"'.format(get_option('prefix') / get_option('localstatedir')),
  '-DSBINDIR="@0@"'.format(get_option('prefix') / get_option('sbindir')),
  '-DSYSCONFDIR="@0@"'.format(get_option('prefix') / get_option('sysconfdir')),
  '-DHAVE_XFCE_REVISION_H=1',
]

add_project_arguments(cc.get_supported_arguments(extra_cflags_check), language: 'c')
add_project_arguments(extra_cflags, language: 'c')
add_project_arguments(feature_cflags, language: 'c')

xfce_revision_h = vcs_tag(
  command: ['git', 'rev-parse', '--short', 'HEAD'],
  fallback: 'UNKNOWN',
  input: 'xfce-revision.h.in',
  output: 'xfce-revision.h',
  replace_string: '@REVISION@',
)

subdir('common')
subdir('completions')
subdir('xfconf')
subdir('docs' / 'reference')
subdir('gsettings-backend')
subdir('po')
subdir('tests')
subdir('xfconf-query')
subdir('xfconfd')
