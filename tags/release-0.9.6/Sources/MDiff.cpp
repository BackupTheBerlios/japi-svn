/*
	Copyright (c) 2007, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*	$Id: MDiff.cpp,v 1.7 2003/12/18 13:46:36 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 03/29/98 15:23:27
	
	The algorithm used in this module comes from:
	
	"An O(ND) Difference Algorithm and Its Variations" by Eugene W. Myers
	published in Algorithmica (1986) 1: 251-266
	
*/

#include "MJapi.h"

#include "MDiff.h"
#include "MError.h"

using namespace std;

MDiff::MDiff(vector<uint32>& vx, vector<uint32>& vy)
{
	mVX = mVY = mD = NULL;

	mN = vx.size();
	mM = vy.size();
	
	mVX = new int32[mN];
	copy(vx.begin(), vx.end(), mVX);
	
	mVY = new int32[mM];
	copy(vy.begin(), vy.end(), mVY);

	int32 diags = mN + mM + 3;
	mD = new int32[diags * 2];
	
	mFD = mD + mM + 1;
	mBD = mD + diags + mM + 1;

	mCX.insert(mCX.begin(), mN + 1, false);
	mCY.insert(mCY.begin(), mM + 1, false);
	
	Seq(0, 0, mN, mM);

	delete [] mVX;		mVX = nil;
	delete [] mVY;		mVY = nil;
	delete [] mD;		mD = nil;
}

MDiff::~MDiff()
{
}

uint32 MDiff::Report(MDiffScript& outList)
{
	uint32 ix = 0, iy = 0, r = 0;
	
	while (ix <= mN or iy <= mM)
	{
		if (mCX[ix] or mCY[iy])
		{
			uint32 lx = ix, ly = iy;
			
			while (mCX[ix] and ix < mN) ix++;
			while (mCY[iy] and iy < mM) iy++;
			
			MDiffInfo di = { lx, ix, ly, iy };
//			lst->AddItem(kIndexLast, &di, sizeof(DiffInfo));
			outList.push_back(di);

			++r;
		}
		
		++ix, ++iy;
	}
	
	return r;
}

int32 MDiff::MiddleSnake(int32 x, int32 y, int32 u, int32 v, int32& px, int32& py)
{
	int32 dmin = x - v;
	int32 dmax = u - y;
	int32 fmid = x - y;
	int32 bmid = u - v;
	int32 fmin = fmid, fmax = fmid;
	int32 bmin = bmid, bmax = bmid;
	bool odd = ((fmid - bmid) & 1) != 0;
	int32 c;

	mFD[fmid] = x;
	mBD[bmid] = u;
	
	for (c = 1;; ++c)
	{
		int32 d;
		
		fmin > dmin ? mFD[--fmin - 1] = -1 : ++fmin;
		fmax < dmax ? mFD[++fmax + 1] = -1 : --fmax;
		for (d = fmax; d >= fmin; d -= 2)
		{
			int32 nx, ny;
			
			if (mFD[d - 1] >= mFD[d + 1])
				nx = mFD[d - 1] + 1;
			else
				nx = mFD[d + 1];
			
			ny = nx - d;
			
			while (nx < u and ny < v and mVX[nx] == mVY[ny])
				++nx, ++ny;
			
			mFD[d] = nx;
			
			if (odd and bmin <= d and d <= bmax and mBD[d] <= nx)
			{
				px = nx;
				py = ny;
				return 2 * c - 1;
			}
		}
		
		bmin > dmin ? mBD[--bmin - 1] = INT_MAX : ++bmin;
		bmax < dmax ? mBD[++bmax + 1] = INT_MAX : --bmax;
		for (d = bmax; d >= bmin; d -= 2)
		{
			int32 nx, ny;
			
			if (mBD[d - 1] < mBD[d + 1])
				nx = mBD[d - 1];
			else
				nx = mBD[d + 1] - 1;
			
			ny = nx - d;
			
			while (nx > x and ny > y and mVX[nx - 1] == mVY[ny - 1])
				--nx, --ny;
			
			mBD[d] = nx;
			
			if (not odd and fmin <= d and d <= fmax and nx <= mFD[d])
			{
				px = nx;
				py = ny;
				return 2 * c;
			}
		}
	}
}

void MDiff::Seq(int32 x, int32 y, int32 u, int32 v)
{
	while (x < u and y < v and mVX[x] == mVY[y])
		x++, y++;
	while (u > x and v > y and mVX[u - 1] == mVY[v - 1])
		u--, v--;
	
	if (x == u)
		while (y < v)
			mCY[y++] = true;
	else if (y == v)
		while (x < u)
			mCX[x++] = true;
	else
	{
		int32 px, py;
		
		int32 c = MiddleSnake(x, y, u, v, px, py);
		
		if (c == 1)	// should never happen
			THROW(("Runtime error in diff"));
		else
		{
			Seq(x, y, px, py);
			Seq(px, py, u, v);
		}
	}
}

