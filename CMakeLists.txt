cmake_minimum_required(VERSION 3.16)
project(FluidX3D LANGUAGES CXX)

# ============================================================================
# 项目配置
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)

# 设置中间文件目录
set(CMAKE_INTERMEDIATE_DIRECTORY ${CMAKE_SOURCE_DIR}/temp)

# ============================================================================
# 源文件
# ============================================================================
set(SOURCES
    src/graphics.cpp
    src/info.cpp
    src/kernel.cpp
    src/lbm.cpp
    src/lodepng.cpp
    src/main.cpp
    src/setup.cpp
    src/shapes.cpp
)

set(HEADERS
    src/defines.hpp
    src/graphics.hpp
    src/info.hpp
    src/kernel.hpp
    src/lbm.hpp
    src/lodepng.hpp
    src/opencl.hpp
    src/setup.hpp
    src/shapes.hpp
    src/units.hpp
    src/utilities.hpp
)

# ============================================================================
# 可执行文件
# ============================================================================
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# ============================================================================
# 包含目录
# ============================================================================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/OpenCL/include
)

# ============================================================================
# OpenCL 库链接
# ============================================================================
# 使用项目自带的 OpenCL 库
target_link_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src/OpenCL/lib
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenCL
)

# ============================================================================
# 编译选项 (针对 MSVC)
# ============================================================================
if(MSVC)
    # Release 优化选项
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/O2>            # MaxSpeed 优化
        $<$<CONFIG:Release>:/Oi>            # 启用内部函数
        $<$<CONFIG:Release>:/Gy>            # 函数级链接
        $<$<CONFIG:Release>:/GL>            # 全程序优化
        $<$<CONFIG:Release>:/fp:fast>       # 快速浮点模型
        /MP                                  # 多处理器编译
        /W3                                  # 警告级别 3
        /sdl                                 # SDL 检查
        /permissive-                         # 一致性模式
    )
    
    # 禁用特定警告 (26451, 6386)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /wd26451
        /wd6386
    )
    
    # 链接选项
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/LTCG>           # 链接时代码生成
        $<$<CONFIG:Release>:/OPT:REF>        # 优化引用
        $<$<CONFIG:Release>:/OPT:ICF>        # COMDAT 折叠
    )
    
    # 控制台子系统
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    
    # 设置字符集为 MultiByte
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _MBCS
    )
endif()

# ============================================================================
# Windows 资源文件 (图标)
# ============================================================================
if(WIN32 AND MSVC)
    # 添加资源文件
    target_sources(${PROJECT_NAME} PRIVATE
        src/resource.rc
    )
endif()

# ============================================================================
# Debug 配置 (可选)
# ============================================================================
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/Od>              # 禁用优化
        $<$<CONFIG:Debug>:/Zi>              # 调试信息
        $<$<CONFIG:Debug>:/RTC1>            # 运行时检查
    )
endif()

# ============================================================================
# 安装规则 (可选)
# ============================================================================
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
