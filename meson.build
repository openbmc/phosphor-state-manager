project('phosphor-state-manager', 'cpp',
        version: '0.1', meson_version: '>=0.49.0',
        default_options: [
          'warning_level=3',
          'werror=true',
          'cpp_std=c++17'
        ])

phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
phosphor_logging = dependency('phosphor-logging')
sdbusplus = dependency('sdbusplus')
sdeventplus = dependency('sdeventplus')

systemd = dependency('systemd')
servicedir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

conf = configuration_data()
conf.set_quoted(
    'HOST_BUSNAME', get_option('HOST_BUSNAME'))
conf.set_quoted(
    'HOST_OBJPATH', get_option('HOST_OBJPATH'))
conf.set_quoted(
    'CHASSIS_BUSNAME', get_option('CHASSIS_BUSNAME'))   
conf.set_quoted(
    'CHASSIS_OBJPATH', get_option('CHASSIS_OBJPATH'))
conf.set_quoted(
    'BMC_BUSNAME', get_option('BMC_BUSNAME'))
conf.set_quoted(
    'BMC_OBJPATH', get_option('BMC_OBJPATH'))
conf.set_quoted(
    'HOST_RUNNING_FILE', get_option('HOST_RUNNING_FILE'))   
conf.set_quoted(
    'HOST_STATE_PERSIST_PATH', get_option('HOST_STATE_PERSIST_PATH'))
conf.set_quoted(
    'POH_COUNTER_PERSIST_PATH', get_option('POH_COUNTER_PERSIST_PATH'))
conf.set_quoted(
    'CHASSIS_STATE_CHANGE_PERSIST_PATH', get_option('CHASSIS_STATE_CHANGE_PERSIST_PATH'))
conf.set(
    'BOOT_COUNT_MAX_ALLOWED', get_option('BOOT_COUNT_MAX_ALLOWED'))
conf.set(
    'CLASS_VERSION', get_option('CLASS_VERSION'))

configure_file(output: 'config.h', configuration: conf)
   
state_inc = include_directories('.')

add_project_link_arguments(['-lstdc++fs'], language: 'cpp')

executable(
  'phosphor-host-state-manager',
  'host_state_manager.cpp',
  'host_state_manager_main.cpp',
  'settings.cpp',
  include_directories: state_inc,
  dependencies: [
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
    ],
  install: true)

executable(
  'phosphor-chassis-state-manager',
  'chassis_state_manager.cpp',
  'chassis_state_manager_main.cpp',
  include_directories: state_inc,
  dependencies: [
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
    ],
  install: true)

executable(
  'phosphor-bmc-state-manager',
  'bmc_state_manager.cpp',
  'bmc_state_manager_main.cpp',
  include_directories: state_inc,
  dependencies: [
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
    ],
  install: true)

executable(
  'phosphor-discover-system-state',
  'discover_system_state.cpp',
  'settings.cpp',
  include_directories: state_inc,
  dependencies: [
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
    ],
  install: true)

executable(
  'phosphor-host-check',
  'host_check_main.cpp',
  include_directories: state_inc,
  dependencies: [
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
    ],
  install: true)

executable(
  'phosphor-systemd-target-monitor',
  'systemd_target_monitor.cpp',
  'systemd_target_parser.cpp',
  'systemd_target_signal.cpp',
  include_directories: state_inc,
  dependencies: [
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
        sdeventplus,
    ],
  install: true)

build_tests = get_option('tests')
  
subdir('test')

