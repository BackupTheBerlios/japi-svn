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
#include <boost/algorithm/string.hpp>

#include "document.hpp"

#include "MError.h"
#include "MePubDocument.h"
#include "MePubItem.h"

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
	xml::node_ptr		inXML,
	ZIPLocalFileHeader&	outFileHeader)
{
	xml::document doc(inXML);
	
	outFileHeader.data.clear();
	
	stringstream s;
	s << doc;
	string xml = s.str();
	
	const uint32 kBufferSize = 4096;
	vector<uint8> b(kBufferSize);
	unsigned char* buffer = &b[0];
	
	z_stream_s z_stream = {};

	int err = deflateInit(&z_stream, Z_DEFAULT_COMPRESSION);
	if (err != Z_OK)
		THROW(("Compressor error: %s", z_stream.msg));

	z_stream.avail_in = xml.length();
	z_stream.next_in = const_cast<unsigned char*>(
		reinterpret_cast<const unsigned char*>(xml.c_str()));
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
	
	outFileHeader.crc = crc32(0, reinterpret_cast<const uint8*>(xml.c_str()), xml.length());
	outFileHeader.compressed_size = outFileHeader.data.length();
	outFileHeader.uncompressed_size = xml.length();
	outFileHeader.deflated = true;
	
	deflateEnd(&z_stream);

#if 1
	vector<uint8> b2(xml.length());
	
	inflateInit(&z_stream);
	
	z_stream.avail_in = outFileHeader.data.length();
	z_stream.next_in = (uint8*)outFileHeader.data.c_str();
	z_stream.total_in = 0;
	
	z_stream.avail_out = xml.length();
	z_stream.next_out = &b2[0];
	z_stream.total_out = 0;
	
	err = inflate(&z_stream, Z_FINISH);
	if (err != Z_OK and err != Z_STREAM_END)
		THROW(("inflate failed: %s (%d)", z_stream.msg, err));

	if (xml != (char*)&b2[0])
		THROW(("inflate failed, not the same"));

cout << "inflate was ok, result is :" << endl
	 << xml << endl << endl;

cout << hex << xml.length() << endl
	 << hex << outFileHeader.data.length() << endl
	 << hex << outFileHeader.crc << endl;
	
	inflateEnd(&z_stream);
#endif
}	

}

MePubDocument::MePubDocument(
	const fs::path&		inProjectFile)
	: MDocument(inProjectFile)
	, mInputFileStream(nil)
	, mRoot("", nil)
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
				
				mRootFile = n->get_attribute("full-path");
			}
			else if (S_ISREG(archive_entry_filetype(entry)))
				content[path] = s;
		}

		xml::document opf(content[mRootFile]);
		
		ParseOPF(mRootFile.branch_path(), *opf.root());
		
		// and now fill in the data for the items we've found
		
		for (map<fs::path,string>::iterator item = content.begin(); item != content.end(); ++item)
		{
			if (item->first == mRootFile)
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
		
//		cout << mContent[mRootFile] << endl;
		
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
	try
	{
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
		rootfile->add_attribute("full-path", mRootFile.string());
		rootfile->add_attribute("media-type", "application/oebps-package+xml");
		rootfiles->add_child(rootfile);
		deflate(container, fh);

		cd.offset = inFile.tellp();
		inFile << fh;
		cd.file = fh;
		dir.push_back(cd);
		
		// rest of the items
		
		vector<MProjectItem*> items;
		mRoot.Flatten(items);
		
		
		
		// now write out directory
		
		ZIPEndOfCentralDirectory end;
		
		end.entries = dir.size();
		end.directory_offset = inFile.tellp();
		
		for (vector<ZIPCentralDirectory>::iterator e = dir.begin(); e != dir.end(); ++e)
			inFile << *e;
		
		end.directory_size = inFile.tellp() - static_cast<streamoff>(end.directory_offset);
		inFile << end;
	}
	catch (...)
	{
	
		throw;	
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

ssize_t MePubDocument::archive_write_callback_cb(
	struct archive*		inArchive,
	void*				inClientData,
	const void*			_buffer,
	size_t				_length)
{
	MePubDocument* epub = reinterpret_cast<MePubDocument*>(inClientData);
	return epub->archive_write_callback(inArchive, _buffer, _length);
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

ssize_t MePubDocument::archive_write_callback(
	struct archive*		inArchive,
	const void*			_buffer,
	size_t				_length)
{
	cout << "write callback" << endl;
	return 0;
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
		
		MProjectGroup* group = mRoot.GetGroupForPath(href.branch_path());
		
		auto_ptr<MePubItem> eItem(new MePubItem(href.leaf(), group, href.branch_path()));
		
		eItem->SetID(item->get_attribute("id"));
		eItem->SetMediaType(item->get_attribute("media-type"));
		
		group->AddProjectItem(eItem.release());
	}

	// sanity checks
	if (mDocumentID.empty())
		THROW(("Missing document identifier in metadata section"));
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
