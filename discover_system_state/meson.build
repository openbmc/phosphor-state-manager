executable(
    'phosphor-discover-system-state',
    'discover_system_state.cpp',
    dependencies: [cereal, libgpiod, phosphorlogging, sdbusplus],
    include_directories: [src_inc],
    implicit_include_directories: true,
    link_with: [settings_lib, utils_lib],
    install: true,
)
