option(BUILD_PLUGIN_VOICE "Voice plugin" OFF)

if (BUILD_PLUGIN_VOICE)
  set( pluginName "ospray_studio_plugin_voice")

  add_library(${pluginName} SHARED
    plugin_voice.cpp
    PanelVoice.cpp
  )

  target_link_libraries(${pluginName} ospray_sg)

  # Only link against imgui if needed (ie, pure file importers don't)
  target_link_libraries(${pluginName} ospray_ui)

  target_link_libraries(${pluginName} stb_image)

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
