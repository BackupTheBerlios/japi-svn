//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <archive.h>
#include <archive_entry.h>
#include <cstring>
#include <zlib.h>
#include <endian.h>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>

#include "document.hpp"

#include "MError.h"
#include "MePubDocument.h"
#include "MePubItem.h"
#include "MTextBuffer.h"
#include "MFile.h"

#ifndef __BYTE_ORDER
#error "Please specify byte order"
#endif

using namespace std;
namespace ba = boost::algorithm;

// --------------------------------------------------------------------

namespace {

const char
	kEPubMimeType[] = "application/epub+zip",
	kMagicMT[] = "mimetypeapplication/epub+zip",
	kContainerNS[] = "urn:oasis:names:tc:opendocument:xmlns:container";

struct ZIPLocalFileHeader
{
						ZIPLocalFileHeader()
						{
							struct tm tm;
							gmtime_r(nil, &tm);
							
							file_mod_date =
								(((tm.tm_year - 80) & 0x7f) << 9) |
								(((tm.tm_mon + 1)   & 0x0f) << 5) |
								( (tm.tm_mday       & 0x1f) << 0);
							
							file_mod_time =
								((tm.tm_hour & 0x1f) << 11) |
								((tm.tm_min  & 0x3f) << 5)  |
								((tm.tm_sec  & 0x3e) >> 1);
						}

	bool				deflated;
	uint32				crc;
	uint32				compressed_size;
	uint32				uncompressed_size;
	uint16				file_mod_time, file_mod_date;
	string				filename;
	string				data;
};

struct ZIPCentralDirectory
{
	ZIPLocalFileHeader	file;
	uint32				offset;
};

struct ZIPEndOfCentralDirectory
{
	uint16				entries;
	uint32				directory_size;
	uint32				directory_offset;
};

inline char* write(uint16 value, char* buffer)
{
#if __BYTE_ORDER == __BIG_ENDIAN
	value = static_cast<uint16>(
			((((uint16)value)<< 8) & 0xFF00)  |
			((((uint16)value)>> 8) & 0x00FF));
#endif
	memcpy(buffer, &value, sizeof(value));
	return buffer + sizeof(value);
}

inline char* write(uint32 value, char* buffer)
{
#if __BYTE_ORDER == __BIG_ENDIAN
	value = static_cast<uint32>(
			((((uint32)value)<<24) & 0xFF000000)  |
			((((uint32)value)<< 8) & 0x00FF0000)  |
			((((uint32)value)>> 8) & 0x0000FF00)  |
			((((uint32)value)>>24) & 0x000000FF));
#endif
	memcpy(buffer, &value, sizeof(value));
	return buffer + sizeof(value);
}

const uint32
	kLocalFileHeaderSignature = 0x04034b50UL,
	kCentralDirectorySignature = 0x02014b50,
	kEndOfCentralDirectorySignature = 0x06054b50;

const uint16
	kVersionMadeBy = 0x0017;

ostream& operator<<(ostream& lhs, ZIPLocalFileHeader& rhs)
{
	char b[30];
	char* p;
	
	uint16 versionNeededToExtract = 0x000a, bitFlag = 0, compressionMethod = 0;

	if (rhs.deflated)
	{
		versionNeededToExtract = 0x0014;
		compressionMethod = 0x0008;
	}
	
	uint16 fileNameLength = rhs.filename.length(), extraFieldLength = 0;
	
	p = write(kLocalFileHeaderSignature, b);
	p = write(versionNeededToExtract, p);
	p = write(bitFlag, p);
	p = write(compressionMethod, p);
	p = write(rhs.file_mod_time, p);
	p = write(rhs.file_mod_date, p);
	p = write(rhs.crc, p);
	p = write(rhs.compressed_size, p);
	p = write(rhs.uncompressed_size, p);
	p = write(fileNameLength, p);
	p = write(extraFieldLength, p);
	
	assert(p == b + sizeof(b));
	
	lhs.write(b, sizeof(b));
	lhs.write(rhs.filename.c_str(), fileNameLength);
	
	if (rhs.data.length() > 0)
		lhs.write(rhs.data.c_str(), rhs.data.length());
	
	rhs.data.clear();
	
	return lhs;
}

ostream& operator<<(ostream& lhs, ZIPCentralDirectory& rhs)
{
	char b[46];
	char* p;
	
	uint16 versionNeededToExtract = 0x000a, bitFlag = 0, compressionMethod = 0;

	if (rhs.file.deflated)
	{
		versionNeededToExtract = 0x0014;
		compressionMethod = 0x0008;
	}
	
	uint16 fileNameLength = rhs.file.filename.length(),
		extraFieldLength = 0, fileCommentLength = 0,
		diskStartNumber = 0, internalFileAttributes = 0;
	
	uint32 externalFileAttributes = 0;
	
	p = write(kCentralDirectorySignature, b);
	p = write(kVersionMadeBy, p);
	p = write(versionNeededToExtract, p);
	p = write(bitFlag, p);
	p = write(compressionMethod, p);
	p = write(rhs.file.file_mod_time, p);
	p = write(rhs.file.file_mod_date, p);
	p = write(rhs.file.crc, p);
	p = write(rhs.file.compressed_size, p);
	p = write(rhs.file.uncompressed_size, p);
	p = write(fileNameLength, p);
	p = write(extraFieldLength, p);
	p = write(fileCommentLength, p);
	p = write(diskStartNumber, p);
	p = write(internalFileAttributes, p);
	p = write(externalFileAttributes, p);
	p = write(rhs.offset, p);

	assert(p == b + sizeof(b));
	
	lhs.write(b, sizeof(b));
	lhs.write(rhs.file.filename.c_str(), fileNameLength);

	return lhs;
}

ostream& operator<<(ostream& lhs, ZIPEndOfCentralDirectory& rhs)
{
	char b[22];
	char* p;
	
	uint16 nul = 0;
	
	p = write(kEndOfCentralDirectorySignature, b);
	p = write(nul, p);	
	p = write(nul, p);
	p = write(rhs.entries, p);
	p = write(rhs.entries, p);
	p = write(rhs.directory_size, p);
	p = write(rhs.directory_offset, p);
	p = write(nul, p);

	lhs.write(b, sizeof(b));

	return lhs;
}

void deflate(
	const string&		inText,	
	ZIPLocalFileHeader&	outFileHeader)
{
	const uint32 kBufferSize = 4096;
	vector<uint8> b(kBufferSize);
	unsigned char* buffer = &b[0];
	
	z_stream_s z_stream = {};

	int err = deflateInit2(&z_stream, Z_BEST_COMPRESSION,
		Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	if (err != Z_OK)
		THROW(("Compressor error: %s", z_stream.msg));

	z_stream.avail_in = inText.length();
	z_stream.next_in = const_cast<unsigned char*>(
		reinterpret_cast<const unsigned char*>(inText.c_str()));
	z_stream.total_in = 0;
	
	z_stream.next_out = buffer;
	z_stream.avail_out = kBufferSize;
	z_stream.total_out = 0;
	
	int action = Z_FINISH;

	for (;;)
	{
		err = deflate(&z_stream, action);
		
		if (z_stream.avail_out < kBufferSize)
			outFileHeader.data.append(buffer, buffer + (kBufferSize - z_stream.avail_out));
		
		if (err == Z_OK)
		{
			z_stream.next_out = buffer;
			z_stream.avail_out = kBufferSize;
			continue;
		}
		
		break;
	}
	
	if (err != Z_STREAM_END)
		THROW(("Deflate error: %s (%d)", z_stream.msg, err));
	
	assert(z_stream.total_out == outFileHeader.data.length());
	
	outFileHeader.crc = crc32(0, reinterpret_cast<const uint8*>(inText.c_str()), inText.length());
	outFileHeader.compressed_size = outFileHeader.data.length();
	outFileHeader.uncompressed_size = inText.length();
	outFileHeader.deflated = true;
	
	deflateEnd(&z_stream);
}	

void deflate(
	xml::node_ptr		inXML,
	ZIPLocalFileHeader&	outFileHeader)
{
	xml::document doc(inXML);
	
	outFileHeader.data.clear();
	
	stringstream s;
	s << doc;
	deflate(s.str(), outFileHeader);
}

}

MePubDocument::MePubDocument(
	const fs::path&		inProjectFile)
	: MDocument(inProjectFile)
	, eCreateItem(this, &MePubDocument::CreateItem)
	, eItemMoved(this, &MePubDocument::ItemMoved)
	, mInputFileStream(nil)
	, mRoot("", nil)
	, mTOC("", nil)
{
	RevertDocument();
}

//MePubDocument::MePubDocument(
//	const fs::path&		inParentDir,
//	const std::string&	inName)
//{
//}

MePubDocument::~MePubDocument()
{
	
}

bool MePubDocument::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	return MDocument::UpdateCommandStatus(inCommand, inMenu, inItemIndex, outEnabled, outChecked);
}

bool MePubDocument::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	return MDocument::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
}

void MePubDocument::ReadFile(
	std::istream&		inFile)
{
	archive* archive = archive_read_new();
	if (archive == nil)
		THROW(("Failed to create archive object"));
	
	mInputFileStream = &inFile;
	
	// TODO: check the magic number of the file
	
	const char kMagicPK[] = "PK";
	
	char pk[sizeof(kMagicPK)] = "";
	char mt[sizeof(kMagicMT)] = "";

	streamsize r = inFile.readsome(pk, strlen(kMagicPK));
	if (r != static_cast<streamsize>(strlen(kMagicPK)) or strcmp(kMagicPK, pk) != 0)
		THROW(("File is not an ePub"));
	
	inFile.seekg(30, ios_base::beg);
	r = inFile.readsome(mt, strlen(kMagicMT));
	if (r != static_cast<streamsize>(strlen(kMagicMT)) or strcmp(kMagicMT, mt) != 0)
		THROW(("File is not an ePub"));
	
	// reset the pointer
	
	inFile.seekg(0, ios_base::beg);
	
	try
	{
		int err = archive_read_support_compression_none(archive);
		if (err == ARCHIVE_OK)
			err = archive_read_support_compression_gzip(archive);
		if (err == ARCHIVE_OK)
			err = archive_read_support_format_zip(archive);
		
		if (err != ARCHIVE_OK)
			THROW(("Error initializing libarchive: %s", 
				archive_error_string(archive)));
		
		err = archive_read_open(archive, this,
			&MePubDocument::archive_open_callback_cb,
			&MePubDocument::archive_read_callback_cb,
			&MePubDocument::archive_close_callback_cb);
		
		// OK, so we've opened our epub file successfully by now.
		// Next thing is to walk the contents and store all the data.
		
		map<fs::path,string> content;
		fs::path rootFile;
		
		for (;;)
		{
			archive_entry* entry;
			
			err = archive_read_next_header(archive, &entry);
			if (err != ARCHIVE_OK)	// done
				break;
			
			fs::path path(archive_entry_pathname(entry));
			
			uint32 l = 0;
			char buffer[1024];
			string s;
			
			while ((r = archive_read_data(archive, buffer, sizeof(buffer))) > 0)
			{
				s.append(buffer, buffer + r);
				l += r;
			}
			
			if (r < 0)
				THROW(("Error reading archive: %s", archive_error_string(archive)));
			
			if (path == "mimetype")
			{
				if (not ba::starts_with(s, "application/epub+zip"))
					THROW(("Invalid ePub file, mimetype is not correct"));
			}
			else if (path == "META-INF/container.xml")
			{
				xml::document container(s);
				xml::node_ptr root = container.root();
				
				if (root->name() != "container" or root->ns() != kContainerNS)
					THROW(("Invalid or unsupported container.xml file"));
				
				xml::node_ptr n = root->find_first_child("rootfiles");
				if (not n)
					THROW(("Invalid container.xml file, <rootfiles> missing."));
				
				n = n->find_first_child("rootfile");
				if (not n)
					THROW(("Invalid container.xml file, <rootfile> missing."));
				
				if (n->get_attribute("media-type") != "application/oebps-package+xml")
					THROW(("Invalid container.xml file, first rootfile should be of type \"application/oebps-package+xml\""));
				
				rootFile = n->get_attribute("full-path");
				mRootFile = rootFile.leaf();
			}
			else if (S_ISREG(archive_entry_filetype(entry)))
				content[path] = s;
		}

		xml::document opf(content[rootFile]);
		
		ParseOPF(rootFile.branch_path(), *opf.root());
		
		fs::path tocFile = rootFile.branch_path() / mTOCFile;
		xml::document ncx(content[tocFile]);
		
		ParseNCX(*ncx.root());
		
		// and now fill in the data for the items we've found
		
		for (map<fs::path,string>::iterator item = content.begin(); item != content.end(); ++item)
		{
			if (item->first == rootFile or item->first == tocFile)
				continue;
			
			MProjectItem* pi = mRoot.GetItem(item->first);
			if (pi == nil)
//				THROW(("Missing item in manifest for %s", item->first.string().c_str()));
				continue;

			MePubItem* epi = dynamic_cast<MePubItem*>(pi);
			if (epi == nil)
				THROW(("Internal error, item is not an ePub item"));
			epi->SetData(item->second);
		}
		
		if (err != ARCHIVE_EOF)
			THROW(("Error reading archive: %s", archive_error_string(archive)));
	}
	catch (...)
	{
		if (archive != nil)
		{
			archive_read_close(archive);
			archive_read_finish(archive);
		}

		mInputFileStream = nil;
		
		throw;
	}	
	
	mInputFileStream = nil;

	archive_read_close(archive);
	archive_read_finish(archive);
	
	SetModified(false);
}

void MePubDocument::WriteFile(
	std::ostream&		inFile)
{
	// sanity check first
	
	if (mRoot.Count() != 1 or dynamic_cast<MProjectGroup*>(mRoot.GetItem(0)) == nil)
		THROW(("An ePub file should have a single directory in the root directory"));
	
	MProjectGroup* oebps = static_cast<MProjectGroup*>(mRoot.GetItem(0));
	fs::path oebpsPath(oebps->GetName());

	fs::path rootFile = oebpsPath / mRootFile;
	fs::path tocFile = oebpsPath / mTOCFile;
	
	// OK, write the file
	
	ZIPLocalFileHeader fh;
	vector<ZIPCentralDirectory> dir;
	ZIPCentralDirectory cd;
	
	// first write out the mimetype, uncompressed
	
	fh.deflated = false;
	fh.data = kEPubMimeType;
	fh.compressed_size = fh.uncompressed_size = fh.data.length();
	fh.crc = crc32(0, reinterpret_cast<const uint8*>(fh.data.c_str()), fh.data.length());
	fh.filename = "mimetype";
	
	cd.offset = inFile.tellp();
	inFile << fh;
	cd.file = fh;
	dir.push_back(cd);
	
	// write META-INF/ directory
	
	fh.filename = "META-INF/";
	fh.deflated = false;
	fh.compressed_size = fh.uncompressed_size = 0;
	fh.data.clear();
	fh.crc = 0;

	cd.offset = inFile.tellp();
	inFile << fh;
	cd.file = fh;
	dir.push_back(cd);
	
	// write META-INF/container.xml
	
	fh.filename = "META-INF/container.xml";
	
	xml::node_ptr container(new xml::node("container"));
	container->add_attribute("version", "1.0");
	container->add_attribute("xmlns", kContainerNS);
	xml::node_ptr rootfiles(new xml::node("rootfiles"));
	container->add_child(rootfiles);
	xml::node_ptr rootfile(new xml::node("rootfile"));
	rootfile->add_attribute("full-path", rootFile.string());
	rootfile->add_attribute("media-type", "application/oebps-package+xml");
	rootfiles->add_child(rootfile);
	deflate(container, fh);

	cd.offset = inFile.tellp();
	inFile << fh;
	cd.file = fh;
	dir.push_back(cd);

	// write renditions root directory (typically OEBPS)

	fh.filename = oebps->GetName() + '/';
	fh.deflated = false;
	fh.compressed_size = fh.uncompressed_size = 0;
	fh.data.clear();
	fh.crc = 0;

	cd.offset = inFile.tellp();
	inFile << fh;
	cd.file = fh;
	dir.push_back(cd);
	
	// the OPF...
	
	fh.filename = rootFile.string();
	xml::node_ptr opf = CreateOPF(oebpsPath);
	deflate(opf, fh);

	cd.offset = inFile.tellp();
	inFile << fh;
	cd.file = fh;
	dir.push_back(cd);
	
	// the NCX...
	
	fh.filename = tocFile.string();
	xml::node_ptr ncx = CreateNCX();
	deflate(ncx, fh);

	cd.offset = inFile.tellp();
	inFile << fh;
	cd.file = fh;
	dir.push_back(cd);
	
	// rest of the items
	
	for (MProjectGroup::iterator item = mRoot.begin(); item != mRoot.end(); ++item)
	{
		MProjectGroup* group = dynamic_cast<MProjectGroup*>(&*item);
		if (group != nil)
		{
			fs::path path(group->GetName());
			while (group->GetParent() != nil)
			{
				group = group->GetParent();
				if (not group->GetName().empty())
					path = group->GetName() / path;
			}
			
			if (path == "META-INF" or path == rootFile.branch_path())
				continue;
			
			fh.filename = path.string() + '/';
			fh.deflated = false;
			fh.compressed_size = fh.uncompressed_size = 0;
			fh.data.clear();
			fh.crc = 0;
	
			cd.offset = inFile.tellp();
			inFile << fh;
			cd.file = fh;
			dir.push_back(cd);

			continue;
		}
		
		MePubItem* file = dynamic_cast<MePubItem*>(&*item);
		if (file != nil)
		{
			fh.filename = file->GetPath().string();
			
			deflate(file->GetData(), fh);
	
			cd.offset = inFile.tellp();
			inFile << fh;
			cd.file = fh;
			dir.push_back(cd);
			
			continue;
		}
	}
	
	// now write out directory
	
	ZIPEndOfCentralDirectory end;
	
	end.entries = dir.size();
	end.directory_offset = inFile.tellp();
	
	for (vector<ZIPCentralDirectory>::iterator e = dir.begin(); e != dir.end(); ++e)
		inFile << *e;
	
	end.directory_size = inFile.tellp() - static_cast<streamoff>(end.directory_offset);
	inFile << end;
}

xml::node_ptr MePubDocument::CreateOPF(
	const fs::path&		inOEBPS)
{
	xml::node_ptr opf(new xml::node("package"));
	opf->add_attribute("version", "2.0");
	opf->add_attribute("unique-identifier", "BookID");
	opf->add_attribute("xmlns", "http://www.idpf.org/2007/opf");
	
	// metadata block
	xml::node_ptr metadata(new xml::node("metadata"));
	metadata->add_attribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
	metadata->add_attribute("xmlns:opf", "http://www.idpf.org/2007/opf");
	opf->add_child(metadata);
	
	// the identifier
	xml::node_ptr identifier(new xml::node("identifier", "dc"));
	identifier->add_attribute("id", "BookID");
	identifier->add_attribute("opf:scheme", mDocumentIDScheme);
	identifier->content(mDocumentID);
	metadata->add_child(identifier);
	
	// other dublin core data
	for (map<string,string>::iterator dc = mDublinCore.begin(); dc != mDublinCore.end(); ++dc)
	{
		xml::node_ptr dcn(new xml::node(dc->first, "dc"));
		if (dc->first == "creator")
			dcn->add_attribute("opf:role", "aut");
		dcn->content(dc->second);
		metadata->add_child(dcn);
	}
	
	// manifest
	xml::node_ptr manifest(new xml::node("manifest"));
	opf->add_child(manifest);

	for (MProjectGroup::iterator item = mRoot.begin(); item != mRoot.end(); ++item)
	{
		MePubItem* ePubItem = dynamic_cast<MePubItem*>(&*item);
		
		if (ePubItem == nil)
			continue;
		
		xml::node_ptr item_node(new xml::node("item"));
		item_node->add_attribute("id", ePubItem->GetID());
		item_node->add_attribute("href", relative_path(inOEBPS, ePubItem->GetPath()).string());
		item_node->add_attribute("media-type", ePubItem->GetMediaType());
		manifest->add_child(item_node);
	}
	
	// add the ncx to the manifest too
	
	xml::node_ptr item_node(new xml::node("item"));
	item_node->add_attribute("id", "ncx");
	item_node->add_attribute("href", mTOCFile.string());
	item_node->add_attribute("media-type", "application/x-dtbncx+xml");
	manifest->add_child(item_node);
	
	// spine. For now we simply write out all file ID's for files having media-type application/xhtml+xml 

	xml::node_ptr spine(new xml::node("spine"));
	spine->add_attribute("toc", "ncx");
	opf->add_child(spine);
	
	for (MProjectGroup::iterator item = mRoot.begin(); item != mRoot.end(); ++item)
	{
		MePubItem* ePubItem = dynamic_cast<MePubItem*>(&*item);
		
		if (ePubItem == nil or ePubItem->GetMediaType() != "application/xhtml+xml")
			continue;
		
		xml::node_ptr itemref(new xml::node("itemref"));
		itemref->add_attribute("idref", ePubItem->GetID());
		spine->add_child(itemref);
	}
	
	return opf;
}

xml::node_ptr MePubDocument::CreateNCX()
{
	xml::node_ptr ncx(new xml::node("ncx"));
	ncx->add_attribute("xmlns", "http://www.daisy.org/z3986/2005/ncx/");
	ncx->add_attribute("version", "2005-1");
	ncx->add_attribute("xml:lang", mDublinCore["language"]);

	// the required head values
	xml::node_ptr head(new xml::node("head"));
	ncx->add_child(head);
	
	xml::node_ptr meta(new xml::node("meta"));
	meta->add_attribute("name", "dtb:uid");
	meta->add_attribute("content", mDocumentID);
	head->add_child(meta);
	
	meta.reset(new xml::node("meta"));
	meta->add_attribute("name", "dtb:depth");
	meta->add_attribute("content", boost::lexical_cast<string>(mTOC.GetDepth() - 1));
	head->add_child(meta);
	
	meta.reset(new xml::node("meta"));
	meta->add_attribute("name", "dtb:totalPageCount");
	meta->add_attribute("content", "0");
	head->add_child(meta);
	
	meta.reset(new xml::node("meta"));
	meta->add_attribute("name", "dtb:maxPageNumber");
	meta->add_attribute("content", "0");
	head->add_child(meta);
	
	// the title
	
	xml::node_ptr docTitle(new xml::node("docTitle"));
	xml::node_ptr text(new xml::node("text"));
	text->content(mDublinCore["title"]);
	docTitle->add_child(text);
	ncx->add_child(docTitle);
	
	// the navMap
	
	xml::node_ptr navMap(new xml::node("navMap"));
	ncx->add_child(navMap);
	
	uint32 id = 0;
	CreateNavMap(&mTOC, navMap, id);
	
	return ncx;
}

void MePubDocument::CreateNavMap(
	MProjectGroup*		inGroup,
	xml::node_ptr		inNavPoint,
	uint32&				ioID)
{
	for (int32 ix = 0; ix < inGroup->Count(); ++ix)
	{
		MePubTOCItem* tocItem = dynamic_cast<MePubTOCItem*>(inGroup->GetItem(ix));
		THROW_IF_NIL(tocItem);
		
		xml::node_ptr navPoint(new xml::node("navPoint"));
		navPoint->add_attribute("id", string("id-") + boost::lexical_cast<string>(ioID));
		navPoint->add_attribute("class", tocItem->GetClass());
		navPoint->add_attribute("playOrder", boost::lexical_cast<string>(ioID));
		++ioID;
		
		xml::node_ptr navLabel(new xml::node("navLabel"));
		xml::node_ptr text(new xml::node("text"));
		text->content(tocItem->GetName());
		navLabel->add_child(text);
		navPoint->add_child(navLabel);
		
		xml::node_ptr content(new xml::node("content"));
		content->add_attribute("src", tocItem->GetSrc());
		navPoint->add_child(content);
		
		if (tocItem->Count() > 0)
			CreateNavMap(tocItem, navPoint, ioID);
		
		inNavPoint->add_child(navPoint);
	}	
}	

ssize_t MePubDocument::archive_read_callback_cb(
	struct archive*		inArchive,
	void*				inClientData,
	const void**		_buffer)
{
	MePubDocument* epub = reinterpret_cast<MePubDocument*>(inClientData);
	return epub->archive_read_callback(inArchive, _buffer);
}

int MePubDocument::archive_open_callback_cb(
	struct archive*		inArchive,
	void*				inClientData)
{
	MePubDocument* epub = reinterpret_cast<MePubDocument*>(inClientData);
	return epub->archive_open_callback(inArchive);
}

int MePubDocument::archive_close_callback_cb(
	struct archive*		inArchive,
	void*				inClientData)
{
	MePubDocument* epub = reinterpret_cast<MePubDocument*>(inClientData);
	return epub->archive_close_callback(inArchive);
}

ssize_t MePubDocument::archive_read_callback(
	struct archive*		inArchive,
	const void**		_buffer)
{
	ssize_t r = mInputFileStream->readsome(mBuffer, sizeof(mBuffer));
	*_buffer = mBuffer;
	return r;
}

int MePubDocument::archive_open_callback(
	struct archive*		inArchive)
{
	return ARCHIVE_OK;
}

int MePubDocument::archive_close_callback(
	struct archive*		inArchive)
{
	return ARCHIVE_OK;
}

MProjectGroup* MePubDocument::GetFiles() const
{
	return const_cast<MProjectGroup*>(&mRoot);
}

MProjectGroup* MePubDocument::GetTOC() const
{
	return const_cast<MProjectGroup*>(&mTOC);
}

void MePubDocument::ParseOPF(
	const fs::path&		inDirectory,
	xml::node&			inOPF)
{
	// fetch the unique-identifier
	string uid = inOPF.get_attribute("unique-identifier");
	if (uid.empty())
		THROW(("Unique Identifier is missing in OPF"));
	
	// fetch the meta data
	xml::node_ptr metadata = inOPF.find_first_child("metadata");
	if (not metadata)
		THROW(("Metadata is missing from OPF"));

	// collect all the Dublin Core information
	
	for (xml::node_ptr dc = metadata->children(); dc; dc = dc->next())
	{
		if (dc->ns() != "http://purl.org/dc/elements/1.1/")
			continue;
		
		if (dc->name() == "identifier")
		{
			if (dc->get_attribute("id") == uid)
			{
				mDocumentID = dc->content();
				mDocumentIDScheme = dc->get_attribute("scheme");
			}
		}
		else if (mDublinCore[dc->name()].empty())
			mDublinCore[dc->name()] = dc->content();
		else
			mDublinCore[dc->name()] = mDublinCore[dc->name()] + "; " + dc->content();
	}
	
	// collect the items from the manifest
	
	xml::node_ptr manifest = inOPF.find_first_child("manifest");
	if (not manifest)
		THROW(("Manifest missing from OPF document"));
	
	for (xml::node_ptr item = manifest->children(); item; item = item->next())
	{
		if (item->name() != "item" or item->ns() != "http://www.idpf.org/2007/opf")
			continue;

		fs::path href = inDirectory / item->get_attribute("href");
		
		if (mTOCFile.empty() and item->get_attribute("media-type") == "application/x-dtbncx+xml")
		{
			mTOCFile = relative_path(inDirectory, href);
			continue;
		}
		
		MProjectGroup* group = mRoot.GetGroupForPath(href.branch_path());
		
		auto_ptr<MePubItem> eItem(new MePubItem(href.leaf(), group));
		
		eItem->SetID(item->get_attribute("id"));
		eItem->SetMediaType(item->get_attribute("media-type"));
		
		group->AddProjectItem(eItem.release());
	}
	
	// the spine
	
	xml::node_ptr spine = inOPF.find_first_child("spine");
	if (not spine)
		THROW(("Spine missing from OPF document"));
	
	for (xml::node_ptr item = spine->children(); item; item = item->next())
	{
		if (item->name() != "itemref" or item->ns() != "http://www.idpf.org/2007/opf")
			continue;
		
		string idref = item->get_attribute("idref");
		mSpine.push_back(idref);
	}

	// sanity checks
	if (mDocumentID.empty())
		THROW(("Missing document identifier in metadata section"));
}

void MePubDocument::ParseNCX(
	xml::node&			inNCX)
{
	xml::node_ptr navMap = inNCX.find_first_child("navMap");
	if (not navMap)
		THROW(("Missing navMap element in NCX file"));

	for (xml::node_ptr n = navMap->children(); n; n = n->next())
	{
		if (n->name() == "navPoint")
			ParseNavPoint(&mTOC, n);
	}
}

void MePubDocument::ParseNavPoint(
	MProjectGroup*		inGroup,
	xml::node_ptr		inNavPoint)
{
	assert(inNavPoint->name() == "navPoint");
	
	xml::node_ptr label = inNavPoint->find_first_child("navLabel");
	if (not label)
		THROW(("Missing navLabel in navPoint"));

	xml::node_ptr name = label->find_first_child("text");
	if (not name)
		THROW(("Missing text in navLabel"));
	
	auto_ptr<MePubTOCItem> np(new MePubTOCItem(name->content(), inGroup));

	xml::node_ptr content = inNavPoint->find_first_child("content");
	if (not content)
		THROW(("Missing content in navPoint"));
	
	np->SetSrc(content->get_attribute("src"));

	np->SetClass(inNavPoint->get_attribute("class"));
	
	for (xml::node_ptr n = inNavPoint->children(); n; n = n->next())
	{
		if (n->name() == "navPoint")
			ParseNavPoint(np.get(), n);
	}
	
	inGroup->AddProjectItem(np.release());
}

string MePubDocument::GetDocumentID() const
{
	return mDocumentID;
}

void MePubDocument::SetDocumentID(
	const string&		inID)
{
	mDocumentID = inID;
	// TODO: check scheme and modify mDocumentIDScheme
	SetModified(true);
}

string MePubDocument::GetDublinCoreValue(
	const string&		inName) const
{
	map<string,string>::const_iterator dc = mDublinCore.find(inName);
	
	string result;
	
	if (dc != mDublinCore.end())
		result = dc->second;
	
	return result;
}

void MePubDocument::SetDublinCoreValue(
	const string&		inName,
	const string&		inValue)
{
	mDublinCore[inName] = inValue;
	SetModified(true);
}

string MePubDocument::GetFileData(
	const fs::path&		inFile)
{
	MProjectItem* pi = mRoot.GetItem(inFile);
	if (pi == nil)
		THROW(("ePub item %s does not exist", inFile.string().c_str()));

	MePubItem* epi = dynamic_cast<MePubItem*>(pi);
	if (epi == nil)
		THROW(("Error, item is not an editable file"));

	return epi->GetData();
}

void MePubDocument::SetFileData(
	const fs::path&		inFile,
	const string&		inText)
{
	MProjectItem* pi = mRoot.GetItem(inFile);
	if (pi == nil)
		THROW(("ePub item %s does not exist", inFile.string().c_str()));

	MePubItem* epi = dynamic_cast<MePubItem*>(pi);
	if (epi == nil)
		THROW(("Error, item is not an editable file"));

	epi->SetData(inText);
	
	epi->SetOutOfDate(true);
	SetModified(true);
}

void MePubDocument::SetModified(
	bool				inModified)
{
	if (inModified == false)
		mRoot.SetOutOfDate(false);
	
	MDocument::SetModified(inModified);
}

// ---------------------------------------------------------------------------
//	MePubDocument::CreateNewGroup

void MePubDocument::CreateNewGroup(
	const string&		inGroupName,
	MProjectGroup*		inGroup,
	int32				inIndex)
{
	MProjectGroup* newGroup = new MProjectGroup(inGroupName, inGroup);
	inGroup->AddProjectItem(newGroup, inIndex);
	SetModified(true);
	eInsertedFile(newGroup);
}

void MePubDocument::CreateItem(
	const string&		inFile,
	MProjectGroup*		inGroup,
	MProjectItem*&		outItem)
{
	fs::path path(MUrl(inFile).GetPath());
	string name = path.leaf();
	
	if (not fs::exists(path))
		THROW(("File %s does not exist?", inFile.c_str()));
	
	if (fs::is_directory(path))
	{
		SetModified(true);

		MProjectGroup* group = new MProjectGroup(name, inGroup);
		int32 index = 0;
		
		vector<string> files;
		MFileIterator iter(path, kFileIter_ReturnDirectories);
		while (iter.Next(path))
		{
			MProjectItem* item = nil;
			CreateItem(path.string(), group, item);
			if (item != nil)
			{
				group->AddProjectItem(item, index);
				++index;
			}
		}

		outItem = group;
	}
	else
	{
		auto_ptr<MePubItem> item(new MePubItem(name, inGroup));
		
		bool isText = false;
		
		if (FileNameMatches("*.css", name))
		{
			item->SetMediaType("text/css");
			isText = true;
		}
		else if (FileNameMatches("*.xml;*.html", name))
		{
			item->SetMediaType("application/xhtml+xml");
			isText = true;
		}
		else if (FileNameMatches("*.ncx", name))
		{
			item->SetMediaType("application/x-dtbncx+xml");
			isText = true;
		}
		else if (FileNameMatches("*.ttf", name))
			item->SetMediaType("application/x-font-ttf");
		else if (FileNameMatches("*.xpgt", name))
			item->SetMediaType("application/vnd.adobe-page-template+xml");
		else if (FileNameMatches("*.png", name))
			item->SetMediaType("image/png");
		else if (FileNameMatches("*.jpg", name))
			item->SetMediaType("image/jpeg");
		else 
			item->SetMediaType("application/xhtml+xml");

		fs::ifstream file(path);

		if (isText)
		{
			MTextBuffer buffer;
			buffer.ReadFromFile(file);
			item->SetData(buffer.GetText());
		}
		else
		{
			// First read the data into a buffer
			streambuf* b = file.rdbuf();
			
			int64 len = b->pubseekoff(0, ios::end);
			b->pubseekpos(0);
		
			if (len < 0)
				THROW(("File is not open?"));
		
			if (len > numeric_limits<uint32>::max())
				THROW(("File too large to open"));
		
			vector<char> data(len);
			b->sgetn(&data[0], len);
			
			item->SetData(string(&data[0], len));
		}

		item->SetID(fs::basename(name));

		outItem = item.release();
	}
}

void MePubDocument::ItemMoved()
{
	SetModified(true);
}

