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
#include "LL.cpp"
#include "AudioFile.h"

using namespace std;


LL<double>* audioLLPos;
uint16_t crc_reg = 0xFFFF;

const float one  = 0.001f;   // Time in seconds
const float zero = 0.0005f; // Time in seconds
const float framerate = 4000.f;

const float oneframe  = (one / 2) * framerate;  // Frames per cycle, two cycles per number
const float zeroframe = (zero / 2) * framerate; // Frames per cycle, two cycles per number

const double on = 0x7FFF;
const double off = 0;

void crc_gen(bool crc_bit) {
    bool carry = 0;
    if ((crc_bit) != ((crc_reg >> 15) & 1)) {
        crc_reg = crc_reg ^ 0x0810;
        carry = 1;
    }
    crc_reg = (((crc_reg << 1) + carry) & 0xffff);
}

void write(bool value) {
    crc_gen(value);
    int len = ((value) ? oneframe : zeroframe);
    for (int onlen = 0; onlen < len; onlen++) {
        audioLLPos->appendToEnd(on);
        audioLLPos = audioLLPos->next;
    }
    for (int offlen = 0; offlen < len; offlen++) {
        audioLLPos->appendToEnd(off);
        audioLLPos = audioLLPos->next;
    }
}

void writeByte(uint8_t value) {
    for (int x = 7; x > -1; x--) {
        write((value>>x)&1);
    }
}

void generateLeader() {
    for (int x = 0; x < 256; x++) writeByte(0xFF);
    // ^ 256 bytes of FF
    write(0);
    // ^ Sync bit
    writeByte(0x16);
    // ^ Sync byte
    crc_reg = 0xFFFF;
    // ^ Reset CRC
}

void generateSilence(int seconds=1) {
    for (int x = 0; x < framerate*seconds; x++) {
        // 1 second of silence at beginning of file
        audioLLPos->appendToEnd(off);
        audioLLPos = audioLLPos->next;
    }
}

void writeCRC() {
    uint16_t crcbyte = crc_reg ^ 0xFFFF;
    writeByte((crcbyte & 0xFF00) >> 8);
    writeByte(crcbyte & 0x00FF);
}

void generateBasicHeader(char* filename, uint16_t segment, uint16_t offset, int size, uint8_t headertype) {
    // Basic Header removal
    generateLeader();
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
    writeByte(size & 0x00FF);
    writeByte((size & 0xFF00) >> 8);
    // ^ File size
    writeByte(segment & 0x00FF);
    writeByte((segment & 0xFF00) >> 8);
    // ^ Segment
    writeByte(offset & 0x00FF);
    writeByte((offset & 0xFF00) >> 8);
    // ^ Offset

    writeByte(0x00);
    for (int x = 0; x < 239; x++) {
        writeByte(0x01);
    }
    // ^ Filler
    writeCRC();

    writeByte(0xFF);
    writeByte(0xFF);
    writeByte(0xFF);
    writeByte(0xFF);
    generateSilence();
}

void binwrite(char* inputfile, int size, char* filename, uint16_t segment, uint16_t offset) {
    generateSilence();
    if (true) {
        generateBasicHeader(filename, segment, offset, size, 0x01);
    }
    bool fileend = false;
    int base = 0;
    int written = 0;
    generateLeader();
    while (!fileend) {
        crc_reg = 0xFFFF;
        for (int x = 0; x < 256; x++) {
            if (base+x+1 > size) fileend = true;
            if (fileend) writeByte(0b00100000); // Fill rest of block with NULL
            else {
                writeByte(inputfile[base+x]);
                written++;
            }
        }
        writeCRC();
        base+=256;
        //cout << "Bytes Processed " << written << endl;
    }
    cout << "Processed " << written << " bytes." << endl;
    writeByte(0xFF);
    writeByte(0xFF);
    writeByte(0xFF);
    writeByte(0xFF);
    // Leadout
    generateSilence();
}

void asciiwrite(char* inputfile, int size, char* filename, uint16_t segment, uint16_t offset) {
    generateSilence();
    generateBasicHeader(filename, segment, offset, size, 0b01000000);

    bool fileend = false;
    int base = 0;
    int written = 0;
    generateLeader();
    while (!fileend) {
        crc_reg = 0xFFFF;
        ((size - written) > 255) ? writeByte(0x00) : writeByte(size-written);
        for (int x = 0; x < 255; x++) {
            if (base+x+1 > size) fileend = true;
            if (fileend) writeByte(0b00100000);
            else {
                (inputfile[base+x] == 0x0A) ? writeByte(0x0D) : writeByte(inputfile[base+x]);
                written++;
            }
        }
        writeCRC();
        base+=255;
    }
    cout << "Processed " << written << " bytes." << endl;
    writeByte(0xFF);
    writeByte(0xFF);
    writeByte(0xFF);
    writeByte(0xFF);
    generateSilence();
}

int depth = 0;

template <typename T>
int lengthOfLL(LL<T>* start) {
    int count = 0; // Initialize count 
    LL<T>* current = start; // Initialize current 
    while (current != NULL) 
    { 
        count++; 
        current = current->next; 
    } 
    return count; 
}

int main(int argc, char *argv[]) {
    cout << "This program is licensed under GPLv2 and comes with ABSOLUTELY NO WARRANTY." << endl;
    if (argc < 6) {
        cout << "Usage: writer <bin, bas> [input] [output] [segment] [offset]" << endl;
        return 0;
    }

    streampos size;
    char* inputfile;
    ifstream file (argv[2], ios::binary|ios::ate);
    if (file.is_open()) {
        size = file.tellg();
        inputfile = new char [size];
        file.seekg(0, ios::beg);
        file.read(inputfile, size);
        file.close();
    }
    else {
        cout << "ERROR: The input file cannot be opened." << endl;
        return 0;
    }
    // At this point the entire file contents is loaded into inputfile
    LL<double>* audioLLBegin = new LL<double>;
    audioLLBegin->data = 0;
    audioLLPos = audioLLBegin;
    unsigned sampleCount = 1;
    
    switch(argv[1][1]) {
        case 'i':
            binwrite(inputfile, size, argv[3], atoi(argv[4]), atoi(argv[5]));
            break;
        case 'a':
            asciiwrite(inputfile, size, argv[3], atoi(argv[4]), atoi(argv[5]));
            break;
        default:
            cout << "Invalid argument." << endl;
            return 0;
    }
    AudioFile<double> audiofile;
    AudioFile<double>::AudioBuffer buffer;
    int length = lengthOfLL<double>(audioLLBegin);
    buffer.resize(1);
    buffer[0].resize(length);
    audiofile.setAudioBufferSize(1, length);
    audiofile.setSampleRate(framerate);
    int index = 0;
    audioLLPos = audioLLBegin;
    
    while (audioLLPos != NULL) {
        buffer[0][index] = audioLLPos->data;
        audioLLPos = audioLLPos->next;
        index++;
    }
    cout << "Processed " << index << " frames." << endl;
    audiofile.setAudioBuffer(buffer);
    audiofile.save(argv[3], AudioFileFormat::Wave);
}