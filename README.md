This is a project written in python that will allow you to create wav audio files that can be loaded through cassette basic on the IBM 5150.

The only requirement should be python 3.

Usage:
asciiwrite.py [ascii file] [tape filename]

This takes in a text file with an ascii basic listing in it, and creates a file that can then be loaded using load.
Known issues:
* Currently files above 255 bytes do not seem to load correctly. I will work on fixing this

Usage:
binwrite.py [binary file] [tape filename] [segment] [offset]

This takes in a binary file and converts it to a memory dump file that can be loaded using bload.
Known issues:
* The segment and offset seem to do nothing, so I tend to just set them to 0 0.
* Loading into memory can be iffy, use def seg and point it somewhere in memory that's not used by basic, loading somewhere used by basic will have strange results.
* Refer to the appendixes in the IBM PC Basic guide for information on how to return from assembly programs, it involves a retf and some stack preservation that I personally have not figured out fully.

cgaprocessing.py converts a 4 color image (That matches the pallets at the top) to a binary file that needs loaded directly into CGA ram. I suggest against using it, but if you decide to try it out you'll need pillow installed.