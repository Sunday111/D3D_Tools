cmake_minimum_required(VERSION 3.5.1)
set(module_name D3D_Tools)
set(D3D_Tools true CACHE BOOL "Wrappers for DirectX calls")
if(NOT ${D3D_Tools})
    return()
endif()
add_library(${module_name} INTERFACE)
target_include_directories(${module_name} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${module_name} INTERFACE d3d11.lib DXGI.lib D3DCompiler.lib)
set(added_module_name ${module_name})
