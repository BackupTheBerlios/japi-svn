//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MLineInfo.h 160 2007-05-30 15:37:02Z maarten $
	Copyright Maarten L. Hekkelman
	Created Tuesday June 22 2004 20:19:39
*/

#ifndef LINEINFO_H
#define LINEINFO_H

struct MLineInfo
{
                    MLineInfo()
                    {
                    	start = state = nl = marked = indent = diff = stmt = brkp = 0;
                    };

                    MLineInfo(
                    	uint32		inStart,
                    	uint16		inState,
                    	bool		inNl = true)
                        : start(inStart)
                        , state(inState)
                        , dirty(true)
                        , nl(inNl)
                        , indent(false)
                        , marked(false)
                        , diff(false)
                        , stmt(false)
                        , brkp(false) {};

	uint32			start;
	uint16			state;
	bool			dirty	: 1;
	bool			nl		: 1;
	bool			indent	: 1;
	bool			marked	: 1;
	bool			diff	: 1;
	bool			stmt	: 1;		// can put a breakpoint here
	bool			brkp	: 1;
};

typedef std::vector<MLineInfo> MLineInfoArray;

#endif // LINEINFO_H
