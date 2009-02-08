//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include "MFile.h"
#include "MePubItem.h"

// ---------------------------------------------------------------------------
//	MePubItem::MePubItem

MePubItem::MePubItem(
	const std::string&	inName,
	MProjectGroup*		inParent)
	: MProjectItem(inName, inParent)
	, mIsOutOfDate(false)
{
}

// ---------------------------------------------------------------------------
//	MePubItem::SetOutOfDate

void MePubItem::SetOutOfDate(
	bool			inIsOutOfDate)
{
	if (mIsOutOfDate != inIsOutOfDate)
	{
		mIsOutOfDate = inIsOutOfDate;
		eStatusChanged(this);
		
		MProjectItem* parent = GetParent();
		while (parent != nil)
		{
			parent->eStatusChanged(parent);
			parent = parent->GetParent();
		}
	}
}

