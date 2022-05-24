/*
 This will take a binary or ascii file and convert it into a recording
 which can then be loaded on an IBM 5150
 Copyright (C) 2021 Mason Gulu
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 If you have any questions or comments you may contact me at mason.gulu@gmail.com
*/
#include <iostream>
#include <fstream>
#include <cfloat>
#include <queue>
#include "AudioFile.h"

using namespace std;

#define version "1.2"


std::queue<double> audioData;
uint16_t CRC = 0xFFFF;

const float SECONDS_ONE  = 0.001f;   // Time in seconds
const float SECONDS_TWO = 0.0005f; // Time in seconds
const float FRAMERATE = 4000.f;

const float FRAMES_ONE  = (SECONDS_ONE / 2) * FRAMERATE;  // Frames per cycle, two cycles per number
const float FRAMES_ZERO = (SECONDS_TWO / 2) * FRAMERATE; // Frames per cycle, two cycles per number

const double VALUE_ON = 0xFFFF;
const double VALUE_OFF = -0xFFFF;

const uint16_t MAX_INPUT_FILE_SIZE = 0xFFFF;

void _writeCRC(bool bit) {
    bool carry = 0;
    if ((bit) != ((CRC >> 15) & 1)) {
        CRC = CRC ^ 0x0810;
        carry = 1;
    }
    CRC = (((CRC << 1) + carry) & 0xffff);
}

void _writeBit(bool valueBit) {
    _writeCRC(valueBit);
    int len = ((valueBit) ? FRAMES_ONE : FRAMES_ZERO);
    for (int onlen = 0; onlen < len; onlen++) {
        audioData.push(VALUE_ON);
    }
    for (int offlen = 0; offlen < len; offlen++) {
        audioData.push(VALUE_OFF);
    }
}

void writeByte(uint8_t value) {
    for (int x = 7; x > -1; x--) {
        _writeBit((value>>x)&1);
    }
}

void generateLeader() {
    for (int x = 0; x < 256; x++) writeByte(0xFF);
    // ^ 256 bytes of FF
    _writeBit(0);
    // ^ Sync bit
    writeByte(0x16);
}

void generateTrailer() {
    writeByte(0xFF);
    writeByte(0xFF);
    writeByte(0xFF);
    writeByte(0xFF);
}

void generateSilence(int seconds=1) {
    for (int x = 0; x < FRAMERATE*seconds; x++) {
        // 1 second of silence at beginning of file
        audioData.push(0);
    }
}

uint8_t MSB(uint16_t value) {
    return ((value & 0xFF00) >> 8);
}

uint8_t LSB(uint16_t value) {
    return (value & 0x00FF);
}

void writeCRC() {
    uint16_t CRCValue = CRC ^ 0xFFFF;
    writeByte(MSB(CRCValue));
    writeByte(LSB(CRCValue));
}

void generateBasicHeader(char* filename, uint16_t segment, uint16_t offset, int size, uint8_t headertype) {
    // Basic Header removal
    generateLeader();
    CRC = 0xFFFF;
    writeByte(0xA5);
    // ^ Marks this block as BASIC
    bool endOfName = false;
    for (int x = 0; x < 8; x++) {
        if (!endOfName && filename[x] == 0) endOfName = true;
        if (endOfName) writeByte(0b00100000);
        else writeByte(filename[x]);
    }
    // ^ Filename
    writeByte(headertype);
    // ^ Memory area
    writeByte(LSB(size));
    writeByte(MSB(size));
    // ^ File size
    writeByte(LSB(segment));
    writeByte(MSB(segment));
    // ^ Segment
    writeByte(LSB(offset));
    writeByte(MSB(offset));
    // ^ Offset

    writeByte(0x00);
    for (int x = 0; x < 239; x++) {
        writeByte(0x01);
    }
    // ^ Filler
    writeCRC();

    generateTrailer();
    generateSilence();
}

void writeBlock(char* inputfile, int size, int &base, int &written, int bytes, bool &fileend, bool replaceEndline=false) {
    for (int x = 0; x < bytes; x++) {
        if (base+x+1 > size) fileend = true;
        if (fileend) writeByte(0b00100000); // Fill rest of block with NULL
        else {
            if (replaceEndline) (inputfile[base+x] == 0x0A) ? writeByte(0x0D) : writeByte(inputfile[base+x]);
            else writeByte(inputfile[base+x]);
            written++;
        }
    }
    writeCRC();
    base+=bytes;
}

void binWrite(char* inputfile, int size, char* filename, uint16_t segment, uint16_t offset, bool basicHeader = true) {
    generateSilence();
    if (basicHeader) {
        generateBasicHeader(filename, segment, offset, size, 0x01);
    }
    bool fileend = false;
    int base = 0;
    int written = 0;
    generateLeader();
    while (!fileend) {
        CRC = 0xFFFF;
        writeBlock(inputfile, size, base, written, 256, fileend);
    }
    cout << "Processed " << written << " bytes." << endl;
    generateTrailer();
    // Leadout
    generateSilence();
}


void imgWrite(char* inputfile, int size, char* filename, uint16_t segment, uint16_t offset) {
    generateSilence(3);
    bool fileend = false;
    int base = 0;
    int written = 0;
    int tracksWritten = 0;
    while (!fileend) {
        generateLeader();
        for (int i = 0; i < 32; i++) {
            CRC = 0xFFFF;
            writeBlock(inputfile, size, base, written, 256, fileend);
        }
        tracksWritten++;
        cout << "Wrote " << tracksWritten << " tracks; " << written << " bytes." << endl;
        if (tracksWritten == 40) fileend=true;
        // For some reason my fileend logic doesn't work in this specific instance, so hardcode it! Filesize is already enforced anyways.
        generateTrailer();
        generateSilence(10);
    }
}

void asciiWrite(char* inputfile, int size, char* filename, uint16_t segment, uint16_t offset) {
    generateSilence();
    generateBasicHeader(filename, segment, offset, size, 0b01000000);

    bool fileend = false;
    int base = 0;
    int written = 0;
    while (!fileend) {
        generateLeader();
        CRC = 0xFFFF;
        ((size - written) > 255) ? writeByte(0x00) : writeByte(size-written);
        writeBlock(inputfile, size, base, written, 255, fileend, true);
        generateTrailer();
        generateSilence();
    }
    cout << "Processed " << written << " bytes." << endl;
}

int main(int argCount, char *argValues[]) {
    cout << "This program is licensed under GPLv2 and comes with ABSOLUTELY NO WARRANTY." << endl;
    cout << "Version " << version << endl;
    if (argCount < 6) {
        cout << "Usage: " << argValues[0] << " <raw, bin, bas, img(*)> [input] [output] [segment] [offset]" << endl;
        cout << "(*) img is for 320kb disk images, see README." << endl;
        return 0;
    }

    streampos ifSize;
    char* ifBuffer;
    ifstream ifStream (argValues[2], ios::binary|ios::ate);
    if (ifStream.is_open()) {
        ifSize = ifStream.tellg();
        ifBuffer = new char [ifSize];
        ifStream.seekg(0, ios::beg);
        ifStream.read(ifBuffer, ifSize);
        ifStream.close();
    }
    else {
        cout << "ERROR: The input file cannot be opened." << endl;
        return 0;
    }
    if ((ifSize > MAX_INPUT_FILE_SIZE) && (argValues[1][2] != 'g')) {
        cout << "ERROR: The input file is larger than " << MAX_INPUT_FILE_SIZE << " bytes." << endl;
        return 0;
    }
    // At this point the entire file contents is loaded into ifBuffer
    unsigned sampleCount = 1;
    
    switch(argValues[1][2]) {
        case 'n': // bin
            binWrite(ifBuffer, (int)ifSize, argValues[3], atoi(argValues[4]), atoi(argValues[5]));
            break;
        case 's': // bas
            asciiWrite(ifBuffer, (int)ifSize, argValues[3], atoi(argValues[4]), atoi(argValues[5]));
            break;
        case 'w': // raw
            binWrite(ifBuffer, (int)ifSize, argValues[3], atoi(argValues[4]), atoi(argValues[5]), false);
            break;
        case 'g': // img
            if (ifSize != 327680) {
                cout << "File is not 320kb." << endl;
                return 0;
            }
            // buffer is already interlaced.
            imgWrite(ifBuffer, (int)ifSize, argValues[3], atoi(argValues[4]), atoi(argValues[5]));
            break;
        default:
            cout << "Invalid argument." << endl;
            return 0;
    }
    AudioFile<double> audiofile;
    AudioFile<double>::AudioBuffer buffer;
    int length = audioData.size();
    buffer.resize(1);
    buffer[0].resize(length);
    audiofile.setAudioBufferSize(1, length);
    audiofile.setSampleRate(FRAMERATE);
    int index = 0;
    
    while (audioData.size() > 0) {
        buffer[0][index] = audioData.front();
        audioData.pop();
        index++;
    }
    cout << "Processed " << index << " frames." << endl;
    audiofile.setAudioBuffer(buffer);
    audiofile.save(argValues[3], AudioFileFormat::Wave);
}
