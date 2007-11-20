#include "MJapieG.h"

#include <boost/filesystem/fstream.hpp>
#include <vector>

#include <mach-o/loader.h>
#include <mach-o/stab.h>
#include <mach-o/dyld.h>
#define _AOUT_INCLUDE_
#include <nlist.h>

#include "MObjectFile.h"
#include "MResources.h"
#include "MPatriciaTree.h"

using namespace std;

namespace {

MPatriciaTree<const void*>	gResources;

bool LookupResourceInImage(
	const struct mach_header&	mh,
	uint32						inOffset,
	const char*					inName,
	const void*&				outData)
{
	bool result = false;
	const struct segment_command*	seg = nil;
	const struct segment_command*	seg_data = nil;
	const struct segment_command*	seg_linkedit = nil;
	const struct symtab_command* symtab = nil;
	
	const char* base = reinterpret_cast<const char*>(&mh);
	const char* ptr = base + sizeof(mach_header);
	
	for (uint32 ix = 0; ix < mh.ncmds; ++ix)
	{
		const struct load_command* cmd = reinterpret_cast<const struct load_command*>(ptr);
		
		switch (cmd->cmd)
		{
			case LC_SEGMENT:
				seg = reinterpret_cast<const struct segment_command*>(ptr);
				if (strcmp(seg->segname, SEG_DATA) == 0)
					seg_data = seg;
				else if (strcmp(seg->segname, SEG_LINKEDIT) == 0)
					seg_linkedit = seg;
				break;
			
			case LC_SYMTAB:
				symtab = reinterpret_cast<const struct symtab_command*>(ptr);
				break;
		}
		
		ptr += cmd->cmdsize;
	}
	
	if (seg_data != nil and seg_linkedit != nil and symtab != nil)
	{
		const struct nlist* symbase =
			reinterpret_cast<const struct nlist*>(base + symtab->symoff + inOffset);

		const char* strings = base + symtab->stroff + inOffset;
		
		const struct nlist* sym = symbase;

		for (uint32 ix = 0; ix < symtab->nsyms; ++ix, ++sym)
		{
			if (sym->n_type != 0x0f or sym->n_un.n_strx == 0)
				continue;
			
			if (strcmp(inName, strings + sym->n_un.n_strx) == 0)
			{
				outData = reinterpret_cast<const void*>(sym->n_value + inOffset);
				result = true;
				break;
			}
		}
	}
	
	return result;
}

bool LookupResource(
	const char*					inName,
	const void*&				outData)
{
	bool result = false;

	uint32  count = _dyld_image_count();
	
	for (uint32 ix = 0; result == false and ix < count; ++ix)
	{
		const struct mach_header* mh = _dyld_get_image_header(ix);
		uint32 offset = _dyld_get_image_vmaddr_slide(ix);
		
		result = LookupResourceInImage(*mh, offset, inName, outData);
	}
	
	return result;
}

}

const void* LoadResource(
	const char*		inName)
{
	const void* result = gResources[inName];
	
	if (result == nil)
	{
		if (LookupResource(inName, result) == false)
			THROW(("Could not find resource '%s'", inName));
		
		gResources[inName] = result;
	}
	
	return result;
}

// --------------------------------------------------------------------

struct MResourceFileItem
{
	uint32		name;
	uint32		data;
	uint32		size;
	
	MResourceFileItem*
				next;

	MResourceFileItem*
				children;

				MResourceFileItem()
					: name(0)
					, data(0)
					, size(0)
					, next(nil)
					, children(nil)
				{
				}

				~MResourceFileItem()
				{
					delete next;
					delete children;
				}
};

struct MResourceFileImp
{
	MResourceFileItem*	root;
	string				data;
	string				names;

	uint32				AddName(
							const char*		inName)
						{
							const char* p = names.c_str();
							const char* end = p + names.length();
							
							for (; p < end; p += strlen(p) + 1)
							{
								if (strcmp(inName, p) == 0)
									break;
							}
							
							if (p == end)
							{
								names.append(inName);
								names.append('\0');
								p = names.c_str() + names.length();
							}

							return p - names.c_str();
						}
	
	uint32				AddData(
							const void*		inData,
							uint32			inSize)
						{
							uint32 offset = data.length();
							data.append(reinterpret_cast<const char*>(inData), inSize);
							return offset;
						}
};

MResourceFile::MResourceFile()
	: mImpl(new MResourceFileImp)
{
	mImpl->root = new MResourceFileItem;
}

MResourceFile::~MResourceFile()
{
	delete mImpl->root;
	delete mImpl;
}

void MResourceFile::Add(
	const char*		inLocale,
	const char*		inName,
	const void*		inData,
	uint32			inSize)
{
	MResourceFileItem* folder = mImpl->root;
	
	if (inLocale != nil)
	{
		for (MResourceFileItem* f = folder->children; f != nil; f = f->next)
		{
			if (strcmp(mImpl->data.c_str(), inLocale) == 0)
			{
				folder = f;
				break;
			}
		}
		
		if (folder == mImpl->root)
		{
			folder = new MResourceFileItem;

			folder->next = mImpl->root->children;
			mImpl->root->children = folder;
			
			folder->name = mImpl->AddName(inLocale);
		}
	}
	
	MResourceFileItem* item = new MResourceFileItem;
	item->name = mImpl->AddName(inName);
	item->data = mImpl->AddData(inData, inSize);
	item->size = inSize;
	
	item->next = folder->children;
	folder->children = item;
}

void MResourceFile::Add(
	const char*		inLocale,
	const char*		inName,
	const MPath&	inFile)
{
	fs::ifstream f(inFile);

	if (not f.is_open())
		THROW(("Could not open resource file"));

	filebuf* b = f.rdbuf();
	
	uint32 size = b->pubseekoff(0, ios::end, ios::in);
	b->pubseekoff(0, ios::beg, ios::in);
	
	char* text = new char[size];
	
	b->sgetn(text, size);
	f.close();
	
	Add(inLocale, inName, text, size);
	
	delete[] text;
}

