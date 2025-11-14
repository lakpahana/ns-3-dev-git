set(example_as_test_suite)
if(${ENABLE_EXAMPLES})
  set(example_as_test_suite
      test/mpi-test-suite.cc
  )
endif()

set(test_sources
    test/wifi-mpi-message-test.cc
)

build_lib(
  LIBNAME mpi
  SOURCE_FILES
    model/distributed-simulator-impl.cc
    model/granted-time-window-mpi-interface.cc
    model/mpi-interface.cc
    model/mpi-receiver.cc
    model/null-message-mpi-interface.cc
    model/null-message-simulator-impl.cc
    model/parallel-communication-interface.h
    model/remote-channel-bundle-manager.cc
    model/remote-channel-bundle.cc
    model/wifi-mpi-messages.cc
  HEADER_FILES
    model/mpi-interface.h
    model/mpi-receiver.h
    model/parallel-communication-interface.h
    model/wifi-mpi-messages.h
  LIBRARIES_TO_LINK
    ${libcore}
    ${libnetwork}
    ${MPI_CXX_LIBRARIES}
  TEST_SOURCES ${example_as_test_suite} ${test_sources}
)
