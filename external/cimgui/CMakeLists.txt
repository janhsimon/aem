set(TARGET_NAME cimgui)

add_library(${TARGET_NAME} STATIC IMPORTED GLOBAL)
target_include_directories(${TARGET_NAME} INTERFACE "include")

if(WIN32)
  set_target_properties(${TARGET_NAME} PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/lib/win/cimgui.lib")
endif()