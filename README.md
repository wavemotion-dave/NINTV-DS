# NINTV-DS
NINTV-DS is an Intellivision console emulator running on the DS/DSi.

Install :
----------
To make this work, place NINTV-DS.NDS on your flashcart or SD card which you can launch.
You must have these 3 files in the same directory as your ROM files:
grom.bin
exec.bin
knowncarts.cfg

Optional is ivoice.bin for Intellivoice games.

The knowncarts.cfg file is supplied with this emulator. The GROM and EXEC binaries are property
of Intellivision and you will have to find them yourself. Don't ask. If you own Intellivision 
Lives (various over the years), you likely have both files already somewhere in your house. 

Features :
----------
 Most things you should expect from an emulator. 
 Most games run at full speed on a DSi or above.
 Many games run very close to full speed on the older DS-LITE and
 DS-PHAT hardware but you can play with config settings to get the 
 most out of these games (even if you have to disable sound to gain the speed).
 
 JLP support for accelerated functions and RAM is supported. When loading a game,
 use the X button to load and force JLP support ON.

Missing / Known Issues :
-----------------------
No ECS support - ECS games will not load/play.
Stampeed - the cattle flicker in weird ways. Happens on Windows version of BLISS as well.
Nova Blast - game will freeze several seconds into playing. No known cause.
Q-Bert - game will lose a life after completing every screen/board. Happens on the Windows 
version of BLISS and also happens on the MAME/MESS emulator for the same game.
DK ARCADE - Minor graphical glitch when the ape climbs the girders. No play issues.

Check for updates on my web site : https://github.com/wavemotion-dave/NINTV-DS

License :
-----------------------
Copyright (c) 2021 Dave Bernazzani (wavemotion-dave)

Copying and distribution of this emulator, it's source code and associated 
readme files, with or without modification, are permitted in any medium without 
royalty provided the this copyright notice is used and wavemotion-dave (NINTV-DS)
and Kyle Davis (BLISS) are thanked profusely. 

The NINTV-DS emulator is offered as-is, without any warranty.


--------------------------------------------------------------------------------
History :
--------------------------------------------------------------------------------
V1.6 : 23-Sep-2021 by wavemotion-dave
  * Save State support added.
  * Fixed EVEN frameskip.
  
V1.5 : 21-Sep-2021 by wavemotion-dave
  * Better .ROM support (CVDEMO will now play)
  * More CP1610 optmization squeezing out a few more frames of performance.
  * Improved Frameskip and Speed options in Config area.
  * Cleanup for custom overlay support.
  * Other minor cleanups where time permitted.
  
V1.4 : 16-Sep-2021 by wavemotion-dave
  * Custom overlay support! See custom-overlay.zip in the distribution.
  * Hide bios files from game listing.
  * New d-pad configuration options.
  * Ability to map DS keys to meta-functions such as load, config, score, etc.
  * Other cleanups as time permitted... 

V1.3 : 12-Sep-2021 by wavemotion-dave
  * Basic JLP support for Accelerated functions and extra 16-bit RAM (hello Grail of the Gods!)
  * Major internal cleanup for better memory management. 
  * Squeezed out a couple more frames of speed improvement.

V1.2 : 10-Sep-2021 by wavemotion-dave
  * More speed - many games now playable full speed on the DS-LITE/PHAT!
  * On the DSi, even the Intellivoice games should be running full speed now.

V1.1 : 09-Sep-2021 by wavemotion-dave
  * Big boost in speed. Just about everything full speed on DSi. 
  * A few more overlays added.

V1.0 : 07-Sep-2021 by wavemotion-dave
  * First major release!

V0.9 : 06-Sep-2021 by wavemotion-dave
  * Major sound improvement (finally!)
  * High Score Support added
  
V0.8 : 05-Sep-2021 by wavemotion-dave
  * Two types of Dual-Action controllers supported (A=Standard, B=buttons on Controller #2)
  * More overlays added (Astrosmash, B-17 Bomber, Atlantis, Space Spartans)
  * Switched to a retro-green font for Game Select/Options
  * More new homebrews supported. 
  * Minor sound improvements.
  
V0.7 : 04-Sep-2021 by wavemotion-dave
  * Ability to save configuration on a per-game basis (START button in Config)
  * Minor sound improvements
  * Moved FPS and Turbo mode to Config
  * More homebrews added to "knowncarts.cfg"

V0.6 : 03-Sep-2021 by wavemotion-dave
  * More speedup and polish - especially for intellivoice games.
  * New config options for frameskip, sound quality
  * New overlays for MINOTAUR and ADVENTURE (in Config settings)
  * New dual-action controller support (in Config settings)
  * Can now map START and SELECT buttons

V0.5 : 02-Sep-2021 by wavemotion-dave
  * First pass release. 


Special Thanks :
---------
BLISS - Intellivision Emulator. Originally developed by Kyle Davis. Contributions by Jesse Litton, Mike Dunston, Joseph Zbiciak.
