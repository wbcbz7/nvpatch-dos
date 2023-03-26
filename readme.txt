   ...NVidia DOS/Win3.x driver patch utility - wbcbz7 27.o3.2o23...

 This utility allows to use certain DOS VESA and Win3.x drivers on newer
 NVidia GPUs, such as Riva TNT family and GeForce 256 up to FX series
 (theoretically up to 7xxx)

 Supported drivers:

 * SciTech Display Doctor (aka UniVBE) 6.53 and 6.7
 * Riva TNT/TNT2 Windows 3.x drivers, v 1.55 (untested)

 **DISCLAMER:** THIS UTILITY IS PROVIDED "AS-IS", I bear no responsibility for
 any harmful or undesired actions or consequences caused by using it on your
 hardware, although the probability of such outcomes are extremely low
 (unless you are using some very very ancient VGA monitor unable to tolerate
 out of spec timings). In other words: use at own risk :)

 ### SciTech Display Doctor
  Although built-in NVidia VBE 3.0 support is near-perfect and compatible with
 large variety of DOS software, it has a few shortcomings (like 15bpp modes
 being missing, or lack of several low-resolution modes like 512x384). In the
 past, utilities like SciTech Display Doctor (also known as UniVBE) were the
 popular choice for fixing VBE issues and providing additional features like
 linear frame buffer, low-resolution modes, etc. Unfortunately, support for
 NVIDIA graphics chips is very sloppy, and anything newer than Riva TNT is
 not actually supported. NVPatch hacks hardware detection routines and makes
 those utilities run on newer hardware.

 Known limitations:

 * only primary VGA output is supported! **DO NOT** try to use patched UniVBE
   on DVI/HDMI displays, as it will blank the display (and possibly crash the
   system) while attempting to set any VESA mode. Besides, DVI support under
   plain DOS on NV chips is far from perfect: image is blurry, frame rate is
   locked to 60fps and some games don't work properly, while running fine on
   VGA.
 * modes above 1024x768 are not tested and may not work properly
 * 8, 15, 16 and 32 bits per pixel modes are supported, while 24bpp is not
   possible on NV hardware and hence not provided by SDD

 #### SDD 6.53

 uses RIVA 128 support code, needs quite an intensive patching ;) 

 Known limitations:

 * VRAM size is limited to 8 MB, anything above breaks scrolling/page flipping
 * max. provided resolution is 1600x1200

 Patch usage:

 1. Install SDD as usual
 2. Copy NVPATCH.EXE to SDD directory, and run it. Check if all files under 
    SDD 6.53 description is marked as "ok", ignore others.
 3. If your graphics card has more than 8 MB of VRAM run `UVCONFIG.EXE -m8912`,
    if everything is OK, run `CONFIG.EXE -m8192`. Else, run `UVCONFIG.EXE` and
    `CONFIG.EXE` without any command line parameters. If both runs succeed,
    then SDD is patched and configured properly.
 4. Run `UNIVBE.EXE`
 5. Run any VESA application (`VBETEST` from SDD package is a perfect match :)

 #### SDD 6.7

 uses RIVA TNT2 support code, may have better GPU compatibility than 6.53

 Known limitations:

 - VRAM size is usually limited to 16 MB
 - unlike 6.53, doesn't provide certain low-resolution VESA modes (namely,
   320x200, 320x400 and 320x480). Since 320x200 VESA is used by some DOS demos
   and games, you have to use native NV VESA BIOS or SDD 6.53 for those apps.

 Patch usage:

 1. Install SDD as usual
 2. Copy `NVPATCH.EXE` to SDD directory, and run it. Check if SDD 6.7
    `UVCONFIG.EXE` is marked as "ok", ignore others.
 3. Run `UVCONFIG.EXE`, if it succeeds, then SDD is patched and configured
    properly.
 4. Run `UNIVBE.EXE`
 5. Run your favorite VESA application :)

 Personally, I would recommend SDD 6.53, and only if it does not work properly,
 then try 6.7

 Tested and working (more or less) fine on:

 * RIVA TNT 16 MB AGP (6.7 works out of the box, 6.53 needs patching)
 * RIVA TNT2 M64 16 MB and 32 MB, both AGP (strangely, while 6.7 declares TNT2
   support, it does work only with TNT2 Ultra, judging by PCI Device ID)
 * Vanta 16 MB and Vanta LT 8 MB, both AGP
 * GeForce 2 MX200 32MB and MX400 64 MB, both AGP
 * GeForce 4 MX440 64 MB AGP
 * GeForce FX5200 and FX5500, both 128MB AGP (the latter has the infamous
   broken VBE3.0 custom refresh rate support in it's video BIOS, and it works
   again with SDD)

Does not work on:

* GeForce 7600GS 256MB PCIe (blank screen upon any VESA mode set, goes back to
  VGA modes fine)


 ### Windows 3.1 Display Driver

 NOTE: unfinished, still broken, moreover, it's buggy even on a TNT itself
 (fonts corruption, DCI not working, etc.). basically you have to patch
 NV4VDD.386 file from driver package, then install the drivers as usual.


 ### compile?

 Install Watcom C/C++ 1.9 or higher, run `make`, grab `NVPATCH.EXE`, done :)

 ### contact?

 email: wbcbz7.at(.)gmail.com // discord: wbcbz7#3519 // telegram: t.me/wbcbz7

 i also have a VOGONS account, but it was banned due to email check failure,
 and i still can't restore it -_-
