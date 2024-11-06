cmake_minimum_required(VERSION 3.10)

# Set the project name
project(central C)

# Add the executable
add_executable(central src/main.c)

# Include directories
target_include_directories(central PRIVATE common)