set(TARGET_NAME viewer)

add_executable(${TARGET_NAME})
target_sources(${TARGET_NAME} PRIVATE "Main.cpp")
target_link_libraries(${TARGET_NAME} PRIVATE glfw)

if(WIN32)
  set (DLLS "${CMAKE_SOURCE_DIR}/external/glfw/lib/win/glfw3.dll")
  foreach (DLL ${DLLS})
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL} "$<TARGET_FILE_DIR:${TARGET_NAME}>")
  endforeach()
endif()