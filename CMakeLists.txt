cmake_minimum_required(VERSION 3.10)

project(LinearAlgebraCalSys LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置可执行文件输出路径
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src  # 指向项目中的 boost 目录
)
# 收集源文件
file(GLOB_RECURSE APP_SOURCES 
    "src/determinant_expansion.cpp"
    "src/fraction.cpp"
    "src/matrix.cpp"
    "src/matrix_operations.cpp"
    "src/operation_step.cpp"
    "src/vector.cpp"
    "src/result.cpp"
    "src/similar_matrix_operations.cpp"
    "src/vectorset_operation.cpp"
    "src/equationset.cpp" # 添加新文件


    "src/main.cpp"
    
    "src/grammar/*.cpp"
    "src/tui/*.cpp"
    "src/utils/*.cpp"
)

file(GLOB_RECURSE TUI_SOURCES 
     "src/determinant_expansion.cpp"
    "src/fraction.cpp"
    "src/matrix.cpp"
    "src/matrix_operations.cpp"
    "src/operation_step.cpp"
    "src/vector.cpp"
    "src/result.cpp"
    "src/similar_matrix_operations.cpp"
    "src/equationset.cpp" # 添加新文件
    
    "src/grammar/*.cpp"
    "src/tui/*.cpp"
    "src/utils/*.cpp"
)

# 添加预编译的资源文件
set(RESOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/res/resource.o)
if(NOT EXISTS ${RESOURCE_FILE})
    message(WARNING "Resource file ${RESOURCE_FILE} not found. It will not be linked.")
    set(APP_SOURCES ${APP_SOURCES}) # Keep APP_SOURCES as is
else()
    # 将 resource.o 添加到源文件列表，以便链接器可以找到它
    # 或者，如果它是特定于Windows的资源，则应从.rc文件编译
    # 此处假设它是一个普通的目标文件
    list(APPEND APP_SOURCES ${RESOURCE_FILE})
endif()


# 添加主可执行文件 ------------------
add_executable(LinearAlgebraCalSys ${APP_SOURCES})

# 添加编译选项
target_compile_options(LinearAlgebraCalSys PRIVATE
 # 用于调试
    -O2 # 用于优化，根据需要取消注释
    -fopenmp # 如果使用OpenMP，根据需要取消注释
)

# 查找 OpenMP 包
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    # 添加 OpenMP 编译和链接标志
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    link_libraries(OpenMP::OpenMP_CXX)
endif()

# 确保 OpenMP 标志被应用到目标
target_link_libraries(LinearAlgebraCalSys PRIVATE OpenMP::OpenMP_CXX)

# 添加链接选项
target_link_options(LinearAlgebraCalSys PRIVATE )

# 链接库
target_link_libraries(LinearAlgebraCalSys PRIVATE
    advapi32
    gdi32
    winmm
    fmt
)

# 添加测试可执行文件 - 第一阶段
add_executable(test_phase1 
    test/test_phase1.cpp
    src/fraction.cpp
    src/matrix.cpp
    src/vector.cpp
)

# 为测试添加编译选项
target_compile_options(test_phase1 PRIVATE -g)

# 为测试添加链接选项
target_link_options(test_phase1 PRIVATE -static)

# 为测试链接库
target_link_libraries(test_phase1 PRIVATE
    advapi32
    gdi32
    winmm
    fmt
)

# 添加测试可执行文件 - 第二阶段
add_executable(test_phase2
    test/test_phase2.cpp
    src/fraction.cpp
    src/matrix.cpp
    src/vector.cpp
    src/operation_step.cpp
    src/matrix_operations.cpp
)

# 为第二阶段测试添加编译选项
target_compile_options(test_phase2 PRIVATE -g)

# 为第二阶段测试添加链接选项
target_link_options(test_phase2 PRIVATE -static)

# 为第二阶段测试链接库
target_link_libraries(test_phase2 PRIVATE
    advapi32
    gdi32
    winmm
    fmt
)

# 添加第三阶段测试可执行文件
add_executable(test_phase3
    test/test_phase3.cpp
    src/fraction.cpp
    src/matrix.cpp
    src/vector.cpp
    src/operation_step.cpp
    src/matrix_operations.cpp
    src/determinant_expansion.cpp
    src/similar_matrix_operations.cpp
    src/equationset.cpp # 添加到测试
)

# 为第三阶段测试添加编译选项
target_compile_options(test_phase3 PRIVATE -g)

# 为第三阶段测试添加链接选项
target_link_options(test_phase3 PRIVATE -static)

# 为第三阶段测试链接库
target_link_libraries(test_phase3 PRIVATE
    advapi32
    gdi32
    winmm
    fmt
)

# 添加第四阶段测试可执行文件
add_executable(test_phase4
    test/test_phase4.cpp
    src/fraction.cpp
    src/matrix.cpp
    src/vector.cpp
    src/operation_step.cpp
    src/matrix_operations.cpp
    src/determinant_expansion.cpp
    src/similar_matrix_operations.cpp
    src/equationset.cpp # 添加到测试
)

# 为第四阶段测试添加编译选项
target_compile_options(test_phase4 PRIVATE -g)

# 为第四阶段测试添加链接选项
target_link_options(test_phase4 PRIVATE -static)

# 为第四阶段测试链接库
target_link_libraries(test_phase4 PRIVATE
    advapi32
    gdi32
    winmm
    fmt
)

# 添加第五阶段测试可执行文件 (方程组求解测试)
add_executable(test_phase5
    test/test_phase5.cpp
    src/fraction.cpp
    src/matrix.cpp
    src/vector.cpp
    src/operation_step.cpp
    src/matrix_operations.cpp
    src/determinant_expansion.cpp
    src/similar_matrix_operations.cpp
    src/equationset.cpp # 添加到测试
)

# 为第五阶段测试添加编译选项
target_compile_options(test_phase5 PRIVATE -g)

# 为第五阶段测试添加链接选项
target_link_options(test_phase5 PRIVATE -static)

# 为第五阶段测试链接库
target_link_libraries(test_phase5 PRIVATE
    advapi32
    gdi32
    winmm
    fmt
)

# 添加鼠标测试可执行文件
# add_executable(test_mouse test/test_mouse.cpp ${TUI_SOURCES})
# target_include_directories(test_mouse PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src) # To find ../src/tui/tui_terminal.h

message(STATUS "Project Name: ${PROJECT_NAME}")
message(STATUS "Executable will be placed in: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "Sources: ${APP_SOURCES}")
message(STATUS "Test executables: test_phase1, test_phase2, test_phase3, test_phase4, test_phase5")
