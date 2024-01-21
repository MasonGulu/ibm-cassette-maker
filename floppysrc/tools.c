
#define _FP_OFF(__p) ((unsigned)(__p))
#define _FP_SEG(__p) ((unsigned)((unsigned long)(void __far*)(__p) >> 16))

extern void print(const char*);
#define SCANCODE(value) (value >> 8)
#define ASCIICODE(value) (value & 0xFF)
extern int read(); // call int16h; scan code, ascii code
extern char format(char sectors, struct format_loc loc, struct format_desc *info, char track);
extern void reset(char drive);
extern char verify(char sectors, struct format_loc loc, struct format_desc *info, char track);
extern int get_seg();

void print_int(unsigned int i) {
    char to_print[6];
    int index = 0;
    to_print[5] = 0;
    for (index = 4; index >= 0; index--) {
        to_print[index] = (i % 10) + 48;
        i /= 10;
        if (i == 0) {
            break;
        }
    }
    print(to_print+index);
}
struct format_desc {
    char track;
    char head;
    char sector; // Start at 1 - 9
    char bps; // Bytes per sector, 2 = 512bits
};

struct format_desc format_table[9] = {
    {0,0,1,2},
    {0,0,2,2},
    {0,0,3,2},
    {0,0,4,2},
    {0,0,5,2},
    {0,0,6,2},
    {0,0,7,2},
    {0,0,8,2},
    {0,0,9,2},
};

struct format_loc {
    char drive;
    char head;
};

struct disk_base {
    char spec1;
    char spec2;
    char motor_wait;
    char N; // bytes per sector (2=512bps)
    char EOT; // last track on the disk (8 default)
    char gap_length; // 0x2A default
    char DTL; // 0xFF default
    char fmt_gap_length; // gap length for format 0x50 default
    char fill_byte; // fill byte for format 0xF6 default
    char settle_time; // head settle time (25ms default)
    char start_time; // motor start time (4 /8ths s default)
} disk_base;

void cpy(char __far * from, char __far * to, int n) {
    int i = 0;
    for (; i < n; i++) {
        to[i] = from[i];
    }
}

char newline[] = "\r\n";

struct disk_base __far * __far * INT_1E = 0x0000 :> 0x0078; // int 1E

struct disk_base __far *OLD_BASE;

char* get_error_str(int code) {
    switch (code) {
        case 1:
            return "Invalid Command";
        case 2:
            return "Cannot Find Address Mark";
        case 3:
            return "Attempted Write on Write Protected Disk";
        case 4:
            return "Sector Not Found";
        case 5:
            return "Reset Failed";
        case 6:
            return "Disk Change Line Active";
        case 7:
            return "Drive Parameter Activity Failed";
        case 8:
            return "DMA Overrun";
        case 9:
            return "Attempt To DMA Over 64kb Boundary";
        case 0xA:
            return "Bad Sector Detected";
        case 0xB:
            return "Bad Track Detected";
        case 0xC:
            return "Media Type Not Found";
        case 0xD:
            return "Invalid Number Of Sectors";
        case 0xE:
            return "Control Data Address Mark Detected";
        case 0xF:
            return "DMA Out Of Range";
        case 0x10:
            return "CRC/ECC Data Error";
        case 0x11:
            return "ECC Corrected Data Error";
        case 0x20:
            return "Controller Failure";
        case 0x40:
            return "Seek Failure";
        case 0x80:
            return "Drive Timed Out, Assumed Not Ready";
        case 0xAA:
            return "Drive Not Ready";
        case 0xCC:
            return "Write Error";
        case 0xE0:
            return "Status Error";
        case 0xFF:
            return "Sense Operation Failed";
        default:
            return "Unknown Error";
    }
}

void update_disk_base(char tracks) {
    int segment = get_seg();
    int offset = (int) &disk_base;
    struct disk_base __far * LOCAL_BASE = segment :> offset;
    OLD_BASE = *INT_1E;
    cpy((char __far*)OLD_BASE, (char __far*)LOCAL_BASE, sizeof(struct disk_base));

    disk_base.EOT = tracks;
    if (tracks == 9) {
        disk_base.gap_length = 0x18; // https://github.com/andreas-jonsson/imagedisk/blob/main/IMD.C#L241C18-L241C19
    }

    *INT_1E = LOCAL_BASE;
}

void revert_disk_base() {
    *INT_1E = OLD_BASE;
}

void update_format_table(char track, char head) {
    int i;
    struct format_desc *desc;
    for (i = 0; i < 9; i++) {
        desc = format_table+i;
        desc->track = track;
        desc->head = head;
    }
}

int do_format(char sectors) {
    char track;
    char head;
    int errors;
    int failed_tracks = 0;
    struct format_loc loc = {0, 0};
    for (track = 0; track < 40; track++) {
        for (head = 0; head < 2; head++) {
            errors = 0;
            print("Formatting Track: ");
            print_int(track);
            print(" Head: ");
            print_int(head);
            print("\r");
            loc.head = head;
            update_format_table(track, head);
            while (1) {
                char status = format(sectors, loc, format_table, track);
                if (status == 96 || status == 0) break;
                print("\r\nFormat errored: ");
                print_int(status);
                print(" ");
                print(get_error_str(status));
                print(newline);
                reset(0);
                errors++;
                if (errors == 3) {
                    print("Failed to format track.\r\n");
                    return 1;
                }
            }
            // format succeeded, now verify
            errors = verify(sectors, loc, format_table, track);
            if (errors == 1) {
                print("\r\nFailed to verify track. It is likely bad.\r\n");
                failed_tracks++;
            }
        }
    }
    if (failed_tracks == 0) {
        print("Disk was successfully formatted.\r\n");
    } else {
        print_int(failed_tracks);
        print(" tracks failed to verify.\r\n");
        print("Disk cannot be for image writing.\r\n");
        return 1;
    }
    return 0;
}
void test() {
    char input;
    int sectors;
    print("Push 8/9 to format\r\n");
    input = ASCIICODE(read());
    if (input == '8') {
        sectors = 8;
    } else if (input == '9') {
        sectors = 9;
    } else {
        print("Cancelled\r\n");
        return;
    }
    update_disk_base(sectors);
    do_format(sectors);
    revert_disk_base();
}
