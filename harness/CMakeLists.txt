# Upstream volume-cartographer requires CMake 3.28. This harness deliberately
# does not: it uses no 3.25+ feature, and the CMake bundled with VS 2022 is
# 3.24. Keeping the floor low means the reproducer runs with what the machine
# already has, without installing anything.
cmake_minimum_required(VERSION 3.20)
project(voxelscale_guard_harness CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /permissive- /utf-8 /EHsc)
endif()

# -----------------------------------------------------------------------------
# villa_sources: the real upstream translation units, compiled unmodified.
# -----------------------------------------------------------------------------
add_library(villa_sources STATIC
  src/villa/Json.cpp
  src/villa/vc/core/util/VoxelSizeMetadata.cpp
  src/villa/vc/core/util/RemoteUrl.cpp
)
target_include_directories(villa_sources PUBLIC
  src/villa
  src/villa/utils/include
  third_party
)

# -----------------------------------------------------------------------------
# VoxelScale Guard's own pure resolution unit (no OpenCV/Qt/Volume dependency).
# -----------------------------------------------------------------------------
add_library(vsguard_resolution STATIC
  src/vsguard/render_voxel_size_resolution.cpp
)
target_include_directories(vsguard_resolution PUBLIC src/vsguard)
target_link_libraries(vsguard_resolution PUBLIC villa_sources)

# -----------------------------------------------------------------------------
# Upstream's own unit test for the metadata resolver, compiled unmodified.
# -----------------------------------------------------------------------------
add_executable(test_upstream_voxel_size_metadata src/villa/test_voxel_size_metadata.cpp)
target_link_libraries(test_upstream_voxel_size_metadata PRIVATE villa_sources)
target_include_directories(test_upstream_voxel_size_metadata PRIVATE src/villa third_party)

# -----------------------------------------------------------------------------
# VoxelScale Guard regression tests.
# -----------------------------------------------------------------------------
add_executable(test_render_voxel_size tests/test_render_voxel_size.cpp)
target_link_libraries(test_render_voxel_size PRIVATE vsguard_resolution villa_sources)
target_include_directories(test_render_voxel_size PRIVATE src/villa third_party src/vsguard)

# -----------------------------------------------------------------------------
# probe_render_voxel_size: the before/after demonstrator.
# -----------------------------------------------------------------------------
add_executable(probe_render_voxel_size tools/probe_render_voxel_size.cpp)
target_link_libraries(probe_render_voxel_size PRIVATE vsguard_resolution villa_sources)
target_include_directories(probe_render_voxel_size PRIVATE src/villa third_party src/vsguard)

enable_testing()
add_test(NAME upstream_voxel_size_metadata COMMAND test_upstream_voxel_size_metadata)
add_test(NAME render_voxel_size COMMAND test_render_voxel_size)
