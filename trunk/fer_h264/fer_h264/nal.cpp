#include "nal.h"

unsigned char NALbytes[500000];
FILE *stream;

// Returns the address of the subsequent NAL unit in the 
// input file.
// Returns 0 if end of stream found.
// fPtr - the current position in the input file
unsigned long findNALstart(unsigned long fPtr)
{
	unsigned char buffer[BUFFER_SIZE];		// the buffer for the bytes read
	unsigned int bytesRead;
	bool startPrefixFound;

	fseek(stream, fPtr, SEEK_SET);

	// read BUFFER_SIZE bytes from the file:
	while (bytesRead = fread(buffer, 1, BUFFER_SIZE, stream))
	{
		startPrefixFound = false;
		for (unsigned int i = 0; i < bytesRead - 3; i++)
		{
			if (buffer[i] == 0)
			{
				if (buffer[i+1] == 0)
				{
					if (buffer[i+2] == 0)
					{
						if (buffer[i+3] == 1)
						{
							// found start code prefix
							startPrefixFound = true;
							// set the file pointer to after the prefix:
							fPtr += i + 4;
							fseek(stream, fPtr, SEEK_SET);
							break;
						}
					}
				}
			}
		}

		if (startPrefixFound) break;

		// start code not found in this
		// access, set the new fPtr position:
		fPtr += BUFFER_SIZE - 3;
		fseek(stream, fPtr, SEEK_SET);
	}

	if (bytesRead == 0)
	{
		return 0;
	}

	return fPtr;
}

// Returns the address of the position after the
// last byte of this NAL (the address of a
// subsequent NAL start code)
// fPtr - the current position in the input file
unsigned long findNALend(unsigned long fPtr)
{
	unsigned char buffer[BUFFER_SIZE];		// the buffer for the bytes read
	unsigned int bytesRead;
	bool startPrefixFound;

	fseek(stream, fPtr, SEEK_SET);

	// read BUFFER_SIZE bytes from the file:
	while (bytesRead = fread(buffer, 1, BUFFER_SIZE, stream))
	{
		startPrefixFound = false;
		for (unsigned int i = 0; i < bytesRead - 2; i++)
		{
			if (buffer[i] == 0)
			{
				if (buffer[i+1] == 0)
				{
					if ((buffer[i+2] == 0) ||
						(buffer[i+2] == 1))
					{
						// found start code prefix
						startPrefixFound = true;
						// set the file pointer to before the prefix:
						fPtr += i;
						fseek(stream, fPtr, SEEK_SET);
						break;
					}
				}
			}
		}

		if (startPrefixFound || bytesRead < BUFFER_SIZE) break;

		// Start code not found in this
		// access, set the new fPtr position.
		// -2 prevents missing the start code
		// if it's on the border between two
		// file accesses
		fPtr += BUFFER_SIZE - 2;
		fseek(stream, fPtr, SEEK_SET);
	}

	if (startPrefixFound == false)
	{
		fPtr += bytesRead;
	}

	return fPtr;
}

// This function implements quite literally the
// NAL unit syntax found in chapter 7.3.1 of
// the H.264 specification.
// *NALbytes: an array of bytes representing
// one NAL unit read from the input stream.
void parseNAL(unsigned int NumBytesInNALunit, NALunit &nal_unit)
{
	nal_unit.forbidden_zero_bit = (bool) (NALbytes[0] >> 7);
	nal_unit.nal_ref_idc = (NALbytes[0] & 0x7f) >> 5;
	nal_unit.nal_unit_type = NALbytes[0] & 0x1f;

	nal_unit.NumBytesInRBSP = 0;
	unsigned int nalUnitHeaderBytes = 1;

	if ((nal_unit.nal_unit_type == 14) ||
		(nal_unit.nal_unit_type == 20))
	{
		perror("NAL unit types 14 and 20 not supported yet.");
		system("pause");
		exit(1);
	}

	if (nal_unit.rbsp_byte == NULL)
	{
		perror("Error allocating memory for RBSP.");
		system("pause");
		exit(1);
	}
	for (unsigned int i = nalUnitHeaderBytes; i < NumBytesInNALunit; i++)
	{
		unsigned int next_bits24 = (unsigned int)NALbytes[i]   << 16 |
								   (unsigned int)NALbytes[i+1] <<  8 |
								   (unsigned int)NALbytes[i+2];
		if ((i+2 < NumBytesInNALunit)  &&
			(next_bits24 == 0x03))
		{
			nal_unit.rbsp_byte[nal_unit.NumBytesInRBSP++] = NALbytes[i];
			nal_unit.rbsp_byte[nal_unit.NumBytesInRBSP++] = NALbytes[i+1];
			i += 2;
			// skip the emulation prevention byte
		}
		else
		{
			nal_unit.rbsp_byte[nal_unit.NumBytesInRBSP++] = NALbytes[i];
		}
	}
}

void getNAL(unsigned long *fPtr, NALunit &nu)
{
	unsigned long startPtr = findNALstart(*fPtr);

	if (startPtr == 0)
	{
		printf("\n\n====================================================\n");
		printf("End of stream found.\n");
		system("pause");
		exit(0);	// the 0 error code indicates that this is not an error
	}

	unsigned long endPtr = findNALend(startPtr);
	unsigned int NumBytesInNALunit = endPtr - startPtr;

	printf("\n\n====================================================\n");
	printf("NAL start found at %d (0x%x)\n", startPtr, startPtr);
	printf("NAL size = %d (0x%x)\n\n", NumBytesInNALunit, NumBytesInNALunit);

	fseek(stream, startPtr, SEEK_SET);
	if (fread(NALbytes, 1, NumBytesInNALunit, stream) != NumBytesInNALunit)
	{
		perror("Error reading NAL unit from file.\n");
		system("pause");
		exit(1);
	}

	*fPtr = endPtr;

	parseNAL(NumBytesInNALunit, nu);
}

// ENCODING:

void writeNAL(unsigned long *fPtr, NALunit nu, unsigned int numBytesInRBSP)
{
	unsigned int pos = 0;
	// start code prefix:
	NALbytes[pos++] = 0;
	NALbytes[pos++] = 0;
	NALbytes[pos++] = 0;
	NALbytes[pos++] = 1;

	NALbytes[pos++] = (nu.forbidden_zero_bit << 7) | (nu.nal_ref_idc << 5) | (nu.nal_unit_type & 31);

	int zeroCounter = 0;
	for(int i = 0; i < numBytesInRBSP; i++)
	{
		if (zeroCounter == 2)
		{
			NALbytes[pos++] = 3;	// insert emulation prevention byte
			zeroCounter = 0;
		}

		NALbytes[pos++] = nu.rbsp_byte[i];

		if (nu.rbsp_byte[i] == 0)
		{
			zeroCounter++;
		}
		else
		{
			zeroCounter = 0;
		}		
	}

	fwrite(NALbytes, 1, pos, stream);
}