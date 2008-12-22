//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MSTYLES_H
#define MSTYLES_H

#include "MLanguage.h"
//#include "MP2PEvents.h"

enum {	// inline text input constants
	kActiveInputArea			= 0,	/* entire active input area */
	kCaretPosition				= 1,	/* specify caret position */
	kRawText 					= 2,	/* specify range of raw text */
	kSelectedRawText 			= 3,	/* specify range of selected raw text */
	kConvertedText				= 4,	/* specify range of converted text */
	kSelectedConvertedText		= 5,	/* specify range of selected converted text */
	kBlockFillText				= 6,	/* Block Fill hilite style */
	kOutlineText 				= 7,	/* Outline hilite style */
	kSelectedText				= 8		/* Selected hilite style */
};

#endif
