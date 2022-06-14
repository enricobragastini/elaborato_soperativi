# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/gianni/Documents/Mentos

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/gianni/Documents/Mentos/build

# Utility rule file for qemu-gdb.

# Include any custom commands dependencies for this target.
include CMakeFiles/qemu-gdb.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/qemu-gdb.dir/progress.make

CMakeFiles/qemu-gdb: mentos/libMentOs.a
	xterm -geometry 160x30-0-0 -e qemu-system-i386 -sdl -serial stdio -vga std -k it -m 1096M -s -S -kernel mentos/kernel.bin -initrd initfs &
	xterm -geometry 160x30-0+0 -e cgdb -x /home/gianni/Documents/Mentos/.gdb.debug &

qemu-gdb: CMakeFiles/qemu-gdb
qemu-gdb: CMakeFiles/qemu-gdb.dir/build.make
.PHONY : qemu-gdb

# Rule to build all files generated by this target.
CMakeFiles/qemu-gdb.dir/build: qemu-gdb
.PHONY : CMakeFiles/qemu-gdb.dir/build

CMakeFiles/qemu-gdb.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/qemu-gdb.dir/cmake_clean.cmake
.PHONY : CMakeFiles/qemu-gdb.dir/clean

CMakeFiles/qemu-gdb.dir/depend:
	cd /home/gianni/Documents/Mentos/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/gianni/Documents/Mentos /home/gianni/Documents/Mentos /home/gianni/Documents/Mentos/build /home/gianni/Documents/Mentos/build /home/gianni/Documents/Mentos/build/CMakeFiles/qemu-gdb.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/qemu-gdb.dir/depend
