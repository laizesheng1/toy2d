cmake_minimum_required(VERSION 3.15)

project(Toy3D LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(Vulkan REQUIRED)
message(STATUS "Vulkan found: ${Vulkan_FOUND}")
find_program(GLSLC_EXECUTABLE glslc REQUIRED)
message(STATUS "glslc found: ${GLSLC_EXECUTABLE}")

set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shader)
set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR})
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
file(GLOB_RECURSE SHADER_SOURCES 
    "${SHADER_SOURCE_DIR}/*.vert"
    "${SHADER_SOURCE_DIR}/*.frag"
)
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SHADER_SPIRV ${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv)

    # �����Զ���������������ɫ��
    add_custom_command(
        OUTPUT ${SHADER_SPIRV}
        COMMAND ${GLSLC_EXECUTABLE} ${SHADER} -o ${SHADER_SPIRV}
        DEPENDS ${SHADER}
        COMMENT "Compiling ${SHADER_NAME} to SPIR-V"
    )

    # �����ɵ� .spv �ļ����ӵ�һ���Զ���Ŀ����
    list(APPEND SPIRV_FILES ${SHADER_SPIRV})
endforeach()

#��������ɫ�������Ϊһ���������Ĳ���
add_custom_target(
    Shaders ALL
    DEPENDS ${SPIRV_FILES}
)

link_directories(${CMAKE_SOURCE_DIR}/packages/glfw/lib)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/packages/glfw/include)
include_directories(${CMAKE_SOURCE_DIR}/packages)
include_directories(${Vulkan_INCLUDE_DIRS})

file(GLOB_RECURSE HEADER_FILES 
    ${CMAKE_SOURCE_DIR}/src/*.h
    ${CMAKE_SOURCE_DIR}/src/*.hpp
)

file(GLOB_RECURSE SOURCE_FILES 
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.c
    ${CMAKE_SOURCE_DIR}/src/*.cc
)
add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

# ��ȷ�����ڱ��� C++ ����֮ǰ����ɫ���Ѿ��������
add_dependencies(${PROJECT_NAME} Shaders)

target_link_libraries(${PROJECT_NAME}  PRIVATE ${Vulkan_LIBRARIES} glfw3.lib)

source_group(TREE "${CMAKE_SOURCE_DIR}/src" 
    PREFIX "Header Files" 
    FILES ${HEADER_FILES}
)

source_group(TREE "${CMAKE_SOURCE_DIR}/src" 
    PREFIX "Source Files" 
    FILES ${SOURCE_FILES}
)
   
