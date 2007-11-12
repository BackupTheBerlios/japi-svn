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

#include "MJapieG.h"

#include <boost/filesystem/fstream.hpp>

#if defined(__APPLE__) and defined(__MACH__)

#include <mach-o/loader.h>

#include "MUtils.h"
#include "MObjectFileImp_macho.h"

using namespace std;

template<class SWAPPER>
void MMachoObjectFileImp::Read(
	struct mach_header&	mh,
	istream&			inData)
{
	SWAPPER	swap;
	
	if (swap(mh.filetype) != MH_OBJECT)
		THROW(("File is not an object file"));
	
	for (uint32 segment = 0; segment < swap(mh.ncmds); ++segment)
	{
		struct load_command lc;
		inData.read((char*)&lc, sizeof(lc));
		
		switch (swap(lc.cmd))
		{
			case LC_SEGMENT:
			{
				struct segment_command sc;
				inData.read(sc.segname, sizeof(sc) - sizeof(lc));
				
				for (uint32 sn = 0; sn < swap(sc.nsects); ++sn)
				{
					struct section section;
					inData.read((char*)&section, sizeof(section));
					
					if (strcmp(section.sectname, SECT_TEXT) == 0)
						mTextSize += swap(section.size);
					else if (strcmp(section.sectname, SECT_DATA) == 0)
						mDataSize += swap(section.size);
				}
				break;
			}
			
			default:
				inData.seekg(swap(lc.cmdsize) - sizeof(lc), ios_base::cur);
		}
	}
}

void MMachoObjectFileImp::SetFile(
	const MPath&		inFile)
{
	mFile = inFile;
	
	mTextSize = 0;
	mDataSize = 0;
	
	fs::ifstream file(mFile, ios::binary);
	if (not file.is_open())
		THROW(("Could not open object file"));
	
	struct mach_header mh;
	file.read((char*)&mh, sizeof(mh));
	
	if (mh.magic == MH_CIGAM)
		Read<swapper>(mh, file);
	else if (mh.magic == MH_MAGIC)
		Read<no_swapper>(mh, file);
	else
		THROW(("File is not an object file"));
}

#endif
