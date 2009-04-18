VMW Add-ons v1.0
By Vincent Duvert <vincent.duvert@free.fr>
VMW backdoor code by Ken Kato (http://chitchat.at.infoseek.co.jp/vmware/)

VMW Add-ons are a set of tools to enhance interaction between Haiku, running in a virtual machine, and the host operating system. It is like the official VMware tools on Windows or Linux.
It currently allows:
 - Clipboard sharing between Haiku and the host OS
 - Mouse sharing: the mouse can seamlessly enter and quit the VM window.
 - Disk compacting: Starts the VMware "shrink" process, which reduce the size of "auto-expanding" virtual disks attached to the virtual machine. The free space on disks is cleaned up previously, in order to get better results.

Installation:
1) Drag and drop the "vmvmouse" file in the archive on the folder "Drop vmwmouse here" ;-)
2) Start the "VMWAddOns" application. It will not show any window, but put a small icon into the Deskbar, allowing you to control sharing options. It will automatically restart at each boot.
VMWAddOns can be placed anywhere, but storing in in the "apps" folder of the root folder is usually a good idea.

Known bugs and limitations:
• If you have a volume with more than 800GB of free space, not all free space will be cleaned up on this volume (only 800GB) before the shrink process.
• When mouse sharing is activated, the cursor speed and acceleration are given by the host operating system (setting them in the "Mouse" control panel will have no effect)/

If you encounter a bug or have a feature request, please use the tracker at :
http://dev.osdrawer.net/projects/vmwaddons/issues

Planned features:
• Shared folders support (or maybe file drag-n-drop, or both)
• Virtual machine window resizing

Licence:
The MIT Licence

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
