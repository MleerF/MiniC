cmake_minimum_required(VERSION 3.20)
project(MinCalendar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# --- Optimización de tamaño / RAM ---
if(MSVC)
  add_compile_options(/O2 /MT /Gy /Gw /GL /utf-8)
  add_link_options(/LTCG /OPT:REF /OPT:ICF)
else()
  add_compile_options(-O2 -flto)
  add_link_options(-flto -s)
endif()

# --- Dependencias ---
include(FetchContent)

set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.90.9)
FetchContent_MakeAvailable(imgui)

# --- Ejecutable ---
add_executable(mincalendar
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/event_store.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/autostart.cpp
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)

target_include_directories(mincalendar PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends)

target_link_libraries(mincalendar PRIVATE glfw)

if(WIN32)
  target_link_libraries(mincalendar PRIVATE opengl32)
else()
  find_package(OpenGL REQUIRED)
  target_link_libraries(mincalendar PRIVATE OpenGL::GL dl pthread)
endif()

# OJO: IMGUI_DISABLE_METRICS_WINDOW → IMGUI_DISABLE_DEBUG_TOOLS (renombrado en 1.90)
target_compile_definitions(mincalendar PRIVATE
  IMGUI_DISABLE_DEMO_WINDOWS
  IMGUI_DISABLE_OBSOLETE_FUNCTIONS
  IMGUI_DISABLE_DEBUG_TOOLS)
# Sin ventana de consola en Windows
if(WIN32)
  set_target_properties(mincalendar PROPERTIES WIN32_EXECUTABLE TRUE)
  target_link_options(mincalendar PRIVATE /ENTRY:mainCRTStartup)
endif()