
test(
    'test_systemd_parser',
    executable(
        'test_systemd_parser',
        'systemd_parser.cpp',
        '../systemd_target_parser.cpp',
        dependencies: [gtest, nlohmann_json_dep, phosphorlogging],
        implicit_include_directories: true,
        include_directories: '../',
    ),
)

test(
    'test_systemd_signal',
    executable(
        'test_systemd_signal',
        'systemd_signal.cpp',
        '../systemd_target_signal.cpp',
        dependencies: [
            gtest,
            libgpiod,
            nlohmann_json_dep,
            phosphorlogging,
            sdbusplus,
            sdeventplus,
        ],
        link_with: [utils_lib],
        implicit_include_directories: true,
        include_directories: '../',
    ),
)

test(
    'test_scheduled_host_transition',
    executable(
        'test_scheduled_host_transition',
        'test_scheduled_host_transition.cpp',
        '../scheduled_host_transition.cpp',
        dependencies: [
            cereal,
            gmock,
            gtest,
            libgpiod,
            phosphorlogging,
            sdbusplus,
            sdeventplus,
        ],
        link_with: [utils_lib],
        implicit_include_directories: true,
        include_directories: '../',
    ),
)

test(
    'test_hypervisor_state',
    executable(
        'test_hypervisor_state',
        'hypervisor_state.cpp',
        '../hypervisor_state_manager.cpp',
        dependencies: [gtest, phosphorlogging, sdbusplus, sdeventplus],
        implicit_include_directories: true,
        include_directories: '../',
    ),
)

test(
    'test_host_reset_recovery',
    executable(
        'test_host_reset_recovery',
        'test_host_reset_recovery.cpp',
        dependencies: [gtest, phosphorlogging, sdbusplus],
        implicit_include_directories: true,
        include_directories: '../',
    ),
)

test(
    'test_host_state_manager',
    executable(
        'test_host_state_manager',
        'test_host_state_manager.cpp',
        dependencies: [
            gtest,
            phosphorlogging,
        ],
        implicit_include_directories: true,
        include_directories: '../',
    ),
)

test(
    'test_bmc_state_manager',
    executable(
        'test_bmc_state_manager',
        'test_bmc_state_manager.cpp',
        dependencies: [
            gtest,
            phosphorlogging,
        ],
        implicit_include_directories: true,
        include_directories: '../',
    ),
)

if get_option('multi-chassis-smp').allowed()
    test(
        'test_chassis_smp',
        executable(
            'test_chassis_smp',
            'test_chassis_smp.cpp',
            'test_chassis_smp_enhanced.cpp',
            'test_chassis_wait_smp.cpp',
            '../chassis_state_manager_smp.cpp',
            '../chassis_wait_for_smp_poweron.cpp',
            dependencies: [
                gmock,
                gtest,
                libgpiod,
                nlohmann_json_dep,
                phosphorlogging,
                sdbusplus,
                sdeventplus,
            ],
            link_with: [utils_lib],
            implicit_include_directories: true,
            include_directories: '../',
        ),
    )
endif
