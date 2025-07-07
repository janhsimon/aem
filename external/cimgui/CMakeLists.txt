set(TARGET_NAME cimgui)

if(WIN32)
  add_library(${TARGET_NAME} STATIC IMPORTED GLOBAL)
elseif(APPLE)
  add_library(${TARGET_NAME} SHARED IMPORTED GLOBAL)
endif()

target_include_directories(${TARGET_NAME} INTERFACE "include")

if(WIN32)
  set_target_properties(${TARGET_NAME} PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/lib/win/cimgui.lib")
elseif(APPLE)
  set_target_properties(${TARGET_NAME} PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/lib/mac/${CMAKE_OSX_ARCHITECTURES}/libcimgui.dylib")
endif()