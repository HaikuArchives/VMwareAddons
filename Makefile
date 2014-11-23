all: build

vmware_tray: vmware_tray/objects/vmware_tray
	cp $< "$@"

vmware_mouse: vmware_mouse/objects/vmware_mouse
	cp $< $@

#vmw_mouse: vmwmouse/objects/vmwmouse
#	cp $< $@

#vmware_fs: vmware_fs/objects/vmware_fs
#	cp $< $@

enhanced_backdoor: enhanced_backdoor/objects/enhanced_backdoor
	cp $< $@

build:
	make -C vmware_tray
	make -C vmware_mouse
#	make -C vmware_fs
#	make -C vmwmouse
	make -C enhanced_backdoor

release: mrproper
	make RELEASE=1 -C vmware_tray
	make RELEASE=1 -C vmware_mouse
#	make RELEASE=1 -C vmware_fs
#	make RELEASE=1 -C vmwmouse
	make RELEASE=1 -C enhanced_backdoor

clean:
	make -C vmware_tray clean
	make -C vmware_mouse
#	make -C vmware_fs
#	make -C vmwmouse
	make -C enhanced_backdoor

mrproper: clean
	rm -f vmware_tray
	rm -f vmware_mouse
#	rm -f vmware_fs
#	rm -f vmw_mouse
	rm -f enhanced_backdoor

.PHONY: build release clean mrproper
