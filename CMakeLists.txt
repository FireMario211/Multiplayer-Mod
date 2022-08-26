cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(PROJECT_NAME Multiplayer) # change this
project(${PROJECT_NAME})

find_file(WINDOWS_HEADER windows.h)
if(NOT WINDOWS_HEADER)
  message(FATAL_ERROR "Can't find windows.h!")
endif()

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
  libs/minhook/include
  libs/gd.h/include
  libs/gd.h/
  libs/cocos-headers/cocos2dx/
  libs/cocos-headers/cocos2dx/include
  libs/cocos-headers/cocos2dx/kazmath/include
  libs/cocos-headers/cocos2dx/platform/win32
  libs/cocos-headers/cocos2dx/platform/third_party/win32
  libs/cocos-headers/cocos2dx/platform/third_party/win32/OGLES
  libs/cocos-headers/extensions/
  libs/socket.io-client-cpp
  libs/socket.io-client-cpp/src
  #libs/pthreads
  #libs/pthreads/include
  
)

add_subdirectory(libs/socket.io-client-cpp)

add_subdirectory(libs/minhook)
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
target_link_libraries(${PROJECT_NAME} minhook sioclient)

target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/libs/cocos-headers/cocos2dx/libcocos2d.lib)
# not needed most of the time
target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/libs/cocos-headers/extensions/libExtensions.lib)
#target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/libs/pthreads/lib/pthreadVC3.lib)