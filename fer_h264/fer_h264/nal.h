#pragma once
#include "stdafx.h"

const unsigned int BUFFER_SIZE = 500000; // the number of bytes read from the file in one access

extern FILE *stream;

typedef struct
{
	bool forbidden_zero_bit;
	unsigned int nal_ref_idc, nal_unit_type, NumBytesInRBSP;
	unsigned char *rbsp_byte;
}
NALunit;

// Allocates the buffers. To be invoked only once upon
// program startup.
void InitNAL();
// Flushes the stream buffer to the output stream.
void FlushNAL();
// Frees allocated memory. Invoke once at program termination.
void CloseNAL();

// Returns the NAL unit data in a NALunit structure.
// fPtr - the current position in the input file
void getNAL(unsigned long *fPtr, NALunit &nu);

void writeNAL(NALunit nu);