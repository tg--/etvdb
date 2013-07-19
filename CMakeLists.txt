cmake_minimum_required(VERSION 2.8)
project(ETVDB)
set(PACKAGE_VERSION 0.1.0)
set(PACKAGE ${CMAKE_PROJECT_NAME})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -std=gnu99")

# enable Debug build
## build with -D DEBUG=ON
option(DEBUG debug OFF)
if(DEBUG)
	add_definitions(-DDEBUG -g -Wall -O0)
endif(DEBUG)

INCLUDE(FindPkgConfig)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(EINA REQUIRED eina)

include_directories(${EINA_INCLUDE_DIRS})

add_subdirectory(external)
add_subdirectory(lib)

# install data
file(GLOB data data/*)
install(FILES ${data} DESTINATION share/etvdb)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)
