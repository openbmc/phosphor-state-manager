executable(
    'phosphor-scheduled-host-transition',
    'scheduled_host_transition_main.cpp',
    'scheduled_host_transition.cpp',
    dependencies: [cereal, libgpiod, phosphorlogging, sdbusplus, sdeventplus],
    include_directories: [src_inc],
    implicit_include_directories: true,
    link_with: [utils_lib],
    install: true,
)
