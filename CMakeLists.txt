################################################################################
# MAIN DECLARATION
################################################################################

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
cmake_policy(SET CMP0074 NEW) # force find_package to take <PackageName>_ROOT variables into account

if(NOT DEFINED PROJECT_ROOT)
  set(PROJECT_ROOT ${CMAKE_SOURCE_DIR} CACHE PATH "Root directory, where the submodules are populated")
endif()


project(
  hrewrite VERSION 0.5.0
  DESCRIPTION "rewriting engine for unranked trees"
  LANGUAGES CXX
)

include(FetchContent)
include(CTest)



################################################################################
# OPTIONS AND COMPILATION FLAGS
################################################################################

option(USE_DEFAULT_FLAGS "Enable default compiler flags" OFF)
option(ENABLE_COVERAGE "Enable coverage" OFF)
option(ENABLE_DOCUMENTATION "Build ${PROJECT_NAME} documentation" OFF)
option(ENABLE_PYTHON "Build ${PROJECT_NAME} python module" OFF)

## Compiler flags
### C++ standard
set(CMAKE_CXX_STANDARD          17 )
set(CMAKE_CXX_EXTENSIONS        OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON )
### fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

### Compiler-dependent flags
# default flags
if (USE_DEFAULT_FLAGS)
  if ($ENV{CXXFLAGS})
    message(WARNING "CXXFLAGS is not empty but ${PROJECT_NAME}_USE_DEFAULT_FLAGS is ON. Your C++ compiler flags will be overriden")
  endif()
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set (CMAKE_CXX_FLAGS "-Wall -Wextra -Wpointer-arith -Wcast-align -fmax-errors=4 -march=native -O3")
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    set (CMAKE_CXX_FLAGS "-Wall -Wcheck -Wpointer-arith")
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set (CMAKE_CXX_FLAGS "-Wall -Wextra -Wpointer-arith -Wno-missing-braces") # missing-braces disabled, else GCC warning
  endif()
endif()

# sanitize build
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set (CMAKE_CXX_FLAGS_SANITIZE "-O0 -g -fsanitize=address -fno-omit-frame-pointer -Wall -Wextra ")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  set (CMAKE_CXX_FLAGS_SANITIZE "-g -O0 -traceback -w2")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set (CMAKE_CXX_FLAGS_SANITIZE "-O0 -g -fsanitize=address -fno-omit-frame-pointer")
else()
  message (WARNING "Unknown C++ compiler ${CMAKE_CXX_COMPILER_ID}")
endif()
set(CMAKE_CXX_FLAGS_SANITIZE  "${CMAKE_CXX_FLAGS_SANITIZE}" CACHE STRING "Flags used by the compiler during sanitize builds" FORCE)
mark_as_advanced(CMAKE_CXX_FLAGS_SANITIZE)

set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
    "Choose the type of build, options are: None Debug Release Sanitize."
    FORCE)

### Default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE) # default to Release
endif()


################################################################################
# DEPENDENCIES
################################################################################

##########################################
# DOCTEST
if (NOT TARGET doctest::doctest)
  find_package(doctest 2.4.11 CONFIG)
endif()
if (NOT TARGET doctest::doctest)
  message("doctest was not found by find_package(). Fetching sources on-the-fly")
  FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
  )
  FetchContent_MakeAvailable(doctest)
endif()

##########################################
# PYTHON AND PYBIND
if (ENABLE_PYTHON)
  # PYTHON 3
  find_package(Python 3 REQUIRED COMPONENTS Interpreter Development)
  # PYTEST
  execute_process(COMMAND "${Python_EXECUTABLE}" "-c" "import pytest"
    RESULT_VARIABLE module_status
  )
  if (NOT ${module_status} EQUAL 0)
      message(FATAL_ERROR " Could not find \"pytest\" python required module")
  endif()
  # PYBIND
  if (NOT TARGET pybind11::pybind11)
    find_package(pybind11 2.10 CONFIG)
  endif()
  if (NOT TARGET pybind11::pybind11)
    message("Pybind11 was not found by find_package(). Fetching sources on-the-fly")
    set(PYBIND11_INSTALL ON CACHE BOOL "${PROJECT_NAME} requires PyBind to export itself" FORCE)
    FetchContent_Declare(
      pybind11
      GIT_REPOSITORY https://github.com/pybind/pybind11.git
      GIT_TAG        v2.10.4
    )
    FetchContent_MakeAvailable(pybind11)
  endif()
endif()

##########################################
# COVERAGE AND STUFF

find_program( MEMORYCHECK_COMMAND valgrind )
set( MEMORYCHECK_COMMAND_OPTIONS "--trace-children=yes --leak-check=full" )

# > Mandatory to put here to have all shared libs compile with the coverage flags
if (ENABLE_COVERAGE)
  include(CodeCoverage)
  APPEND_COVERAGE_COMPILER_FLAGS()
endif()


################################################################################
# COMPILATION
################################################################################

##########################################
# C++ LIBRARY
set(LIBRARY_NAME ${PROJECT_NAME}_lib)
add_library(${LIBRARY_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME}_lib) # create alias
# target_include_directories(${LIBRARY_NAME} INTERFACE ${PROJECT_SOURCE_DIR}/${PROJECT_NAME})
target_include_directories(${LIBRARY_NAME} INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)
# install includes
install(DIRECTORY ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}
  DESTINATION include
  FILES_MATCHING
    PATTERN "*.hpp"
)

##########################################
# TESTS
set(TEST_NAME ${PROJECT_NAME}_test)
file(GLOB_RECURSE test_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/tests/*.test.cpp")
add_executable(${TEST_NAME} ${test_files})

target_link_libraries(${TEST_NAME}
  PUBLIC
    hrewrite_lib
    doctest::doctest
)
install(TARGETS ${TEST_NAME} RUNTIME DESTINATION bin)

add_test(
  NAME ${TEST_NAME}
  COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}
)
set_tests_properties(${TEST_NAME}
  PROPERTIES
    LABELS unit_tests
    SERIAL_RUN $true
    PROCESSORS 4
)

##########################################
# PYTHON API
if (ENABLE_PYTHON)
  set(PYTHON_NAME ${PROJECT_NAME}_python)
  # execute_process (
  #   COMMAND ${Python_EXECUTABLE} -m site --user-site
  #   OUTPUT_VARIABLE PYTHON_INSTALL_PATH_ROOT
  # )
  # set(PYTHON_INSTALL_PATH ${PYTHON_INSTALL_PATH_ROOT}/${PROJECT_NAME})
  set(PYTHON_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}/lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages/hrewrite/")

  file(GLOB_RECURSE python_files CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/python/*.cpp")
  add_library(${PYTHON_NAME} SHARED ${python_files})
  target_link_libraries(${PYTHON_NAME}
    PUBLIC
      hrewrite_lib
      Python::Python
      pybind11::pybind11_headers
  )
  install(TARGETS ${PYTHON_NAME} LIBRARY DESTINATION ${PYTHON_INSTALL_PATH})

  file(GLOB_RECURSE py_files CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/python/*.py")
  foreach (py_file ${py_files})
    file(RELATIVE_PATH py_file_rel  ${CMAKE_CURRENT_SOURCE_DIR}/python ${py_file})

    get_filename_component(py_dir_rel "${py_file_rel}" DIRECTORY)
    install(FILES       "${py_file}"
            DESTINATION "${PYTHON_INSTALL_PATH}/${py_dir_rel}"
            COMPONENT   "Python files")
  endforeach ()
endif()

##########################################
# PYTHON API TESTS

if (ENABLE_PYTHON)
  set(PYTHON_TEST_NAME ${PROJECT_NAME}_python_test)

  # create symlink to the compiled library
  add_test(
    NAME ${PYTHON_TEST_NAME}_symlink
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_BINARY_DIR}/lib${PROJECT_NAME}_python.so ${PROJECT_SOURCE_DIR}/python/lib${PROJECT_NAME}_python.so)
  # actual test
  set(pytest_cmd ${Python_EXECUTABLE} -u -m pytest --rootdir=${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/tests_python -ra -v -s)
  if (ENABLE_COVERAGE)
    set(pytest_cmd coverage run -m ${pytest_cmd})
  endif()

  add_test(
    NAME ${PYTHON_TEST_NAME}
    COMMAND ${pytest_cmd}
  )

  set_tests_properties(
    ${PYTHON_TEST_NAME}
    PROPERTIES
      LABELS unit_tests
      ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}:$ENV{PYTHONPATH}"
      SERIAL_RUN true
      PROCESSORS 4
  )

endif()



################################################################################
# DOCUMENTATION
################################################################################

if (ENABLE_DOCUMENTATION)
  # Download bootstrap style
  include(FetchContent)
  FetchContent_Declare(
    sphinx_bootstrap_theme
    GIT_REPOSITORY https://github.com/ryan-roemer/sphinx-bootstrap-theme.git
    GIT_TAG        v0.8.0
  )
  FetchContent_GetProperties(sphinx_bootstrap_theme)
  if (NOT sphinx_bootstrap_theme_POPULATED)
    FetchContent_Populate(sphinx_bootstrap_theme)
  endif()

  include(UseBreathe)
  add_breathe_doc(TARGET_NAME ${PROJECT_NAME}_doc
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/doc
    DOXY_FILE  ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in
    CONF_FILE  ${CMAKE_CURRENT_SOURCE_DIR}/doc/conf.py.in
    BUILD_DIR  ${CMAKE_CURRENT_BINARY_DIR}/_build
    CACHE_DIR  ${CMAKE_CURRENT_BINARY_DIR}/_doctrees
    HTML_DIR   ${CMAKE_CURRENT_BINARY_DIR}/html
    #ENV_PATH    ${CMAKE_BINARY_DIR}
    COMMENT "HTML documentation"
  )

  install(DIRECTORY   ${CMAKE_CURRENT_BINARY_DIR}/html
          DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc)
endif()


################################################################################
# SUMMARY
################################################################################

get_directory_property(top_lvl_cmakelists PARENT_DIRECTORY)
if(top_lvl_cmakelists STREQUAL ${PROJECT_SOURCE_DIR})
  include(FeatureSummary)
  feature_summary(WHAT ALL)
endif()
