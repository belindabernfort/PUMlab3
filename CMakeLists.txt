##################################################################################################
#                                                                                                #
# TNM090 Particle System                                                                         #
#                                                                                                #
# Copyright (c) 2013 Alexander Bock                                                              #
#                                                                                                #
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software  #
# and associated documentation files (the "Software"), to deal in the Software without           #
# restriction, including without limitation the rights to use, copy, modify, merge, publish,     #
# distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the  #
# Software is furnished to do so, subject to the following conditions:                           #
#                                                                                                #
# The above copyright notice and this permission notice shall be included in all copies or       #
# substantial portions of the Software.                                                          #
#                                                                                                #
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING  #
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND     #
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   #
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, #
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.        #
#                                                                                                #
##################################################################################################

####################
# General settings #
####################
# We want to use the C++11 standard and GCC requires with flag
project(ParticleSimulator)
cmake_minimum_required(VERSION 2.8)
if (UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif ()

set(GHOUL_ROOT_DIR "${PROJECT_SOURCE_DIR}/ghoul")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${GHOUL_ROOT_DIR}/ext)

################
# Source files #
################
# Set all the sources that we will need in this project
set(ParticleSimulator_Simulator_SOURCES
    # Add your new source (.cpp) files here
)

set(ParticleSimulator_Simulator_HEADERS
    # add your new header (.h) files here
)

# Then the main source and the GUI sources
set(ParticleSimulator_GUI_SOURCES main.cpp gui.cpp renderer.cpp)
set(ParticleSimulator_GUI_HEADERS gui.h renderer.h)

################
# Dependencies #
################
# Include the General Helpful Open Utility Library
set(GHOUL_ROOT_DIR "${PROJECT_SOURCE_DIR}/ghoul")
add_subdirectory(ghoul)
include_directories("${GHOUL_ROOT_DIR}/include")

# Include then openGL Mathematics library
find_package(GLM REQUIRED)
include_directories(${GLM_INCLUDE_DIRS})

# If we are on windows, we might need to hardcode the path to the provided GLEW libraries
if (WIN32)
  find_package(GLEW)
  if (NOT GLEW_FOUND)
    message(STATUS "Including hard-coded GLEW library")
    set(GLEW_INCLUDE_DIRS ${GHOUL_ROOT_DIR}/ext/glew/include)
    set(GLEW_LIBRARIES ${GHOUL_ROOT_DIR}/ext/glew/lib/glew32.lib)
  endif (NOT GLEW_FOUND)
else ()
  # On other systems, it should be enough to just 'find' them
  find_package(GLEW REQUIRED)
endif ()
include_directories(${GLEW_INCLUDE_DIRS})

# Include the OpenGL library
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

# Include Qt 
set(QT_USE_QTOPENGL TRUE)
find_package(Qt4 REQUIRED)
include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})
# Wrap the headers to autogenerate the qt metaobject files
qt4_wrap_cpp(ParticleSimulator_GUI_HEADERS_MOC ${ParticleSimulator_GUI_HEADERS})


##############
# Executable #
##############
# Create the Particle Simulator exectuable
add_executable(ParticleSimulator
    ${ParticleSimulator_GUI_SOURCES}
    ${ParticleSimulator_GUI_HEADERS}
    ${ParticleSimulator_GUI_HEADERS_MOC}
    ${ParticleSimulator_Simulator_SOURCES}
    ${ParticleSimulator_Simulator_HEADERS}
)
target_link_libraries(ParticleSimulator Ghoul ${QT_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES})

# On Windows, we want to automatically copy all the necessary dll files into the build directory
if (WIN32)
    add_custom_command(
        TARGET ParticleSimulator 
        POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GHOUL_ROOT_DIR}/ext/glew/lib/glew32.dll" "$<TARGET_FILE_DIR:ParticleSimulator>"
        POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GHOUL_ROOT_DIR}/ext/glew/lib/glew32mx.dll" "$<TARGET_FILE_DIR:ParticleSimulator>"
    )
    if (CMAKE_SIZEOF_VOID_P EQUAL 4)
        # 32 bit
        add_custom_command(
            TARGET ParticleSimulator 
            POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GHOUL_ROOT_DIR}/ext/il/lib/win32/DevIL.dll" "$<TARGET_FILE_DIR:ParticleSimulator>"
            POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GHOUL_ROOT_DIR}/ext/il/lib/win32/ILU.dll" "$<TARGET_FILE_DIR:ParticleSimulator>"
            POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GHOUL_ROOT_DIR}/ext/il/lib/win32/ILUT.dll" "$<TARGET_FILE_DIR:ParticleSimulator>"
        )
    else ()
        # 64 bit
        add_custom_command(
            TARGET ParticleSimulator 
            POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GHOUL_ROOT_DIR}/ext/il/lib/win64/DevIL.dll" "$<TARGET_FILE_DIR:ParticleSimulator>"
            POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GHOUL_ROOT_DIR}/ext/il/lib/win64/ILU.dll" "$<TARGET_FILE_DIR:ParticleSimulator>"
            POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${GHOUL_ROOT_DIR}/ext/il/lib/win64/ILUT.dll" "$<TARGET_FILE_DIR:ParticleSimulator>"
        )
    endif ()
endif ()
