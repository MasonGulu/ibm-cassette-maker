This is a project written in python that will allow you to create wav audio files that can be loaded through cassette basic on the IBM 5150.

It's still somewhat early on in development, so things are strange and obscure.


Usage:
asciiwrite.py [ascii file] [tape filename]
This takes in a text file with an ascii basic listing in it, and creates a file that can then be loaded using load.
Known issues:
* Newlines are not properly represented, this results in a one line basic program that may not function correctly.

binwrite.py [binary file] [tape filename] [segment] [offset]
This takes in a binary file and converts it to a memory dump file that can be loaded using bload.
Known issues:
* The segment and offset seem to do nothing, so I tend to just set them to 0 0.
* Loading into memory can be iffy, use def seg and point it somewhere in memory that's not used by basic, loading somewhere used by basic will have strange results.
* Refer to the appendixes in the IBM PC Basic guide for information on how to return from assembly programs, it involves a retf and some stack preservation that I personally have not figured out fully.
