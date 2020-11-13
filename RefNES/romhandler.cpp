
#include "common.h"
#include <stdio.h>
#include "CRCLookups.h"

unsigned char prg_count;
unsigned char chrsize;
unsigned char ines_flags6;
unsigned char singlescreen;
unsigned char ines_flags10;
unsigned char iNESMapper;
bool fourScreen = false;
unsigned int SRAMSize = 0;
unsigned int CHRRAMSize = 0;
unsigned int PRGRAMSize = 0;
Mapper* mapper;
unsigned char* ROMCart;

void CleanUpROMMem()
{
    if (ROMCart != NULL)
    {
        free(ROMCart);
    }
}

void LoadRomToMemory(FILE * RomFile, long lSize)
{
    if (ROMCart == NULL)
    {
        ROMCart = (unsigned char*)malloc(lSize);
    }
    else
    {
        ROMCart = (unsigned char*)realloc(ROMCart, lSize);
    }

    fread(ROMCart, 1, lSize, RomFile);
    //CopyRomToMemory();
}

void AssignMapper()
{

    if (mapper != NULL)
    {
        free(mapper);
    }
    
    CPU_LOG("Assigning mapper %d\n", iNESMapper);
    switch (iNESMapper)
    {
        case 0:
            mapper = new NROM(SRAMSize, PRGRAMSize, CHRRAMSize);
            break;
        case 1:
            mapper = new MMC1(SRAMSize, PRGRAMSize, CHRRAMSize);
            break;
        case 2:
            mapper = new UXROM(SRAMSize, PRGRAMSize, CHRRAMSize);
            break;
        case 3:
            mapper = new CNROM(SRAMSize, PRGRAMSize, CHRRAMSize);
            break;
        case 4:
            mapper = new MMC3(SRAMSize, PRGRAMSize, CHRRAMSize);
            break;
        case 7:
            mapper = new AXROM(SRAMSize, PRGRAMSize, CHRRAMSize);
            break;
        case 9:
            mapper = new MMC2(SRAMSize, PRGRAMSize, CHRRAMSize);
            break;
        case 10:
            mapper = new MMC4(SRAMSize, PRGRAMSize, CHRRAMSize);
            break;
        default:
            mapper = new NROM(SRAMSize, PRGRAMSize, CHRRAMSize);
            break;
    }
}

int LoadRom(const char *Filename) {
    FILE * pFile;  //Create File Pointer
    long lSize;   //Need a variable to tell us the size
    unsigned char ROMHeader[16];
    unsigned int inesIdent;

    fopen_s(&pFile, Filename, "rb"); //Open the file, args r = read, b = binary

    if (pFile != NULL)  //If the file exists
    {
        //Reset();
        fseek(pFile, 0, SEEK_END); //point it at the end
        lSize = ftell(pFile) - 16; //Save the filesize
        rewind(pFile); //Start the file at the beginning again
        fread(&ROMHeader, 1, 16, pFile); //Read in the file
        inesIdent = (ROMHeader[0] + (ROMHeader[1] << 8) + (ROMHeader[2] << 16) + (ROMHeader[3] << 24));


        if (inesIdent != 0x1A53454E)
        {
            CPU_LOG("Header not on ROM file. MagicNumber = %x\n", inesIdent);
            fclose(pFile); //Close the file
        }
        else
        {
            prg_count = ROMHeader[4];
            chrsize = ROMHeader[5];
            ines_flags6 = ROMHeader[6];
            singlescreen = 0;
            ines_flags10 = ROMHeader[10];
            iNESMapper = ((ROMHeader[6] >> 4) & 0xF) | (ROMHeader[7] & 0xF0);
            fourScreen = (ROMHeader[6] >> 3) & 0x1;
            //memset(CartridgeSRAM, 0, sizeof(CartridgeSRAM));
            CPU_LOG("prgsize=%d, chrsize=%d, flags6=%x, flags8=%x, flags10=%x mapper=%d\n", prg_count, chrsize, ines_flags6, ROMHeader[8],  ines_flags10, iNESMapper);
            fseek(pFile, 16, SEEK_SET); //Point it past the header
            LoadRomToMemory(pFile, lSize);
            //fread(CPUMemory, 1, lSize, pFile); //Read in the file
            fclose(pFile); //Close the file
        }
        LookUpCRC();
        AssignMapper();

        return 0;
    }
    else
    {
        //User cancelled, either way, do nothing.
        return 1;
    }

}