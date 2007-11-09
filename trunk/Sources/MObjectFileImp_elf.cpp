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

#if not (defined(__APPLE__) and defined(__MACH__))

#include "MUtils.h"
#include "MObjectFileImp_elf.h"

using namespace std;

template
<
	class		SWAPPER,
	typename	Elf_Ehdr,
	typename	Elf_Shdr
>
void MELFObjectFileImp::Read(
	Elf_Ehdr&		eh,
	istream&		inData)
{
	SWAPPER	swap;

	char* stringtable = nil;
	uint32 stringtablesize = 0;

	try
	{
		inData.seekg(eh.e_shoff, ios::beg);
		
		for (uint32 section = 0; section < swap(eh.e_shnum); ++section)
		{
			Elf_Shdr sh;
			inData.read((char*)&sh, sizeof(sh));
			
			if (swap(sh.sh_type) == SHT_STRTAB)
			{
				stringtablesize = swap(sh.sh_size);
				
				stringtable = new char[stringtablesize];
				
				inData.seekg(swap(sh.sh_offset), ios::beg);
				inData.read(stringtable, stringtablesize);
				break;
			}
		}
		
		if (stringtable != nil)
		{
			inData.seekg(eh.e_shoff, ios::beg);
			
			for (uint32 section = 0; section < swap(eh.e_shnum); ++section)
			{
				Elf_Shdr sh;
				inData.read((char*)&sh, sizeof(sh));
				
				if (strcmp(stringtable + swap(sh.sh_name), ".text") == 0)
					mTextSize += swap(sh.sh_size);
				else if (strcmp(stringtable + swap(sh.sh_name), ".data") == 0)
					mDataSize += swap(sh.sh_size);
			}
		}
	}
	catch (...) {}

	if (stringtable != nil)
		delete[] stringtable;
}

void MELFObjectFileImp::SetFile(
	const MPath&		inFile)
{
	mFile = inFile;
	
	mTextSize = 0;
	mDataSize = 0;
	
	fs::ifstream file(mFile, ios::binary);
	if (not file.is_open())
		THROW(("Could not open object file"));
	
	char ident[EI_NIDENT];
	file.read(ident, EI_NIDENT);
	
	if (strncmp(ident, ELFMAG, SELFMAG) != 0)
		THROW(("File is not an object file"));
	
	if (ident[EI_CLASS] == ELFCLASS32)
	{
		Elf32_Ehdr hdr;
		file.seekg(0);
		file.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
		
		if (ident[EI_DATA] == ELFDATA2LSB)
			Read<lsb_swapper, Elf32_Ehdr, Elf32_Shdr>(hdr, file);
		else if (ident[EI_DATA] == ELFDATA2MSB)
			Read<msb_swapper, Elf32_Ehdr, Elf32_Shdr>(hdr, file);
		else
			THROW(("File is not an object file"));		
	}
	else if (ident[EI_CLASS] == ELFCLASS64)
	{
		Elf64_Ehdr hdr;
		file.seekg(0);
		file.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
		
		if (ident[EI_DATA] == ELFDATA2LSB)
			Read<lsb_swapper, Elf64_Ehdr, Elf64_Shdr>(hdr, file);
		else if (ident[EI_DATA] == ELFDATA2MSB)
			Read<msb_swapper, Elf64_Ehdr, Elf64_Shdr>(hdr, file);
		else
			THROW(("File is not an object file"));		
	}
	else
		THROW(("File is not an object file"));
}

#endif
