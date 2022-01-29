# Version 1.1
# Convert ascii basic files into audio files suitable for loading on the IBM PC 5150
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
import sys, wave, os


one = 0.001 # Time in seconds
zero = 0.0005 # Time in seconds

framerate = 4000 # Frames / second
oneframe = (one / 2) * framerate # Frames per cycle, two cycles per number
zeroframe = (zero / 2) * framerate # Frames per cycle, two cycles per number


on = 0x7FFF
off = 0
sys.argv = sys.argv[1:]
if len(sys.argv) < 2:
	print("Usage:")
	print("asciiwrite [input file] [basic filename]")
	sys.exit(0)

outputf = wave.open(sys.argv[1]+"_ASCII.wav",'wb')
outputf.setframerate(framerate)
outputf.setnchannels(1)
outputf.setsampwidth(2)

global crc_reg
inputf = open(sys.argv[0], 'rb')

crc_reg = 0xFFFF

def crc_gen(crc_bit):
	global crc_reg
	carry = 0
	if (crc_bit) != (crc_reg & 0x8000) >> 15:
		# These are unequal
		crc_reg = crc_reg ^ 0x0810
		carry = 1
	crc_reg = ((crc_reg << 1) + carry) & 0xffff
	print(crc_reg)

def write(value):
	crc_gen(value)
	length = (int(zeroframe), int(oneframe))
	values = (off, on)
	for onlen in range(0, length[value]):
		outputf.writeframes(int.to_bytes(on, 2, "little"))
		
	for offlen in range(0, length[value]):
		outputf.writeframes(int.to_bytes(off, 2, "little"))

def writeByte(value):
	for x in range(7, -1, -1):
		write((value & (1 << x)) >> x)

filename = sys.argv[1]

# Add silence
for x in range(0, framerate):
	outputf.writeframes(int.to_bytes(0, 1, "little"))

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
# Flag for ascii listing
writeByte(0b01000000)
# Write filesize word
filesize = (os.stat(sys.argv[0]).st_size).to_bytes(2, "little")
writeByte(filesize[0])
writeByte(filesize[1])
# Segment word
writeByte(0x60)
writeByte(0x00)
# offset word
writeByte(0x1E)
writeByte(0x80)

# Filler
writeByte(0x00)
for x in range(0, 239):
	writeByte(0x01)

# Write crc
crcbyte = (crc_reg^0xffff).to_bytes(2, "little")
writeByte(crcbyte[1])
writeByte(crcbyte[0])

writeByte(0xFF)
writeByte(0xFF)
writeByte(0xFF)
writeByte(0xFF)

# Add silence
for x in range(0, framerate):
	outputf.writeframes(int.to_bytes(0, 1, "little"))


byteswritten = 0

byte = inputf.read(255)
while byte:
	# Every single block of data in an ascii file needs a lead in
	for x in range(0, 256):
		writeByte(0xff)
	# Write 256 bytes of 1s

	write(0) #Sync bit
	writeByte(0x16) #Sync byte
	crc_reg = 0xFFFF
	# Datablock, 254 and 255 because the data block is 256 bytes minus the 1 byte remainder header
	if (os.stat(sys.argv[0]).st_size - byteswritten) > 255:
		writeByte(0x00)
	else:
		writeByte(os.stat(sys.argv[0]).st_size - byteswritten + 1)
	
	for x in range(0, 255):
		if(byte[x] == 0x0A):
			writeByte(0x0D)
		else:
			writeByte(byte[x])
		byteswritten = byteswritten + 1
		if (x + 1) >= len(byte) and x != 254:
			print("End of file reached.")
			# reached the end of file
			for y in range(x, 254):
				writeByte(0b00100000)
			break
	crcbyte = (crc_reg^0xffff).to_bytes(2, "little")
	writeByte(crcbyte[1])
	writeByte(crcbyte[0])
	byte = inputf.read(255)
	print(str(byteswritten) + " bytes written.")
	writeByte(0xFF)
	writeByte(0xFF)
	writeByte(0xFF)
	writeByte(0xFF)
	# 4 byte trailer

	# Add a half second of silence
	for x in range(0, framerate):
		outputf.writeframes(int.to_bytes(0, 1, "little"))





# Add silence
for x in range(0, framerate):
	outputf.writeframes(int.to_bytes(0, 1, "little"))
