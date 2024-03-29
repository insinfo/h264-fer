#include "headers_and_parameter_sets.h"
#include "mocomp.h"
#include "ref_frames.h"
#include "mode_pred.h"
#include "h264_math.h"

int L_Temp_4x4_refPart[9][9];
int C_Temp_4x4_refPart[2][3][3];
int PR_WIDTH = 9;

void FillTemp_4x4_refPart(frame_type *ref, int org_Lx, int org_Ly, int org_Cx, int org_Cy) {
	int x,y,sx,sy;
	for(y=0; y<9; ++y) {
		sy=org_Ly+y;
		if(sy<0) sy=0;
		if(sy >= frame.Lheight) sy=frame.Lheight-1;
		for(x=0; x<9; ++x) {
			sx=org_Lx+x;
			if (sx < 0) sx = 0;
			if (sx >= frame.Lwidth) sx = frame.Lwidth-1;
			L_Temp_4x4_refPart[y][x]=ref->L[sy*frame.Lwidth+sx];
		}
	}
	for(y=0; y<3; ++y) {
		sy=org_Cy+y;
		if(sy<0) sy=0;
		if(sy>=frame.Cheight) sy=frame.Cheight-1;
		for(x=0; x<3; ++x) {
			sx=org_Cx+x;
			if (sx < 0) sx = 0;
			if (sx >= frame.Cwidth) sx = frame.Cwidth-1;
			C_Temp_4x4_refPart[0][y][x] = ref->C[0][sy*frame.Cwidth+sx];
			C_Temp_4x4_refPart[1][y][x] = ref->C[1][sy*frame.Cwidth+sx];
		}
	}
}


#define Tap6Filter(E,F,G,H,I,J) Border(((E)-5*(F)+20*(G)+20*(H)-5*(I)+(J)+16)>>5)
#define Border(i) (((i)>0)?(((i)<255)?(i):255):0)

//int Border(int i) {
//  if(i<0) return 0; else if(i>255) return 255; else return i;
//}

#define iffrac(x,y) if(frac==y*4+x)
#define Middle(a,b) (((a)+(b)+1)>>1)
#define p(x,y) data[(y)*PR_WIDTH+(x)]

inline int L_MC_frac_interpol(int *data, int frac) {
  int b,cc,dd,ee,ff,h,j,m,s;
  iffrac(0,0) return p(0,0);
  b=Tap6Filter(p(-2,0),p(-1,0),p(0,0),p(1,0),p(2,0),p(3,0));
  iffrac(1,0) return Middle(p(0,0),b);
  iffrac(2,0) return b;
  iffrac(3,0) return Middle(b,p(1,0));
  h=Tap6Filter(p(0,-2),p(0,-1),p(0,0),p(0,1),p(0,2),p(0,3));
  iffrac(0,1) return Middle(p(0,0),h);
  iffrac(0,2) return h;
  iffrac(0,3) return Middle(h,p(0,1));
  iffrac(1,1) return Middle(b,h);
  m=Tap6Filter(p(1,-2),p(1,-1),p(1,0),p(1,1),p(1,2),p(1,3));
  iffrac(3,1) return Middle(b,m);
  s=Tap6Filter(p(-2,1),p(-1,1),p(0,1),p(1,1),p(2,1),p(3,1));
  iffrac(1,3) return Middle(h,s);
  iffrac(3,3) return Middle(s,m);
  cc=Tap6Filter(p(-2,-2),p(-2,-1),p(-2,0),p(-2,1),p(-2,2),p(-2,3));
  dd=Tap6Filter(p(-1,-2),p(-1,-1),p(-1,0),p(-1,1),p(-1,2),p(-1,3));
  ee=Tap6Filter(p(2,-2),p(2,-1),p(2,0),p(2,1),p(2,2),p(2,3));
  ff=Tap6Filter(p(3,-2),p(3,-1),p(3,0),p(3,1),p(3,2),p(3,3));
  j=Tap6Filter(cc,dd,h,m,ee,ff);
  iffrac(2,2) return j;
  iffrac(2,1) return Middle(b,j);
  iffrac(1,2) return Middle(h,j);
  iffrac(2,3) return Middle(j,s);
  iffrac(3,2) return Middle(j,m);
  return 128;  // some error
}

void FillInterpolSubMBPart(int predL[16][16][16], int sx, int sy, int *data) {
  int b,cc,dd,ee,ff,h,j,m,s;
  predL[0][sy][sx] = p(0,0);
  b=Tap6Filter(p(-2,0),p(-1,0),p(0,0),p(1,0),p(2,0),p(3,0));
  predL[1][sy][sx] = Middle(p(0,0),b);
  predL[2][sy][sx] =  b;
  predL[3][sy][sx] =  Middle(b,p(1,0));
  h=Tap6Filter(p(0,-2),p(0,-1),p(0,0),p(0,1),p(0,2),p(0,3));
  predL[4][sy][sx] =  Middle(p(0,0),h);
  predL[8][sy][sx] =  h;
  predL[12][sy][sx] =  Middle(h,p(0,1));
  predL[5][sy][sx] =  Middle(b,h);
  m=Tap6Filter(p(1,-2),p(1,-1),p(1,0),p(1,1),p(1,2),p(1,3));
  predL[7][sy][sx] = Middle(b,m);
  s=Tap6Filter(p(-2,1),p(-1,1),p(0,1),p(1,1),p(2,1),p(3,1));
  predL[13][sy][sx] = Middle(h,s);
  predL[15][sy][sx] = Middle(s,m);
  cc=Tap6Filter(p(-2,-2),p(-2,-1),p(-2,0),p(-2,1),p(-2,2),p(-2,3));
  dd=Tap6Filter(p(-1,-2),p(-1,-1),p(-1,0),p(-1,1),p(-1,2),p(-1,3));
  ee=Tap6Filter(p(2,-2),p(2,-1),p(2,0),p(2,1),p(2,2),p(2,3));
  ff=Tap6Filter(p(3,-2),p(3,-1),p(3,0),p(3,1),p(3,2),p(3,3));
  j=Tap6Filter(cc,dd,h,m,ee,ff);
  predL[10][sy][sx] = j;
  predL[6][sy][sx] = Middle(b,j);
  predL[9][sy][sx] = Middle(h,j);
  predL[14][sy][sx] = Middle(j,s);
  predL[11][sy][sx] = Middle(j,m);
}

void FillInterpolatedMB(int predL[16][16][16], int predCr[16][8][8], int predCb[16][8][8], frame_type *refPic,
                        int mbPartIdx,
                        int subMbIdx, 
						int subMbPartIdx) {
	int x,y, org_x,org_y; // org_x and org_y are origin point of 4x4 submbpart in current macroblock
	org_y = ((subMbIdx & 2)<<2) + ((subMbPartIdx & 2)<<1);
	org_x = ((subMbIdx & 1)<<3) + ((subMbPartIdx & 1)<<2);
	int xAl = ((mbPartIdx%PicWidthInMbs)<<4) + org_x;
	int yAl = ((mbPartIdx/PicWidthInMbs)<<4) + org_y;
	// Fills temp tables used in fractional interpolation (luma) and linear interpolation (chroma).
	FillTemp_4x4_refPart(refPic, xAl - 2, yAl - 2, xAl/2, yAl/2);

	for(y=0; y<4; ++y)
		for(x=0; x<4; ++x)
		{
			FillInterpolSubMBPart(predL, org_x+x, org_y+y, &(L_Temp_4x4_refPart[y+2][x+2]));
		}

	org_x/=2; org_y/=2; // Chroma resolution is halved luma resolution.
    
	for (int frac = 0; frac < 16; frac++)
	{
		int xLinear = frac&3;
		int yLinear = (frac&0x0c) >> 2;
		// CB component - iCbCr=0
		for(y=0; y<2; ++y)
			for(x=0; x<2; ++x)
				predCb[frac][org_y+y][org_x+x] = //0;
				  ((8-xLinear)*(8-yLinear) * C_Temp_4x4_refPart[0][y][x]  +
					  xLinear *(8-yLinear) * C_Temp_4x4_refPart[0][y][x+1]+
				   (8-xLinear)*   yLinear  * C_Temp_4x4_refPart[0][y+1][x]  +
					  xLinear *   yLinear  * C_Temp_4x4_refPart[0][y+1][x+1] + 32) >> 6; // Linear interpolation and rounding.
		// CR component - iCbCr=1
		for(y=0; y<2; ++y)
			for(x=0; x<2; ++x)
				predCr[frac][org_y+y][org_x+x] = //0;
				  ((8-xLinear)*(8-yLinear) * C_Temp_4x4_refPart[1][y][x]  +
					  xLinear *(8-yLinear) * C_Temp_4x4_refPart[1][y][x+1]+
				   (8-xLinear)*   yLinear  * C_Temp_4x4_refPart[1][y+1][x]  +
					  xLinear *   yLinear  * C_Temp_4x4_refPart[1][y+1][x+1] + 32) >> 6; // Linear interpolation and rounding.
	}
}

void MotionCompensateSubMBPart(int predL[16][16], int predCr[8][8], int predCb[8][8], frame_type *refPic,
                        int mbPartIdx,
                        int subMbIdx, 
						int subMbPartIdx) {
	int x,y, org_x,org_y; // org_x and org_y are origin point of 4x4 submbpart in current macroblock
	int mvx, mvy;
	org_y = ((subMbIdx & 2)<<2) + ((subMbPartIdx & 2)<<1);
	org_x = ((subMbIdx & 1)<<3) + ((subMbPartIdx & 1)<<2);
	mvx = mvL0x[mbPartIdx][subMbIdx][subMbPartIdx];
	mvy = mvL0y[mbPartIdx][subMbIdx][subMbPartIdx];
	int xAl = ((mbPartIdx%PicWidthInMbs)<<4) + org_x;
	int yAl = ((mbPartIdx/PicWidthInMbs)<<4) + org_y;
	// Fills temp tables used in fractional interpolation (luma) and linear interpolation (chroma).
	FillTemp_4x4_refPart(refPic, xAl + (mvL0x[mbPartIdx][subMbIdx][subMbPartIdx]>>2) - 2, yAl + (mvL0y[mbPartIdx][subMbIdx][subMbPartIdx]>>2) - 2, xAl/2 + (mvL0x[mbPartIdx][subMbIdx][subMbPartIdx]>>3), yAl/2 + (mvL0y[mbPartIdx][subMbIdx][subMbPartIdx]>>3));

	int frac=(mvL0y[mbPartIdx][subMbIdx][subMbPartIdx]&3)*4+(mvL0x[mbPartIdx][subMbIdx][subMbPartIdx]&3);
	for(y=0; y<4; ++y)
		for(x=0; x<4; ++x)
		{
			predL[org_y+y][org_x+x] = L_MC_frac_interpol(&(L_Temp_4x4_refPart[y+2][x+2]), frac);
		}

	org_x/=2; org_y/=2; // Chroma resolution is halved luma resolution.
    
	int xLinear = mvL0x[mbPartIdx][subMbIdx][subMbPartIdx]&7;
	int yLinear = mvL0y[mbPartIdx][subMbIdx][subMbPartIdx]&7;
	// CB component - iCbCr=0
	for(y=0; y<2; ++y)
		for(x=0; x<2; ++x)
			predCb[org_y+y][org_x+x] = //0;
			  ((8-xLinear)*(8-yLinear) * C_Temp_4x4_refPart[0][y][x]  +
				  xLinear *(8-yLinear) * C_Temp_4x4_refPart[0][y][x+1]+
			   (8-xLinear)*   yLinear  * C_Temp_4x4_refPart[0][y+1][x]  +
				  xLinear *   yLinear  * C_Temp_4x4_refPart[0][y+1][x+1] + 32) >> 6; // Linear interpolation and rounding.

	// CR component - iCbCr=1
	for(y=0; y<2; ++y)
		for(x=0; x<2; ++x)
			predCr[org_y+y][org_x+x] = //0;
			  ((8-xLinear)*(8-yLinear) * C_Temp_4x4_refPart[1][y][x]  +
				  xLinear *(8-yLinear) * C_Temp_4x4_refPart[1][y][x+1]+
			   (8-xLinear)*   yLinear  * C_Temp_4x4_refPart[1][y+1][x]  +
				  xLinear *   yLinear  * C_Temp_4x4_refPart[1][y+1][x+1] + 32) >> 6; // Linear interpolation and rounding.
}

// In baseline profile there is only refPicL0 used. There's no weighted prediction.
// In global structure "MB_pred_info * infos" are placed all information needed for
// decode process (motion compensation).
void Decode(int predL[16][16], int predCr[8][8], int predCb[8][8])
{
	// Smallest granularity for macroblock is 8x8 subpart (and in that case partition to 4x4 can be used).
	// After DeriveMVs called, all MV are prepared (for every subMB and every subMBPart - granularity to part 4x4 sized).
	frame_type * refPic = RefPicList0[*(refIdxL0+CurrMbAddr)].frame;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			MotionCompensateSubMBPart(predL, predCr, predCb, refPic, CurrMbAddr, i, j);
}
