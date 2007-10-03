/*
	Copyright (c) 2006, Maarten L. Hekkelman
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
