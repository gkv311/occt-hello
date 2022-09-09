cmake_minimum_required (VERSION 2.8)

# common solution for multiple independent projects
project (occt-hello)

add_subdirectory (occt-ais-hello)
add_subdirectory (occt-ais-object)
add_subdirectory (occt-ais-offscreen)
add_subdirectory (occt-draw-plugin)
add_subdirectory (occt-xcaf-shape)
