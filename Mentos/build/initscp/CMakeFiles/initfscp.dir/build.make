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

# Include any dependencies generated for this target.
include initscp/CMakeFiles/initfscp.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include initscp/CMakeFiles/initfscp.dir/compiler_depend.make

# Include the progress variables for this target.
include initscp/CMakeFiles/initfscp.dir/progress.make

# Include the compile flags for this target's objects.
include initscp/CMakeFiles/initfscp.dir/flags.make

initscp/CMakeFiles/initfscp.dir/src/initfscp.c.o: initscp/CMakeFiles/initfscp.dir/flags.make
initscp/CMakeFiles/initfscp.dir/src/initfscp.c.o: ../initscp/src/initfscp.c
initscp/CMakeFiles/initfscp.dir/src/initfscp.c.o: initscp/CMakeFiles/initfscp.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/gianni/Documents/Mentos/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object initscp/CMakeFiles/initfscp.dir/src/initfscp.c.o"
	cd /home/gianni/Documents/Mentos/build/initscp && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT initscp/CMakeFiles/initfscp.dir/src/initfscp.c.o -MF CMakeFiles/initfscp.dir/src/initfscp.c.o.d -o CMakeFiles/initfscp.dir/src/initfscp.c.o -c /home/gianni/Documents/Mentos/initscp/src/initfscp.c

initscp/CMakeFiles/initfscp.dir/src/initfscp.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/initfscp.dir/src/initfscp.c.i"
	cd /home/gianni/Documents/Mentos/build/initscp && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/gianni/Documents/Mentos/initscp/src/initfscp.c > CMakeFiles/initfscp.dir/src/initfscp.c.i

initscp/CMakeFiles/initfscp.dir/src/initfscp.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/initfscp.dir/src/initfscp.c.s"
	cd /home/gianni/Documents/Mentos/build/initscp && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/gianni/Documents/Mentos/initscp/src/initfscp.c -o CMakeFiles/initfscp.dir/src/initfscp.c.s

# Object files for target initfscp
initfscp_OBJECTS = \
"CMakeFiles/initfscp.dir/src/initfscp.c.o"

# External object files for target initfscp
initfscp_EXTERNAL_OBJECTS =

initscp/initfscp: initscp/CMakeFiles/initfscp.dir/src/initfscp.c.o
initscp/initfscp: initscp/CMakeFiles/initfscp.dir/build.make
initscp/initfscp: initscp/CMakeFiles/initfscp.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/gianni/Documents/Mentos/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable initfscp"
	cd /home/gianni/Documents/Mentos/build/initscp && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/initfscp.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
initscp/CMakeFiles/initfscp.dir/build: initscp/initfscp
.PHONY : initscp/CMakeFiles/initfscp.dir/build

initscp/CMakeFiles/initfscp.dir/clean:
	cd /home/gianni/Documents/Mentos/build/initscp && $(CMAKE_COMMAND) -P CMakeFiles/initfscp.dir/cmake_clean.cmake
.PHONY : initscp/CMakeFiles/initfscp.dir/clean

initscp/CMakeFiles/initfscp.dir/depend:
	cd /home/gianni/Documents/Mentos/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/gianni/Documents/Mentos /home/gianni/Documents/Mentos/initscp /home/gianni/Documents/Mentos/build /home/gianni/Documents/Mentos/build/initscp /home/gianni/Documents/Mentos/build/initscp/CMakeFiles/initfscp.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : initscp/CMakeFiles/initfscp.dir/depend
