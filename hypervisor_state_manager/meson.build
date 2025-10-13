executable(
    'phosphor-hypervisor-state-manager',
    'hypervisor_state_manager.cpp',
    'hypervisor_state_manager_main.cpp',
    dependencies: [
        phosphordbusinterfaces,
        phosphorlogging,
        sdbusplus,
        sdeventplus,
    ],
    include_directories: [src_inc],
    implicit_include_directories: true,
    link_with: [settings_lib],
    install: true,
)
