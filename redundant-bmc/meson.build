librbmc_sources = files(
  'manager.cpp',
  'role_determination.cpp',
)

librbmc_deps = [
  sdbusplus,
  phosphordbusinterfaces,
  phosphorlogging,
]

librbmc_lib = static_library(
  'rbmc',
  librbmc_sources,
  dependencies: [
    librbmc_deps
  ]
)

librbmc_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with: librbmc_lib,
  dependencies: [
    sdbusplus,
    phosphordbusinterfaces,
    phosphorlogging,
  ]
)

executable('phosphor-rbmc-state-manager',
           'rbmc_state_manager_main.cpp',
            dependencies: [
                librbmc_dep
            ],
    install: true
)

if get_option('tests').allowed()
    subdir('test')
endif