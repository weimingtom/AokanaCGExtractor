# AokanaCGExtractor
Simple CG Extractor of the Japanese Visual Novel game 'Aono Kanatano Four Rhythm(蒼の彼方のフォーリズム)'

## Description
This program extract CG using Memory Hooking. So you need original copy of the game.

## How to use
Run the game and press 'Game Start' or 'Extra > CG Views'.
Execute AokanaCGExtractor and press start button.

Then you will see the window like this.
 - insert image

When you play the game or click the CG, AokanaCGExtractor captures memory and shows list of the captured images.
 - insert image

Select the image in the list to see preview. If you select multiple images, it merges images.
 - insert image

You can save selected images into PNG Files

## Important
You may have to change DebugThread in Main.cpp to work properly.
Do not press start on 'Title' and 'Message Log'. It may corrupt.

## Dependencies
libpng (http://www.libpng.org/pub/png/libpng.html)
zlib (http://www.zlib.net)
