set(TARGET_NAME ktx)

add_library(${TARGET_NAME} SHARED IMPORTED GLOBAL)
target_include_directories(${TARGET_NAME} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/include" "${CMAKE_CURRENT_LIST_DIR}/include/ktx")

if(WIN32)
  set_target_properties(${TARGET_NAME} PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_LIST_DIR}/lib/win/ktx.lib")
elseif(APPLE)
  set_target_properties(${TARGET_NAME} PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/lib/mac/${CMAKE_OSX_ARCHITECTURES}/libktx.4.dylib")
endif()