executable(
    'phosphor-secure-boot-check',
    'secure_boot_check.cpp',
    dependencies: [sdbusplus, phosphorlogging, libgpiod],
    include_directories: [src_inc],
    implicit_include_directories: true,
    link_with: [utils_lib],
    install: true,
)
