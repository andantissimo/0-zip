cmake_minimum_required(VERSION 3.8)

project(0Zip CXX)

include(CheckIncludeFileCXX)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

set(CMAKE_CXX_FLAGS         "-Wall")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS_DEBUG "-g -fprofile-instr-generate -fcoverage-mapping"
		CACHE STRING "" FORCE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 --coverage" CACHE STRING "" FORCE)
endif()

check_include_file_cxx("filesystem"              HAVE_FILESYSTEM)
check_include_file_cxx("experimental/filesystem" HAVE_EXPERIMENTAL_FILESYSTEM)
if(HAVE_FILESYSTEM OR HAVE_EXPERIMENTAL_FILESYSTEM)
	find_package(Boost REQUIRED COMPONENTS locale program_options)
else()
	find_package(Boost REQUIRED COMPONENTS locale program_options filesystem)
endif()
include_directories(${Boost_INCLUDE_DIRS})

file(GLOB SOURCE_FILES src/*.cc)
if(APPLE)
	file(GLOB APPLE_FILES src/apple/*.mm)
	set_source_files_properties(${APPLE_FILES} PROPERTIES
		COMPILE_FLAGS "-x objective-c++")
	set(CMAKE_EXE_LINKER_FLAGS "-framework Foundation -w")
	add_executable(0z ${SOURCE_FILES} ${APPLE_FILES})
elseif(WIN32)
	file(GLOB WIN32_FILES src/win32/*.cc)
	add_executable(0z ${SOURCE_FILES} ${WIN32_FILES})
else()
	file(GLOB POSIX_FILES src/posix/*.cc)
	add_executable(0z ${SOURCE_FILES} ${POSIX_FILES})
endif()

if(HAVE_FILESYSTEM OR HAVE_EXPERIMENTAL_FILESYSTEM)
	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT APPLE)
		target_link_libraries(0z c++fs)
	elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
		target_link_libraries(0z stdc++fs)
	endif()
endif()
target_link_libraries(0z ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})

install(TARGETS 0z RUNTIME DESTINATION "${CMAKE_INSTALL_FULL_BINDIR}")
