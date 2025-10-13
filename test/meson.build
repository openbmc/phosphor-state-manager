
test(
    'test_systemd_parser',
    executable(
        'test_systemd_parser',
        'systemd_parser.cpp',
        '../systemd_target_monitor/systemd_target_parser.cpp',
        dependencies: [gtest, nlohmann_json_dep],
        implicit_include_directories: true,
        include_directories: '../',
    ),
)

test(
    'test_systemd_signal',
    executable(
        'test_systemd_signal',
        'systemd_signal.cpp',
        '../systemd_target_monitor/systemd_target_signal.cpp',
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
        link_with: [utils_lib],
    ),
)

test(
    'test_scheduled_host_transition',
    executable(
        'test_scheduled_host_transition',
        'test_scheduled_host_transition.cpp',
        '../scheduled_host_transition/scheduled_host_transition.cpp',
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
        link_with: [utils_lib],
    ),
)

test(
    'test_hypervisor_state',
    executable(
        'test_hypervisor_state',
        'hypervisor_state.cpp',
        '../hypervisor_state_manager/hypervisor_state_manager.cpp',
        dependencies: [gtest, phosphorlogging, sdbusplus, sdeventplus],
        implicit_include_directories: true,
        include_directories: '../',
    ),
)
