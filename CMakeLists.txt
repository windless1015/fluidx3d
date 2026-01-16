cmake_minimum_required(VERSION 3.16)
project(FluidX3D LANGUAGES CXX CUDA)

# ============================================================================
# 项目配置
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)

# 设置中间文件目录
set(CMAKE_INTERMEDIATE_DIRECTORY ${CMAKE_SOURCE_DIR}/temp)

# ============================================================================
# 模块化源文件布局
# ============================================================================

# Core 模块 (头文件)
set(CORE_HEADERS
    src/core/defines.hpp
    src/core/opencl.hpp
    src/core/units.hpp
    src/core/utilities.hpp
)

# LBM 模块
set(LBM_SOURCES
    src/lbm/lbm.cpp
    src/lbm/lbm_graphics.cpp
    src/lbm/kernel.cpp
    src/lbm/shapes.cpp
    src/cuda/lbm_cuda.cu
)
set(LBM_HEADERS
    src/lbm/lbm.hpp
    src/lbm/lbm_graphics.hpp
    src/lbm/lbm_interface.hpp
    src/lbm/kernel.hpp
    src/lbm/shapes.hpp
    src/cuda/lbm_cuda.hpp
    src/cuda/cuda_utils.hpp
)

# Render 模块
set(RENDER_SOURCES
    src/render/graphics.cpp
    src/render/lodepng.cpp
)
set(RENDER_HEADERS
    src/render/graphics.hpp
    src/render/lodepng.hpp
)

# OpenGL (GLFW + GLAD) rendering
set(RENDER_GL_SOURCES
    src/render_gl/gl_renderer.cpp
    src/render_gl/glfw_window.cpp
    src/render_gl/gl_raytrace.cpp
)
set(RENDER_GL_HEADERS
    src/render_gl/gl_renderer.hpp
    src/render_gl/glfw_window.hpp
    src/render_gl/gl_raytrace.hpp
)

# App 模块
set(APP_SOURCES
    src/app/main.cpp
    src/app/info.cpp
    src/app/setup.cpp
    src/app/scenes/dam_break.cpp
    src/app/runtime/sim_loop.cpp
)
set(APP_HEADERS
    src/app/info.hpp
    src/app/app_graphics.hpp
    src/app/setup.hpp
    src/app/scenes/dam_break.hpp
    src/app/runtime/sim_loop.hpp
    src/lbm_api/LBMConfig.h
    src/lbm_api/LBMCore.h
    src/lbm_api/FreeSurfaceModule.h
    src/lbm_api/LBMModel.h
)

# 合并所有源文件
set(ALL_SOURCES
    ${LBM_SOURCES}
    ${RENDER_SOURCES}
    ${RENDER_GL_SOURCES}
    ${APP_SOURCES}
)

set(ALL_HEADERS
    ${CORE_HEADERS}
    ${LBM_HEADERS}
    ${RENDER_HEADERS}
    ${RENDER_GL_HEADERS}
    ${APP_HEADERS}
)

# ============================================================================
# 可执行文件
# ============================================================================
add_executable(${PROJECT_NAME} ${ALL_SOURCES} ${ALL_HEADERS})

# ============================================================================
# GLFW + GLAD (OpenGL)
# ============================================================================
include(FetchContent)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)
add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

# ============================================================================
# Visual Studio 文件夹结构 (与磁盘目录一致)
# ============================================================================
source_group("core" FILES ${CORE_HEADERS})
source_group("lbm" FILES ${LBM_SOURCES} ${LBM_HEADERS})
source_group("render" FILES ${RENDER_SOURCES} ${RENDER_HEADERS})
source_group("render_gl" FILES ${RENDER_GL_SOURCES} ${RENDER_GL_HEADERS})
source_group("app" FILES ${APP_SOURCES} ${APP_HEADERS})

# ============================================================================
# 包含目录
# ============================================================================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/lbm
    ${CMAKE_SOURCE_DIR}/src/render
    ${CMAKE_SOURCE_DIR}/src/render_gl
    ${CMAKE_SOURCE_DIR}/src/app
    ${CMAKE_SOURCE_DIR}/src/cuda
    ${CMAKE_SOURCE_DIR}/src/OpenCL/include
)

# ============================================================================
# OpenCL 库链接
# ============================================================================
target_link_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/OpenCL/lib
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenCL
    glfw
    glad
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE opengl32)
endif()

# ============================================================================
# 编译选项 (针对 MSVC)
# ============================================================================
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:/O2>
        $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:/Oi>
        $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:/Gy>
        $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:/GL>
        $<$<AND:$<CONFIG:Release>,$<COMPILE_LANGUAGE:CXX>>:/fp:fast>
        $<$<COMPILE_LANGUAGE:CXX>:/MP>
        $<$<COMPILE_LANGUAGE:CXX>:/W3>
        $<$<COMPILE_LANGUAGE:CXX>:/sdl>
        $<$<COMPILE_LANGUAGE:CXX>:/permissive->
    )
    
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/wd26451>
        $<$<COMPILE_LANGUAGE:CXX>:/wd6386>
    )
    
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
    )
    
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _MBCS
    )
endif()

# ============================================================================
# Windows 资源文件 (图标)
# ============================================================================
if(WIN32 AND MSVC)
    target_sources(${PROJECT_NAME} PRIVATE
        src/resource.rc
    )
endif()

# ============================================================================
# Debug 配置
# ============================================================================
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:CXX>>:/Od>
        $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:CXX>>:/Zi>
        $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANGUAGE:CXX>>:/RTC1>
    )
endif()

# ============================================================================
# 安装规则
# ============================================================================
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
