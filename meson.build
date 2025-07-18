project(
    'phosphor-state-manager',
    'cpp',
    default_options: ['warning_level=3', 'werror=true', 'cpp_std=c++23'],
    meson_version: '>=1.1.1',
    license: 'Apache-2.0',
    version: '0.1',
)
cpp = meson.get_compiler('cpp')

build_host_gpios = get_option('host-gpios')

conf = configuration_data()
conf.set_quoted('HOST_SCHED_OBJPATH', get_option('host-sched-objpath'))
conf.set_quoted('HYPERVISOR_BUSNAME', get_option('hypervisor-busname'))
conf.set_quoted('HYPERVISOR_OBJPATH', get_option('hypervisor-objpath'))
conf.set_quoted('HOST_STATE_PERSIST_PATH', get_option('host-state-persist-path'))
conf.set_quoted(
    'POH_COUNTER_PERSIST_PATH',
    get_option('poh-counter-persist-path'),
)
conf.set_quoted(
    'CHASSIS_STATE_CHANGE_PERSIST_PATH',
    get_option('chassis-state-change-persist-path'),
)
conf.set_quoted(
    'SCHEDULED_HOST_TRANSITION_PERSIST_PATH',
    get_option('scheduled-host-transition-persist-path'),
)
conf.set('BOOT_COUNT_MAX_ALLOWED', get_option('boot-count-max-allowed'))
conf.set('CLASS_VERSION', get_option('class-version'))
conf.set_quoted('SYSFS_SECURE_BOOT_PATH', get_option('sysfs-secure-boot-path'))
conf.set_quoted('SYSFS_ABR_IMAGE_PATH', get_option('sysfs-abr-image-path'))
if build_host_gpios.allowed()
    conf.set_quoted('HOST_GPIOS_BUSNAME', get_option('host-gpios-busname'))
    conf.set_quoted('HOST_GPIOS_OBJPATH', get_option('host-gpios-objpath'))
endif

conf.set10(
    'ONLY_RUN_APR_ON_POWER_LOSS',
    get_option('only-run-apr-on-power-loss'),
)

conf.set10('RUN_APR_ON_PINHOLE_RESET', get_option('run-apr-on-pinhole-reset'))

conf.set10('RUN_APR_ON_WATCHDOG_RESET', get_option('run-apr-on-watchdog-reset'))

conf.set10('RUN_APR_ON_SOFTWARE_RESET', get_option('run-apr-on-software-reset'))

conf.set_quoted('SYSFS_TPM_DEVICE_PATH', get_option('sysfs-tpm-device-path'))

conf.set_quoted(
    'SYSFS_TPM_MEASUREMENT_PATH',
    get_option('sysfs-tpm-measurement-path'),
)

conf.set10(
    'ONLY_ALLOW_BOOT_WHEN_BMC_READY',
    get_option('only-allow-boot-when-bmc-ready'),
)

# globals shared across applications
conf.set_quoted('BASE_FILE_DIR', '/run/openbmc/')

conf.set_quoted('CHASSIS_LOST_POWER_FILE', '/run/openbmc/chassis@{}-lost-power')

conf.set_quoted('HOST_RUNNING_FILE', '/run/openbmc/host@{}-on')

conf.set_quoted('CHASSIS_ON_FILE', '/run/openbmc/chassis@{}-on')

conf.set(
    'CHECK_FWUPDATE_BEFORE_DO_TRANSITION',
    get_option('check-fwupdate-before-do-transition').allowed(),
)

configure_file(output: 'config.h', configuration: conf)

if (get_option('warm-reboot').allowed())
    add_project_arguments('-DENABLE_WARM_REBOOT', language: 'cpp')
endif

if (get_option('force-warm-reboot').allowed())
    add_project_arguments('-DENABLE_FORCE_WARM_REBOOT', language: 'cpp')
endif

if (get_option('apply-power-policy-bmc-ready').allowed())
    add_project_arguments('-DAPPLY_POWER_POLICY_WHEN_BMC_READY', language: 'cpp')
endif

nlohmann_json_dep = dependency('nlohmann_json', include_type: 'system')
sdbusplus = dependency('sdbusplus')
sdeventplus = dependency('sdeventplus')
phosphorlogging = dependency('phosphor-logging')
phosphordbusinterfaces = dependency('phosphor-dbus-interfaces')
libgpiod = dependency(
    'libgpiod',
    version: '>=1.4.1',
    default_options: ['bindings=cxx'],
)

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
    required: false,
)
if not has_cereal
    cereal_opts = import('cmake').subproject_options()
    cereal_opts.add_cmake_defines(
        {'BUILD_TESTS': 'OFF', 'SKIP_PERFORMANCE_COMPARISON': 'ON'},
    )
    cereal_proj = import('cmake').subproject(
        'cereal',
        options: cereal_opts,
        required: false,
    )
    assert(cereal_proj.found(), 'cereal is required')
    cereal = cereal_proj.dependency('cereal')
endif

executable(
    'phosphor-host-state-manager',
    'host_state_manager.cpp',
    'host_state_manager_main.cpp',
    'settings.cpp',
    'host_check.cpp',
    'utils.cpp',
    dependencies: [
        cereal,
        libgpiod,
        phosphordbusinterfaces,
        phosphorlogging,
        sdbusplus,
        sdeventplus,
    ],
    implicit_include_directories: true,
    install: true,
)

executable(
    'phosphor-hypervisor-state-manager',
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
    install: true,
)

executable(
    'phosphor-chassis-state-manager',
    'chassis_state_manager.cpp',
    'chassis_state_manager_main.cpp',
    'utils.cpp',
    dependencies: [
        cereal,
        libgpiod,
        nlohmann_json_dep,
        phosphordbusinterfaces,
        phosphorlogging,
        sdbusplus,
        sdeventplus,
    ],
    implicit_include_directories: true,
    install: true,
)

executable(
    'phosphor-chassis-check-power-status',
    'chassis_check_power_status.cpp',
    'utils.cpp',
    dependencies: [libgpiod, phosphordbusinterfaces, phosphorlogging, sdbusplus],
    implicit_include_directories: true,
    install: true,
)

executable(
    'phosphor-bmc-state-manager',
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
    install: true,
)

executable(
    'phosphor-discover-system-state',
    'discover_system_state.cpp',
    'settings.cpp',
    'utils.cpp',
    dependencies: [cereal, libgpiod, phosphorlogging, sdbusplus],
    implicit_include_directories: true,
    install: true,
)

executable(
    'phosphor-systemd-target-monitor',
    'systemd_service_parser.cpp',
    'systemd_target_monitor.cpp',
    'systemd_target_parser.cpp',
    'systemd_target_signal.cpp',
    'utils.cpp',
    dependencies: [
        CLI11,
        libgpiod,
        nlohmann_json_dep,
        phosphorlogging,
        sdbusplus,
        sdeventplus,
    ],
    implicit_include_directories: true,
    install: true,
)

executable(
    'phosphor-scheduled-host-transition',
    'scheduled_host_transition_main.cpp',
    'scheduled_host_transition.cpp',
    'utils.cpp',
    dependencies: [cereal, libgpiod, phosphorlogging, sdbusplus, sdeventplus],
    implicit_include_directories: true,
    install: true,
)

executable(
    'phosphor-host-reset-recovery',
    'host_reset_recovery.cpp',
    dependencies: [phosphorlogging, sdbusplus],
    implicit_include_directories: true,
    install: true,
)

executable(
    'phosphor-secure-boot-check',
    'secure_boot_check.cpp',
    'utils.cpp',
    dependencies: [sdbusplus, phosphorlogging, libgpiod],
    implicit_include_directories: true,
    install: true,
)

if get_option('install-utils').allowed()
    install_data(
        'obmcutil',
        install_mode: 'rwxr-xr-x',
        install_dir: get_option('bindir'),
    )
endif

install_data(
    'scripts/host-reboot',
    install_mode: 'rwxr-xr-x',
    install_dir: get_option('libexecdir') / 'phosphor-state-manager',
)

systemd = dependency('systemd')
systemd_system_unit_dir = systemd.get_variable('systemd_system_unit_dir')
subdir('service_files')
subdir('target_files')

if build_host_gpios.allowed()
    subdir('host_condition_gpio')
endif

datadir = join_paths(get_option('sysconfdir'), 'phosphor-systemd-target-monitor')
subdir('data')

build_tests = get_option('tests')

# If test coverage of source files within the root directory are wanted,
# need to define and build the tests from here
if build_tests.allowed()
    gtest = dependency('gtest', main: true, disabler: true, required: false)
    gmock = dependency('gmock', disabler: true, required: false)

    if not gtest.found() or not gmock.found()
        gtest_proj = import('cmake').subproject('googletest', required: false)
        if gtest_proj.found()
            gtest = declare_dependency(
                dependencies: [
                    dependency('threads'),
                    gtest_proj.dependency('gtest'),
                    gtest_proj.dependency('gtest_main'),
                ],
            )
            gmock = gtest_proj.dependency('gmock')
        else
            assert(
                not get_option('tests').enabled(),
                'Googletest is required if tests are enabled',
            )
        endif
    endif

    test(
        'test_systemd_parser',
        executable(
            'test_systemd_parser',
            './test/systemd_parser.cpp',
            'systemd_target_parser.cpp',
            dependencies: [gtest, nlohmann_json_dep],
            implicit_include_directories: true,
            include_directories: '../',
        ),
    )

    test(
        'test_systemd_signal',
        executable(
            'test_systemd_signal',
            './test/systemd_signal.cpp',
            'systemd_target_signal.cpp',
            'utils.cpp',
            dependencies: [
                gtest,
                libgpiod,
                nlohmann_json_dep,
                phosphorlogging,
                sdbusplus,
                sdeventplus,
            ],
            implicit_include_directories: true,
            include_directories: '../',
        ),
    )

    test(
        'test_scheduled_host_transition',
        executable(
            'test_scheduled_host_transition',
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
            include_directories: '../',
        ),
    )

    test(
        'test_hypervisor_state',
        executable(
            'test_hypervisor_state',
            './test/hypervisor_state.cpp',
            'hypervisor_state_manager.cpp',
            dependencies: [gtest, phosphorlogging, sdbusplus, sdeventplus],
            implicit_include_directories: true,
            include_directories: '../',
        ),
    )
endif
