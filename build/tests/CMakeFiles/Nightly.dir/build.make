# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.28

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
CMAKE_SOURCE_DIR = /home/carolina/Documentos/masters/1st_year/1st_semester/SDLE/project/g12/external/cppzmq

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/carolina/Documentos/masters/1st_year/1st_semester/SDLE/project/g12/build

# Utility rule file for Nightly.

# Include any custom commands dependencies for this target.
include tests/CMakeFiles/Nightly.dir/compiler_depend.make

# Include the progress variables for this target.
include tests/CMakeFiles/Nightly.dir/progress.make

tests/CMakeFiles/Nightly:
	cd /home/carolina/Documentos/masters/1st_year/1st_semester/SDLE/project/g12/build/tests && /usr/bin/ctest -D Nightly

Nightly: tests/CMakeFiles/Nightly
Nightly: tests/CMakeFiles/Nightly.dir/build.make
.PHONY : Nightly

# Rule to build all files generated by this target.
tests/CMakeFiles/Nightly.dir/build: Nightly
.PHONY : tests/CMakeFiles/Nightly.dir/build

tests/CMakeFiles/Nightly.dir/clean:
	cd /home/carolina/Documentos/masters/1st_year/1st_semester/SDLE/project/g12/build/tests && $(CMAKE_COMMAND) -P CMakeFiles/Nightly.dir/cmake_clean.cmake
.PHONY : tests/CMakeFiles/Nightly.dir/clean

tests/CMakeFiles/Nightly.dir/depend:
	cd /home/carolina/Documentos/masters/1st_year/1st_semester/SDLE/project/g12/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/carolina/Documentos/masters/1st_year/1st_semester/SDLE/project/g12/external/cppzmq /home/carolina/Documentos/masters/1st_year/1st_semester/SDLE/project/g12/external/cppzmq/tests /home/carolina/Documentos/masters/1st_year/1st_semester/SDLE/project/g12/build /home/carolina/Documentos/masters/1st_year/1st_semester/SDLE/project/g12/build/tests /home/carolina/Documentos/masters/1st_year/1st_semester/SDLE/project/g12/build/tests/CMakeFiles/Nightly.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : tests/CMakeFiles/Nightly.dir/depend

