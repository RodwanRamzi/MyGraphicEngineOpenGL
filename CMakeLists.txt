cmake_minimum_required(VERSION 3.10)
project(MyGraphicEngineOpenGL)

set(CMAKE_CXX_STANDARD 17)

include_directories(
    . 
    imgui
)

set(SOURCES
    main.cpp
    Subsystems.cpp
    Subsystems.h
    GameObject.cpp
    GameObject.h
    imgui/imgui.cpp
    imgui/imgui.h
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_impl_glfw.cpp
    imgui/imgui_impl_glfw.h
    imgui/imgui_impl_opengl3.cpp
    imgui/imgui_impl_opengl3.h
    imgui/imgui_internal.h
    imgui/imconfig.h
    imgui/imstb_rectpack.h
    imgui/imstb_textedit.h
    imgui/imstb_truetype.h
)

add_executable(${MyGraphicEngineOpenGL} ${SOURCES})

find_package(glfw3 REQUIRED)
find_package(glad REQUIRED)
find_package(assimp REQUIRED)

target_link_libraries(${MyGraphicEngineOpenGL} 
    glfw 
    glad 
    assimp
)
