CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
SET(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_LEGACY_CYGWIN_WIN32 0) # Remove when CMake >= 2.8.4 is required

PROJECT("runtest")

SET(CMAKE_C_FLAGS " -std=c99 -O3 -Wall -Wextra -Wimplicit -L../../phantom/lib/")

INCLUDE_DIRECTORIES ( "/usr/include" )
ADD_EXECUTABLE(runtests main.c utils.c panic.c)
TARGET_LINK_LIBRARIES(runtests kern )
TARGET_LINK_LIBRARIES(runtests cunit dl )

ADD_CUSTOM_TARGET(test "./runtests" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" VERBATIM)
ADD_SUBDIRECTORY(suites)
