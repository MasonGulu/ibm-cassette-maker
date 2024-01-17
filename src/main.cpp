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
#include <queue>
#include "AudioFile.h"

using namespace std;

#define version "1.3"
#define LSB(value) (value & 0x00FF)
#define MSB(value) (value >> 16)

std::queue<int8_t> audioData;
uint16_t CRC = 0xFFFF;

const float SECONDS_ONE  = 0.001f;   // Time in seconds
const float SECONDS_TWO = 0.0005f; // Time in seconds
float FRAMERATE = 4000.f;

float FRAMES_ONE  = (SECONDS_ONE / 2) * FRAMERATE;  // Frames per cycle, two cycles per number
float FRAMES_ZERO = (SECONDS_TWO / 2) * FRAMERATE; // Frames per cycle, two cycles per number

const int8_t VALUE_ON = 127;
const int8_t VALUE_OFF = -128;

const uint16_t MAX_INPUT_FILE_SIZE = 0xFFFF;

void set_framerate(float framerate) {
    FRAMERATE = framerate;
    FRAMES_ONE  = (SECONDS_ONE / 2) * FRAMERATE;
    FRAMES_ZERO = (SECONDS_TWO / 2) * FRAMERATE;
}

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

void write_byte(uint8_t value) {
    for (int x = 7; x > -1; x--) {
        _writeBit((value>>x)&1);
    }
}

void generate_leader() {
    for (int x = 0; x < 256; x++) write_byte(0xFF);
    // ^ 256 bytes of FF
    _writeBit(0);
    // ^ Sync bit
    write_byte(0x16);
}

void generate_trailer() {
    write_byte(0xFF);
    write_byte(0xFF);
    write_byte(0xFF);
    write_byte(0xFF);
}

void generate_silence(int seconds=1) {
    for (int x = 0; x < FRAMERATE*seconds; x++) {
        // 1 second of silence at beginning of file
        audioData.push(0);
    }
}

void write_CRC() {
    uint16_t CRCValue = CRC ^ 0xFFFF;
    write_byte(MSB(CRCValue));
    write_byte(LSB(CRCValue));
}

void generate_basic_header(const char* basic_filename, uint16_t segment, uint16_t offset, int size, uint8_t headertype) {
    // Basic Header removal
    generate_leader();
    CRC = 0xFFFF;
    write_byte(0xA5);
    // ^ Marks this block as BASIC
    bool endOfName = false;
    for (int x = 0; x < 8; x++) {
        if (!endOfName && basic_filename[x] == 0) endOfName = true;
        if (endOfName) write_byte(0b00100000);
        else write_byte(basic_filename[x]);
    }
    // ^ Filename
    write_byte(headertype);
    // ^ Memory area
    write_byte(LSB(size));
    write_byte(MSB(size));
    // ^ File size
    write_byte(LSB(segment));
    write_byte(MSB(segment));
    // ^ Segment
    write_byte(LSB(offset));
    write_byte(MSB(offset));
    // ^ Offset

    write_byte(0x00);
    for (int x = 0; x < 239; x++) {
        write_byte(0x01);
    }
    // ^ Filler
    write_CRC();

    generate_trailer();
    generate_silence();
}

void write_block(char* inputfile, int size, int &base, int &written, int bytes, bool &fileend, bool replaceEndline=false) {
    for (int x = 0; x < bytes; x++) {
        if (base+x+1 > size) fileend = true;
        if (fileend) write_byte(0b00100000); // Fill rest of block with NULL
        else {
            if (replaceEndline) (inputfile[base+x] == 0x0A) ? write_byte(0x0D) : write_byte(inputfile[base+x]);
            else write_byte(inputfile[base+x]);
            written++;
        }
    }
    write_CRC();
    base+=bytes;
}

void bin_write(char* inputfile, int size, const char* basic_filename, uint16_t segment, uint16_t offset, bool basicHeader = true) {
    generate_silence();
    if (basicHeader) {
        generate_basic_header(basic_filename, segment, offset, size, 0x01);
    }
    bool fileend = false;
    int base = 0;
    int written = 0;
    generate_leader();
    while (!fileend) {
        CRC = 0xFFFF;
        write_block(inputfile, size, base, written, 256, fileend);
    }
    cout << "Processed " << written << " bytes." << endl;
    generate_trailer();
    // Leadout
    generate_silence();
}


void img_write(char* inputfile, int size, const char* basic_filename, uint16_t segment, uint16_t offset, bool _) {
    generate_silence(3);
    bool fileend = false;
    int base = 0;
    int written = 0;
    int tracksWritten = 0;
    while (!fileend) {
        generate_leader();
        for (int i = 0; i < 32; i++) {
            CRC = 0xFFFF;
            write_block(inputfile, size, base, written, 256, fileend);
        }
        tracksWritten++;
        cout << "Wrote " << tracksWritten << " tracks; " << written << " bytes." << endl;
        if (tracksWritten == 40) fileend=true;
        // For some reason my fileend logic doesn't work in this specific instance, so hardcode it! Filesize is already enforced anyways.
        generate_trailer();
        generate_silence(10);
    }
}

void ascii_write(char* inputfile, int size, const char* basic_filename, uint16_t segment, uint16_t offset, bool _) {
    generate_silence();
    generate_basic_header(basic_filename, segment, offset, size, 0b01000000);

    bool fileend = false;
    int base = 0;
    int written = 0;
    while (!fileend) {
        generate_leader();
        CRC = 0xFFFF;
        ((size - written) > 255) ? write_byte(0x00) : write_byte(size-written);
        write_block(inputfile, size, base, written, 255, fileend, true);
        generate_trailer();
        generate_silence();
    }
    cout << "Processed " << written << " bytes." << endl;
}

typedef void (*write_func_t) (char*,int,const char*,uint16_t,uint16_t,bool);

// Parsed arguments
char* if_buffer;
int if_size;
write_func_t process_func;
bool do_header;
string basic_filename;
string filename;
uint16_t segment = 0;
uint16_t offset = 0;


int parse_flags(int argc, char* argv[]) {
    for (int i = 4; i < argc; i++) {
        if (strncmp(argv[i], "-86box", 6) == 0) {
            set_framerate(44100.0f);
        } else if (strncmp(argv[i], "-segment=", 9) == 0) {
            segment = atoi(argv[i+9]);
        } else if (strncmp(argv[i], "-offset=", 8) == 0) {
            offset = atoi(argv[i+8]);
        } else {
            printf("Unrecognized argument %s.\n", argv[i]);
        }
    }
}

int parse_args(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Usage: %s <raw, bin, bas, img(*)> input basic_name [flags]\n", argv[0]);
        printf("(*) img is for 320kb disk images, see README.\n");
        printf("basic_name up to 8 character BASIC filename.\n\n");
        printf("Flags:\n");
        printf("-segment=<value>\n-offset=<value>\n");
        printf("-86box Change bitrate to 44.1kHz for 86box compat.\n");
        return 1;
    }

    parse_flags(argc, argv);

    basic_filename = string(argv[3]).substr(0, 8);
    filename = basic_filename.append(".wav");

    ifstream ifStream (argv[2], ios::binary|ios::ate);
    if (ifStream.is_open()) {
        if_size = ifStream.tellg();
        if_buffer = new char [if_size];
        ifStream.seekg(0, ios::beg);
        ifStream.read(if_buffer, if_size);
        ifStream.close();
    }
    else {
        cout << "ERROR: The input file cannot be opened." << endl;
        return 1;
    }
    if ((if_size > MAX_INPUT_FILE_SIZE) && (argv[1][2] != 'g')) {
        cout << "ERROR: The input file is larger than " << MAX_INPUT_FILE_SIZE << " bytes." << endl;
        return 1;
    }
    do_header = true;
    switch(argv[1][2]) {
        case 'n': // bin
            process_func = bin_write;
            break;
        case 's': // bas
            process_func = ascii_write;
            break;
        case 'w': // raw
            process_func = bin_write;
            do_header = false;
            break;
        case 'g': // img
            if (if_size != 327680) {
                cout << "File is not 320kb." << endl;
                return 1;
            }
            process_func = ascii_write;
            break;
        default:
            cout << "Invalid argument." << endl;
            return 1;
    }
    return 0;
}

void process_data() {
    process_func(if_buffer, if_size, basic_filename.c_str(), segment, offset, do_header);

    AudioFile<double> audiofile;
    AudioFile<double>::AudioBuffer buffer;
    int length = audioData.size();
    buffer.resize(1);
    buffer[0].resize(length);
    audiofile.setAudioBufferSize(1, length);
    audiofile.setSampleRate(FRAMERATE);
    audiofile.setBitDepth(8);
    int index = 0;
    
    while (audioData.size() > 0) {
        buffer[0][index] = audioData.front();
        audioData.pop();
        index++;
    }
    cout << "Processed " << index << " frames." << endl;
    audiofile.setAudioBuffer(buffer);
    audiofile.save(filename, AudioFileFormat::Wave);
}

int main(int argc, char *argv[]) {
    cout << "This program is licensed under GPLv2 and comes with ABSOLUTELY NO WARRANTY." << endl;
    cout << "Version " << version << endl;

    int arg_result = parse_args(argc, argv);
    if (arg_result != 0) {
        return arg_result;
    }

    process_data();
}
