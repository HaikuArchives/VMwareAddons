NAME = vmware_time
TYPE = APP
SRCS = VMWareTime.cpp ../shared_sources/VMWBackdoor.cpp ../shared_sources/VMWCoreBackdoor.cpp ../shared_sources/VMWAddOnsSettings.cpp
APP_MIME_SIG = application/x-vnd.Haiku-VMWareTime
INSTALL_DIR = /boot/system/non-packaged/bin
LIBS = be
LIBPATHS = 
SYSTEM_INCLUDE_PATHS = 
LOCAL_INCLUDE_PATHS = ../shared_sources 

## include the makefile-engine
DEVEL_DIRECTORY := \
	$(shell findpaths -r "makefile_engine" B_FIND_PATH_DEVELOP_DIRECTORY)
include $(DEVEL_DIRECTORY)/etc/makefile-engine
