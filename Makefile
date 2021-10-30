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


.PHONY: build release clean
