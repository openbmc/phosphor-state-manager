project(
    'phosphor-state-manager',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++17'
    ],
    license: 'Apache-2.0',
    version: '0.1',
)

conf = configuration_data()
conf.set_quoted(
    'HOST_BUSNAME', get_option('host-busname'))
conf.set_quoted(
    'HOST_OBJPATH', get_option('host-objpath'))
conf.set_quoted(
    'CHASSIS_BUSNAME', get_option('chassis-busname'))
conf.set_quoted(
    'CHASSIS_OBJPATH', get_option('chassis-objpath'))
conf.set_quoted(
    'BMC_BUSNAME', get_option('bmc-busname'))
conf.set_quoted(
    'BMC_OBJPATH', get_option('bmc-objpath'))
conf.set_quoted(
    'HOST_RUNNING_FILE', get_option('host-running-file'))
conf.set_quoted(
    'HOST_STATE_PERSIST_PATH', get_option('host-state-persist-path'))
conf.set_quoted(
    'POH_COUNTER_PERSIST_PATH', get_option('poh-counter-persist-path'))
conf.set_quoted(
    'CHASSIS_STATE_CHANGE_PERSIST_PATH', get_option('chassis-state-change-persist-path'))
conf.set(
    'BOOT_COUNT_MAX_ALLOWED', get_option('boot-count-max-allowed'))
conf.set(
    'CLASS_VERSION', get_option('class-version'))

configure_file(output: 'config.h', configuration: conf)

sdbusplus = dependency('sdbusplus')
sdeventplus = dependency('sdeventplus')
phosphorlogging = dependency('phosphor-logging')
phosphordbusinterfaces = dependency('phosphor-dbus-interfaces')

cppfs = meson.get_compiler('cpp').find_library('stdc++fs')

executable('phosphor-host-state-manager',
            'host_state_manager.cpp',
            'host_state_manager_main.cpp',
            'settings.cpp',
            dependencies: [
            sdbusplus, sdeventplus, phosphorlogging,
            phosphordbusinterfaces, cppfs
            ],
    implicit_include_directories: true,
    install: true
)

executable('phosphor-chassis-state-manager',
            'chassis_state_manager.cpp',
            'chassis_state_manager_main.cpp',
            dependencies: [
            sdbusplus, sdeventplus, phosphorlogging,
            phosphordbusinterfaces, cppfs
            ],
    implicit_include_directories: true,
    install: true
)

executable('phosphor-bmc-state-manager',
            'bmc_state_manager.cpp',
            'bmc_state_manager_main.cpp',
            dependencies: [
            sdbusplus, sdeventplus, phosphorlogging,
            phosphordbusinterfaces, cppfs
            ],
    implicit_include_directories: true,
    install: true
)

executable('phosphor-discover-system-state',
            'discover_system_state.cpp',
            'settings.cpp',
            dependencies: [
            sdbusplus, phosphorlogging
            ],
    implicit_include_directories: true,
    install: true
)

executable('phosphor-host-check',
            'host_check_main.cpp',
            dependencies: [
            sdbusplus, phosphorlogging
            ],
    implicit_include_directories: true,
    install: true
)

executable('phosphor-systemd-target-monitor',
            'systemd_target_monitor.cpp',
            'systemd_target_parser.cpp',
            'systemd_target_signal.cpp',
            dependencies: [
            sdbusplus, sdeventplus, phosphorlogging
            ],
    implicit_include_directories: true,
    install: true
)
