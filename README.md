# NINTV-DS
Nintellivision - an Intellivision Emulator for the Nintendo DS/DSi

NINTV-DS is an Intellivision console emulator running on the DS/DSi.

Install :
----------
To make this work, place NINTV-DS.NDS on your flashcart or SD card which you can launch.
You must have these 3 files in the same directory as your .int ROM files:
grom.bin
exec.bin
knowncarts.cfg

The knowncarts.cfg file is supplied with this emulator. The GROM and EXEC binaries are property
of Intellivision and you will have to find them yourself. Don't ask. If you own Intellivision 
Lives (various over the years), you likely have both files already somewhere in your house. 

Features :
----------
 Most things you should expect from an emulator. 
 Most games run at full speed on a DSi or above.
 Most games will NOT run full speed on the older DS. 

Missing :
---------
Intellivoice games will run but only about 50FPS. 

Check updates on my web site : https://github.com/wavemotion-dave/NINTV-DS

--------------------------------------------------------------------------------
History :
--------------------------------------------------------------------------------
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

