cmake_minimum_required (VERSION 3.13)

project (occt-ais-hello)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../adm/cmake" ${CMAKE_MODULE_PATH})

set (APP_VERSION_MAJOR 1)
set (APP_VERSION_MINOR 0)
set (APP_TARGET occt-ais-hello)
set (CMAKE_CXX_STANDARD 11)

find_package (OpenGL      REQUIRED)
find_package (OpenCASCADE REQUIRED)
if (NOT OpenCASCADE_FOUND)
  message (FATAL_ERROR "could not find OpenCASCADE, please set OpenCASCADE_DIR variable" )
else()
  message (STATUS "Using OpenCASCADE from \"${OpenCASCADE_INSTALL_PREFIX}\"" )
  message (STATUS "OpenCASCADE_INCLUDE_DIR=${OpenCASCADE_INCLUDE_DIR}")
  message (STATUS "OpenCASCADE_LIBRARY_DIR=${OpenCASCADE_LIBRARY_DIR}")
endif()

# compiler flags
if (MSVC)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise /EHa /MP")
  string (REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  add_definitions (-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
else()
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -fPIC")
  add_definitions(-DOCC_CONVERT_SIGNALS)
endif()

add_executable (${APP_TARGET}
  OcctAisHello.cpp ReadMe.md)

# extra search paths
include_directories(${OpenCASCADE_INCLUDE_DIR})
link_directories   (${OpenCASCADE_LIBRARY_DIR})

# define dependencies
set (anOcctLibs TKOpenGl TKV3d TKService TKPrim TKTopAlgo TKGeomAlgo TKBRep TKGeomBase TKG3d TKG2d TKMath TKernel)
target_link_libraries (${PROJECT_NAME} PRIVATE ${anOcctLibs})

target_link_libraries (${PROJECT_NAME} PRIVATE ${OPENGL_LIBRARIES})
if (APPLE)
  find_library (Appkit_LIB NAMES AppKit)
  target_link_libraries (${PROJECT_NAME} PRIVATE ${Appkit_LIB})
  target_link_libraries (${PROJECT_NAME} PRIVATE objc)
elseif (UNIX)
  target_link_libraries (${PROJECT_NAME} PRIVATE EGL)
  target_link_libraries (${PROJECT_NAME} PRIVATE X11)
  target_link_libraries (${PROJECT_NAME} PRIVATE dl)
  target_link_libraries (${PROJECT_NAME} PRIVATE pthread)
endif()

# auxiliary development environment
if (MSVC)
  set (3RDPARTY_DLL_DIRS "" CACHE STRING "Paths to external DLLs separated by semicolon (FreeImage, FreeType, etc.)")

  get_target_property (aTKernelRel "TKernel" IMPORTED_LOCATION_RELEASE)
  get_target_property (aTKernelDbg "TKernel" IMPORTED_LOCATION_DEBUG)
  get_filename_component (OpenCASCADE_BINARY_DIR_RELEASE ${aTKernelRel} DIRECTORY)
  get_filename_component (OpenCASCADE_BINARY_DIR_DEBUG   ${aTKernelDbg} DIRECTORY)
  if (NOT EXISTS "${OpenCASCADE_BINARY_DIR_DEBUG}" AND EXISTS "${OpenCASCADE_BINARY_DIR_RELEASE}")
    set (OpenCASCADE_BINARY_DIR_DEBUG "${OpenCASCADE_BINARY_DIR_RELEASE}")
  elseif (NOT EXISTS "${OpenCASCADE_BINARY_DIR_RELEASE}" AND EXISTS "${OpenCASCADE_BINARY_DIR_DEBUG}")
    set (OpenCASCADE_BINARY_DIR_RELEASE "${OpenCASCADE_BINARY_DIR_DEBUG}")
  endif()

  set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=%PATH%;$<IF:$<CONFIG:Debug>,${OpenCASCADE_BINARY_DIR_DEBUG},${OpenCASCADE_BINARY_DIR_RELEASE}>;${3RDPARTY_DLL_DIRS}")
endif()
