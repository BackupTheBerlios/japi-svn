//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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
