
#include "common.h"
#include "cpu.h"
#include "memory.h"
#include <stdio.h>




int LoadRom(const char *Filename) {
	FILE * pFile;  //Create File Pointer
	long lSize;   //Need a variable to tell us the size
	unsigned char ROMHeader[16];
	unsigned int inesIdent;

	fopen_s(&pFile, Filename, "rb");     //Open the file, args r = read, b = binary

	if (pFile != NULL)  //If the file exists
	{
		//Reset();
		fseek(pFile, 0, SEEK_END); //point it at the end
		lSize = ftell(pFile) - 16; //Save the filesize
		rewind(pFile); //Start the file at the beginning again
		fread(&ROMHeader, 1, 16, pFile); //Read in the file
		inesIdent = (ROMHeader[0] + (ROMHeader[1] << 8) + (ROMHeader[2] << 16) + (ROMHeader[3] << 24));

		if (inesIdent != 0x53454E)
		{
			CPU_LOG("Header not on ROM file. MagicNumber = %x", inesIdent);
			fclose(pFile); //Close the file
		}
		else
		{
			CPU_LOG("Rom Spec Version %d.%d\n", (ROMHeader[5] >> 4) & 0xF, ROMHeader[5] & 0xF);
			//Check the spec isnt newer than what is currently implemented!
			if (ROMHeader[5] > 0x13)
			{
				return 2;
			}
			fseek(pFile, 16, SEEK_SET); //Point it past the header
			LoadRomToMemory(pFile, lSize);
			//fread(CPUMemory, 1, lSize, pFile); //Read in the file
			fclose(pFile); //Close the file
		}
		return 0;
	}
	else
	{
		//User cancelled, either way, do nothing.
		return 1;
	}

}