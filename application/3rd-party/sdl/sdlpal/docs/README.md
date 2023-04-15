SDLPAL
======
[![Travis CI](https://travis-ci.com/sdlpal/sdlpal.svg?branch=master)](https://travis-ci.com/sdlpal/sdlpal)
[![AppVeyor](https://ci.appveyor.com/api/projects/status/github/sdlpal/sdlpal?branch=master&svg=true)](https://ci.appveyor.com/project/palxex/sdlpal-itfml)
[![Download](https://api-prd.cloudsmith.io/badges/version/sdlpal/sdlpal/raw/wii/latest/x/?render=true) ](https://cloudsmith.io/~sdlpal/repos/sdlpal/groups)

***SDLPAL*** is an SDL-based cross-platform reimplementation of the classic Chinese RPG game *Xiān jiàn Qí Xiá Zhuàn (Simplified Chinese: 仙剑奇侠传, Traditional Chinese: 仙劍奇俠傳)* (also known as *Chinese Paladin* or *Legend of Sword and Fairy*, or *PAL* for short).

[![Google Play](https://sdlpal.github.io/images/googleplay.png)](https://play.google.com/store/apps/details?id=com.sdlpal.sdlpal)

If you would like to get the nightly build from Google Play Store, [please enroll](https://play.google.com/apps/testing/com.sdlpal.sdlpal) (WARNING: might be unstable).

[Try the online demo for now!](https://sdlpal.github.io/demo/sdlpal.html)
=======
Check the link above for a Web-based demo of SDLPAL. It should work on most of modern browsers (e.g., Google Chrome, Mozilla Firefox, Safari, ...), but problems might exist on some browsers. Before you can enjoy the game, please prepare a zipped file containing the game resource data.

gitee镜像
=======
中国大陆地区用户如从GitHub下载代码存在困难，请使用[gitee镜像](https://gitee.com/sdlpal/sdlpal) (每小时自动更新)。

LICENSE
=======

SDLPAL was originally created by [Wei Mingzhi](https://github.com/weimzh/) from 2009. Now it is owned by the SDLPAL development team. Please see [AUTHORS](AUTHORS) for full author list.
```
Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
Copyright (c) 2011-2022, SDLPAL development team.
All rights reserved.
```
SDLPAL is distributed under the terms of **GNU General Public License, version 3** as published by the [Free Software Foundation](http://www.fsf.org/). See [LICENSE](LICENSE) for details.

Many of the ideas of this program are based on documents from [PAL Research Project](https://github.com/palxex/palresearch), and portions of the code are based on the work done by Baldur and [louyihua](https://github.com/louyihua).

This program made extensive use of the following libraries:
* [SDL](http://www.libsdl.org/)
* [SDL_mixer](http://www.libsdl.org/projects/SDL_mixer/)
* [libmad](http://www.underbit.com/products/mad/)
* [libogg & libvorbis](http://www.vorbis.com/)
* [libopus & opusfile](https://www.opus-codec.org/)
* [FLTK](http://www.fltk.org)
* [TinySoundFont](https://github.com/schellingb/TinySoundFont)
* OPL player from [Adplug](http://adplug.sourceforge.net/)
* OPL emulation cores from [DOSBOX project](http://www.dosbox.com), [MAME project](http://mamedev.org/) and [Chocolate Doom project](https://github.com/chocolate-doom/chocolate-doom)
* Audio resampler from [foo_input_adplug](https://www.foobar2000.org/components/view/foo_input_adplug)
* AVI player from [ffmpeg](https://ffmpeg.org/)
* Image decoder from [stb](https://github.com/nothings/stb)

This program does **NOT** include any code or data files of the original game, which are proprietary and copyrighted by [SoftStar](http://www.softstar.com.tw/) Inc.

## FAQ

See [wiki](../../wiki)

Building the game
=================

Currently, SDLPAL supports the following platforms:
* Desktop platforms: *Windows* (both *Windows Desktop* and *Universal Windows Platform*), *GNU/Linux* and *macOS*
* Mobile platforms: *Android*, *iOS* and *Windows*
* Home consoles: *3DS*, *WII*, *PSP* and others (some of them are not actively maintained now)

Before start building, you may find some inspiration from reading [Our travis scripts](.travis.yml).

To build SDLPAL, there are three major steps you need to do:
1. Clone the source code from GitHub into your local folder using `git` or through corresponding GUI:
```shell
$ cd <parent-path-of-sdlpal>
$ git clone https://github.com/sdlpal/sdlpal.git
```
2. Update all the submodules used by SDLPAL using `git submodule` or through corresponding GUI:
```shell
$ cd <parent-path-of-sdlpal>/sdlpal
$ git submodule update --init --recursive
```
3. Follow the platform-specific steps listed below.

Windows
-------

### Visual Studio

To build SDLPAL as a Windows **desktop** app, you can use ***Microsoft Visual Studio 2017*** to open the solution file *`sdlpal.sln`* under the *`win32`* directory.

To build SDLPAL as a **Universal Windows Platform** app, you can use ***Microsoft Visual Studio 2017*** to open the solution file *`SDLPal.UWP.sln`* under the *`winrt`* directory.

### MinGW

To build SDLPAL as a Windows **desktop** app, you can also use ***MinGW***. Steps for building under MinGW varies depends on the compiling environment you have:

* If you need to compile SDLPAL under **Windows** shell environment, please go to the root of the source code tree and type:
```cmd
C:\sdlpal> cd win32
C:\sdlpal> make -f Makefile.mingw
```

* If you need to compile SDLPAL under **msys** shell environment, please go to the root of the source code tree and type:
```bash
$ cd win32
$ make
```

* If you need to cross-compile SDLPAL under **GNU/Linux** shell environment, please go to the root of the source code tree and type:
```bash
$ cd win32
$ # This builds a 32-bit executable.
$ make HOST=i686-w64-mingw32-
$ # This builds a 64-bit executable.
$ make HOST=x86_64-w64-mingw32-
```


GNU/Linux or Unix
-----------------

To build the game, please go to the root of the source code tree and type:
```shell
$ cd unix
$ make
```
You also need to have SDL 2.0 development files installed in the system. The compiled executable should be generated with the filename *`sdlpal`* at the current directory. By default, SDLPAL uses the FLTK library to provide setting GUI at launch. If you do not want to use the library, please define he macro `PAL_NO_LAUNCH_UI` in the `Makefile`. SDLPAL should also be able to compile and run under other Unix-like systems, however it's not tested.


macOS
-----

To compile, open *`Pal.xcodeproj`* with `Xcode`, and click Build. You need to have SDL framework installed at *`/Library/Frameworks`*.

iOS
---

To compile, please first install dependencies via CocoaPods following the above instruments, then open the project *`ios/SDLPal/SDLPal.xcworkplace`* with `Xcode`, and click Build.
```shell
$ cd iOS/SDLPAL
$ sudo gem install cocoapods # ONLY need do once on one machine
$ pod install # ONLY need do once in one repository
```

Android
-------

To build the game, open the `android` directory through ***Android Studio***, and click `Make Project`.

* NOTE: `android/app/src/main/java/org/libsdl/app` is a link to `3rd/SDL/android-project/app/src/main/java/org/libsdl/app`. Deal with it properly if your git system does not create link automatically.

* NOTE: For Windows users, please put the repo at the root of a disk partition. A long path may cause compilation to fail.

Nintendo 3DS
------------

To build the game, please go to the root of the source code tree and type:
```shell
cd 3ds
make
make cia
```
You need to have *DevkitPro ARM* and *SDL 1.2* for 3DS portlib installed. Creating a CIA package is not required to play the game, but in order to to that, a seperate *makerom* tool is required. The compiled executable should be generated with the filename *`sdlpal`* at the current directory.
 
Nintendo Wii
------------

To build the game, please go to the root of the source code tree and type:
```shell
cd wii
make
```
You need to have *DevkitPro PPC* and *SDL 1.2* for Wii portlib installed.

Other platforms
---------------

To be written.


Choosing the battle system
==========================

By default, SDLPAL builds a *"classic"* turn-based battle system which is designed to be 100% the same as the original game.

SDLPAL also provides a revised battle system (***deprecated*** and will be removed in future) which is more exciting yet somewhat harder than the original game. If you prefer this battle system, please define the macro `ENABLE_REVISIED_BATTLE` in *`Makefile`* or in *`common.h`* and recompile the project.


Running the game
================

The data files required for running the game are not included with the source package due to copyright issues. You need to obtain them from a licensed copy of the game before you can run the game.

To run the game, copy all the files in the original game CD to a directory, then copy the built SDLPAL executable to the same directory, and run the executable.

Note that the filenames of data files should be all in lower-case under systems that use case-sensitive filesystems such as GNU/Linux or other Unix-like operating systems.

If you prefer using MIDI as background music source, please note that the MIDI playing feature is not yet complete under every supported platform. Currently, **offical** support is provided under *Windows Desktop*, *Universal Windows Platform*, *Android*, *iOS* and *macOS*. There is also a preliminary support for *GNU/Linux* that relys upon package *timidity*. Other platforms do not support playing MIDI for now.


Configuring the game
====================

PAL has several variants using different and incompatible resource files, and SDLPAL supports several configuration options for supporting such variants. The default values are used to support the resources from original DOS version. If you want to change these configurations, you have two options: through the configuration GUI or by manipulating the configuration file *`sdlpal.cfg`* manually.

GUI
---
The configuration GUI provides options for you to change the most common configuration options. If you launch SDLPAL for the first time, it will bring you to the configuration GUI by default. Once you have saved configurations from the GUI, the GUI will not to show again on subsequent launches. However, you have the opportunity to bring the GUI back on fatal game program errors or through the in-game system menu.

Currently, the configuration GUI is available under the following platforms:
* Desktop platforms: *Windows* (both *Windows Desktop* and *Universal Windows Platform*) and *GNU/Linux*
* Mobile platforms: *Android*, *Windows* and *iOS*

Configuration GUIs for *macOS* is still unavailable for now and we welcome contributions to implement it.

Manually
--------
To set the configuration options manually, create a file named as *`sdlpal.cfg`* (make sure to use lower-case file name in case-sensitive filesystems) in the game directory created by the above step. Please refer to the [example file](sdlpal.cfg.example) for format specfication.


Reporting issues
================

If you find any issues of SDLPAL, please feel free to report them to the development team through GitHub's issue tracking system using either English or Chinese.


Contributing to the game
========================

Any original code & documentation contributions are welcomed as long as the contributed code & documentation is licensed under GPL. You can use GitHub's pull request system to submit your changes to the main repository here. But remember, as a step to keep the quality of code, you should write corresponding unit tests before your changes can be merged. The guidance of writting unit tests can be found [here](/tests).
