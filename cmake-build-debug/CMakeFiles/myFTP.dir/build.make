# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.13

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake

# The command to remove a file.
RM = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/ozaki/CLionProjects/myftp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/ozaki/CLionProjects/myftp/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/myFTP.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/myFTP.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/myFTP.dir/flags.make

CMakeFiles/myFTP.dir/myftpc.c.o: CMakeFiles/myFTP.dir/flags.make
CMakeFiles/myFTP.dir/myftpc.c.o: ../myftpc.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/ozaki/CLionProjects/myftp/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/myFTP.dir/myftpc.c.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/myFTP.dir/myftpc.c.o   -c /Users/ozaki/CLionProjects/myftp/myftpc.c

CMakeFiles/myFTP.dir/myftpc.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/myFTP.dir/myftpc.c.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/ozaki/CLionProjects/myftp/myftpc.c > CMakeFiles/myFTP.dir/myftpc.c.i

CMakeFiles/myFTP.dir/myftpc.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/myFTP.dir/myftpc.c.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/ozaki/CLionProjects/myftp/myftpc.c -o CMakeFiles/myFTP.dir/myftpc.c.s

CMakeFiles/myFTP.dir/myftpd.c.o: CMakeFiles/myFTP.dir/flags.make
CMakeFiles/myFTP.dir/myftpd.c.o: ../myftpd.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/ozaki/CLionProjects/myftp/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/myFTP.dir/myftpd.c.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/myFTP.dir/myftpd.c.o   -c /Users/ozaki/CLionProjects/myftp/myftpd.c

CMakeFiles/myFTP.dir/myftpd.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/myFTP.dir/myftpd.c.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/ozaki/CLionProjects/myftp/myftpd.c > CMakeFiles/myFTP.dir/myftpd.c.i

CMakeFiles/myFTP.dir/myftpd.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/myFTP.dir/myftpd.c.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/ozaki/CLionProjects/myftp/myftpd.c -o CMakeFiles/myFTP.dir/myftpd.c.s

# Object files for target myFTP
myFTP_OBJECTS = \
"CMakeFiles/myFTP.dir/myftpc.c.o" \
"CMakeFiles/myFTP.dir/myftpd.c.o"

# External object files for target myFTP
myFTP_EXTERNAL_OBJECTS =

myFTP: CMakeFiles/myFTP.dir/myftpc.c.o
myFTP: CMakeFiles/myFTP.dir/myftpd.c.o
myFTP: CMakeFiles/myFTP.dir/build.make
myFTP: CMakeFiles/myFTP.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/ozaki/CLionProjects/myftp/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable myFTP"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/myFTP.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/myFTP.dir/build: myFTP

.PHONY : CMakeFiles/myFTP.dir/build

CMakeFiles/myFTP.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/myFTP.dir/cmake_clean.cmake
.PHONY : CMakeFiles/myFTP.dir/clean

CMakeFiles/myFTP.dir/depend:
	cd /Users/ozaki/CLionProjects/myftp/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/ozaki/CLionProjects/myftp /Users/ozaki/CLionProjects/myftp /Users/ozaki/CLionProjects/myftp/cmake-build-debug /Users/ozaki/CLionProjects/myftp/cmake-build-debug /Users/ozaki/CLionProjects/myftp/cmake-build-debug/CMakeFiles/myFTP.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/myFTP.dir/depend
