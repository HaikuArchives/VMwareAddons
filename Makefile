all: build

enhanced_backdoor: enhanced_backdoor/objects/enhanced_backdoor
	cp $< $@

vmware_fs: vmware_fs/objects/vmware_fs
	cp $< $@

vmware_mouse: vmware_mouse/objects/vmware_mouse
	cp $< $@

vmware_video_accelerant: vmware_video/accelerant/objects/vmware.accelerant
	cp $< "$@"

vmware_video_kernel: vmware_video/kernel/objects/vmware
	cp $< "$@"
	
vmware_tray: vmware_tray/objects/vmware_tray
	cp $< "$@"


build:
	make -C enhanced_backdoor
	make -C vmware_fs
	make -C vmware_mouse
	make -C vmware_tray
	make -C vmware_video/accelerant
	make -C vmware_video/kernel

release: mrproper
	make RELEASE=1 -C enhanced_backdoor
	make RELEASE=1 -C vmware_fs
	make RELEASE=1 -C vmware_mouse
	make RELEASE=1 -C vmware_tray
	make RELEASE=1 -C vmware_video/accelerant
	make RELEASE=1 -C vmware_video/kernel
clean:
	make -C enhanced_backdoor clean
	make -C vmware_fs clean
	make -C vmware_mouse clean
	make -C vmware_tray clean
	make -C vmware_video/accelerant clean
	make -C vmware_video/kernel clean

install: build
	# if we overwrite an in-use driver things get crashy, so delete those first
	rm -f /boot/system/non-packaged/add-ons/kernel/drivers/dev/graphics/vmware
	rm -f /boot/system/non-packaged/add-ons/accelerants/vmware.accelerant
	rm -f /boot/system/non-packaged/add-ons/input_server/filters/vmware_mouse
	rm -f /boot/system/non-packaged/add-ons/kernel/file_systems/vmwfs

	# now reinstall everything
	make -C vmware_fs install
	make -C vmware_mouse install
	make -C enhanced_backdoor install
	make -C vmware_tray install
	make -C vmware_video/accelerant install
	make -C vmware_video/kernel install

	mkdir -p "$(HOME)/config/settings/deskbar/menu/Desktop applets"
	ln -sf /boot/system/non-packaged/bin/vmware_tray "$(HOME)/config/settings/deskbar/menu/Desktop applets/VMware add-ons"

.PHONY: build release clean
