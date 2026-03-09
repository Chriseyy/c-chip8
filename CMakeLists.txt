cmake_minimum_required(VERSION 3.10.0)
project(c-chip8 VERSION 0.1.0 LANGUAGES C)

add_executable(c-chip8 main.c chip8.c)

target_link_libraries(c-chip8 raylib GL m pthread dl rt X11)