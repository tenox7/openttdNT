OpenTTD README
Last updated:    $LastChangedDate: 2004-04-04 00:12:06 +0200 (Sun, 04 Apr 2004) $
Release version: 0.2.0
------------------------------------------------------------------------

Table of Contents:
------------------
1.0) About
2.0) Contacting
 * 2.1 Reporting Bugs
3.0) Supported Platforms
4.0) Running OpenTTD
5.0) OpenTTD features
6.0) Configuration File
7.0) Compiling
X.X) Credits

1.0) About:
---- ------ 
OpenTTD is a clone of Transport Tycoon Deluxe originally written by Chris Sawyer.
It attempts to mimic the original game as close as possible while extending it
with new features.

2.0) Contacting:
---- ----------
The easiest way to contact the OpenTTD team is by submitting bug reports or
commenting in our forums, or chat with us on irc (#openttd on irc.freenode.net).

On http://www.tt-forums.net/viewforum.php?f=29 you find a forum for OpenTTD.

2.1) Reporting Bugs:
---- ---------------
To report a bug, please create a SourceForge account and follow the bugs
link from our homepage. Please make sure the bug is reproducible, and
still occurs in the latest daily build/current CVS version. Also please
briefly look through the existing bug reports to see that the bug is not
already known.

The url to the sourceforge page is: http://sourceforge.net/projects/openttd/
Click on "Bugs" to see the bug tracker.

Please include the following information in your bug report:
        - OpenTTD version (PLEASE test the latest CVS/Daily build)
        - Bug details, including instructions on reproducing
        - Platform and Compiler (Win32, Linux, FreeBSD, ...)
        - Attach a save game or screenshot if possible
        - If this bug only occurred recently, please note the last
          version without the bug, and the first version including
          the bug. That way we can fix it quicker by looking at the
          changes made.

3.0) Supported Platforms:
---- --------------------
OpenTTD has been ported to a few platforms and operating system. It shouldn't
be very difficult to port it to a new platform. The currently working platforms
are:

	Windows - Win32 GDI (faster) or SDL
	Linux - SDL
	FreeBSD - SDL
	MacOS - SDL
	BeOS - SDL

4.0) Running OpenTTD:
---- ----------------
Before you run OpenTTD, you need to put the game's datafiles in
the same directory as the executable. You need the following files
from the original windows version of TTD.

IMPORTANT! You need the files from the WINDOWS version of TTD.

List of required files:
sample.cat
trg1r.grf
trgcr.grf
trghr.grf
trgir.grf
trgtr.grf

If you want music you need to copy the gm/ folder from windows TTD.


5.0) OpenTTD features:
---- -----------------

OpenTTD has a lot of features, and all can't be mentioned here. 
* Use Ctrl to place presignals.
* Ctrl-d toggles double mode on win32

6.0) Configuration File:
---- -------------------
The configuration file for openttd is of a simple windows-like .INI
format. It's mostly undocumented.

7.0) Compiling:
---- ----------
Windows:
  You need Microsoft Visual Studio 6 or .NET. Open the project file
  and it should build automatically. In case you don't build with SDL,
  you need to remove WITH_SDL from the project settings.

Unix:
  OpenTTD can be built either with "make" or with "jam". To build with
  "make", just type "make", or "gmake" on non-gnu systems. To build with "jam"
  first run "./configure" and then "jam". Note that you need SDL to compile
  OpenTTD.

MacOS:
  Use "make".

BeOS:
  Use "jam".


X.X) Credits:
---- --------
The OpenTTD team:
  Ludvig Strigeus (ludde)   - OpenTTD author, main coder.
  Serge Paquet (vurlix)     - Assistant project manager and coder.
  Owen Rudge (orudge)       - Contributor, forum host.
  Bjarni Corfitzen (Bjarni) - MacOS port
  Cian Duffy (MYOB)         - BeOS port / manual writing

Thanks to:
  Mike Ragsdale - OpenTTD installer
  Bug Reporters - Thanks for all bug reports.
  Josef Drexler - For his great work on TTDPatch.
  Marcin Grzegorczyk - For his TTDPatch work and documentation of TTD internals.
  Chris Sawyer - For an amazing game.
