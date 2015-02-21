# AokanaCGExtractor
Simple CG Extractor of the Japanese Visual Novel game 'Aono Kanatano Four Rhythm(蒼の彼方のフォーリズム)'

## Description
This program extract CG using Memory Hooking. So you need original copy of the game.

I tested this program on Windows 8.1 x64 Enterprise (Japanese) and Windows 8.1 x64 Enterprise (Korean, Change locale to 932).
Also tested on Mac OS X 10.10.1 with wine 1.6.2.

## How to use
Run the game and press 'Game Start' or 'Extra > CG Views'.
Execute AokanaCGExtractor and press start button.

Then you will see the window like this.
 ![Start Window](/Screenshots/Screenshot01.JPG?raw=true "Screenshot of the window")

When you play the game or click the CG, AokanaCGExtractor captures memory and shows list of the captured images.
 ![Captured1](/Screenshots/Screenshot02.JPG?raw=true "Usage at CG Views")

Select the image in the list to see preview. If you select multiple images, it merges images.
 ![Select1](/Screenshots/Screenshot04.JPG?raw=true "One item selected")
 ![Select2](/Screenshots/Screenshot05.JPG?raw=true "Two item selected")
 ![Select3](/Screenshots/Screenshot06.JPG?raw=true "Two item selected in in-game")

You can save selected images into PNG Files

## Dependencies
* libpng (http://www.libpng.org/pub/png/libpng.html)
* zlib (http://www.zlib.net)
