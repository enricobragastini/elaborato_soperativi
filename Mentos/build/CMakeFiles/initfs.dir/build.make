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

# Utility rule file for initfs.

# Include any custom commands dependencies for this target.
include CMakeFiles/initfs.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/initfs.dir/progress.make

CMakeFiles/initfs: initscp/initfscp
	echo ---------------------------------------------
	echo Initializing\ 'initfs'...
	echo ---------------------------------------------
	./initscp/initfscp -s /home/gianni/Documents/Mentos/files -t /home/gianni/Documents/Mentos/build/initfs -m /dev
	echo ---------------------------------------------
	echo Done!
	echo ---------------------------------------------

initfs: CMakeFiles/initfs
initfs: CMakeFiles/initfs.dir/build.make
.PHONY : initfs

# Rule to build all files generated by this target.
CMakeFiles/initfs.dir/build: initfs
.PHONY : CMakeFiles/initfs.dir/build

CMakeFiles/initfs.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/initfs.dir/cmake_clean.cmake
.PHONY : CMakeFiles/initfs.dir/clean

CMakeFiles/initfs.dir/depend:
	cd /home/gianni/Documents/Mentos/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/gianni/Documents/Mentos /home/gianni/Documents/Mentos /home/gianni/Documents/Mentos/build /home/gianni/Documents/Mentos/build /home/gianni/Documents/Mentos/build/CMakeFiles/initfs.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/initfs.dir/depend

