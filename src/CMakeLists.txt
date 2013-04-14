cmake_minimum_required(VERSION 2.8)
project( sagmm )

# The version number.
SET( ${sagmm}_MAJOR_VERSION 0 )
SET( ${sagmm}_MINOR_VERSION 1 )
SET( ${sagmm}_PATCH_LEVEL 0 )

SET( CMAKE_C_FLAGS "-Wall -g" )

FILE ( GLOB SRCS *.cpp *.h )

INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}/")
INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}/../include")
INCLUDE_DIRECTORIES("${PROJECT_BINARY_DIR}/include")
INCLUDE_DIRECTORIES("/opt/local/include")
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} /opt/local/lib)

#delete generated files
SET_DIRECTORY_PROPERTIES (
    PROPERTIES
        ADDITIONAL_MAKE_CLEAN_FILES "${sagmm}_SOURCE_DIR}/*~"
)

#delete bin files in make clean
SET_DIRECTORY_PROPERTIES (
    PROPERTIES
    ADDITIONAL_MAKE_CLEAN_FILES "${sagmm}_BINARY_DIR}/*"
)

# Set DEBUG option
OPTION(DEBUG "DEBUG option to print out more info" ON)

#find_library(Log4 log4cxx PATHS /opt/local/lib)
find_library(Logging log4cplus PATHS /opt/local/lib)

# openCV library
FIND_PACKAGE( OpenCV REQUIRED )

ADD_EXECUTABLE( sagmm ${SRCS} )
TARGET_LINK_LIBRARIES( sagmm ${OpenCV_LIBS} ${Logging} )
