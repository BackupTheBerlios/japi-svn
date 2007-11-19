#include "MJapieG.h"

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
	}			item;
};

struct MResourceIndex
{
	uint32			sig;		// check
	uint32			count;		// number of resources
	MResourceItem	items[1];
};

extern MResourceIndex*	gResourceIndex;
extern char				gResourceStrTable[];
extern char				gResourceData[];

bool FindResourceData(
	string			inPath,
	MResourceItem&	inItem,
	uint32&			outOffset,
	uint32&			outSize)
{
	assert(inItem.kind == kResourceFolderKind);
	
	string name;
	
	string::size_type p = inPath.find('/');
	if (p != string::npos)
	{
		name = inPath.substr(0, p);
		inPath.erase(0, p + 1);
	}
	else
		name = inPath;
	
	bool result = false;
	
	if (inItem.kind == kResourceFolderKind and
		inItem.item.folder.count > 0)
	{
		int32 L = inItem.item.folder.offset;
		int32 R = L + inItem.item.folder.count - 1;
		
		while (L <= R)
		{
			int32 i = (L + R) / 2;
			
			MResourceItem& item = gResourceIndex->items[i];
			
			int d = name.compare(0, string::npos, gResourceStrTable + item.name);
			if (d < 0)
				i = L + 1;
			else if (d > 0)
				i = R + 1;
			else
			{
				if (item.kind == kResourceFolderKind)
					result = FindResourceData(inPath, item, outOffset, outSize);
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
	
}

