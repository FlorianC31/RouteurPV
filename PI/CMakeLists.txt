cmake_minimum_required(VERSION 3.16)

project(serial_data)

set(CMAKE_CXX_STANDARD 11)

# Ajout de l'exécutable
add_executable(PiRouteur src/main.cpp)

# Inclusion de la bibliothèque pigpio
target_link_libraries(PiRouteur pigpio)
