executable(
    'phosphor-host-state-manager',
    'host_state_manager.cpp',
    'host_state_manager_main.cpp',
    'host_check.cpp',
    dependencies: [
        cereal,
        libgpiod,
        phosphordbusinterfaces,
        phosphorlogging,
        sdbusplus,
        sdeventplus,
    ],
    include_directories: [src_inc],
    implicit_include_directories: true,
    link_with: [settings_lib, utils_lib],
    install: true,
)
