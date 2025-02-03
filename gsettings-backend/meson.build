if get_option('gsettings-backend')
  shared_module(
    'xfconfgsettingsbackend',
    [
      'xfconf-giomodule.c',
      'xfconf-gsettings-backend.c',
      'xfconf-gsettings-backend.h',
    ],
    gnu_symbol_visibility: 'hidden',
    c_args: [
      '-DG_LOG_DOMAIN="@0@"'.format('gsettings-backend'),
    ],
    include_directories: [
      include_directories('..'),
    ],
    dependencies: [
      gio,
      gmodule,
    ],
    link_with: [
      xfconf_gvaluefuncs,
      xfconf,
    ],
    install: true,
    install_dir: gio.get_variable(
      pkgconfig: 'giomoduledir',
      pkgconfig_define: ['libdir', get_option('prefix') / get_option('libdir')]
    ),
  )
endif
