cmake_minimum_required(VERSION 3.27)
project(kaminari LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Torch REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)
find_program(KAMINARI_GLSLC glslc HINTS ENV VULKAN_SDK PATH_SUFFIXES bin REQUIRED)

set(KAMINARI_SHADER "${CMAKE_CURRENT_SOURCE_DIR}/shaders/add.comp")
set(KAMINARI_SPV "${CMAKE_BINARY_DIR}/add.spv")
set(KAMINARI_SHADER_HEADER "${CMAKE_BINARY_DIR}/generated/add_spirv.h")

add_custom_command(
  OUTPUT "${KAMINARI_SPV}"
  COMMAND "${KAMINARI_GLSLC}" --target-env=vulkan1.3 -fshader-stage=comp
          "${KAMINARI_SHADER}" -o "${KAMINARI_SPV}"
  DEPENDS "${KAMINARI_SHADER}"
  VERBATIM)
add_custom_command(
  OUTPUT "${KAMINARI_SHADER_HEADER}"
  COMMAND "${CMAKE_COMMAND}" -E env
          "KAMINARI_SPV=${KAMINARI_SPV}"
          "KAMINARI_HEADER=${KAMINARI_SHADER_HEADER}"
          "${Python3_EXECUTABLE}"
          "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_spirv.py"
  DEPENDS "${KAMINARI_SPV}"
          "${CMAKE_CURRENT_SOURCE_DIR}/tools/embed_spirv.py"
  VERBATIM)

pybind11_add_module(
  _C
  csrc/Module.cpp
  csrc/Guard.cpp
  csrc/Runtime.cpp
  csrc/Allocator.cpp
  csrc/VulkanMemory.cpp
  csrc/Kernel.cpp
  csrc/Ops.cpp
  "${KAMINARI_SHADER_HEADER}")
target_compile_definitions(_C PRIVATE TORCH_EXTENSION_NAME=_C)
target_include_directories(_C PRIVATE ${TORCH_INCLUDE_DIRS} "${CMAKE_BINARY_DIR}/generated")
target_compile_options(_C PRIVATE ${TORCH_CXX_FLAGS})
find_library(TORCH_PYTHON_LIBRARY torch_python PATHS "${TORCH_INSTALL_PREFIX}/lib" REQUIRED)
target_link_libraries(
  _C
  PRIVATE ${TORCH_LIBRARIES} ${TORCH_PYTHON_LIBRARY} Vulkan::Vulkan)

set_target_properties(_C PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/torch_kaminari"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/torch_kaminari"
)

foreach(file __init__.py device_module.py _utils.py)
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/torch_kaminari/${file}"
    "${CMAKE_BINARY_DIR}/torch_kaminari/${file}" COPYONLY)
endforeach()

install(DIRECTORY "${CMAKE_BINARY_DIR}/torch_kaminari/" DESTINATION .)
