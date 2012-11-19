PROJECT(ninjabird-common)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
INCLUDE(CheckIncludeFile)

# define some global build options (use ccmake to configure):
OPTION(CMN_SHARED "Build as shared libraries instead of static." OFF)

# update the cmake module path and define path aliases:
SET(REPO_ROOT_DIR                          "${PROJECT_SOURCE_DIR}")
SET(REPO_CMAKE_DIR                         "${PROJECT_SOURCE_DIR}/cmake")
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${REPO_CMAKE_DIR}")

# set global compiler options like always compiling with maximum warning level.
IF(MSVC)
  IF(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    STRING(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  ELSE()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  ENDIF()
ELSEIF(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-invalid-offsetof")
ENDIF(MSVC)

# search in the root directory for include files:
INCLUDE_DIRECTORIES("${REPO_ROOT_DIR}")

# recurse into subdirectories and process their CMakeLists.txt:
ADD_SUBDIRECTORY(common)
