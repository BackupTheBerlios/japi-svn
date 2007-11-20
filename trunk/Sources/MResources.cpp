#include "MJapieG.h"

#include <boost/filesystem/fstream.hpp>
#include <vector>

#include "MObjectFile.h"
#include "MResources.h"

using namespace std;

namespace {

enum {
	kResourceFolderKind	= 'fold',
	kResourceItemKind	= 'item'
};

struct MResourceItem
{
	uint32		name;
	uint32		kind;
	union
	{
		struct DataItem
		{
			uint32	offset;
			uint32	size;
		}			data;
		struct FolderItem
		{
			uint32	offset;
			int32	count;
		}			folder;
	}				item;
};

struct MResourceIndex
{
	char			sig[4];		// check
	uint32			count;		// number of resources
	uint32			strtable_offset;
	uint32			data_offset;
	MResourceItem	items[1];
	
	const char*		str(
						uint32	inOffset) const
					{
						return sig + strtable_offset + inOffset;
					}

	const void*		data(
						uint32	inOffset) const
					{
						return sig + data_offset + inOffset;
					}
};

extern const MResourceIndex	gResourceIndex;

bool FindResourceData(
	const string&			inPath,
	const MResourceItem&	inItem,
	uint32&					outOffset,
	uint32&					outSize)
{
	assert(inItem.kind == kResourceFolderKind);
	
	string name, path(inPath);
	
	string::size_type p = path.find('/');
	if (p != string::npos)
	{
		name = path.substr(0, p);
		path.erase(0, p + 1);
	}
	else
		name = path;
	
	bool result = false;
	
	if (inItem.kind == kResourceFolderKind and
		inItem.item.folder.count > 0)
	{
		int32 L = inItem.item.folder.offset;
		int32 R = L + inItem.item.folder.count - 1;
		
		while (L <= R)
		{
			int32 i = (L + R) / 2;
			
			const MResourceItem& item = gResourceIndex.items[i];
			
			int d = name.compare(0, string::npos, gResourceIndex.str(item.name));
			if (d < 0)
				i = L + 1;
			else if (d > 0)
				i = R + 1;
			else
			{
				if (item.kind == kResourceFolderKind)
					result = FindResourceData(path, item, outOffset, outSize);
				else
				{
					outOffset = item.item.data.offset;
					outSize = item.item.data.size;
					result = true;
				}
				
				break;
			}
		}
	}
	
	return result;
}

}

bool LoadResource(
	const char*		inName,
	const void*&	outData,
	uint32&			outSize)
{
	bool result = false;
	uint32 offset;
	
	const char* lang = getenv("LANG");
	
	if (lang != nil)
	{
		string locale(lang);
		
		string::size_type p = locale.find('.');
		if (p != string::npos)
			locale.erase(p, string::npos);
		
		result = FindResourceData(locale + '/' + inName,
			gResourceIndex.items[0], offset, outSize);
		
		if (result == false and locale.length() == 5 and locale[3] == '_')
		{
			locale.erase(3, string::npos);

			result = FindResourceData(locale + '/' + inName,
				gResourceIndex.items[0], offset, outSize);
		}
	}
	
	if (result == false)
		result = FindResourceData(inName, gResourceIndex.items[0], offset, outSize);
	
	if (result)
		outData = gResourceIndex.data(offset);
	
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

uint32 CountResourceFileItems(
	MResourceFileItem*	inItem)
{
	uint32 result = 1;
	
	if (inItem->children != nil)
		result += CountResourceFileItems(inItem->children);
	
	if (inItem->next != nil)
		result += CountResourceFileItems(inItem->next);
	
	return result;
}

void StoreItems(
	MResourceFileItem*		inItem,
	vector<MResourceItem>&	ioItems,
	uint32&					outOffset,
	uint32&					outCount)
{
	outOffset = ioItems.size();
	outCount = 0;
	
	for (MResourceFileItem* item = inItem; item != nil; item = item->next)
	{
		MResourceItem n;
		n.name = item->name;
		if (item->children != nil)
			n.kind = kResourceFolderKind;
		else
		{
			n.kind = kResourceItemKind;
			n.item.data.offset = item->data;
			n.item.data.size = item->size;
		}
		
		ioItems.push_back(n);
		++outCount;
	}

	uint32 i = outOffset;
	for (MResourceFileItem* item = inItem; item != nil; item = item->next, ++i)
	{
		if (item->children != nil)
		{
			uint32 offset, count;
			StoreItems(item->children, ioItems, offset, count);
			ioItems[i].item.folder.offset = offset;
			ioItems[i].item.folder.count = count;
		}
	}
}

void MResourceFile::Write(
	const MPath&	inFile)
{
	MResourceIndex index = { { 'r', 's', 'r', 'c' } };
	
	vector<MResourceItem> items;
	items.reserve(index.count);
	items.push_back(MResourceItem());
	
	uint32 offset, count;
	
	StoreItems(mImpl->root, items, offset, count);
	index.count = items.size();

	items[0].kind = kResourceFolderKind;
	items[0].item.folder.offset = offset;
	items[0].item.folder.count = count;
	
	index.strtable_offset = sizeof(index) + (items.size() - 1) * sizeof(MResourceItem);
	index.data_offset = index.strtable_offset + mImpl->names.length();

	// we've got all data now, write out to new object file
	
//	fs::ofstream f(inFile, ios::binary | ios::trunc);
//	
//	if (not f.is_open())
//		THROW(("Failed to create new object file"));

	uint32 size = index.data_offset + mImpl->data.length();
	char* buffer = new char[size];
	
	memcpy(buffer, &index, sizeof(index) - sizeof(MResourceItem));
	memcpy(buffer + sizeof(index) - sizeof(MResourceItem),
			&items[0], sizeof(MResourceItem) * items.size());
	memcpy(buffer, mImpl->names.c_str(), mImpl->names.length());
	memcpy(buffer, mImpl->data.c_str(), mImpl->data.length());
	
	MObjectFile file;
	file->AddGlobal("gResourceIndex", buffer, size);
	file->Write(inFile);
}

