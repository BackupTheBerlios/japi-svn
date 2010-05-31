//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOBJECTFILEIMP_ELF_H
#define MOBJECTFILEIMP_ELF_H

#include <elf.h>

#include "MObjectFile.h"
#include "MUtils.h"

template<MTargetCPU CPU>
struct MCPUTraits
{
};

template<>
struct MCPUTraits<eCPU_PowerPC_32>
{
	typedef Elf32_Ehdr	Elf_Ehdr;
	typedef Elf32_Shdr	Elf_Shdr;
	typedef Elf32_Sym	Elf_Sym;
	
	typedef msb_swapper	swapper;
	
	enum {
		ElfClass	= ELFCLASS32,
		ElfData		= ELFDATA2MSB,
		ElfMachine	= EM_PPC
	};
};

template<>
struct MCPUTraits<eCPU_PowerPC_64>
{
	typedef Elf64_Ehdr	Elf_Ehdr;
	typedef Elf64_Shdr	Elf_Shdr;
	typedef Elf64_Sym	Elf_Sym;
	
	typedef msb_swapper	swapper;
	
	enum {
		ElfClass	= ELFCLASS64,
		ElfData		= ELFDATA2MSB,
		ElfMachine	= EM_PPC64
	};
};

template<>
struct MCPUTraits<eCPU_x86_64>
{
	typedef Elf64_Ehdr	Elf_Ehdr;
	typedef Elf64_Shdr	Elf_Shdr;
	typedef Elf64_Sym	Elf_Sym;

	typedef lsb_swapper	swapper;
	
	enum {
		ElfClass	= ELFCLASS64,
		ElfData		= ELFDATA2LSB,
		ElfMachine	= EM_X86_64
	};
};

template<>
struct MCPUTraits<eCPU_386>
{
	typedef Elf32_Ehdr	Elf_Ehdr;
	typedef Elf32_Shdr	Elf_Shdr;
	typedef Elf32_Sym	Elf_Sym;
	
	typedef lsb_swapper	swapper;
	
	enum {
		ElfClass	= ELFCLASS32,
		ElfData		= ELFDATA2LSB,
		ElfMachine	= EM_386
	};
};

template<
	MTargetCPU CPU,
	typename traits = MCPUTraits<CPU>
>
struct MELFObjectFileImp : public MObjectFileImp
{
	typedef typename traits::swapper	swapper;
	typedef typename traits::Elf_Ehdr	Elf_Ehdr;
	typedef typename traits::Elf_Shdr	Elf_Shdr;
	typedef typename traits::Elf_Sym	Elf_Sym;

	virtual void	Read(
						const fs::path&	inFile);

	void			Write(
						const fs::path&	inFile);
};

MObjectFileImp* CreateELFObjectFileImp(
	MTargetCPU		inTarget);

MObjectFileImp* CreateELFObjectFileImp(
	const fs::path&	inFile);

#endif
