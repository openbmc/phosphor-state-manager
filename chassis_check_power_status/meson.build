executable(
    'phosphor-chassis-check-power-status',
    'chassis_check_power_status.cpp',
    dependencies: [libgpiod, phosphordbusinterfaces, phosphorlogging, sdbusplus],
    include_directories: [src_inc],
    implicit_include_directories: true,
    link_with: [utils_lib],
    install: true,
)
