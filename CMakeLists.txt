cmake_minimum_required(VERSION 3.0)
project(XPlaneCord)

set(XP_SDK "${CMAKE_CURRENT_LIST_DIR}/../../SDK" CACHE STRING "Path to the XPlane sdk")

set(CMAKE_CXX_STANDARD 17)

set(XP_SDK_INCLUDE "${XP_SDK}/CHeaders")
if (MSVC)
    set(XP_LINK_DIRS "${XP_SDK}/Libraries/Win")
    set(XP_LINK_LIBS "XPLM_64 XPWidgets_64")
endif()

add_library(XPlaneCord SHARED src/xplane_cord.cpp)
target_include_directories(XPlaneCord PRIVATE ${XP_SDK_INCLUDE})
if(XP_LINK_DIRS AND XP_LINK_LIBS)
    target_link_directories(XPlaneCord PRIVATE ${XP_LINK_DIRS})
    target_link_libraries(XPlaneCord PRIVATE ${XP_LINK_LIBS})
endif()

if(WIN32)
    target_compile_definitions(XPlaneCord PRIVATE WIN)
elseif(UNIX)
    target_compile_definitions(XPlaneCord PRIVATE LIN)
else()
    message(FATAL_ERROR "Only windows and linux/unix are supported")
endif()
set_target_properties(XPlaneCord PROPERTIES
        PREFIX ""
        SUFFIX ".xpl")