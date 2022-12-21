gtest = dependency('gtest', main: true, disabler: true, required: false)
gmock = dependency('gmock', disabler: true, required: false)
if not gtest.found() or not gmock.found()
  gtest_opts = import('cmake').subproject_options()
  gtest_opts.add_cmake_defines({'CMAKE_CXX_FLAGS': '-Wno-pedantic'})
  gtest_proj = import('cmake').subproject(
    'googletest',
    options: gtest_opts,
    required: false)
  if gtest_proj.found()
    gtest = declare_dependency(
      dependencies: [
        dependency('threads'),
        gtest_proj.dependency('gtest'),
        gtest_proj.dependency('gtest_main'),
      ])
    gmock = gtest_proj.dependency('gmock')
  else
    assert(not get_option('tests').enabled(), 'Googletest is required')
  endif
endif

test(
    'test_systemd_parser',
    executable('test_systemd_parser',
        'systemd_parser.cpp',
        '../systemd_target_parser.cpp',
        dependencies: [
            gtest,
            nlohmann_json,
        ],
        implicit_include_directories: true,
        include_directories: '../'
    )
)

test(
    'test_systemd_signal',
    executable('test_systemd_signal',
        'systemd_signal.cpp',
        '../systemd_target_signal.cpp',
        '../utils.cpp',
        dependencies: [
            gtest,
            libgpiod,
            nlohmann_json,
            phosphorlogging,
            sdbusplus,
            sdeventplus,
        ],
        implicit_include_directories: true,
        include_directories: '../'
    )
)

test(
    'test_scheduled_host_transition',
    executable('test_scheduled_host_transition',
        'test_scheduled_host_transition.cpp',
        '../scheduled_host_transition.cpp',
        '../utils.cpp',
        dependencies: [
            cereal,
            gmock,
            gtest,
            libgpiod,
            phosphorlogging,
            sdbusplus,
            sdeventplus,
        ],
        implicit_include_directories: true,
        include_directories: '../'
    )
)

test(
    'test_hypervisor_state',
    executable('test_hypervisor_state',
        'hypervisor_state.cpp',
        '../hypervisor_state_manager.cpp',
        dependencies: [
            gtest,
            phosphorlogging,
            sdbusplus,
            sdeventplus,
        ],
        implicit_include_directories: true,
        include_directories: '../'
    )
)