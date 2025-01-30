if get_option('gtk-doc')
  xfconf_doc_name = 'xfconf'

  conf_data = configuration_data()
  conf_data.set_quoted('PACKAGE_NAME', xfconf_doc_name)
  conf_data.set_quoted('PACKAGE_VERSION', meson.project_version())
  conf_data.set_quoted('PACKAGE_STRING', '@0@-@1@'.format(xfconf_doc_name, meson.project_version()))

  configure_file(
    input: 'gtkdocentities.ent.in',
    output: 'gtkdocentities.ent',
    configuration: conf_data,
  )

  gnome.gtkdoc(
    xfconf_doc_name,
    src_dir: [
      'common',
      'xfconf',
      'xfconfd',
    ],
    ignore_headers: [
      'xfconf-gdbus-bindings.h',
      'xfconf-visibility.h',
      'xfconf-cache.h',
      'xfconf-private.h',
      'xfconf-backend-factory.h',
      'xfconf-backend-perchannel-xml.h',
      'xfconf-daemon.h',
      'xfconf-lifecycle-manager.h',
      'xfconf-locking-utils.h',
    ],
    main_xml: 'xfconf-docs.xml',
    dependencies: xfconf_dep,
    gobject_typesfile: 'xfconf.types',
    install: true,
    check: false,
  )
endif
