/*
	Copyright (c) 2007, Maarten L. Hekkelman
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

#ifndef MOBJECTFILEIMP_ELF_H
#define MOBJECTFILEIMP_ELF_H

#include <elf.h>

#include "MObjectFile.h"

enum MTargetCPU {
	eCPU_386,
	eCPU_x86_64,
	eCPU_PowerPC
};

template<MTargetCPU CPU>
struct MCPUTraits
{
};

template<>
struct MCPUTraits<eCPU_PowerPC>
{
	
};

template<>
struct MCPUTraits<eCPU_x86_64>
{
	typedef Elf64_Ehdr	Elf_Ehdr;
	typedef Elf64_Shdr	Elf_Shdr;
	typedef Elf64_Sym	Elf_Sym;
	
	enum {
		ElfClass	= ELFCLASS64,
		ElfData		= ELFDATA2LSB,
		ElfMachine	= EM_X86_64
	};
};

struct MELFObjectFileImp : public MObjectFileImp
{
	template
	<
		class		SWAPPER,
		typename	Elf_Ehdr,
		typename	Elf_Shdr
	>
	void			Read(
						Elf_Ehdr&		eh,
						std::istream&	inData);

	virtual void	Read(
						const MPath&	inFile);

	void			Write(
						const MPath&	inFile);

	template<MTargetCPU CPU>
	void			WriteForCPU(
						const MPath&	inFile);
};

#endif
