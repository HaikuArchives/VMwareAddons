# VMW Add-ons v1.2.0

By Vincent Duvert <vincent.duvert@free.fr>
GitHub page: https://github.com/HaikuArchives/VMwareAddons

VMW Add-ons are a set of tools to enhance interaction between Haiku, running in a virtual machine, and the host operating system. It is like the official VMware tools on Windows or Linux.

**It currently provides**:
 - Clipboard sharing between Haiku and the host OS.
 - Mouse sharing: the mouse can seamlessly enter and quit the VM window.
 - Disk compacting: Starts the VMware "shrink" process, which reduce the size of "auto-expanding" virtual disks attached to the virtual machine. The free space on disks is cleaned up previously, in order to get better results.

**Known bugs and limitations**:
 - If you have a volume with more than 800GB of free space, not all free space will be cleaned up on this volume — only 800GB — before the shrink process.
 - When mouse sharing is activated, the cursor speed and acceleration are given by the host operating system (setting them in the "Mouse" control panel will have no effect).

If you encounter a bug or have a feature request, please use the tracker at:
https://github.com/HaikuArchives/VMwareAddons/issues

**Planned features**:
 - Shared folders support (or maybe file drag-n-drop, or both)
 - Virtual machine window resizing
