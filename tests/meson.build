# order accounts
dirs = [
  'set-properties',
  'has-properties',
  'get-properties',
  'reset-properties',
  'property-changed-signal',
  'object-bindings',
  'others',
  'list-channels',
  'tests-end',
]

foreach dir : dirs
  sources = run_command('sh', '-c', 'echo @0@'.format(dir) / '*.c', check: true)

  foreach source : sources.stdout().strip().split(' ')
    bin = fs.stem(source)

    e = executable(
      bin,
      sources: source,
      include_directories: [
        include_directories('..'),
      ],
      dependencies: [
        glib,
        gio_unix,
      ],
      link_with: [
        xfconf,
      ],
      install: false,
    )

    test(
      bin,
      files('tests-driver.sh'),
      args: ['xfconfd', e],
      is_parallel: false
    )
  endforeach
endforeach
