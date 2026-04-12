cmake_minimum_required(VERSION 4.00)
project(GraphicEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui")
set(IMGUI_BACKEND_DIR "${IMGUI_DIR}/backend")
set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/Core")
set(SOIL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/soil")
set(SOIL_LIB_DIR "${SOIL_DIR}/lib")

include_directories(
  ${IMGUI_DIR}
  ${IMGUI_BACKEND_DIR}
  ${SOIL_DIR}
  ${CORE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

file(GLOB_RECURSE SRC_FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/Source/**/*.cpp"
  "${IMGUI_DIR}/*.cpp"
  "${IMGUI_BACKEND_DIR}/*.cpp"
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp REQUIRED CONFIG)

add_library(SOIL STATIC IMPORTED)
set_target_properties(SOIL PROPERTIES 
  IMPORTED_LOCATION "${SOIL_LIB_DIR}/libSOIL.a"
)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${HDR_FILES})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    ${GLFW_LIBRARIES}
    OpenGL::GL
    GLEW::GLEW
    # SOIL
    glm::glm
    assimp::assimp
)

target_link_directories(${PROJECT_NAME} PRIVATE
  ${GLFW_INCLUDE_DIRS}
)


