$Id: BUILD,v 1.11 2005/10/23 01:35:59 titer Exp $

BUILD file for HandBrake <http://handbrake.m0k.org/>

Building HandBrake
==================

You can build HandBrake on BeOS, MacOS X and Linux. If you'd like to
port it to another OS, email me (titer@m0k.org).

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
+ nasm (Only for x86. On Mac OS X Intel, Nasm CVS is required)

Step 2: build
=============

Run `./configure && jam'. This will build every library HandBrake
requires, then HandBrake itself.
