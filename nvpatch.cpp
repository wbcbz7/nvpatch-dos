#include <stdint.h>
#include <stdio.h>
#include <conio.h>
#include <i86.h>
#include "tinypci.h"

#define arrayof(a) (sizeof(a) / sizeof(a[0]))

// read one byte
uint8_t read_byte(FILE *f, uint32_t pos) {
    uint8_t data;
    fseek(f, pos, SEEK_SET);
    fread(&data, sizeof(data), 1, f);
    return data;
}

// read word
uint16_t read_word(FILE *f, uint32_t pos) {
    uint16_t data;
    fseek(f, pos, SEEK_SET);
    fread(&data, sizeof(data), 1, f);
    return data;
}

// read dword
uint32_t read_dword(FILE *f, uint32_t pos) {
    uint32_t data;
    fseek(f, pos, SEEK_SET);
    fread(&data, sizeof(data), 1, f);
    return data;
}

// read string of bytes
void read_bytes(FILE *f, uint32_t pos, void* data, uint32_t size) {
    fseek(f, pos, SEEK_SET);
    fread(data, size, 1, f);
}

// patch one byte 
void patch_byte(FILE *f, uint32_t pos, uint8_t data) {
    fseek(f, pos, SEEK_SET);
    fwrite(&data, sizeof(data), 1, f);
}

// patch word
void patch_word(FILE *f, uint32_t pos, uint16_t data) {
    fseek(f, pos, SEEK_SET);
    fwrite(&data, sizeof(data), 1, f);
}

// patch dword
void patch_dword(FILE *f, uint32_t pos, uint32_t data) {
    fseek(f, pos, SEEK_SET);
    fwrite(&data, sizeof(data), 1, f);
}

// patch string of bytes
void patch_bytes(FILE *f, uint32_t pos, void* data, uint32_t size) {
    fseek(f, pos, SEEK_SET);
    fwrite(data, size, 1, f);
}

// patch multiple locations with same value
void patch_byte_list(FILE *f, uint32_t *pos, uint32_t poslen, uint8_t data) {
    while (poslen-- != 0) {
        patch_byte(f, *pos++, data);
    }
}
void patch_word_list(FILE *f, uint32_t *pos, uint32_t poslen, uint16_t data) {
    while (poslen-- != 0) {
        patch_word(f, *pos++, data);
    }
}
void patch_dword_list(FILE *f, uint32_t *pos, uint32_t poslen, uint32_t data) {
    while (poslen-- != 0) {
        patch_dword(f, *pos++, data);
    }
}

// --------------
// nv realmode backdoor
uint32_t nvRead(uint32_t addr) {
    // write address to realmode backdoor
    outpw(0x3d4, 0x0338);
    outpd(0x3d0, addr);

    // read data
    outpw(0x3d4, 0x0538);
    return inpd(0x3d0);
}

void nvWrite(uint32_t addr, uint32_t data) {
    // write address to realmode backdoor
    outpw(0x3d4, 0x0338);
    outpd(0x3d0, addr);

    // read data
    outpw(0x3d4, 0x0538);
    outpd(0x3d0, data);
}

void nvLock() {
    outpw(0x3d4, 0x0038);
}

uint32_t nvGetAddress() {
    outpw(0x3d4, 0x0338);
    return inpd(0x3d0);
}

void nvSetAddress(uint32_t addr) {
    outpw(0x3d4, 0x0338);
    outpd(0x3d0, addr);
}

void nvCrtcUnlock() {
    outpw(0x3d4, 0x571f);
}

void nvCrtcLock() {
    outpw(0x3d4, 0x991f);
}

// ---------------------
// get xtal frequency in kHz
uint32_t getXtalFreq() {
    // unlock
    nvCrtcUnlock();
    uint32_t savedAddr = nvGetAddress();

    // read PEXTDEV, get frequency
    uint32_t pextdev = nvRead(0x000101000);
    uint32_t xtalfreq = (pextdev & (1 << 22)) ? 27000 : ((pextdev & (1 << 6)) ? 14318 : 13500);

    // lock it back
    nvSetAddress(savedAddr);
    nvLock();
    nvCrtcLock();

    return xtalfreq;
}

// ---------------------
// patch info

struct patchdata_t {
    pciDeviceList* pcidev;
    uint32_t vendorid, deviceid;
    uint32_t xtal_freq;
};

// list of files to scan
struct filescan_t {
    char        *filename;
    uint32_t     fsize;
    uint32_t     match_ofs;
    uint32_t     match_sig;
    int        (*patchfunc)(FILE *f, patchdata_t* pd);
    char        *desc;
};

int sdd653_uvconfig_patch(FILE *f, patchdata_t* pd) {
    // 0x28081: riva128 vendor id (word)
    patch_word(f, 0x28081, pd->vendorid);
    // 0x6ADD8: riva128 vendor id (word)
    patch_word(f, 0x6ADD8, pd->vendorid);
    // 0x6ADDA: riva128 device id (word)
    patch_word(f, 0x6ADDA, pd->deviceid);

    if ((pd->vendorid < 0x80) && (read_byte(f, 0x2805E) != 0x66)) {
        // short vendor ID
        // 0x2807F: riva128 device id (BYTE)
        patch_byte(f, 0x2807F, pd->vendorid & 0xFF);
    } else {
        // long vendor ID - do beeeeg patch (disables NV1 support)
        // 0x2805C: 74 -> EB
        // 0x2805E: C4 5E 06 26 C7 47 02 01 00 -> 66 68 <device id dword> EB 1A 90
        // 0x2807D: 66 6A <device id lobyte> -> EB DF 90
        uint8_t patch1[] = {0x66, 0x68, 0x00, 0x00, 0x00, 0x00, 0xEB, 0x1A, 0x90};
        uint8_t patch2[] = {0xEB, 0xDF, 0x90};
        patch1[2] = (pd->deviceid >> 0) & 0xFF;
        patch1[3] = (pd->deviceid >> 8) & 0xFF;

        patch_byte(f, 0x2805C, 0xEB);
        patch_bytes(f, 0x2805E, patch1, sizeof(patch1));
        patch_bytes(f, 0x2807D, patch2, sizeof(patch2));
    }

    // 0x2AFBD: 7C -> EB : skip VESA info string check
    patch_byte(f, 0x2AFBD, 0xEB);

    // SDD 6.53 code uses sequencer for unlocking Riva128 extended registers,
    // while TNT+ have the same unlock in CRTC
    // example: UNIVBE.DRV 0x4568: mov dx, 0x3C4 / mov ax, 0x5706 / out dx, ax
    // apparently NV BIOS up to GF2MX(?) unlocks extended regs if VESA 0x4F07 is invoked,
    // so UNIVBE driver seems to work fine. on the other hand, GF4MX/FX BIOS locks extregs
    // before returning to the caller, and SDD is stuck with broken scrolling and "out of range"

    // so patch it out
    uint32_t nvlock_port_patch[] = {
        0x4C228, 0x4C31C, 0x4C34E, 0x4C374, 0x4C506, 0x4C8C6, 0x4C901, 0x4C934, 0x5CFCC, 0x5D0F6
    };
    uint32_t nvlock_data_patch[] = {
        0x4C22B, 0x4C31F, 0x4C351, 0x4C377, 0x4C509, 0x4C8C9, 0x4C904, 0x4C937, 0x5CFC8, 0x5D0F2
    };
    patch_word_list(f, nvlock_port_patch, arrayof(nvlock_port_patch), 0x03D4);
    patch_word_list(f, nvlock_data_patch, arrayof(nvlock_data_patch), 0x571F);

    // patch xtal frequency
    patch_dword(f, 0x1BA71, (pd->xtal_freq << 16) / 1000);

    // TODO: 
    return 0;   // success
}

int sdd67_uvconfig_patch(FILE *f, patchdata_t* pd) {
    if (pd->deviceid <= 0x20) {
        // riva 128/TNT don't need to be patched
        printf("patch not required, skip\n");
        return 1;
    }

    // 0x6B2EE: tnt2 device id (word)
    patch_word(f, 0x6B2EE, pd->deviceid);

    if ((pd->vendorid < 0x80) && (read_byte(f, 0x2999A) != 0x66)) {
        // short vendor ID
        // 0x2807F: tnt2 device id (BYTE)
        patch_byte(f, 0x2807F, pd->vendorid & 0xFF);
    } else {
        // long vendor ID - do beeeeg patch (disables TNT support iirc)
        // 0x29998: 74 -> EB
        // 0x2999A: C4 5E 06 26 C7 47 02 03 00 -> 66 68 <device id dword> EB 1A 90
        // 0x299B9: 66 6A <device id lobyte> -> EB DF 90
        uint8_t patch1[] = {0x66, 0x68, 0x00, 0x00, 0x00, 0x00, 0xEB, 0x1A, 0x90};
        uint8_t patch2[] = {0xEB, 0xDF, 0x90};
        patch1[2] = (pd->deviceid >> 0) & 0xFF;
        patch1[3] = (pd->deviceid >> 8) & 0xFF;

        patch_byte(f, 0x29998, 0xEB);
        patch_bytes(f, 0x2999A, patch1, sizeof(patch1));
        patch_bytes(f, 0x299B9, patch2, sizeof(patch2));
    }

    // 0x2CAA2: 7C -> EB : skip VESA info string check
    patch_byte(f, 0x2CAA2, 0xEB);

    // patch xtal frequency
    patch_dword(f, 0x29BA0, (pd->xtal_freq << 16) / 1000);

    return 0;   // success
}

int sdd653_config_patch(FILE *f, patchdata_t* pd) {
    // 0x314F7: riva128 device id (dword)
    patch_dword(f, 0x314F7, pd->deviceid);
    // 0x314FC: riva128 vendor id (dword)
    patch_dword(f, 0x314FC, pd->vendorid);
    // 0x9FDB4: riva128 vendor id (word)
    patch_word(f, 0x9FDB4, pd->vendorid);
    // 0x9FDB6: riva128 device id (word)
    patch_word(f, 0x9FDB6, pd->deviceid);

    // 0x33D4E: 7C -> EB : skip VESA info string check
    patch_byte(f, 0x33D4E, 0xEB);

    // patch xtal frequency
    patch_dword(f, 0x2673F, (pd->xtal_freq << 16) / 1000);

    return 0;
}

int nv4vdd_patch(FILE *f, patchdata_t* pd) {
    // TODO: try to find where and why fonts are corrupted and fix it
    // TODO2: still does not work (memory size mismatch?)

    // vendor/device ID patch
    if (pd->deviceid < 0x30) {
        // all TNT/TNT2 are supported out of the box, incl. Vanta/M64, skip patching
        printf("patch not required, skip\n");
        return 1;
    }
    
    // 0x37CFB: tnt device id (dword)
    patch_word(f, 0x37CFB, pd->deviceid);

    // patch xtal frequency
    if (pd->xtal_freq == 27000) {
        patch_dword(f, 0x13F19, pd->xtal_freq);
        patch_dword(f, 0x2E95D, pd->xtal_freq);
    }
    return 0;
}

filescan_t filescan[] = {
    // SDD 6.53 UVCONFIG.EXE
    {"UVCONFIG.EXE", 451936, 0x31EA5, 0x54696353, sdd653_uvconfig_patch, "SDD 6.53 UVCONFIG.EXE"},
    // SDD 6.53 CONFIG.EXE
    {"CONFIG.EXE",   668095, 0x9AB8D, 0x54696353, sdd653_config_patch, "SDD 6.53 CONFIG.EXE"},
    // SDD 6.7  UVCONFIG.EXE
    {"UVCONFIG.EXE", 457724, 0x3AF2F, 0x54696353, sdd67_uvconfig_patch, "SDD 6.7  UVCONFIG.EXE"},
    // Win3.x driver NV4VDD.VXD
    {"NV4VDD.386",   241271, 0x2A140, 0x45D8908B, nv4vdd_patch, "Win3.x driver NV4VDD.386"},      
    {0}
};


int main() {
    printf("NVidia DOS/Win3.x drivers patch utility - wbcbz7 24.o3.2o23\n");
    
    // TODO: command line arguments

    // find graphics devices..
    pciDeviceList pcilist;
    pciClass scanClass = {-1, 0, 0, 3}; // VGA device
    if (tinypci::enumerateByClass(&pcilist, 1, scanClass) == 0) {
        printf("error: unable to enumerate PCI devices!\n");
        return 1;
    }
    if (pcilist.vendorId != 0x10DE) {
        printf("error: no NVidia display controllers available!\n");
        return 1;
    }
    
    // print info:
    printf("PCI device: %02X:%02X.%01X = %04X:%04X\n", 
        pcilist.address.bus, pcilist.address.device, pcilist.address.function,
        pcilist.vendorId, pcilist.deviceId);
    
    patchdata_t patchdata = {&pcilist, pcilist.vendorId, pcilist.deviceId, getXtalFreq() };
    printf("XTAL frequency: %d.%03d MHz\n", patchdata.xtal_freq / 1000, patchdata.xtal_freq % 1000);
    printf("--------------------------\n");

    // scan all files
    FILE *f = NULL;
    filescan_t *fscan = filescan;
    uint32_t total_patched = 0; 
    for (; fscan->desc != 0; fscan++, total_patched++) {
        printf("patch %s...", fscan->desc); fflush(stdout);

        f = fopen(fscan->filename, "r+b");
        if (f == NULL) {
            printf("not found\n", fscan->filename);
            continue;
        }

        // check file size
        fseek(f, 0, SEEK_END);
        size_t fsize = ftell(f);
        if (fsize != fscan->fsize) {
            printf("size mismatch (%d != %d), skip\n", fsize, fscan->fsize);
            fclose(f);
            continue;
        }

        // check signature
        if (read_dword(f, fscan->match_ofs) != fscan->match_sig) {
            printf("signature mismatch, skip\n");
            fclose(f);
            continue;
        }
        
        // patch it!
        if ((fscan->patchfunc != NULL) && (fscan->patchfunc(f, &patchdata)) == 0) {
            printf("ok\n");
        } 
        fclose(f);
    }

    if (total_patched == 0) {
        printf("nothing to patch\n");
    } else {
        printf("done\n");
    }
    return 0;
}
