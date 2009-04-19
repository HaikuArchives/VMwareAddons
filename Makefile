CC=gcc
CXX=g++
XRES=xres
CFLAGS=-I. -O2 -W -Wall -Wno-multichar -ansi -pedantic -Werror -DPERSISTENT_TRAY
LDFLAGS1=-lbe 
LDFLAGS2=-nostart -Xlinker -soname=vmwmouse -Xlinker --no-undefined /system/servers/input_server -lbe

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

vmwaddons.zip: all
	rm -rf root
	
	mkdir -p 'root/apps/VMW Add-ons'
	cp VMWAddOns 'root/apps/VMW Add-ons'
	cp readme.txt 'root/apps/VMW Add-ons'
	
	mkdir -p root/common/boot/post_install
	echo '#!/bin/sh'							> root/common/boot/post_install/run_vmwaddons.sh
	echo										>> root/common/boot/post_install/run_vmwaddons.sh
	echo '/boot/apps/VMW\ Add-ons/VMWAddOns &'	>> root/common/boot/post_install/run_vmwaddons.sh
	chmod a+x root/common/boot/post_install/run_vmwaddons.sh

	mkdir -p root/home/config/add-ons/input_server/filters
	cp vmwmouse root/home/config/add-ons/input_server/filters
	(cd root && zip -r -9 ../vmwaddons apps common home)
	
	rm -rf root

.PHONY: clean mrproper
