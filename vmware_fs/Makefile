## BeOS Generic Makefile v2.1 ##

## Fill in this file to specify the project being created, and the referenced
## makefile-engine will do all of the hard work for you. This handles both
## Intel and PowerPC builds of the BeOS.

## Application Specific Settings ---------------------------------------------

# specify the name of the binary
NAME= vmwfs

# specify the type of binary
#	APP:	Application
#	SHARED:	Shared library or add-on
#	STATIC:	Static library archive
#	DRIVER: Kernel Driver
TYPE= DRIVER

#	add support for new Pe and Eddie features
#	to fill in generic makefile

#%{
# @src->@

#	specify the source files to use
#	full paths or paths relative to the makefile can be included
# 	all files, regardless of directory, will have their object
#	files created in the common object directory.
#	Note that this means this makefile will not work correctly
#	if two source files with the same name (source.c or source.cpp)
#	are included from different directories. Also note that spaces
#	in folder names do not work well with this makefile.
SRCS= dir_operations.cpp file_operations.cpp kernel_interface.cpp \
	../shared_sources/VMWCoreBackdoor.cpp VMWNode.cpp \
	VMWSharedFolders.cpp vnode_operations.cpp volume_operations.cpp


#	specify the resource files to use
#	full path or a relative path to the resource file can be used.
RSRCS=

# @<-src@
#%}

#	end support for Pe and Eddie

#	specify additional libraries to link against
#	there are two acceptable forms of library specifications
#	-	if your library follows the naming pattern of:
#		libXXX.so or libXXX.a you can simply specify XXX
#		library: libbe.so entry: be
#
#	- 	if your library does not follow the standard library
#		naming scheme you need to specify the path to the library
#		and it's name
#		library: my_lib.a entry: my_lib.a or path/my_lib.a
#  ---
#
#  Haiku drivers cannot depend on libstdc++.a, we use libsupc++-kernel.a instead.
#  For gcc2 we link just libgcc.a, as there's no libsupc++-kernel.a
#
#  As stated by https://gcc.gnu.org/onlinedocs/libstdc++/faq.html#faq.what_is_libsupcxx
#    If the only functions from libstdc++.a which you need are language support functions
#    (those listed in clause 18 of the standard, e.g., new and delete),
#    then try linking against libsupc++.a, which is a subset of libstdc++.a
#    (Using gcc instead of g++ and explicitly linking in libsupc++.a via -lsupc++ for the final
#    link step will do it). This library contains only those support routines, one per object file.
#    But if you are using anything from the rest of the library, such as IOStreams or vectors,
#    then you'll still need pieces from libstdc++.a
LIBS= $(if $(filter $(CC_VER), 2), gcc, gcc-kernel supc++-kernel)

#	specify additional paths to directories following the standard
#	libXXX.so or libXXX.a naming scheme. You can specify full paths
#	or paths relative to the makefile. The paths included may not
#	be recursive, so include all of the paths where libraries can
#	be found. Directories where source files are found are
#	automatically included.
LIBPATHS=

#	additional paths to look for system headers
#	thes use the form: #include <header>
#	source file directories are NOT auto-included here
SYSTEM_INCLUDE_PATHS =

#	additional paths to look for local headers
#	thes use the form: #include "header"
#	source file directories are automatically included
LOCAL_INCLUDE_PATHS =

#	specify the level of optimization that you desire
#	NONE, SOME, FULL
OPTIMIZE= SOME

#	specify any preprocessor symbols to be defined. The symbols will not
#	have their values set automatically; you must supply the value (if any)
#	to use. For example, setting DEFINES to "DEBUG=1" will cause the
#	compiler option "-DDEBUG=1" to be used. Setting DEFINES to "DEBUG"
#	would pass "-DDEBUG" on the compiler's command line.
DEFINES=

#	specify special warning levels
#	if unspecified default warnings will be used
#	NONE = supress all warnings
#	ALL = enable all warnings
WARNINGS = ALL

#	specify whether image symbols will be created
#	so that stack crawls in the debugger are meaningful
#	if TRUE symbols will be created
SYMBOLS = TRUE

#	specify debug settings
#	if TRUE will allow application to be run from
#	a source-level debugger
DEBUGGER =

ifeq ($(RELEASE),1)
	OPTIMIZE= FULL
	SYMBOLS= FALSE
	DEBUGGER=
endif

#	specify additional compiler flags for all files
COMPILER_FLAGS = -Werror -Wall -Wconversion -Wno-multichar

#	specify additional linker flags
LINKER_FLAGS = 

INSTALL_DIR = /boot/system/non-packaged/add-ons/kernel/file_systems

## include the makefile-engine
DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine
