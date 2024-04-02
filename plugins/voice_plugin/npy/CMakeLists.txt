add_library(npy INTERFACE)
target_include_directories(npy
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>)