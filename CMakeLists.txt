cmake_minimum_required(VERSION 3.20)
project(MyGraphicEngineOpenGL)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================================
# تعريفات المعالج
# ==========================================
add_compile_definitions(
    _CONSOLE
    JPH_PROFILE_ENABLED
    JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
    JPH_USE_AVX2
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

# ==========================================
# ملفات المصدر
# ==========================================
add_executable(MyGraphicEngineOpenGL
    Main.cpp
    Camera.cpp
    ComputeShader.cpp
    EBO.cpp
    Editor.cpp
    GameObject.cpp
    BloomPass.cpp
    AsyncLoader.cpp	
    CSM.cpp	
    glad.c
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_impl_glfw.cpp
    imgui/imgui_impl_opengl3.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    Libraries/include/imguizmo/ImGuizmo.cpp
    LuaManager.cpp
    LUT.cpp
    Mesh.cpp
    ScriptComponent.cpp
    shaderClass.cpp
    Model.cpp
    stb.cpp
    Terrain.cpp
    Texture.cpp
    VAO.cpp
    VBO.cpp
    src/utils/PathUtils.cpp
    src/utils/FileUtils.cpp
    src/core/JoltCallbacks.cpp
    src/renderer/Frustum.cpp
    src/physics/Collision.cpp
    src/editor/Theme.cpp
)

# ملفات Jolt Physics (جمع تلقائي)
file(GLOB_RECURSE JOLT_SOURCES "Libraries/include/Jolt/*.cpp")
target_sources(MyGraphicEngineOpenGL PRIVATE ${JOLT_SOURCES})

# ==========================================
# مسارات الـ Include
# ==========================================
target_include_directories(MyGraphicEngineOpenGL PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/include/sol
    ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/include/lua
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

# ==========================================
# مسارات المكتبات (Library Directories)
# ==========================================
target_link_directories(MyGraphicEngineOpenGL PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/lib
)



# ==========================================
# مسارات Windows SDK lib (kernel32.lib, user32.lib, etc.)
# ==========================================
file(GLOB WINSDK_VERSIONS "C:/Program Files (x86)/Windows Kits/10/Lib/10.*")

if(WINSDK_VERSIONS)
    list(SORT WINSDK_VERSIONS)
    list(REVERSE WINSDK_VERSIONS)
    list(GET WINSDK_VERSIONS 0 WINSDK_LATEST)
    
    target_link_directories(MyGraphicEngineOpenGL PRIVATE
        "${WINSDK_LATEST}/um/x64"
        "${WINSDK_LATEST}/ucrt/x64"
    )
    message(STATUS "Windows SDK lib: ${WINSDK_LATEST}")
else()
    message(FATAL_ERROR "Windows SDK NOT FOUND! Please install Windows 10/11 SDK.")
endif()

# ==========================================
# مسار مكتبات MSVC (msvcprt.lib, libcpmt.lib)
# ==========================================
if(MSVC)
    string(REGEX REPLACE ".*/MSVC/([0-9]+\\.[0-9]+\\.[0-9]+)/.*" "\\1" MSVC_VER "${CMAKE_CXX_COMPILER}")
    
    file(GLOB MSVC_LIB_DIRS
        "F:/Microsoft Visual Studio 2022/2022/BuildTools/VC/Tools/MSVC/${MSVC_VER}/lib/x64"
        "C:/Program Files/Microsoft Visual Studio/2022/*/VC/Tools/MSVC/${MSVC_VER}/lib/x64"
        "C:/Program Files (x86)/Microsoft Visual Studio/2022/*/VC/Tools/MSVC/${MSVC_VER}/lib/x64"
    )
    
    if(MSVC_LIB_DIRS)
        list(GET MSVC_LIB_DIRS 0 MSVC_LIB_PATH)
        target_link_directories(MyGraphicEngineOpenGL PRIVATE "${MSVC_LIB_PATH}")
        message(STATUS "MSVC lib: ${MSVC_LIB_PATH}")
    else()
        message(FATAL_ERROR "MSVC lib NOT FOUND for version ${MSVC_VER}!")
    endif()
endif()

# ==========================================
# البحث عن OpenGL
# ==========================================
find_package(OpenGL REQUIRED)

# ==========================================
# ربط المكتبات
# ==========================================
target_link_libraries(MyGraphicEngineOpenGL PRIVATE
    glfw3.lib
    OpenGL::GL
    lua54.lib
)

# ==========================================
# نسخ ملفات الـ DLL بعد البناء
# ==========================================
add_custom_command(TARGET MyGraphicEngineOpenGL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/bin"
    "$<TARGET_FILE_DIR:MyGraphicEngineOpenGL>"
    COMMENT "Copying DLLs..."
)

# ==========================================
# نسخ الـ Shaders (من جذر المشروع — ملفات بامتدادات)
# ==========================================
file(GLOB SHADER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.geom"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.comp"
)

foreach(shader ${SHADER_FILES})
    add_custom_command(TARGET MyGraphicEngineOpenGL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${shader}"
        "$<TARGET_FILE_DIR:MyGraphicEngineOpenGL>"
        COMMENT "Copying shader: ${shader}"
    )
endforeach()

# ==========================================
# نسخ ملفات JSON (Levels وغيرها)
# ==========================================
file(GLOB JSON_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.json")
foreach(json ${JSON_FILES})
    add_custom_command(TARGET MyGraphicEngineOpenGL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${json}"
        "$<TARGET_FILE_DIR:MyGraphicEngineOpenGL>"
    )
endforeach()

# ==========================================
# نسخ الفولدرات (Models, Levels, Scripts, cubemap, luts, textures...)
# ==========================================
foreach(folder_name "models" "Models" "Levels" "levels" "Scripts" "scripts" "cubemap" "luts" "textures" "Textures" "Audio" "Fonts")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${folder_name}")
        add_custom_command(TARGET MyGraphicEngineOpenGL POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/${folder_name}"
            "$<TARGET_FILE_DIR:MyGraphicEngineOpenGL>/${folder_name}"
            COMMENT "Copying folder: ${folder_name}"
        )
    endif()
endforeach()

# ==========================================
# نسخ أي صور في الجذر
# ==========================================
file(GLOB ROOT_IMAGES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.jpg"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.jpeg"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hdr"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.bmp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.tga"
)
foreach(img ${ROOT_IMAGES})
    add_custom_command(TARGET MyGraphicEngineOpenGL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${img}"
        "$<TARGET_FILE_DIR:MyGraphicEngineOpenGL>"
    )
endforeach()

# ==========================================
# نسخ كل ملفات .gltf و .glb و .fbx في الجذر
# ==========================================
file(GLOB ROOT_MODELS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.gltf"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.glb"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.fbx"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.FBX"
)
foreach(model ${ROOT_MODELS})
    add_custom_command(TARGET MyGraphicEngineOpenGL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${model}"
        "$<TARGET_FILE_DIR:MyGraphicEngineOpenGL>"
    )
endforeach()

# ==========================================
# رسالة نهائية
# ==========================================
message(STATUS "========================================")
message(STATUS "  Rodwan Engine - Configuration Complete")
message(STATUS "  Source: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "  Output: $<TARGET_FILE_DIR:MyGraphicEngineOpenGL>")
message(STATUS "========================================")