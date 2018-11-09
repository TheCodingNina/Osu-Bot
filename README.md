# Osu!Bot V2
A cursor dancing bot that can, fully autonomous, play the Rhythm game called Osu! (standard mode).

USE THIS WITH RESPONSABLITY!!
I DO NOT, I REPEAT, I DO NOT TAKE RESPONSABILITY IF YOU GET YOUR ACCOUNT BANNED/RESTRICTED ON OSU!

**PROTIP: It is safer to logout of your account before letting Osu!Bot play any beatmap.**

## Video Showcase Playlist
https://www.youtube.com/playlist?list=PLkSZ7HI4dIRgKJmiZ_L2XXK7PqpaEbNSl

## Downloadable Executables
https://github.com/DDDinggo22/Osu-Bot/releases/

## Usage Instructions
0. Unzip anywhere you like.
1. Execute (Osu!Bot V2.exe).
2. Select osu! 'songs' folder (default location like: C:\Users\\[USERNAME]\AppData\local\osu!\songs).  
   (This step only needed on first startup or after deletion of Osu!Bot 'Data' Folder.)
3. Play any map.
4. Customize Dance if you like.
5. Play next map.

## Remarks
### Issues
 * If the cursor seems to freeze in place after starting any beatmap and osu! recently had an update, it mostlikely is a outdated 'TimerPointer'.
   You can find a working one below here. If that one doesn't seem to work either, you can try to find it yourself with the instructions below that. Or just give me a ping/make an issue with the tag TimerPointer. And I'll update it as soon as possible.

 * Osu!Bot seems to have issues in the 'Release x64' build with finding the 'TimerPointer'. So for the time being, only the x86 will be packed/zipped in the downloadable releases.

### Compatibility
 * If Osu!Bot can't find the beatmap you can manually select it by pressing the select/change button to the right of the 'Auto Open song'.
 
### Features
 * Osu!Bot is able to play with any mod!
   BUT if you want it to play with 'Hardrock' You need to manually enable the hardrock by checking 'Hardrock (flip)'.
   
 * Osu!Bot has a slider that can increase how strong the Dance moves are!
 
 * Osu!Bot can Dance with multiple styles! Try them out, mix different styles for circles, sliders and spinners!
   Not All styles for sliders/spinners are implemented yet, but will fallback to their defaults.
   
 * Osu!Bot will remember your (Dance) Settings upon restart with its configFile!
   Only need to set once and play as much as you want!

## Timer Pointer
| Name       	| Hex Value	|
| ------------- | -------------	|
| ThreadOffset	| -32C		|
| Offset0	| 0      	|
| Offset1	| 1F0      	|
| Offset2	| 22C		|
| Offset3	| 2D8		|
| Offset4	| 514		|

### To find this pointer yourself
Search for the time with something like Cheat Engine, this can be done easily in edit mode on osu!, and make a pointer of that address. Then put the pointer as the TimerPointer for Osu!Bot. You can change this via the settings tab or manually in the configFile.
