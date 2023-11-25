set(TARGET_NAME zlibstatic)

add_library(${TARGET_NAME} STATIC IMPORTED GLOBAL)
target_include_directories(${TARGET_NAME} INTERFACE "include")

if(WIN32)
  set_target_properties(${TARGET_NAME} PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_LIST_DIR}/lib/win/zlibstatic.lib")
endif()


set(TARGET_NAME assimp)

add_library(${TARGET_NAME} SHARED IMPORTED GLOBAL)
target_include_directories(${TARGET_NAME} INTERFACE "include")
target_link_libraries(${TARGET_NAME} INTERFACE zlibstatic)

if(WIN32)
  set_target_properties(${TARGET_NAME} PROPERTIES IMPORTED_IMPLIB "${CMAKE_CURRENT_LIST_DIR}/lib/win/assimp.lib")
endif()