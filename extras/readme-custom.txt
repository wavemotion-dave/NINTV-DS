You can create your own custom overlays for Nintellivision. 

A custom overlay will have a .ovl extention and will have the same base filename as the original ROM.

In Configuration, you simply select the "Custom" option for Overlays... if the corresponding .ovl file is found, it will be used. Otherwise you'll just get the default 0-9,CLR,ENT overlay.

In this ZIP, you will find the all-important GRIT tool (freely released via the associated license file). I've also provided an example custom overlay file for Astrosmash.

To create this astroshash.ovl file, you start with a .png file that is exactly 256x256 pixels. I've supplied the astrosmash.png as your template.

Only the top 192 pixels are available to display - the font for the main screen, file selection, configuration, etc. is below the 192 pixel mark. 
Don't touch that stuff unless you REALLY know what's going on:)

With that as your template, you can use GIMP to load this up and edit it as you like. 
It's only 256 colors so you will be limited - but that's enough to do some awesome things.

Once you have your overlay changed, save it back out exactly as it is.. 8-bit PNG 
(just use Gimp "Overwrite" from the file menu so nothing gets changed).

Then you must run the command line tool. Open a command prompt and run GRIT on the new graphic file as follows:

grit astrosmash.png -o astrosmash.s -gt -mrt -mR8 -mLs -gzl -mzl

Now that will generate 2 files... astrosmash.s and astrosmash.h - only only need the .s file which will be renamed to .ovl

Open the new astrosmash.s (rename it to astrosmash.ovl at some point) and perform some edits on it.

Search globally for .word and replace with .tile
Edit the Pal: block of .hword lines and replace every .hword with .pal
Edit the Map: Search for .hword and replace every .hword with .map

Remove all the lines that aren't .tile, .pal or .map - in theory you could leave them in but they aren't processed.

While you're at it, the end of the big sections (.tile, .map and .pal) might not be fully padded to 8 entries... so if you see a line like this:

.tile 0x06F03FF9,0xF03FF3FF,0xF07FF101,0xF7D8C107,0xD277F07F,0x2182F804

Just pad it out with 0x0000s so the lines are of equal length in a block:

.tile 0x06F03FF9,0xF03FF3FF,0xF07FF101,0xF7D8C107,0xD277F07F,0x2182F804,0x00000000,0x00000000

Now at the top of the file add your .ovl lines which map the keypad controller to X1,X2 and Y1, Y2 coordinates. You can get your coordinates for your custom buttons in GIMP.
I usually go a little wider than the buttons so that they are easy to press with your thumb when playing. These .ovl lines must be in this exact order - KEY1 first, KEY2 
next, etc. Do not change the order (but you can change the text to the extreme right as "comments").

In the end you should end up with a file that is formatted and laid out just like the example astrosmash.ovl - this is the only file that needs to be placed on your 
SD or Flash cart. It should be the same base filename as the game you are creating the overlay for (just with a .ovl extension instead of .int or .rom). 
In Configuration, select the overlay as "Custom".

Not trivial... I realize that. But it's workable and you can create as many custom overlays as you want. Once you get the hang of it, the above steps won't take more than 10 minutes..
the harder part will be creating the actual graphic. Once you have the .ovl file, that's all you need. You can put that .ovl on any of your systems and it will work.
these manual steps above save me a ton of programming time - so don't complain too much :)

