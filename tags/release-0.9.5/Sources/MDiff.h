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

/*	$Id: PDiff.h,v 1.2 2003/12/18 13:46:36 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten L. Hekkelman
	
	Created: 03/29/98 15:21:00
*/

#ifndef MDIFF_H
#define MDIFF_H

#include <vector>

struct MDiffInfo
{
	uint32	mA1;
	uint32	mA2;
	uint32	mB1;
	uint32	mB2;
};

typedef std::vector<MDiffInfo>	MDiffScript;

class MDiff
{
  public:
					MDiff(std::vector<uint32>& vecA, std::vector<uint32>& vecB);
					~MDiff();
		
		uint32		Report(MDiffScript& outList);

  private:
		int32		MiddleSnake(int32 x, int32 y, int32 u, int32 v, int32& px, int32& py);
		void		Seq(int32 x, int32 y, int32 u, int32 v);
		
		uint32		mM, mN;
		int32*		mVX;
		int32*		mVY;
		int32*		mFD;
		int32*		mBD;
		int32*		mD;
		std::vector<bool> mCX, mCY;
};

#endif
