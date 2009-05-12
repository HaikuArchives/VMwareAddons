CC=gcc
CXX=g++
XRES=xres
LDFLAGS1=-lbe 
LDFLAGS2=-nostart -Xlinker -soname=vmwmouse -Xlinker --no-undefined /system/servers/input_server -lbe

ifeq ($(RELEASE),1)
	CFLAGS=-I. -O2 -W -Wall -Wno-multichar -ansi -pedantic -Werror -DPERSISTENT_TRAY
else
	CFLAGS=-I. -g -W -Wall -Wno-multichar -ansi -pedantic -Werror
endif

all: menu_app vmwmouse

menu_app: VMWAddOns VMWAddOns.rsrc
	$(XRES) -o $^
	mimeset VMWAddOns

vmwmouse: VMWMouseFilter.o VMWAddOnsSettings.o VMWBackdoor.o
	$(CXX) -o "$@" $^ $(LDFLAGS2)
	mimeset "$@"

VMWAddOns: VMWAddOnsApp.o VMWAddOnsTray.o VMWAddOnsSettings.o VMWBackdoor.o VMWAddOnsCleanup.o VMWAddOnsSelectWindow.o VMWAddOnsStatusWindow.o
	$(CXX) -o "$@" $^ $(LDFLAGS1)

%.o: src/%.cpp
	$(CXX) -o $@ -c $< $(CFLAGS)

%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.rsrc: src/%.rdef
	cat $< | gcc -E -D__HAIKU__ - | egrep -v '^#' | rc --auto-names -o $@ -

clean:
	rm -rf *.o *.rsrc

mrproper: clean
	rm -f VMWAddOns vmwmouse vmwaddons.zip

release:
	@echo Building...
	@make mrproper
	@make RELEASE=1

optpkg: release
	@echo Creating optional package...
	
	@mkdir -p 'root/apps/VMW Add-ons'
	@cp VMWAddOns 'root/apps/VMW Add-ons'
	@cp readme.txt 'root/apps/VMW Add-ons'
	
	@mkdir -p root/common/boot/post_install
	@touch 										  root/common/boot/post_install/run_vmwaddons.sh
	@echo '#!/bin/sh'							>>root/common/boot/post_install/run_vmwaddons.sh
	@echo										>>root/common/boot/post_install/run_vmwaddons.sh
	@echo '/boot/apps/VMW\ Add-ons/VMWAddOns &'	>>root/common/boot/post_install/run_vmwaddons.sh
	@chmod a+x root/common/boot/post_install/run_vmwaddons.sh
	
	@touch 																	  root/.OptionalPackageDescription
	@echo "Package:		VMW Add-Ons"										>>root/.OptionalPackageDescription
	@echo "Version:		1.0.0"												>>root/.OptionalPackageDescription
	@echo "Copyright:	2009 Vincent Duvert"								>>root/.OptionalPackageDescription
	@echo "Description:	Tools for using Haiku in a VMware virtual machine." >>root/.OptionalPackageDescription
	@echo "License:		MIT"												>>root/.OptionalPackageDescription
	@echo "URL:			http://dev.osdrawer.net/projects/show/vmwaddons"	>>root/.OptionalPackageDescription

	@mkdir -p root/home/config/add-ons/input_server/filters
	@cp vmwmouse root/home/config/add-ons/input_server/filters
	@rm vmwaddons.zip
	@(cd root && zip -r -9 ../vmwaddons apps common home .OptionalPackageDescription)
	
	@rm -rf root
	
zip: release
	@echo Creating zip...
	
	@mkdir 'VMW Add-Ons'
	@cp VMWAddOns 'VMW Add-Ons'
	@cp readme.txt 'VMW Add-Ons'
	@cp vmwmouse 'VMW Add-Ons'

	ln -s /boot/home/config/add-ons/input_server/filters 'VMW Add-Ons/Drop vmwmouse here'
	
	@rm -f vmwaddons.zip
	
	@zip -r -y -9 vmwaddons 'VMW Add-Ons'
	
	@rm -rf 'VMW Add-Ons'
	

.PHONY: clean mrproper release optpkg zip
