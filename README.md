# IBMWRITER
This project has been ported to C++. The python version has been archived at https://github.com/MasonGulu/ibm-cassette-maker-py.
This will convert binary and ascii files into a format that can be loaded over the IBM PC's cassette port.

Usage:
ibmwriter <bin, bas, raw, img> input basic_name [flags]

Modes:
* bin - write a binary file with basic header
* bas - write an ascii basic listing with basic header
* raw - write a binary file without a basic header
* img - write a 320kb disk image with proper delays and spacing.
  * See below.

Flags:
* Set the segment/offset in the basic header. Have yet to find anything that this matters for. Defaults to 0.
  * `-segment=<value>`
  * `-offset=<value>`
* `-86box` Increase the bitrate to 44.1kHz for [86box](https://86box.net/) compat. ~11x larger files and I've yet to have issues loading at the 4kHz default bitrate.

# Cassette loading tips
I recommend maxing out your volume. If you constantly get errors when loading over cassette try a different playback device.

When writing a 320kb disk if you encounter an error loading a track you can easily resume from where you left off. To the left of the most recent "Expecting" message is a 2 digit number in hexidecimal. Open your diskimage audio file in a visual audio editor, count out the "blocks" of audio (starting at 0) until you reach that hex number. Push a button on the 5150 to try to load off cassette again, and resume the audio file right before the block you counted.

# Writing a 320kb boot disk over the cassette port.
This program contains additional functionality to enable creating a 320kb boot disk over the cassette port.
## Quickstart
diskimage.wav is a Dos 3.3 boot disk with debug.com, a text editor, and several disk tools.
Download floppy.wav and diskimage.wav from the releases section.
* Boot your 5150/compatible to BASIC and run the following commands.
  * def seg = &h2000
  * bload"floppy",0
* Play floppy.wav into your cassette input.
  * Once the basic header plays you should see "floppy.M found", let the file continue loading.
* Once the file finishes loading run the following commands.
  * o = 0
  * call o
* Insert a 5 1/4" disk into drive A
* Push 1 to format the disk.
  * If the disk fails to format it probably has bad sectors, try a different disk.
* Push 2 to write the disk.
  * Push anyting but escape at the prompt and start playing diskimage.wav.
  * **THIS WILL TAKE 40+ MINUTES.**

## Using your own disk images
The disk image you want to use has to be in an img raw format, and *must* be 320kb. This program does not support writing 360kb disks.
* Take your disk image and run ibmwriter with the following parameters
  * ibmwriter img disk.img disk
* Follow the instructions under Quickstart for the audio files you created.

# floppy.bin
This is an 8086 assembly program that will format a floppy and then write track by track information that's loaded from the cassette port.
**This program is only intended to be run on an IBM 5150 and does NOT follow standards expected by later PCs. The presense of an XT-IDE will cause the program to function incorrectly. REMOVE YOUR XT-IDE FROM THE 5150/compatible BEFORE ATTEMPTING TO USE THIS PROGRAM.**

## Assembling floppy.bin
If you want to assemble the floppy formatting/writing program follow these steps.
* Ensure your working directory is the floppytools/ directory
* Install nasm and run the following command
  * nasm main.asm -o floppy.bin
* Run ibmwriter with the following parameters, adjusting to fit your directory structure. This will produce a file called *floppy*. This is a wave file, you can rename the file to include the extension afterwards.
  * ibmwriter bin floppy.bin floppy

