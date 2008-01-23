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

#include "MJapi.h"

#include <boost/filesystem/fstream.hpp>

#if not (defined(__APPLE__) and defined(__MACH__))

#include "MUtils.h"
#include "MObjectFileImp_elf.h"
#include "MError.h"

using namespace std;

//template
//<
//	class		SWAPPER,
//	typename	Elf_Ehdr,
//	typename	Elf_Shdr
//>
//void MELFObjectFileImp::Read(
//	Elf_Ehdr&		eh,
//	istream&		inData)
//{
//}

template
<
	MTargetCPU	CPU,
	typename	traits
>
void MELFObjectFileImp<CPU,traits>::Read(
	const MPath&		inFile)
{
	fs::ifstream file(inFile, ios::binary);
	if (not file.is_open())
		THROW(("Failed to open object file"));

	swapper	swap;

	char* stringtable = nil;
	uint32 stringtablesize = 0;

	try
	{
		Elf_Ehdr eh;
		
		file.read((char*)&eh, sizeof(eh));
		
		file.seekg(swap(eh.e_shoff), ios::beg);
		
		for (uint32 section = 0; section < swap(eh.e_shnum); ++section)
		{
			Elf_Shdr sh;
			file.read((char*)&sh, sizeof(sh));
			
			if (swap(sh.sh_type) == SHT_STRTAB)
			{
				stringtablesize = swap(sh.sh_size);
				
				stringtable = new char[stringtablesize];
				
				file.seekg(swap(sh.sh_offset), ios::beg);
				file.read(stringtable, stringtablesize);
				break;
			}
		}
		
		if (stringtable != nil)
		{
			file.seekg(swap(eh.e_shoff), ios::beg);
			
			for (uint32 section = 0; section < swap(eh.e_shnum); ++section)
			{
				Elf_Shdr sh;
				file.read((char*)&sh, sizeof(sh));
				
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

template
<
	MTargetCPU	CPU,
	typename	traits
>
void MELFObjectFileImp<CPU, traits>::Write(
	const MPath&		inFile)
{
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
	
	uint32 data_size = sym_offset;

	uint32 symtab_off = sym_offset;
	assert((sizeof(Elf_Sym) % 8) == 0);
	
	uint32 symtab_size = syms.size() * sizeof(sym);
	uint32 strtab_off = WriteDataAligned(f, &syms[0], symtab_size, 8);
	uint32 shstrtab_off = WriteDataAligned(f, strtab.c_str(), strtab.length(), 8);

	string shstrtab;
	(void)AddNameToNameTable(shstrtab, "");		// null name
	(void)AddNameToNameTable(shstrtab, ".text");
	(void)AddNameToNameTable(shstrtab, ".rsrc_data");
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
			AddNameToNameTable(shstrtab, ".rsrc_data"),
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

MObjectFileImp* CreateELFObjectFileImp(
	MTargetCPU		inTarget)
{
	MObjectFileImp* result = nil;

	switch (inTarget)
	{
		case eCPU_386:
			result = new MELFObjectFileImp<eCPU_386>();
			break;
		
		case eCPU_x86_64:
			result = new MELFObjectFileImp<eCPU_x86_64>();
			break;
		
		case eCPU_PowerPC_32:
			result = new MELFObjectFileImp<eCPU_PowerPC_32>();
			break;
		
		case eCPU_PowerPC_64:
			result = new MELFObjectFileImp<eCPU_PowerPC_64>();
			break;
		
		default:
			THROW(("Unsupported object file"));	
	}
	
	return result;
}

MObjectFileImp* CreateELFObjectFileImp(
	const MPath&	inFile)
{
	MTargetCPU target = eCPU_Unknown;
	
	fs::ifstream file(inFile, ios::binary);
	if (not file.is_open())
		THROW(("Could not open object file"));
	
	char ident[EI_NIDENT];
	file.read(ident, EI_NIDENT);
	
	if (strncmp(ident, ELFMAG, SELFMAG) != 0 or
		(ident[EI_DATA] != ELFDATA2LSB and ident[EI_DATA] != ELFDATA2MSB) or
		(ident[EI_CLASS] != ELFCLASS32 and ident[EI_CLASS] != ELFCLASS64))
	{
		THROW(("File is not a supported object file"));
	}
	
	if (ident[EI_DATA] == ELFDATA2LSB)
	{
		if (ident[EI_CLASS] == ELFCLASS64)
			target = eCPU_x86_64;
		else
			target = eCPU_386;
	}
	else
	{
		if (ident[EI_CLASS] == ELFCLASS32)
			target = eCPU_PowerPC_32;
		else
			target = eCPU_PowerPC_64;
	}
	
	return CreateELFObjectFileImp(target);
}

#endif
