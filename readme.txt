The below text is written for Simutrans Standard. This file is part of the
Simutrans Extended source, which is a fork of Simutrans Standard. Most of
the information below still applies, but note in particular the following
differences:

The source for Simutrans Extended is stored using Git version control, and
can be found at:
https://github.com/jamespetts/simutrans-extended.git

References to 'Simutrans/sim' and 'sim' should be assumed to refer to the
root directory of the Git repository.

- Aug 2018

###########################################################################

How to compile
--------------

Congratulations, you checked out the simutrans source. To compile it,
you have two options, either using Microsoft Visual C++ Express (which
is free in Version 7.0 or up) or GCC.

To compile you will need the following libraries:
libz (https://www.zlib.net/)
libpng (http://www.libpng.org/pub/png/)
libbz2.lib (compile from source from https://sourceware.org/bzip2/downloads.html)

The following are also recommendend, but optional
libfreetype (https://www.freetype.org/)
libminiupnpc (http://miniupnp.free.fr/ or https://miniupnp.tuxfamily.org/)

For the recommended SDL2-support you need
libSDL2 [better than libSDL] (https://libsdl.org/)
libSDL_mixer (https://www.libsdl.org/projects/SDL_mixer/)

The link for allegro lib is (but the allegro backend has not been
tested for a long time):
https://liballeg.org/old.html

To make life easier, you can follow the instructions to compile OpenTTD:
http://wiki.openttd.org/Category:Compiling_OpenTTD
A system set up for OpenTTD will also compile simutrans (except for
bzlib2, see below sections).

If you are on a MS Windows machine, download either MS VC Express or
MingW. The latter is easier to use as part of the DEV-C++ IDE, which
makes the installation of additional libraries like libz and libsdl and so
on very easy. However, to compile the command line is easier.

For all other systems, it is recommended you get latest GCC 3.46 or higher
and matching zlib, libbzip2, and libpng and a game library. For linux
systems you may have to use tools like apt-get or yast2.

To built on Haiku you must use GCC4 (type "setarch x86" in the current
nightlies). To incorporate bz2lib, download make bz2lib and add them
manually (via FLAGS = -I/dwonloadeddir -L/downloadeddir).

A subversion will be also a good idea. You can find some of them on:
https://subversion.apache.org/
or you some other client.

Check out the latest source from the SVN or check out a certain revision.
I recommend always to use the latest source, since it does not make any
sense to work with buggy code.

The address is:
svn://tron.homeunix.org/simutrans
username is "anon"!

A commandline would look like this:
svn checkout svn://tron.homeunix.org/simutrans --username=anon


IMPORTANT:
----------

If you want to contribute, read the coding guidelines in
sim/documentation/coding_styles.txt


The following instructions are for GCC systems:
-----------------------------------------------

Go to Simutrans/sim.

Then copy the file sim/config.template to sim/config.default and edit the
file. You need to specify:
- frontend (gdi, allegro, sdl)
- color depth (usually 16)
- system (you should know it)

I recommend to uncomment #DEBUG=1 and #OPTIMISE = 1 (i.e. removing the #).

For allegro or libsdl you may need to define the path of the config file
(or at least on win98 and empty path).

Finally type make. If you want a smaller program and do not care about error
messages, you can comment out #DEBUG=1 and run strip sim resp. strip sim.exe
after compile and linking.

For users on window systems:
To debug, I recommend to run drmingw -i once in a shell. You will get a
caller history in case of an error. gdb does not really work well and is a
pain to use with the text interface.


The following instructions are for MS Visual C Express:
-------------------------------------------------------

Download Visual Express C++ (tested for 2008)
http://www.microsoft.com/express/Downloads/

In the OpenTTD wiki you will also find some useful instruction on
setting up MSVC:
http://wiki.openttd.org/Microsoft_Visual_C%2B%2B_2008_Express_Editions

You will need some libraries like zlib etc. Any version which is
compatible with VC will do. Take a look at the instructions for
compiling OpenTTD, which contains a libpng and a zlib suited for
simutrans too:
http://binaries.openttd.org/extra/openttd-useful/

The bzip2 source tarball comes with an archive where you can easily built
your own libbz2.lib file. Or use the one posted in the forum:
http://forum.simutrans.com/index.php?topic=652.msg37080#msg37080

For debugging, you have to set the correct working directory, i.e. the
directory where the pak/ folders are located and use the -use_workdir
command line option.

Berlin, Mai 2011
