$Id: BUILD,v 1.2 2004/02/13 13:45:50 titer Exp $

BUILD file for HandBrake <http://handbrake.m0k.org/>

Building HandBrake
==================

Step 1: get jam
===============

Jam <http://www.perforce.com/jam/jam.html> is a make replacement.
Get it for your operating system:
 + OS X: get <http://handbrake.m0k.org/files/jam-2.5rc3-OSX.zip> and
   copy jam to /usr/local/bin/. Do not use the modified jam shipped with
   the developer tools (/Developer/Private/jam).
 + Debian: `apt-get install jam'
Or get the sources from the link above and compile it yourself.

Step 2 (optional): set custom compiler flags
============================================

If you want to optimize the build for a specific CPU, edit the Jamrules
file and modify the "OPTIM = ..." line by adding the needed flags.

Step 3: build
=============

Run `jam'. This will build every library HandBrake requires, then
HandBrake itself.

