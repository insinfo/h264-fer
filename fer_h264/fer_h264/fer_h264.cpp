// fer_h264.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "nal.h"
#include "fileIO.h"
#include "rbsp_decoding.h"
#include "rbsp_IO.h"
#include "h264_globals.h"
#include "residual_tables.h"
#include "ref_frames.h"
#include "expgolomb.h"
#include "rbsp_encoding.h"
#include "moestimation.h"
#include "openCL_functions.h"

void decode()
{
	stream=fopen("big_buck_bunny.264","rb");
	yuvoutput = fopen("Bourne.y4m","wb");

	generate_residual_level_tables();
	InitNAL();

	NALunit nu;
	nu.rbsp_byte = new unsigned char[500000];

	unsigned long int ptr=0;
	while(1)
	{
		getNAL(&ptr, nu);

		if (nu.NumBytesInRBSP==0)
		{
			break;
		}

		RBSP_decode(nu);
	}		

	CloseNAL();
	fclose(stream);
	fclose(yuvoutput);
}

void encode()
{
	stream = fopen("big_buck_bunny.264", "wb");
	yuvinput = fopen("big_buck_bunny.y4m", "rb");
	yuvoutput = fopen("reference.yuv","wb");

	generate_residual_level_tables();
	init_expgolomb_UC_codes();
	InitNAL();
	InitCL();

	frameCount = 0;
	NALunit nu;
	nu.rbsp_byte = new unsigned char[500000];

	nu.forbidden_zero_bit = 0;

	LoadY4MHeader();

	// write sequence parameter set:
	nu.nal_ref_idc = 1;			// non-zero for sps
	nu.nal_unit_type = NAL_UNIT_TYPE_SPS;
	RBSP_encode(nu);
	writeNAL(nu);

	// write picture paramater set:
	nu.nal_ref_idc = 1;			// non-zero for pps
	nu.nal_unit_type = NAL_UNIT_TYPE_PPS;
	RBSP_encode(nu);
	writeNAL(nu);

	nu.nal_ref_idc = 1;		// non-zero for reference
	InitializeInterpolatedRefFrame();
	while (ReadFromY4M() != -1)
	{		
		frameCount++;
		if (frameCount < 200) continue;

		printf("Frame #%d\n", frameCount);
		//writeToYUV();

		nu.nal_unit_type = selectNALUnitType();
		RBSP_encode(nu);

		writeNAL(nu);
		FillInterpolatedRefFrame();

		if (frameCount == 203) break;
	}

	CloseCL();
	CloseNAL();
	fclose(stream);
	fclose(yuvinput);
	fclose(yuvoutput);
}

int _tmain(int argc, _TCHAR* argv[])
{
	//decode();
	encode();

	return 0;
}

