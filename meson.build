project(
    'phosphor-state-manager',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++20'
    ],
    meson_version: '>= 0.57.0',
    license: 'Apache-2.0',
    version: '0.1',
)
cpp = meson.get_compiler('cpp')

build_host_gpios = get_option('host-gpios')

conf = configuration_data()
conf.set_quoted(
    'HOST_BUSNAME', get_option('host-busname'))
conf.set_quoted(
    'HOST_OBJPATH', get_option('host-objpath'))
conf.set_quoted(
    'HYPERVISOR_BUSNAME', get_option('hypervisor-busname'))
conf.set_quoted(
    'HYPERVISOR_OBJPATH', get_option('hypervisor-objpath'))
conf.set_quoted(
    'CHASSIS_BUSNAME', get_option('chassis-busname'))
conf.set_quoted(
    'CHASSIS_OBJPATH', get_option('chassis-objpath'))
conf.set_quoted(
    'BMC_BUSNAME', get_option('bmc-busname'))
conf.set_quoted(
    'BMC_OBJPATH', get_option('bmc-objpath'))
conf.set_quoted(
    'HOST_STATE_PERSIST_PATH', get_option('host-state-persist-path'))
conf.set_quoted(
    'POH_COUNTER_PERSIST_PATH', get_option('poh-counter-persist-path'))
conf.set_quoted(
    'CHASSIS_STATE_CHANGE_PERSIST_PATH', get_option('chassis-state-change-persist-path'))
conf.set_quoted(
    'SCHEDULED_HOST_TRANSITION_PERSIST_PATH', get_option('scheduled-host-transition-persist-path'))
conf.set_quoted(
    'SCHEDULED_HOST_TRANSITION_BUSNAME', get_option('scheduled-host-transition-busname'))
conf.set(
    'BOOT_COUNT_MAX_ALLOWED', get_option('boot-count-max-allowed'))
conf.set(
    'CLASS_VERSION', get_option('class-version'))
if build_host_gpios.enabled()
    conf.set_quoted(
        'HOST_GPIOS_BUSNAME', get_option('host-gpios-busname'))
    conf.set_quoted(
        'HOST_GPIOS_OBJPATH', get_option('host-gpios-objpath'))
endif

# globals shared across applications
conf.set_quoted(
    'HOST_RUNNING_FILE', '/run/openbmc/host@%d-on')

conf.set_quoted(
    'CHASSIS_ON_FILE', '/run/openbmc/chassis@%d-on')

configure_file(output: 'config.h', configuration: conf)

if(get_option('warm-reboot').enabled())
    add_project_arguments('-DENABLE_WARM_REBOOT',language:'cpp')
endif

sdbusplus = dependency('sdbusplus')
sdeventplus = dependency('sdeventplus')
phosphorlogging = dependency('phosphor-logging')
phosphordbusinterfaces = dependency('phosphor-dbus-interfaces')
libgpiod = dependency('libgpiod', version : '>=1.4.1')
fmt = dependency('fmt')

if cpp.has_header('nlohmann/json.hpp')
    nlohmann_json = declare_dependency()
else
    subproject('nlohmann-json')
    nlohmann_json = declare_dependency(
        include_directories: [
            'subprojects/nlohmann-json/single_include',
            'subprojects/nlohmann-json/single_include/nlohmann',
        ]
    )
endif

if cpp.has_header('CLI/CLI.hpp')
    CLI11 = declare_dependency()
else
    CLI11 = dependency('CLI11')
endif

# Get Cereal dependency.
cereal = dependency('cereal', required: false)
has_cereal = cpp.has_header_symbol(
    'cereal/cereal.hpp',
    'cereal::specialize',
    dependencies: cereal,
    required: false)
if not has_cereal
    cereal_opts = import('cmake').subproject_options()
    cereal_opts.add_cmake_defines({'BUILD_TESTS': 'OFF'})
    cereal_proj = import('cmake').subproject(
        'cereal',
        options: cereal_opts,
        required: false)
    assert(cereal_proj.found(), 'cereal is required')
    cereal = cereal_proj.dependency('cereal')
endif

executable('phosphor-host-state-manager',
            'host_state_manager.cpp',
            'host_state_manager_main.cpp',
            'settings.cpp',
            'host_check.cpp',
            'utils.cpp',
            dependencies: [
                cereal,
                fmt,
                libgpiod,
                phosphordbusinterfaces,
                phosphorlogging,
                sdbusplus,
                sdeventplus,
            ],
    implicit_include_directories: true,
    install: true
)

executable('phosphor-hypervisor-state-manager',
            'hypervisor_state_manager.cpp',
            'hypervisor_state_manager_main.cpp',
            'settings.cpp',
            dependencies: [
                phosphordbusinterfaces,
                phosphorlogging,
                sdbusplus,
                sdeventplus,
            ],
    implicit_include_directories: true,
    install: true
)

executable('phosphor-chassis-state-manager',
            'chassis_state_manager.cpp',
            'chassis_state_manager_main.cpp',
            'utils.cpp',
            dependencies: [
                cereal,
                fmt,
                libgpiod,
                nlohmann_json,
                phosphordbusinterfaces,
                phosphorlogging,
                sdbusplus,
                sdeventplus,
            ],
    implicit_include_directories: true,
    install: true
)

executable('phosphor-chassis-check-power-status',
            'chassis_check_power_status.cpp',
            'utils.cpp',
            dependencies: [
                libgpiod,
                phosphordbusinterfaces,
                phosphorlogging,
                sdbusplus,
            ],
    implicit_include_directories: true,
    install: true
)

executable('phosphor-bmc-state-manager',
            'bmc_state_manager.cpp',
            'bmc_state_manager_main.cpp',
            'utils.cpp',
            dependencies: [
                libgpiod,
                phosphordbusinterfaces,
                phosphorlogging,
                sdbusplus,
                sdeventplus,
            ],
    implicit_include_directories: true,
    install: true
)

executable('phosphor-discover-system-state',
            'discover_system_state.cpp',
            'settings.cpp',
            'utils.cpp',
            dependencies: [
                cereal,
                libgpiod,
                phosphorlogging,
                sdbusplus,
            ],
    implicit_include_directories: true,
    install: true
)

executable('phosphor-systemd-target-monitor',
            'systemd_service_parser.cpp',
            'systemd_target_monitor.cpp',
            'systemd_target_parser.cpp',
            'systemd_target_signal.cpp',
            dependencies: [
                CLI11,
                nlohmann_json,
                phosphorlogging,
                sdbusplus,
                sdeventplus,
            ],
    implicit_include_directories: true,
    install: true
)

executable('phosphor-scheduled-host-transition',
            'scheduled_host_transition_main.cpp',
            'scheduled_host_transition.cpp',
            'utils.cpp',
            dependencies: [
                cereal,
                libgpiod,
                phosphorlogging,
                sdbusplus,
                sdeventplus,
            ],
    implicit_include_directories: true,
    install: true
)

executable('phosphor-host-reset-recovery',
            'host_reset_recovery.cpp',
            dependencies: [
                phosphorlogging,
                sdbusplus,
            ],
    implicit_include_directories: true,
    install: true
)

install_data('obmcutil',
        install_mode: 'rwxr-xr-x',
        install_dir: get_option('bindir')
)

systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_variable(
    pkgconfig : 'systemdsystemunitdir')
subdir('service_files')
subdir('target_files')

if build_host_gpios.enabled()
    subdir('host_condition_gpio')
endif

datadir = join_paths(
    get_option('sysconfdir'),
    'phosphor-systemd-target-monitor'
)
subdir('data')

build_tests = get_option('tests')

# If test coverage of source files within the root directory are wanted,
# need to define and build the tests from here
if not build_tests.disabled()
gtest = dependency('gtest', main: true, disabler: true, required: build_tests)
gmock = dependency('gmock', disabler: true, required: build_tests)
  test(
      'test_systemd_parser',
      executable('test_systemd_parser',
          './test/systemd_parser.cpp',
          'systemd_target_parser.cpp',
          dependencies: [
              gtest,
              nlohmann_json,
          ],
          implicit_include_directories: true,
          include_directories: '../'
      )
  )

  test(
      'test_systemd_signal',
      executable('test_systemd_signal',
          './test/systemd_signal.cpp',
          'systemd_target_signal.cpp',
          dependencies: [
              gtest,
              nlohmann_json,
              phosphorlogging,
              sdbusplus,
              sdeventplus,
          ],
          implicit_include_directories: true,
          include_directories: '../'
      )
  )

  test(
      'test_scheduled_host_transition',
      executable('test_scheduled_host_transition',
          './test/test_scheduled_host_transition.cpp',
          'scheduled_host_transition.cpp',
          'utils.cpp',
          dependencies: [
              cereal,
              gmock,
              gtest,
              libgpiod,
              phosphorlogging,
              sdbusplus,
              sdeventplus,
          ],
          implicit_include_directories: true,
          include_directories: '../'
      )
  )

  test(
      'test_hypervisor_state',
      executable('test_hypervisor_state',
          './test/hypervisor_state.cpp',
          'hypervisor_state_manager.cpp',
          dependencies: [
              gtest,
              phosphorlogging,
              sdbusplus,
              sdeventplus,
          ],
          implicit_include_directories: true,
          include_directories: '../'
      )
  )
endif
