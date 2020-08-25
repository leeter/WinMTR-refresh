cmake_minimum_required(VERSION 3.10)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)
# set the project name
project(WinMTR VERSION 0.10.0.1)

#if (MSVC_VERSION GREATER_EQUAL "1900")
#    include(CheckCXXCompilerFlag)
#    CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
#    if (_cpp_latest_flag_supported)
#        add_compile_options("/std:c++latest")
#    endif()
#endif()

add_definitions(-D_AFXDLL -DUNICODE -D_UNICODE)
set(CMAKE_MFC_FLAG 1)
file(GLOB WinMTR_HEADERS *.h)
file(GLOB WinMTR_SOURCE *.cpp)
file(GLOB WinMTR_RESOURCE *.rc)



# add the executable
add_executable(WinMTR WIN32 ${WinMTR_HEADERS} ${WinMTR_SOURCE} ${WinMTR_RESOURCE})


configure_file(WinMTRConfig.h.in WinMTRConfig.h)

#target_include_directories(WinMTR PUBLIC
#                           "${PROJECT_BINARY_DIR}"
#                           "${CMAKE_CURRENT_SOURCE_DIR}"
#                           )
target_link_libraries(WinMTR onecore.lib)


