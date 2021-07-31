from PIL import Image
import sys 

colorsCGA0 = [0x000000, 0x5c9c0c, 0x993100, 0x9e5a02]
colorsCGA1 = [0x000000, 0x55a6ab, 0x954eab, 0xababab]
colorsCGAC = [0x000000, 0x55a6ab, 0x993100, 0xababab]

highres = False # Highres mode, automatically detected for full screen images
selected = colorsCGAC # Which color pallette to use
tolerance = 0x030303

sys.argv = sys.argv[1:]
if len(sys.argv) < 2:
	print("Usage:")
	print("cgaprocessing [input file] [output file]")
	sys.exit(0)

im = Image.open(sys.argv[0])

size = im.size
if size[0] > 320:
    # High res mode
    highres = True 
if ((size[0] % 8 > 0) and highres) or ((size[0] % 4 > 0) and not highres):
    # Check width of image
    print("Warning, the image does not line up with byte boundaries!")
    print("This is only really an issue if you're doing partial images.")
if (size[0] % 64 > 0) or (highres and (size[0] % 256 > 0)):
    print("Warning, this image's width does not line up with the data block of a cassette.")
    print("This can be ignored if you're not loading this from cassette.")
if (size[0] * size[1]) % 64 > 0:
    print("Warning, the images' whole size does not line up with cassette data blocks")
    print("This can be ignored if not loading from cassette")

def checkInTolerance(value, expected, tolerance):
    if (value >= (expected - tolerance)) and (value <= (expected + tolerance)):
        return True 
    else:
        return False

def getBinPixel(im,x,y):
    pixel = im.getpixel((x,y))
    vpixel = (pixel[0] << 16) + (pixel[1] << 8) + pixel[2]
    if highres:
        # Highres is mono
        if vpixel > 0x777777:
            # White
            return 1
        else:
            # Black
            return 0
    # Lowres is 4 color
    for x in range(0, 4):
        if checkInTolerance(vpixel, selected[x], tolerance):
            return x 
    print("Undermined pixel color.", hex(vpixel), x, y)
    return 0
    

even = []
odd = []
tmpint = 0
tmppos = 8
outputfile = open(sys.argv[1], "wb")
if im.mode == "RGB":
    for y in range(0, size[1]):
        for x in range(0, size[0]):
            if highres:
                tmppos = tmppos - 1
            else:
                tmppos = tmppos - 2
            tmpint = tmpint + (getBinPixel(im,x,y) << tmppos)
            if (tmppos == 0):
                # This is the end of the byte.
                tmppos = 8
                if (y % 2 == 0):
                    even.append(tmpint.to_bytes(1,"little")) # Note, this is called even because I'm starting at 0. CGA starts at 1, so this is technically odd
                else:
                    odd.append(tmpint.to_bytes(1,"little"))
                    # I hate emulating bitwise stuff, and then converting my emulated bitwise stuff into actual byte objects resulting in the question of am I accidentally reversing what I already did? or is this the correct orientation, and then when you look at it later you are incredibly confused on if I intended on reversing the order of the bits, or if it just miraculously lined up. yes this line is long.
                tmpint = 0
    count = 0 # Amount of bytes written
    evenfile = open(sys.argv[1]+"odd", "wb") # Yes I am mixing up the filenames. This is because CGA starts at 1, python 0.
    for byte in even:
        count = count + 1
        outputfile.write(byte)
        evenfile.write(byte)
    evenfile.close()
    for byte in range(count, 8192):
        outputfile.write(int.to_bytes(0,1,"little"))
    oddfile = open(sys.argv[1]+"even", "wb")
    for byte in odd:
        outputfile.write(byte)
        oddfile.write(byte)
    oddfile.close()

    outputfile.close()

else:
    print("Image mode "+im.mode+" not supported!")