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

void MELFObjectFileImp::Read(
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

uint32 WriteDataAligned(
	ofstream&	inStream,
	const void*	inData,
	uint32		inSize,
	uint32		inAlignment = 1)
{
	inStream.write(reinterpret_cast<const char*>(inData), inSize);

	if (inAlignment > 1)
	{
		while ((inStream.tellp() % inAlignment) != 0)
			inStream.put('\0');
	}

	return inStream.tellp();
}

enum {
	kNullSection,
	kTextSection,
	kDataSection,
	kBssSection,
	kShStrtabSection,
	kSymtabSection,
	kStrtabSection,
	
	kSectionCount
};

enum {
	kNullSymbol,
	kTextSectionSymbol,
	kDataSectionSymbol,
	kBssSectionSymbol,
	kGlobalSymbol,
	
	kSymbolCount
};

template<MTargetCPU CPU>
void MELFObjectFileImp::WriteForCPU(
	const MPath&		inFile)
{
	typedef MCPUTraits<CPU>	traits;
	
	typedef typename traits::Elf_Ehdr	Elf_Ehdr;
	typedef typename traits::Elf_Shdr	Elf_Shdr;
	typedef typename traits::Elf_Sym	Elf_Sym;
	
	fs::ofstream f(inFile, ios::binary | ios::trunc);
	if (not f.is_open())
		THROW(("Failed to open object file for writing"));

	Elf_Ehdr eh = {
							// e_ident
		{ ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3,
			traits::ElfClass, traits::ElfData, EV_CURRENT },
		ET_REL,				// e_type
		traits::ElfMachine,	// e_machine
		EV_CURRENT,			// e_version
		0,					// e_entry
		0,					// e_phoff
		0,					// e_shoff
		0,					// e_flags
		sizeof(Elf_Ehdr),	// e_ehsize
		0,					// e_phentsize
		0,					// e_phnum
		sizeof(Elf_Shdr),	// e_shentsize
		kSectionCount,		// e_shnum
		kShStrtabSection	// e_shstrndx
	};

	uint32 data_offset = WriteDataAligned(f, &eh, sizeof(eh), 16);

	string strtab;
	AddNameToNameTable(strtab, "");		// null name
	
	Elf_Sym sym = {};
	vector<Elf_Sym> syms;
	
		// kNullSymbol
	syms.push_back(sym);
	
		// text section symbol
	sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_SECTION);
	sym.st_shndx = kTextSection;
	syms.push_back(sym);
	
		// data section symbol
	sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_SECTION);
	sym.st_shndx = kDataSection;
	syms.push_back(sym);
	
		// bss section symbol
	sym.st_info = ELF32_ST_INFO(STB_LOCAL, STT_SECTION);
	sym.st_shndx = kBssSection;
	syms.push_back(sym);

	uint32 sym_offset = data_offset;

	for (MGlobals::iterator g = mGlobals.begin(); g != mGlobals.end(); ++g)
	{
		sym.st_name = AddNameToNameTable(strtab, g->name);
		sym.st_value = sym_offset - data_offset;
		sym.st_size = g->data.length();
		sym.st_info = ELF32_ST_INFO(STB_GLOBAL, STT_OBJECT);
		sym.st_shndx = kDataSection;
		
		syms.push_back(sym);
		
		sym_offset = WriteDataAligned(f, g->data.c_str(), g->data.length(), 8);
	}
	
	uint32 data_size = sym_offset - data_size;

	uint32 symtab_off = sym_offset;
	assert((sizeof(Elf_Sym) % 8) == 0);
	
	uint32 symtab_size = syms.size() * sizeof(sym);
	uint32 strtab_off = WriteDataAligned(f, &syms[0], symtab_size, 8);
	uint32 shstrtab_off = WriteDataAligned(f, strtab.c_str(), strtab.length(), 8);

	string shstrtab;
	(void)AddNameToNameTable(shstrtab, "");		// null name
	(void)AddNameToNameTable(shstrtab, ".text");
	(void)AddNameToNameTable(shstrtab, ".data");
	(void)AddNameToNameTable(shstrtab, ".bss");
	(void)AddNameToNameTable(shstrtab, ".shstrtab");
	(void)AddNameToNameTable(shstrtab, ".symtab");
	(void)AddNameToNameTable(shstrtab, ".strtab");
	
	eh.e_shoff = WriteDataAligned(f, shstrtab.c_str(), shstrtab.length(), 16);

	Elf_Shdr sh[kSectionCount] = {
		{			// kNullSection
		},
		{			// kTextSection
							// sh_name
			AddNameToNameTable(shstrtab, ".text"),
			SHT_PROGBITS,	// sh_type
							// sh_flags
			SHF_ALLOC|SHF_EXECINSTR,
			0,				// sh_addr
			data_offset,	// sh_offset
			0,				// sh_size
			0,				// sh_link
			0,				// sh_info
			4,				// sh_addralign
			0				// sh_entsize
		},
		{			// kDataSection
							// sh_name
			AddNameToNameTable(shstrtab, ".data"),
			SHT_PROGBITS,	// sh_type
							// sh_flags
			SHF_ALLOC|SHF_WRITE,
			0,				// sh_addr
			data_offset,	// sh_offset
			data_size,		// sh_size
			0,				// sh_link
			0,				// sh_info
			4,				// sh_addralign
			0				// sh_entsize
		},
		{			// kBssSection
							// sh_name
			AddNameToNameTable(shstrtab, ".bss"),
			SHT_NOBITS,		// sh_type
							// sh_flags
			SHF_ALLOC|SHF_WRITE,
			0,				// sh_addr
			eh.e_shoff,		// sh_offset
			0,				// sh_size
			0,				// sh_link
			0,				// sh_info
			4,				// sh_addralign
			0				// sh_entsize
		},
		{			// kShStrtabSection
							// sh_name
			AddNameToNameTable(shstrtab, ".shstrtab"),
			SHT_STRTAB,		// sh_type
			0,				// sh_flags
			0,				// sh_addr
			shstrtab_off,	// sh_offset
			shstrtab.length(),// sh_size
			0,				// sh_link
			0,				// sh_info
			1,				// sh_addralign
			0				// sh_entsize
		},
		{			// kSymtabSection
							// sh_name
			AddNameToNameTable(shstrtab, ".symtab"),
			SHT_SYMTAB,		// sh_type
							// sh_flags
			0,
			0,				// sh_addr
			symtab_off,		// sh_offset
			symtab_size,	// sh_size
			6,				// sh_link
			kGlobalSymbol,	// sh_info
			8,				// sh_addralign
			sizeof(Elf_Sym)	// sh_entsize
		},
		{			// kStrtabSection
							// sh_name
			AddNameToNameTable(shstrtab, ".strtab"),
			SHT_STRTAB,		// sh_type
							// sh_flags
			0,
			0,				// sh_addr
			strtab_off,		// sh_offset
			strtab.length(),// sh_size
			0,				// sh_link
			0,				// sh_info
			1,				// sh_addralign
			0				// sh_entsize
		},
	};
	
	WriteDataAligned(f, sh, sizeof(sh), 1);
	
	f.flush();
	
	f.seekp(0);
	WriteDataAligned(f, &eh, sizeof(eh));
}

void MELFObjectFileImp::Write(
	const MPath&	inFile)
{
	WriteForCPU<eCPU_x86_64>(inFile);
}

#endif

