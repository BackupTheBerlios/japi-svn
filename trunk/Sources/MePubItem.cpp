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

void MePubItem::GuessMediaType()
{
	if (FileNameMatches("*.css", mName))
		SetMediaType("text/css");
	else if (FileNameMatches("*.xml;*.html", mName))
		SetMediaType("application/xhtml+xml");
	else if (FileNameMatches("*.ncx", mName))
		SetMediaType("application/x-dtbncx+xml");
	else if (FileNameMatches("*.ttf", mName))
		SetMediaType("application/x-font-ttf");
	else if (FileNameMatches("*.xpgt", mName))
		SetMediaType("application/vnd.adobe-page-template+xml");
	else if (FileNameMatches("*.png", mName))
		SetMediaType("image/png");
	else if (FileNameMatches("*.jpg", mName))
		SetMediaType("image/jpeg");
	else 
		SetMediaType("application/xhtml+xml");
}
