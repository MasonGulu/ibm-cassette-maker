# Version 1.1
# Convert binary files into audio files suitable for loading with bload in Cassette Basic on the IBM PC 5150
# Copyright (C) 2021 Mason Gulu
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.

#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

# If you have any questions or comments you may contact me at mason.gulu@gmail.com
import sys
import wave 
import os 
from datetime import datetime
startTime = datetime.now()

one = 0.001 # Time in seconds
zero = 0.0005 # Time in seconds

framerate = 4000 # Frames / second
oneframe = (one / 2) * framerate # Frames per cycle, two cycles per number
zeroframe = (zero / 2) * framerate # Frames per cycle, two cycles per number



on = 0x7FFF
off = 0
sys.argv = sys.argv[1:]
if len(sys.argv) < 4:
	print("Usage:")
	print("binwrite [input file] [basic filename] [segment] [offset] <Don't generate header>")
	print("ANYTHING being within the <don't generate header> area will disable generating a header.")
	sys.exit(0)

outputf = wave.open(sys.argv[1]+'_BIN.wav','wb')
outputf.setframerate(framerate)
outputf.setnchannels(1)
outputf.setsampwidth(2)

secondsperbyte = 0.00210746417984189723
print("Estimated time: ", os.stat(sys.argv[0]).st_size*secondsperbyte, "seconds.")

segment = int(sys.argv[2]).to_bytes(2, "little")
offset = int(sys.argv[3]).to_bytes(2, "little")

global crc_reg
inputf = open(sys.argv[0], 'rb')

crc_reg = 0xFFFF

def crc_gen(crc_bit):
	global crc_reg
	carry = 0
	if (crc_bit) != ((crc_reg >> 15) & 1): # I think I sped this up a bit
		# These are unequal
		crc_reg = crc_reg ^ 0x0810
		carry = 1
	crc_reg = ((crc_reg << 1) + carry) & 0xffff

def write(value):
	crc_gen(value)
	length = (int(zeroframe), int(oneframe))
	for onlen in range(0, length[value]):
		outputf.writeframes(int.to_bytes(on, 2, "little"))
		
	for offlen in range(0, length[value]):
		outputf.writeframes(int.to_bytes(off, 2, "little"))

def writeByte(value):
	for x in range(7, -1, -1):
		write((value >> x) & 1)
		# No idea why I did it the other way before. This is about 17% faster in limited testing.


filename = sys.argv[1]

# Add silence
for x in range(0, framerate):
	outputf.writeframes(int.to_bytes(0, 1, "little"))
if len(sys.argv) < 5:
	for x in range(0, 256):
		writeByte(0xff)
		# Write 256 bytes of 1s

	write(0) #Sync bit
	writeByte(0x16) #Sync byte

	#reset crc calculation
	crc_reg = 0xFFFF
	writeByte(0xA5)

	for x in range(0, 8):
		if (x >= len(filename)):
			writeByte(0b00100000)
		else:
			writeByte(ord(filename[x]))
	# Flag for memory area
	writeByte(0x01)
	# Write filesize word
	filesize = (os.stat(sys.argv[0]).st_size).to_bytes(2, "little")
	writeByte(filesize[0])
	writeByte(filesize[1])

	# Segment word
	writeByte(segment[0])
	writeByte(segment[1])
	# offset word
	writeByte(offset[0])
	writeByte(offset[1])

	# Filler
	writeByte(0x00)
	for x in range(0, 239):
		writeByte(0x01)

	# Write crc
	print("CRC is " + str(crc_reg))
	crcbyte = (crc_reg^0xffff).to_bytes(2, "little")
	writeByte(crcbyte[1])
	writeByte(crcbyte[0])
	print(crcbyte[1], crcbyte[0])

	writeByte(0xFF)
	writeByte(0xFF)
	writeByte(0xFF)
	writeByte(0xFF)

	print("BASIC header finished at "+str(datetime.now() - startTime))

	# Add silence
	for x in range(0, framerate):
		outputf.writeframes(int.to_bytes(0, 1, "little"))
else:
	print("Not generating a BASIC header")

for x in range(0, 256):
	writeByte(0xff)
	# Write 256 bytes of 1s

write(0) #Sync bit
writeByte(0x16) #Sync byte
byteswritten = 0

byte = inputf.read(256)
while byte:
	crc_reg = 0xFFFF
	for x in range(0, 256):
		writeByte(byte[x])
		byteswritten = byteswritten + 1
		if (x + 1) >= len(byte) and x != 255:
			print("EOF reached.")
			# reached the end of file
			for y in range(x, 255):
				# fill rest of block with null
				writeByte(0b00100000)
			break
	crcbyte = (crc_reg^0xffff).to_bytes(2, "little")
	writeByte(crcbyte[1])
	writeByte(crcbyte[0])
	byte = inputf.read(256)
	print("Written bytes:", byteswritten)

writeByte(0xFF)
writeByte(0xFF)
writeByte(0xFF)
writeByte(0xFF)


# Add silence
for x in range(0, round(framerate/4)):
	outputf.writeframes(int.to_bytes(0, 4, "little"))

print("Finished at "+str(datetime.now() - startTime))
