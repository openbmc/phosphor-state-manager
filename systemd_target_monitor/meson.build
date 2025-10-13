executable(
    'phosphor-systemd-target-monitor',
    'systemd_service_parser.cpp',
    'systemd_target_monitor.cpp',
    'systemd_target_parser.cpp',
    'systemd_target_signal.cpp',
    dependencies: [
        CLI11,
        libgpiod,
        nlohmann_json_dep,
        phosphorlogging,
        sdbusplus,
        sdeventplus,
    ],
    include_directories: [src_inc],
    implicit_include_directories: true,
    link_with: [utils_lib],
    install: true,
)
