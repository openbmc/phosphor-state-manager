oe_sdk = get_option('oe-sdk')
if oe_sdk.enabled()
  # Setup OE SYSROOT
  OECORE_TARGET_SYSROOT = run_command('sh', '-c', 'echo $OECORE_TARGET_SYSROOT').stdout().strip()
  if OECORE_TARGET_SYSROOT == ''
      error('Unable to get $OECORE_TARGET_SYSROOT, check your environment.')
  endif
  message('OE_SYSROOT: ' + OECORE_TARGET_SYSROOT)
  rpath = ':'.join([OECORE_TARGET_SYSROOT + '/lib', OECORE_TARGET_SYSROOT + '/usr/lib'])
  ld_so = run_command('sh', '-c', 'find ' + OECORE_TARGET_SYSROOT + '/lib/ld-*.so | sort -r -n | head -n1').stdout().strip()
  dynamic_linker = ['-Wl,-dynamic-linker,' + ld_so]
else
  dynamic_linker = []
endif

gtest = dependency('gtest', main: true, disabler: true, required: build_tests)
gmock = dependency('gmock', disabler: true, required: build_tests)

test(
    'systemd_parser',
    executable(
        'systemd_parser',
        'systemd_parser.cpp',
        '../systemd_target_parser.cpp',
        link_args: dynamic_linker,
        build_rpath: oe_sdk.enabled() ? rpath : '',
        dependencies: [
            gmock,
            gtest,
            phosphor_logging,
            phosphor_dbus_interfaces,
            sdbusplus,
            sdeventplus,
        ],
        include_directories: '..',
    )
)

test(
    'systemd_signal',
    executable(
        'systemd_signal',
        'systemd_signal.cpp',
        '../systemd_target_signal.cpp',
        link_args: dynamic_linker,
        build_rpath: oe_sdk.enabled() ? rpath : '',
        dependencies: [
            gmock,
            gtest,
            phosphor_logging,
            phosphor_dbus_interfaces,
            sdbusplus,
            sdeventplus,
        ],
        include_directories: '..',
    )
)
