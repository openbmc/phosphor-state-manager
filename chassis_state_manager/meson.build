
executable(
    'phosphor-chassis-state-manager',
    'chassis_state_manager.cpp',
    'chassis_state_manager_main.cpp',
    dependencies: [
        cereal,
        libgpiod,
        nlohmann_json_dep,
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
