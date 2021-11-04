# VMware Add-ons

By Vincent Duvert <vincent.duvert@free.fr>  
GitHub page: https://github.com/HaikuArchives/VMwareAddons

VMW Add-ons is a set of tools to enhance interaction between Haiku running in a virtual machine, and the host operating system. 
It's similar to the official VMware Tools on Windows or Linux.

### Features  
 - Clipboard sharing between Haiku and the host OS. Use the VMware add-ons desktop applet to enable or disable the sharing.
 - Mouse sharing: the mouse can seamlessly enter and leave the VM window.
 - Shared folders: easily share files between Haiku and the host OS. The shared folders can be mounted in Haiku using `mount -t vmwfs ~/shared`.
 - Disk compacting: starts the VMware "shrink" process, which reduces the size of "auto-expanding" virtual disks attached to the virtual machine. The free space on disks is cleaned up previously, in order to get better results.
 - Graphics driver for VMware: you can choose your preferred resolution using the Screen preflet in Haiku.
 - Graphics driver for VirtualBox: change the graphics controller in the VM settings to `VMSVGA`, and set the video memory size to 64 MB to be able to use higher resolutions.

### Known bugs and limitations  
 - If you have a volume with more than 800GB of free space, not all free space will be cleaned up on the volume before the shrink process — just 800GB.
 - When mouse sharing is activated, the cursor speed and acceleration are defined by the host operating system (setting them in the "Mouse" control panel will have no effect).

If you encounter a bug or have a feature request, please use the tracker at:
https://github.com/HaikuArchives/VMwareAddons/issues

### Planned features
 - Virtual machine window resizing
