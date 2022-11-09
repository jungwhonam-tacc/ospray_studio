option(BUILD_PLUGIN_STREAMER "Streamer plugin" OFF)

if (BUILD_PLUGIN_STREAMER)  
  set( pluginName "ospray_studio_plugin_streamer")

  add_library(${pluginName} SHARED
    plugin_streamer.cpp
    PanelStreamer.cpp
  )

  target_link_libraries(${pluginName} ospray_sg ospray_ui)

  # Only link against imgui if needed (ie, pure file importers don't)
  target_link_libraries(${pluginName} imgui)

  # Add external libraries for this plugin
  add_subdirectory(external)
  target_link_libraries(${pluginName} network)

  target_include_directories(${pluginName}
    PRIVATE ${CMAKE_SOURCE_DIR}
  )

  install(TARGETS ${pluginName}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT lib
    # on Windows put the dlls into bin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT lib
  )

endif()
