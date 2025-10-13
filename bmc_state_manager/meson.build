executable(
    'phosphor-bmc-state-manager',
    'bmc_state_manager.cpp',
    'bmc_state_manager_main.cpp',
    dependencies: [
        libgpiod,
        phosphordbusinterfaces,
        phosphorlogging,
        sdbusplus,
        sdeventplus,
    ],
    include_directories: [src_inc],
    implicit_include_directories: true,
    link_with: [utils_lib],
    install: true,
)
