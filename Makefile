all: build VMWAddOns vmw_mouse

VMWAddOns: vmwaddons/obj.x86/vmwaddons
	cp $< "$@"

vmw_mouse: vmwmouse/obj.x86/vmwmouse
	cp $< $@

build:
	make -C vmwaddons
	make -C vmwmouse

release: mrproper
	make RELEASE=1 -C vmwaddons
	make RELEASE=1 -C vmwmouse

optpkg: release
	mkdir -p 'root/apps/VMW Add-ons'
	cp vmwaddons/obj.x86/vmwaddons 'root/apps/VMW Add-ons/VMWAddOns'
	cp readme.txt 'root/apps/VMW Add-ons'

	mkdir -p root/common/boot/post_install
	cp post_install_script.sh root/common/boot/post_install/run_vmwaddons.sh

	cp OptionalPackageDescription root/.OptionalPackageDescription

	mkdir -p root/home/config/add-ons/input_server/filters
	cp vmwmouse/obj.x86/vmwmouse root/home/config/add-ons/input_server/filters

	rm -f vmwaddons.zip

	(cd root && zip -r -9 ../vmwaddons apps common home .OptionalPackageDescription)

	rm -rf root

zip: release
	mkdir 'VMW Add-Ons'

	cp vmwaddons/obj.x86/vmwaddons 'VMW Add-Ons/VMWAddOns'
	cp readme.txt 'VMW Add-Ons'
	cp vmwmouse/obj.x86/vmwmouse 'VMW Add-Ons'

	ln -s /boot/home/config/add-ons/input_server/filters 'VMW Add-Ons/Drop vmwmouse here'

	rm -f vmwaddons.zip

	zip -r -y -9 vmwaddons 'VMW Add-Ons'

	rm -rf 'VMW Add-Ons'

clean:
	make -C vmwaddons clean
	make -C vmwmouse clean

mrproper: clean
	rm -f VMWAddOns
	rm -f vmw_mouse

.PHONY: build release optpkg zip clean mrproper
