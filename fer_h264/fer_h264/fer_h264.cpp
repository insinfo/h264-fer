// fer_h264.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "nal.h"
#include "fileIO.h"
#include "rbsp_decoding.h"
#include "rawreader.h"
#include "h264_globals.h"

void decode()
{
	FILE *stream;
	stream=fopen("Bourne.264","rb");
	yuvoutput = fopen("Bourne.yuv","wb");

	unsigned long int ptr=0;
	while(1)
	{
		NALunit nu = getNAL(stream, &ptr);

		if (nu.NumBytesInRBSP==0)
		{
			break;
		}

		RBSP_decode(nu);
	}		

	fclose(stream);
	fclose(yuvoutput);
}

void encode()
{
	FILE *stream;
	stream = fopen("big_buck_bunny.264", "wb");
	yuvinput = fopen("big_buck_bunny.y4m", "rb");
}


int _tmain(int argc, _TCHAR* argv[])
{
	decode();	

	return 0;
}
