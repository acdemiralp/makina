##################################################    Project     ##################################################
cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
project               (makina VERSION 1.0 LANGUAGES C CXX)
list                  (APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set_property          (GLOBAL PROPERTY USE_FOLDERS ON)
set                   (CMAKE_BUILD_TYPE Release)
set                   (CMAKE_CXX_STANDARD 23)
set                   (CMAKE_CXX_VISIBILITY_PRESET hidden)
set                   (CMAKE_VISIBILITY_INLINES_HIDDEN 1)

if   (MSVC)
  add_compile_options (/W4)
else ()
  add_compile_options (-Wall -Wextra -pedantic)
endif()

##################################################    Options     ##################################################
option      (BUILD_SHARED_LIBS "Build shared (dynamic) libraries." ON)
option      (MAKINA_BUILD_TESTS "Build tests." OFF)
option      (MAKINA_USE_MPI "Use MPI for distributed compute (default: OFF)." OFF)

set         (MAKINA_DEVICE_SYSTEM OMP CACHE STRING "Device system (default: OMP).")
set_property(CACHE MAKINA_DEVICE_SYSTEM PROPERTY STRINGS CPP CUDA OMP TBB)

##################################################    Sources     ##################################################
file(GLOB_RECURSE PROJECT_HEADERS include/*.h include/*.hpp include/*.ipp)
file(GLOB_RECURSE PROJECT_SOURCES source/*.c source/*.cpp source/*.cu)
file(GLOB_RECURSE PROJECT_CMAKE_UTILS cmake/*.cmake)
file(GLOB_RECURSE PROJECT_MISC *.bat *.gitignore *.md *.py *.sh *.txt)
set (PROJECT_EXPORT_HPP include/${PROJECT_NAME}/export.hpp)

# Hack for compiling .cu files as .cpp.
if   (NOT ${MAKINA_DEVICE_SYSTEM} STREQUAL CUDA)
  set_source_files_properties(${PROJECT_SOURCES} PROPERTIES LANGUAGE CXX)
endif()

set (PROJECT_FILES 
  ${PROJECT_HEADERS} 
  ${PROJECT_SOURCES} 
  ${PROJECT_CMAKE_UTILS} 
  ${PROJECT_MISC} 
  ${PROJECT_EXPORT_HPP})
  
include            (assign_source_group)
assign_source_group(${PROJECT_FILES})

##################################################  Dependencies  ##################################################
include(import_library)

find_package     (Eigen3 REQUIRED NO_MODULE)
list             (APPEND PROJECT_LIBRARIES Eigen3::Eigen)

find_package     (nlohmann_json CONFIG REQUIRED)
list             (APPEND PROJECT_LIBRARIES nlohmann_json::nlohmann_json)

find_package     (Taskflow CONFIG REQUIRED)
list             (APPEND PROJECT_LIBRARIES Taskflow::Taskflow)

find_package     (thrust CONFIG REQUIRED)
list             (APPEND PROJECT_LIBRARIES thrust::thrust)

if    (MAKINA_BUILD_TESTS)
  find_package   (doctest CONFIG REQUIRED)
  list           (APPEND PROJECT_LIBRARIES doctest::doctest)
  set            (PROJECT_DEPENDENCY_DOCTEST "find_dependency(doctest)")
endif ()

if    (MAKINA_USE_MPI)
  find_package   (MPI REQUIRED COMPONENTS C)
  list           (APPEND PROJECT_LIBRARIES MPI::MPI_C)
  list           (APPEND PROJECT_COMPILE_DEFINITIONS -DMAKINA_USE_MPI)
  set            (PROJECT_DEPENDENCY_MPI "find_dependency(MPI)")
endif ()

if    (${MAKINA_DEVICE_SYSTEM} STREQUAL CUDA)
  enable_language(CUDA)
  find_package   (CUDA REQUIRED)
  set            (CMAKE_CUDA_ARCHITECTURES "all-major")
  import_library (CUDA_INCLUDE_DIRS CUDA_LIBRARIES)
  list           (APPEND PROJECT_COMPILE_DEFINITIONS -DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CUDA)
  list           (APPEND PROJECT_COMPILE_OPTIONS $<$<COMPILE_LANGUAGE:CUDA>:--extended-lambda --expt-relaxed-constexpr --use_fast_math>)
  set            (PROJECT_DEPENDENCY_DEVICE "find_dependency(CUDA)")
elseif(${MAKINA_DEVICE_SYSTEM} STREQUAL OMP)
  find_package   (OpenMP REQUIRED)
  list           (APPEND PROJECT_LIBRARIES OpenMP::OpenMP_CXX)
  list           (APPEND PROJECT_COMPILE_DEFINITIONS -DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_OMP )
  set            (PROJECT_DEPENDENCY_DEVICE "find_dependency(OpenMP)")
elseif(${MAKINA_DEVICE_SYSTEM} STREQUAL TBB)
  find_package   (TBB REQUIRED)
  list           (APPEND PROJECT_LIBRARIES TBB::tbb TBB::tbbmalloc)
  list           (APPEND PROJECT_COMPILE_DEFINITIONS -DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_TBB )
  set            (PROJECT_DEPENDENCY_DEVICE "find_dependency(TBB)")
elseif(${MAKINA_DEVICE_SYSTEM} STREQUAL CPP)
  list           (APPEND PROJECT_COMPILE_DEFINITIONS -DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CPP )
endif ()

##################################################    Targets     ##################################################
add_library(${PROJECT_NAME} ${PROJECT_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
  $<INSTALL_INTERFACE:include> PRIVATE source)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDE_DIRS})
target_link_libraries     (${PROJECT_NAME} PUBLIC ${PROJECT_LIBRARIES})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${PROJECT_COMPILE_DEFINITIONS})
target_compile_options    (${PROJECT_NAME} PUBLIC ${PROJECT_COMPILE_OPTIONS})
set_target_properties     (${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

if(NOT BUILD_SHARED_LIBS)
  string               (TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)
  set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS -D${PROJECT_NAME_UPPER}_STATIC)
endif()

##################################################    Postbuild   ##################################################
include               (GenerateExportHeader)
string                (TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)
generate_export_header(${PROJECT_NAME} 
  EXPORT_FILE_NAME     ${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME}/export.hpp
  EXPORT_MACRO_NAME    ${PROJECT_NAME_UPPER}_EXPORT
  STATIC_DEFINE        ${PROJECT_NAME_UPPER}_STATIC
)

##################################################    Testing     ##################################################
if(MAKINA_BUILD_TESTS)
  enable_testing       ()
  set                  (TEST_MAIN_NAME test_main)
  set                  (TEST_MAIN_SOURCES tests/internal/main.cpp)
  add_library          (${TEST_MAIN_NAME} OBJECT ${TEST_MAIN_SOURCES})
  target_link_libraries(${TEST_MAIN_NAME} ${PROJECT_NAME})
  set_property         (TARGET ${TEST_MAIN_NAME} PROPERTY FOLDER tests)
  assign_source_group  (${TEST_MAIN_SOURCES})

  file(GLOB_RECURSE PROJECT_TEST_CPPS tests/*.c tests/*.cpp tests/*.cu)
  
  # Hack for compiling .cu files as .cpp.
  if   (NOT ${MAKINA_DEVICE_SYSTEM} STREQUAL CUDA)
    set_source_files_properties(${PROJECT_TEST_CPPS} PROPERTIES LANGUAGE CXX)
  endif()

  foreach(_SOURCE ${PROJECT_TEST_CPPS})
    get_filename_component(_NAME      ${_SOURCE}    NAME_WE  )
    get_filename_component(_DIRECTORY ${_SOURCE}    DIRECTORY)
    get_filename_component(_PARENT    ${_DIRECTORY} NAME     )
    add_executable        (${_NAME} ${_SOURCE} $<TARGET_OBJECTS:${TEST_MAIN_NAME}>)
    target_link_libraries (${_NAME} ${PROJECT_NAME} ${PROJECT_LIBRARIES})
    add_test              (${_NAME} ${_NAME})
    set_property          (TARGET ${_NAME} PROPERTY FOLDER tests/${_PARENT})
    assign_source_group   (${_SOURCE})
  endforeach()
endif()

##################################################  Installation  ##################################################
install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}-config
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)
install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME})
install(EXPORT  ${PROJECT_NAME} FILE   ${PROJECT_NAME}-targets.cmake NAMESPACE ${PROJECT_NAME}:: DESTINATION cmake)
export (TARGETS ${PROJECT_NAME} FILE   ${PROJECT_NAME}-targets.cmake)

include(CMakePackageConfigHelpers)
configure_package_config_file   (
  ${PROJECT_NAME}-config.cmake.in  
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake INSTALL_DESTINATION cmake)
write_basic_package_version_file(
  ${PROJECT_NAME}-config-version.cmake 
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
  DESTINATION cmake)