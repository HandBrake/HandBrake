$Id: BUILD,v 1.11 2005/10/23 01:35:59 titer Exp $

BUILD file for HandBrake <http://handbrake.m0k.org/>

Building HandBrake
==================

You can build HandBrake on BeOS, MacOS X, Linux, and on Windows using Cygwin.
If you'd like to port it to another OS, email me (titer@m0k.org).

Step 1: get needed tools
========================

+ gcc and g++
    There are usually included in your OS' dev tools. On BeOS/Zeta, the
    default gcc isn't enough, gcc 2.95.3 is required.
+ jam
    I use 2.5rc3, earlier versions might cause issues.
    On BeOS, you can download it at <http://www.haiku-os.org/develop.php>.
    On OS X, you cannot use the modified jam shipped with the developer
    tools, use this one instead:
    <http://download.m0k.org/handbrake/jam-2.5rc3-OSX.zip>.
    On Cygwin, get the jam source from
    http://public.perforce.com/public/jam/index.html,
    compile it with gcc in Cygwin, and put the jam executable somewhere on
    your path.
+ nasm (Only for x86. On Mac OS X Intel, Xcode 2.4.1 include it)

Cygwin setup:
    There are a couple extra things required to make the code build for Cygwin.
    Make sure you have /bin and /usr/bin on your path *first*, before
    the Windows paths.  Otherwise the Cygwin find.exe won't be used, and
    lame won't build.
    Also, I have not tried to build this with MinGW, I've only built it with
    the Cygwin environment, so you should build with Cygwin gcc and dev tools.
    Unfortunately this means you need to have cygwin1.dll around to run the
    HBTest.exe program, but that's the way it is for now, until I get time
    to try building it with MinGW.  If you want to try making it work with
    MinGW, then go for it!


Step 2: build
=============

Run `./configure && jam'. This will build every library HandBrake
requires, then HandBrake itself.


Experimental UB build on MacOsX
===============================

Run `make'. This will build libmediafork, MediaFork and MediaForkCLI as Universal Binary.
This build method use precompiled contrib libraries. Script to build those binaries are provided too.
All the build is handled by Xcode 2.4.1, should work on powerPC and Intel Macs.



