cmake_minimum_required(VERSION 3.0)
project(XPlaneCord)

option(32_BIT "Wether build should produce 32bit binaries." OFF)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(DARWIN TRUE)
endif()

if(UNIX AND NOT DARWIN)
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fPIC)
endif()

if(MSVC AND 32_BIT)
    message(FATAL_ERROR "Cannot force 32bit for MSVC, use a different build generator with CMake!")
else()
    if(32_BIT)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
    endif()
endif()


set(BUILD_EXAMPLES CACHE BOOL OFF FORCE)
set(BUILD_SHARED_LIBS OFF)

add_subdirectory(3rdparty/discord-rpc)
set(3RD_PARTY_INCLUDE
        3rdparty/discord-rpc/include
        3rdparty/X-RAAS2/api/c)

set(3RD_PARTY_SOURCE
        3rdparty/X-RAAS2/api/c/XRAAS_ND_msg_decode.c)

set(XP_SDK "${CMAKE_CURRENT_LIST_DIR}/../../SDK" CACHE STRING "Path to the XPlane sdk")

set(CMAKE_CXX_STANDARD 17)

set(XP_SDK_INCLUDE
        "${XP_SDK}/CHeaders/XPLM"
        "${XP_SDK}/CHeaders/Widgets"
        "${XP_SDK}/CHeaders/Wrappers")

set(SOURCE
        src/xplane_cord.cpp)

add_library(XPlaneCord SHARED ${SOURCE} ${3RD_PARTY_SOURCE})
if(WIN32)
    target_compile_definitions(XPlaneCord PRIVATE WIN IBM)
    if(MINGW)
        set(CMAKE_CXX_FLAGS "-static-libstdc++ -static-libgcc -static ${CMAKE_CXX_FLAGS}")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")
    endif()
    set(XP_LINK_DIRS "${XP_SDK}/Libraries/Win")
    if(32_BIT)
        set(XP_LINK_LIBS XPLM XPWidgets)
    else()
        set(XP_LINK_LIBS XPLM_64 XPWidgets_64)
    endif()
elseif(UNIX AND NOT DARWIN)
    target_compile_definitions(XPlaneCord PRIVATE LIN)
elseif(APPLE)
    target_compile_definitions(XPlaneCord PRIVATE APL)
    target_link_libraries(XPlaneCord PRIVATE "-framework XPLM" "-framework XPWidgets")
    set_target_properties(XPlaneCord PROPERTIES LINK_FLAGS "-Wl,-F${XP_SDK}/Libraries/Mac")
else()
    message(FATAL_ERROR "Only Unix/Linux, OSX and windows are supported!")
endif()

target_include_directories(XPlaneCord PRIVATE ${XP_SDK_INCLUDE} ${3RD_PARTY_INCLUDE})
if(XP_LINK_DIRS AND XP_LINK_LIBS)
    target_link_directories(XPlaneCord PRIVATE ${XP_LINK_DIRS})
    target_link_libraries(XPlaneCord PRIVATE ${XP_LINK_LIBS})
endif()
target_link_libraries(XPlaneCord PRIVATE discord-rpc)

set_target_properties(XPlaneCord PROPERTIES
        PREFIX ""
        SUFFIX ".xpl")

if(UNIX AND NOT DARWIN AND XPLANE_PATH)
    add_custom_target(copy
        COMMAND
            mkdir -p ${XPLANE_PATH}/Resources/plugins/XPlaneCord && cp $<TARGET_FILE:XPlaneCord> ${XPLANE_PATH}/Resources/plugins/XPlaneCord/64/lin.xpl)
    add_dependencies(copy XPlaneCord)
endif()