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
#include <mach-o/stab.h>
#include <nlist.h>

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

void MMachoObjectFileImp::Read(
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

void WriteDataAligned(
	ofstream&	inStream,
	const void*	inData,
	uint32		inSize,
	uint32		inAlignment = 1)
{
	inStream.write(reinterpret_cast<const char*>(inData), inSize);

	inSize %= inAlignment;
	while (inSize-- > 0)
		inStream.put('\0');
}

void MMachoObjectFileImp::Write(
	const MPath&		inFile)
{
	fs::ofstream f(inFile, ios::binary | ios::trunc);
	if (not f.is_open())
		THROW(("Failed to open object file for writing"));

	string names;
	names.append("\0", 1);	// add the null name

	uint32
		kSegmentCmdSize = 
			sizeof(segment_command) +
			3 * sizeof(section),
		kSizeOfCmnds =
			kSegmentCmdSize +
			sizeof(symtab_command) +
			sizeof(dysymtab_command),
		kDataOffset =
			kSizeOfCmnds + sizeof(mach_header);
	
	struct mach_header mh = {
		MH_MAGIC,				// signature
		CPU_TYPE_POWERPC,		// cputype
		0,						// cpusubtype
		MH_OBJECT,				// filetype
		3,						// ncmds
		kSizeOfCmnds,			// sizeofcmds
		MH_SUBSECTIONS_VIA_SYMBOLS	// flags
	};
	
	WriteDataAligned(f, &mh, sizeof(mh));
	
	uint32 size = 0, alignment;
	
	alignment = 4;	// for now
	
	for (MGlobals::iterator g = mGlobals.begin(); g != mGlobals.end(); ++g)
	{
		(void)AddNameToNameTable(names, g->name.c_str());
		
		uint32 gs = g->size;
		if (g + 1 != mGlobals.end() and (gs % alignment) != 0)
			gs = ((gs / alignment) + 1) * alignment;
		size += gs;
	}
	
	struct segment_command sc = {
		LC_SEGMENT,				// cmd
		kSegmentCmdSize,		// cmdsize
		"",						// segname
		0,						// vmaddr
		size,					// vmsize
		kDataOffset,			// fileoff
		size,					// filesize
		VM_PROT_ALL,			// maxprot
		VM_PROT_ALL,			// initprot
		3,						// nsects
		0						// flags
	};

	WriteDataAligned(f, &sc, sizeof(sc));
	
	struct section sects[3] = {
		{
			"__text",			// sectname
			"__TEXT",			// segname
			0,					// addr
			0,					// size
			kDataOffset,		// offset
			0,					// align
			0,					// reloff
			0,					// nreloc
			S_ATTR_PURE_INSTRUCTIONS,
							// flags
		},
		{
			{ '_', '_', 'p', 'i', 'c', 's', 'y', 'm', 'b', 'o', 'l', 's', 't', 'u', 'b', '1'  },
								// sectname
			"__TEXT",			// segname
			0,					// addr
			0,					// size
			kDataOffset,		// offset
			0,					// align
			0,					// reloff
			0,					// nreloc
			S_ATTR_PURE_INSTRUCTIONS | S_SYMBOL_STUBS, // flags
			0,
			0x00000020
		},
		{
			"__data",			// sectname
			"__DATA",			// segname
			0,					// addr
			size,				// size
			kDataOffset,		// offset
			2,					// align
			0,					// reloff
			0,					// nreloc
			0,					// flags
		}
	};

	WriteDataAligned(f, &sects, sizeof(sects));

	uint32 dataoff = kDataOffset + size;
	if (dataoff & 1)
		++dataoff;
	
	uint32 stroff = kDataOffset + size + sizeof(struct nlist) * mGlobals.size();
	if (stroff & 1)
		++stroff;

	struct symtab_command st = {
		LC_SYMTAB,				// cmd
		sizeof(symtab_command),	// cmdsize
		dataoff,				// symoff
		mGlobals.size(),		// nsyms
		stroff,					// stroff
		names.length()			// strsize	
	};

	WriteDataAligned(f, &st, sizeof(st));
	
	struct dysymtab_command dst = {
		LC_DYSYMTAB,				// cmd
		sizeof(dysymtab_command),	// cmdsize
		0,							// ilocalsym
		0,							// nlocalsym
		0,							// iextdefsym
		mGlobals.size(),			// nextdefsym
		mGlobals.size(),			// iundefsym
		0							// nundefsym
	};
	
	WriteDataAligned(f, &dst, sizeof(dst));
	
	struct nlist* strtab = new struct nlist[mGlobals.size()];
	struct nlist* sym = strtab;
	
	size = 0;
	for (MGlobals::iterator g = mGlobals.begin(); g != mGlobals.end(); ++g, ++sym)
	{
		uint32 gs = g->size;
		if ((gs % alignment) != 0)
			gs = ((gs / alignment) + 1) * alignment;
		
		sym->n_name = reinterpret_cast<char*>(AddNameToNameTable(names, g->name.c_str()));
		sym->n_type = 0x0f; //N_SECT | N_EXT;
		sym->n_other = 3;
		sym->n_desc = 0;
		sym->n_value = size;

		uint32 align = alignment;
		if (g + 1 == mGlobals.end())
			align = 2;

		WriteDataAligned(f, g->data, g->size, align);

		size += gs;
	}
	
	WriteDataAligned(f, strtab, sizeof(struct nlist) * mGlobals.size());
	WriteDataAligned(f, names.c_str(), names.length());
}

#endif
