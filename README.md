# Osu!Bot V2
A cursor dancing bot that can, fully autonomous, play the Rhythm game called Osu! (standard mode).

USE THIS WITH RESPONSABLITY!!
I DO NOT, I REPEAT, I DO NOT TAKE RESPONSABILITY IF YOU GET YOUR ACCOUNT BANNED/RESTRICTED ON OSU!

## Video Showcase
https://www.youtube.com/watch?v=kP2mhnL4_CI

## Usage Instructions
0. Unzip anywhere you like.
1. Execute (Osu!Bot V2.exe).
2. Select osu! 'songs' folder (default location like: C:\Users\\[USERNAME]\AppData\local\osu!\songs).  
   (Only needed on first start or after deletion of Osu!Bot Data Folder.)
3. Check 'Auto Open Song'.
4. Play any map.
5. Play next map.

### Compatibility
 * If the cursor seems to freeze in place after starting any beatmap, it mostlikely is a outdated 'TimerPointer'.
   You can find a working one below here. If that one doesn't seem to work either, you can try to find it yourself with the instructions below that. Or just give me a ping/make an issue with the tag TimerPointer. And I'll update it as soon as possible.

 * If Osu!Bot can't find the beatmap you can manually select it by pressing the select/change button  
   to the right of the 'Auto Open song'.

 * Osu!Bot is able to play with any mod!  
   BUT if you want it to play with 'Hardrock' You need to manually enable the hardrock by checking 'Hardrock (flip)'.

## Timer Pointer

| Name       	| Hex Value	|
| ------------- | -------------	|
| ThreadOffset	| -32C		|
| Offset0	| 48C      	|
| Offset1	| 27C      	|
| Offset2	| F8		|
| Offset3	| 7B0		|
| Offset4	| 70		|

### To find this pointer yourself
Search for the time with something like Cheat Engine, this can be done easily in edit mode on osu!, and make a pointer of that address. Then put the pointer as the TimerPointer for Osu!Bot. You can change this via the settings tab or manually in the configFile.
