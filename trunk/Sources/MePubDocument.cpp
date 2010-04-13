//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <zlib.h>
#include <uuid/uuid.h>
#include <set>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>

#include "boost/archive/iterators/base64_from_binary.hpp"
#include "boost/archive/iterators/binary_from_base64.hpp"
#include "boost/archive/iterators/transform_width.hpp"

#include <boost/foreach.hpp>

#include <openssl/pem.h>
#include <openssl/aes.h>

#include <zeep/xml/document.hpp>

#include "MError.h"
#include "MTextDocument.h"
#include "MePubDocument.h"
#include "MePubItem.h"
#include "MePubContentFile.h"
#include "MTextBuffer.h"
#include "MFile.h"
#include "MResources.h"
#include "MStrings.h"
#include "MGlobals.h"
#include "MAlerts.h"
#include "MUtils.h"
#include "MMessageWindow.h"

#include "MXHTMLTools.h"
#include "MePubServer.h"

#define foreach BOOST_FOREACH

using namespace std;
namespace xml = zeep::xml;
namespace ba = boost::algorithm;
namespace io = boost::iostreams;
namespace bai = boost::archive::iterators;

// --------------------------------------------------------------------

namespace {
	
const uint32
	kZipLengthAtEndMask	= 8;

const char
	kEPubMimeType[] = "application/epub+zip",
	kMagicMT[] = "mimetypeapplication/epub+zip",
	kContainerNS[] = "urn:oasis:names:tc:opendocument:xmlns:container",
	kAdobeAdeptNS[] = "http://ns.adobe.com/adept";

struct ZIPLocalFileHeader
{
						ZIPLocalFileHeader()
						{
							time_t now = time(nil);
							
							struct tm tm = {};
							gmtime_r(&now, &tm);
						
							file_mod_date =
								(((tm.tm_year - 1980) & 0x7f) << 9) |
								(((tm.tm_mon + 1)     & 0x0f) << 5) |
								( (tm.tm_mday         & 0x1f) << 0);
							
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
	*buffer++ = value & 0x0FF; value >>= 8;
	*buffer++ = value & 0x0FF;

	return buffer;
}

inline char* write(uint32 value, char* buffer)
{
	*buffer++ = value & 0x0FF; value >>= 8;
	*buffer++ = value & 0x0FF; value >>= 8;
	*buffer++ = value & 0x0FF; value >>= 8;
	*buffer++ = value & 0x0FF;

	return buffer;
}

inline char* read(uint16& value, char* buffer)
{
	char* p = buffer + sizeof(value);
	
	value <<= 8; value |= static_cast<uint8>(*--p);
	value <<= 8; value |= static_cast<uint8>(*--p);
	
	return buffer + sizeof(value);
}

inline char* read(uint32& value, char* buffer)
{
	char* p = buffer + sizeof(value);

	value <<= 8; value |= static_cast<uint8>(*--p);
	value <<= 8; value |= static_cast<uint8>(*--p);
	value <<= 8; value |= static_cast<uint8>(*--p);
	value <<= 8; value |= static_cast<uint8>(*--p);

	return buffer + sizeof(value);
}

const uint32
	kLocalFileHeaderSignature = 0x04034b50UL,
	kCentralDirectorySignature = 0x02014b50,
	kEndOfCentralDirectorySignature = 0x06054b50;

const uint16
	kVersionMadeBy = 0x0017;

uint32 write_next_file(ostream& lhs, ZIPLocalFileHeader& rhs)
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
	
	uint32 result = sizeof(b) + fileNameLength + rhs.data.length();
	
	rhs.data.clear();
	
	return result;
}

bool read_next_file(istream& s, ZIPLocalFileHeader& fh)
{
	char b[30];
	
	s.read(b, 4);

	if (b[0] != 'P' or b[1] != 'K')
		THROW(("Invalid ePub file"));
	
	if (b[2] == '0' and b[3] == '0')			// split
		THROW(("Split ePub archives are not supported"));
	
	if ((b[2] == '\001' and b[3] == '\002') or	// central directory
		(b[2] == '\005' and b[3] == '\006'))	// end of archive record
	{
		return false;
	}

	// now it must be a regular file entry, otherwise we bail out
	if (not (b[2] == '\003' and b[3] == '\004'))
		THROW(("Invalid ePub file, perhaps it is damaged"));

	s.read(b + 4, sizeof(b) - 4);

	uint16 versionNeededToExtract, bitFlag, compressionMethod, fileNameLength, extraFieldLength;
	
	char* p = read(versionNeededToExtract, b + 4);
	p = read(bitFlag, p);
	p = read(compressionMethod, p);
	p = read(fh.file_mod_time, p);
	p = read(fh.file_mod_date, p);
	p = read(fh.crc, p);
	p = read(fh.compressed_size, p);
	p = read(fh.uncompressed_size, p);
	p = read(fileNameLength, p);
	p = read(extraFieldLength, p);

	// sanity checks
	if (compressionMethod != 0 and compressionMethod != 8)
		THROW(("Unsupported compression method used in ePub file"));

	assert(p == b + sizeof(b));
	
	// read file name
	
	vector<char> fn(fileNameLength);
	s.read(&fn[0], fileNameLength);
	fh.filename.assign(&fn[0], fileNameLength);	
	
	// skip over the extra data
	s.seekg(extraFieldLength, ios::cur);
	
	// OK, now read in the data and inflate if required
	fh.data.clear();

	// save the offset in the stream to be able to seek back if needed
	std::streamsize offset = s.tellg();
	
	if (compressionMethod == 0) // no compression
	{
		if (fh.compressed_size > 0)
		{
			vector<char> b(fh.compressed_size);
			s.read(&b[0], fh.compressed_size);
			
			fh.data.assign(&b[0], fh.compressed_size);
		}
	}
	else	 // inflate
	{
		if (fh.compressed_size == 0 and not (bitFlag & kZipLengthAtEndMask))
			THROW(("Invalid ePub file, missing compressed size"));
		
		// setup a boost::iostreams pair to read the compressed data
		io::zlib_params params;
		params.noheader = true;	// don't read header, i.e. true deflate compression
		params.calculate_crc = true;
		
		io::zlib_decompressor z_stream(params);
		
		io::filtering_streambuf<io::input> in;
		in.push(z_stream);
		in.push(s);
		
		io::filtering_ostream out(io::back_inserter(fh.data));
		io::copy(in, out);
		
		s.seekg(offset + z_stream.filter().total_in(), ios::beg);
		
		// now read the trailing length if needed
		
		if (bitFlag & kZipLengthAtEndMask)
		{
			char b2[16];
			s.read(b2, sizeof(b2));

			uint32 signature;
			char* p = read(signature, b2);
			
			if (signature == 0x08074b50UL)
				p = read(fh.crc, p);
			else
			{
				fh.crc = signature;
				s.seekg(-4, ios::cur);
			}
			
			p = read(fh.compressed_size, p);
			p = read(fh.uncompressed_size, p);
		}
		
		// OK, so decompression succeeded, now validate the data

//		if (compressed_size != fh.compressed_size)
//			THROW(("ePub data, compressed data is wrong size"));

		if (static_cast<uint32>(z_stream.total_out()) != fh.uncompressed_size)
			THROW(("ePub data, uncompressed data is wrong size"));
		
		if (z_stream.crc() != fh.crc)
			THROW(("ePub data, bad crc"));
	}
	
	return true;
}

uint32 write_central_directory(ostream& lhs, ZIPCentralDirectory& rhs)
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
	
	return sizeof(b) + fileNameLength;
}

void write_directory_end(ostream& lhs, ZIPEndOfCentralDirectory& rhs)
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
}

void deflate(
	const string&		inText,	
	ZIPLocalFileHeader&	outFileHeader)
{
	io::zlib_params params;
	params.noheader = true;			// don't read header, i.e. true deflate compression
	params.calculate_crc = true;
	
	outFileHeader.data.clear();
	
	io::zlib_compressor z_stream(params);

	stringstream in(inText);
	
	io::filtering_streambuf<io::output> out;
	out.push(z_stream);
	out.push(io::back_inserter(outFileHeader.data));
	
	io::copy(in, out);
	
	outFileHeader.crc = z_stream.crc();
	outFileHeader.compressed_size = outFileHeader.data.length();
	outFileHeader.uncompressed_size = inText.length();
	outFileHeader.deflated = true;
}	

void deflate(
	xml::element*		inXML,
	ZIPLocalFileHeader&	outFileHeader)
{
	xml::document doc;
	doc.child(inXML);
	
	outFileHeader.data.clear();
	
	stringstream s;
	s << doc;
	deflate(s.str(), outFileHeader);
}

void DecryptBookKey(
	const string&	inBase64Key,
	AES_KEY&		outBookKey)
{
	vector<uint8> adeptKey;
	decode_base64(inBase64Key, adeptKey);
	
	if (adeptKey.size() != 128)
		THROW(("Invalid key length, expected 128 bytes"));
	
	static RSA* rsa = nil;
	if (rsa == nil and fs::exists(gPrefsDir / "adeptkey.der"))
	{
		fs::ifstream adeptKeyFile(gPrefsDir / "adeptkey.der", ios::binary);
		if (adeptKeyFile.is_open())
		{
			streambuf* b = adeptKeyFile.rdbuf();
			
			int64 len = b->pubseekoff(0, ios::end);
			b->pubseekpos(0);
			
			vector<char> data(len);
			b->sgetn(&data[0], len);
		
			// put the data into a BIO
			BIO *mem = BIO_new_mem_buf(&data[0], len);
			rsa = d2i_RSAPrivateKey_bio(mem, nil);
			BIO_free(mem);
		}
	}

	if (rsa == nil)
		THROW(("Failed to read private ADEPT key from '%s'",
			(gPrefsDir / "adeptkey.der").string().c_str()));
	
	uint8 b[16];
	int r = RSA_private_decrypt(128, &adeptKey[0], b, rsa, RSA_PKCS1_PADDING);
	if (r != 16)
		THROW(("Error decrypting book key, expected 16, got %d", r));
	
	AES_set_decrypt_key(b, 128, &outBookKey);
}

void DecryptFile(
	string&			ioFile,
	const AES_KEY&	inKey)
{
	vector<char> b(ioFile.length());

	uint8* iv = reinterpret_cast<uint8*>(const_cast<char*>(ioFile.c_str()));
	uint8* src = iv + 16;
	
	AES_cbc_encrypt(src, (uint8*)&b[0], ioFile.length(), &inKey, iv, AES_DECRYPT);
	
	io::zlib_params params;
	params.noheader = true;	// don't read header, i.e. true deflate compression
	params.calculate_crc = true;
	io::zlib_decompressor z_stream(params);
	
	io::filtering_streambuf<io::input> in;
	in.push(z_stream);
	
	io::filtering_istream bin(boost::make_iterator_range(b));
	in.push(bin);
	
	ioFile.clear();
	io::filtering_ostream out(io::back_inserter(ioFile));
	io::copy(in, out);
}

string ISODate()
{
	time_t now = time(nil);
	
	struct tm tm = {};
	gmtime_r(&now, &tm);
	
	char s[11] = "";
	strftime(s, sizeof(s), "%Y-%m-%d", &tm);
	return s;
}

}

MePubDocument::MePubDocument(
	const MFile&		inFile)
	: MDocument(inFile)
	, eCreateItem(this, &MePubDocument::CreateItem)
	, eItemMoved(this, &MePubDocument::ItemMoved)
	, eItemRemoved(this, &MePubDocument::ItemMoved)
	, eItemRenamed(this, &MePubDocument::ItemRenamed)
	, mRoot("", nil)
	, mTOC("", nil)
{
	try
	{
		MePubServer::Instance();
	}
	catch (...) {}
}

MePubDocument::MePubDocument()
	: MDocument(MFile())
	, eCreateItem(this, &MePubDocument::CreateItem)
	, eItemMoved(this, &MePubDocument::ItemMoved)
	, eItemRemoved(this, &MePubDocument::ItemMoved)
	, eItemRenamed(this, &MePubDocument::ItemRenamed)
	, mRoot("", nil)
	, mTOC("", nil)
{
	try
	{
		MePubServer::Instance();
	}
	catch (...) {}
}

MePubDocument::~MePubDocument()
{
}

MePubDocument* MePubDocument::GetFirstEPubDocument()
{
	MePubDocument* result = nil;
	MDocument* doc = MDocument::GetFirstDocument();

	while (doc != nil and result == nil)
	{
		result = dynamic_cast<MePubDocument*>(doc);
		doc = doc->GetNextDocument();
	}
	
	return result;
}

MePubDocument* MePubDocument::GetNextEPubDocument()
{
	MePubDocument* result = nil;
	MDocument* doc = GetNextDocument();

	while (doc != nil and result == nil)
	{
		result = dynamic_cast<MePubDocument*>(doc);
		doc = doc->GetNextDocument();
	}
	
	return result;
}

void MePubDocument::AddDocument(
	MTextDocument*		inDocument)
{
	string name;

	if (inDocument->IsSpecified())
		name = inDocument->GetFile().GetFileName();
	else
		name = _("Untitled");

	MProjectGroup* oebps = dynamic_cast<MProjectGroup*>(mRoot.GetItem(0));
	
	MePubItem* item = new MePubItem(name, oebps);
	item->GuessMediaType();
	item->SetID("main");
	item->SetOutOfDate(true);
	oebps->AddProjectItem(item);
	
	eFileItemInserted(item);
	
	MFile file(new MePubContentFile(this, item->GetPath()));
	inDocument->DoSaveAs(file);
}

void MePubDocument::InitializeNew()
{
	mrsrc::rsrc rsrc("empty_xhtml_file.xhtml");
	if (not rsrc)
		THROW(("Missing xhtml resource"));
	
	MProjectGroup* oebps = new MProjectGroup("OEBPS", &mRoot);
	mRoot.AddProjectItem(oebps);
	
	mRootFile = "OEBPS/book.opf";
	mTOCFile = "OEBPS/book.ncx";
	
	MePubItem* item = new MePubItem("main.xhtml", oebps);
	item->SetData(string(rsrc.data(), rsrc.size()));
	item->GuessMediaType();
	item->SetID("main");
	oebps->AddProjectItem(item);
	
	MePubTOCItem* toc = new MePubTOCItem("Main", &mTOC);
	toc->SetSrc("main.xhtml");
	mTOC.AddProjectItem(toc);
	
	GenerateNewDocumentID();
	
	mDublinCore["title"] = "Untitled";
	mDublinCore["language"] = "en";
	mDublinCore["date"] = ISODate();
}

void MePubDocument::ImportOEB(
	const MFile&		inOEB)
{
	if (not inOEB.IsLocal())
		THROW(("import oeb works on local files only for now, sorry"));
	
	MProjectGroup* oebps = new MProjectGroup("OEBPS", &mRoot);
	mRoot.AddProjectItem(oebps);
	
	mRootFile = "OEBPS/book.opf";
	mTOCFile = "OEBPS/book.ncx";

	mDublinCore["date"] = ISODate();
	
	fs::ifstream opfFile(inOEB.GetPath(), ios::binary);
	if (not opfFile.is_open())
		THROW(("Failed to open opf file"));

	xml::document opf(opfFile);
	MMessageList problems;
	ParseOPF(fs::path("OEBPS"), opf.child(), problems);

	fs::path dir = inOEB.GetPath().parent_path();
	
	for (MProjectGroup::iterator item = mRoot.begin(); item != mRoot.end(); ++item)
	{
		MePubItem* epi = dynamic_cast<MePubItem*>(&*item);
		
		if (epi == nil)
			continue;
		
		epi->GuessMediaType();
		
		fs::path path = dir / relative_path("OEBPS", epi->GetPath());
		
		if (not fs::exists(path))
			problems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("Could not locate file ") + path.string());
		else
		{
			fs::ifstream in(path, ios::binary);
			string data;
			io::filtering_ostream out(io::back_inserter(data));
			copy(in, out);
			
			if (epi->GetMediaType() == "application/xhtml+xml")
			{
				MXHTMLTools::Problems xhtmlProblems;
				MXHTMLTools::ConvertAnyToXHTML(data, xhtmlProblems);
				
				for (MXHTMLTools::Problems::iterator p = xhtmlProblems.begin(); p != xhtmlProblems.end(); ++p)
				{
					if (p->kind == MXHTMLTools::info)
						continue;

					MMessageKind kind;
					if (p->kind == MXHTMLTools::error)
						kind = kMsgKindError;
					else if (p->kind == MXHTMLTools::warning)
						kind = kMsgKindWarning;
					else if (p->kind == MXHTMLTools::info)
						kind = kMsgKindNote;
					else 
						kind = kMsgKindNone;
					
					problems.AddMessage(kind, MFile(new MePubContentFile(this, path)),
						p->line, 0, 0, p->message);
				}
				
				MePubTOCItem* toc = new MePubTOCItem(epi->GetID(), &mTOC);
				toc->SetSrc((relative_path("OEBPS", epi->GetPath())).string());
				mTOC.AddProjectItem(toc);
			}

			epi->SetData(data);
		}
	}
	
	if (not problems.empty())
	{
		MMessageWindow* w = new MMessageWindow("");
		w->SetMessages(_("Problems found in imported OEB"), problems);
		w->Show();
	}
}

void MePubDocument::ReadFile(
	std::istream&		inFile)
{
	ZIPLocalFileHeader fh;
	
	MMessageList problems;
	set<fs::path> encrypted;
	
	// read first file, should be the mimetype file
	map<fs::path,string> content;
	bool keyDecrypted = false, hasMimeType = false, firstItem = true;
	
	while (read_next_file(inFile, fh))
	{
		if (fh.filename == "mimetype" and ba::starts_with(fh.data, "application/epub+zip"))
		{
			hasMimeType = true;
			if (not firstItem)
				problems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("Invalid ePub file, mimetype should be first file"));
			firstItem = false;
			continue;
		}
		
		if (ba::ends_with(fh.filename, "/") or fh.uncompressed_size == 0)
			continue;
		
		fs::path path(fh.filename);

		if (path == "META-INF/container.xml")
		{
			xml::document container(fh.data);
			xml::element* root = container.child();
			
			if (root->name() != "container" or root->ns() != kContainerNS)
				problems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("Invalid or unsupported container.xml file"));
			
			xml::element* n = root->find_first("rootfiles");
			if (not n)
				THROW(("Invalid container.xml file, <rootfiles> missing."));
			
			n = n->find_first("rootfile");
			if (not n)
				THROW(("Invalid container.xml file, <rootfile> missing."));
			
			if (n->get_attribute("media-type") != "application/oebps-package+xml")
				problems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("Invalid container.xml file, first rootfile should be of type \"application/oebps-package+xml\""));
			
			mRootFile = n->get_attribute("full-path");
		}
		else if (path == "META-INF/encryption.xml")
		{
			xml::document encryption(fh.data);
			xml::element* root = encryption.child();
			
			if (root->name() != "encryption" or root->ns() != kContainerNS)
				problems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("Invalid or unsupported encryption.xml file"));
			
			foreach (xml::element* n, root->children<xml::element>())
			{
				if (n->name() != "EncryptedData")
					continue;
				
				xml::element* cd = n->find_first("CipherData");
				if (not cd)
					continue;
				
				xml::element* cr = cd->find_first("CipherReference");
				if (not cr)
					continue;
				
				fs::path uri = cr->get_attribute("URI");
				encrypted.insert(uri);
			}
		}
		else if (path == "META-INF/rights.xml")
		{
			xml::document rights(fh.data);
			xml::element* root = rights.child();
			
			if (root->name() != "rights" or root->ns() != kAdobeAdeptNS)
			{
				problems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("Invalid or unsupported rights.xml file"));
				continue;
			}
			
			xml::element* licenseToken = root->find_first("licenseToken");
			if (not licenseToken)
			{
				problems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("licenseToken not found in rights file"));
				continue;
			}
			
			xml::element* encryptedKey = licenseToken->find_first("encryptedKey");
			if (not encryptedKey)
			{
				problems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("encryptedKey not found in rights file"));
				continue;
			}
			
			try
			{
				DecryptBookKey(encryptedKey->content(), mKey);
				keyDecrypted = true;
			}
			catch (exception& e)
			{
				problems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("This book is encrypted and something went wrong trying to decrypt it:"));
				problems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, e.what());
			}
		}
		else
			content[path] = fh.data;
	}

	if (not hasMimeType)
		problems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("Invalid ePub file, mimetype file is missing"));

	xml::document opf(content[mRootFile]);
	
	ParseOPF(mRootFile.parent_path(), opf.child(), problems);
	
	// decrypt all the files, if we can
	if (keyDecrypted and not encrypted.empty())
	{
		for (map<fs::path,string>::iterator item = content.begin(); item != content.end(); ++item)
		{
			if (encrypted.count(item->first))
				DecryptFile(item->second, mKey);
		}
	}
	
	fs::path tocFile;
	
	if (not mTOCFile.empty() and (encrypted.count(mTOCFile) == 0 or keyDecrypted))
	{
		try
		{
			xml::document ncx(content[mTOCFile]);
			ParseNCX(ncx.child());
		}
		catch (exception& e)
		{
			problems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _(e.what()));
		}
	}
	else
		mTOCFile = mRootFile.parent_path() / "book.ncx";
	
	// and now fill in the data for the items we've found
	
	for (map<fs::path,string>::iterator item = content.begin(); item != content.end(); ++item)
	{
		if (item->first == mRootFile or item->first == mTOCFile)
			continue;
		
		MProjectItem* pi = mRoot.GetItem(item->first);
		MePubItem* epi;
		
		if (pi == nil)
		{
			if (item->first.filename() == ".DS_Store")
				continue;
			
			// this item was not mentioned in the spine, add it here
			MProjectGroup* group = mRoot.GetGroupForPath(item->first.parent_path());
			epi = new MePubItem(item->first.filename(), group);
			group->AddProjectItem(epi);
			epi->GuessMediaType();
		}
		else
			epi = dynamic_cast<MePubItem*>(pi);

		if (epi == nil)
			THROW(("Internal error, item is not an ePub item"));

		epi->SetData(item->second);
		
		if (keyDecrypted == false and encrypted.count(item->first))
			epi->SetEncrypted(true);
		
		if (mLinear.count(epi->GetID()))
			epi->SetLinear(true);
	}

	if (not problems.empty())
	{
		MMessageWindow* w = new MMessageWindow("");
		w->SetMessages(_("Problems found in ePub file"), problems);
		w->Show();
	}
	
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

	if (mTOCFile.empty())
		mTOCFile = oebpsPath / "book.ncx";
	
	if (mRootFile.empty())
		mRootFile = oebpsPath / "book.opf";
	
	// OK, write the file
	ZIPLocalFileHeader fh;
	vector<ZIPCentralDirectory> dir;
	ZIPCentralDirectory cd;
	uint32 offset = 0;
	
	// first write out the mimetype, uncompressed
	
	fh.deflated = false;
	fh.data = kEPubMimeType;
	fh.compressed_size = fh.uncompressed_size = fh.data.length();
	fh.crc = crc32(0, reinterpret_cast<const uint8*>(fh.data.c_str()), fh.data.length());
	fh.filename = "mimetype";
	
	cd.offset = offset;
	offset += write_next_file(inFile, fh);
	cd.file = fh;
	dir.push_back(cd);
	
	// write META-INF/container.xml
	
	fh.filename = "META-INF/container.xml";
	
	xml::element* container(new xml::element("container"));
	container->set_attribute("version", "1.0");
	container->set_attribute("xmlns", kContainerNS);
	xml::element* rootfiles(new xml::element("rootfiles"));
	container->append(rootfiles);
	xml::element* rootfile(new xml::element("rootfile"));
	rootfile->set_attribute("full-path", mRootFile.string());
	rootfile->set_attribute("media-type", "application/oebps-package+xml");
	rootfiles->append(rootfile);
	deflate(container, fh);

	cd.offset = offset;
	offset += write_next_file(inFile, fh);
	cd.file = fh;
	dir.push_back(cd);
	
	// the OPF...
	
	fh.filename = mRootFile.string();
	xml::element* opf = CreateOPF(oebpsPath);
	deflate(opf, fh);

	cd.offset = offset;
	offset += write_next_file(inFile, fh);
	cd.file = fh;
	dir.push_back(cd);
	
	// the NCX...
	
	fh.filename = mTOCFile.string();
	xml::element* ncx = CreateNCX();
	deflate(ncx, fh);

	cd.offset = offset;
	offset += write_next_file(inFile, fh);
	cd.file = fh;
	dir.push_back(cd);
	
	// rest of the items
	
	for (MProjectGroup::iterator item = mRoot.begin(); item != mRoot.end(); ++item)
	{
		MePubItem* file = dynamic_cast<MePubItem*>(&*item);
		if (file != nil)
		{
			fh.filename = file->GetPath().string();
			
			deflate(file->GetData(), fh);
	
			cd.offset = offset;
			offset += write_next_file(inFile, fh);
			cd.file = fh;
			dir.push_back(cd);
		}
	}
	
	// now write out directory
	
	ZIPEndOfCentralDirectory end;
	
	end.entries = dir.size();
	end.directory_offset = offset;
	
	for (vector<ZIPCentralDirectory>::iterator e = dir.begin(); e != dir.end(); ++e)
		offset += write_central_directory(inFile, *e);
	
	end.directory_size = offset - static_cast<streamoff>(end.directory_offset);
	write_directory_end(inFile, end);
}

xml::element* MePubDocument::CreateOPF(
	const fs::path&		inOEBPS)
{
	xml::element* opf(new xml::element("package"));
	opf->set_attribute("version", "2.0");
	opf->set_attribute("unique-identifier", "BookID");
	opf->set_attribute("xmlns", "http://www.idpf.org/2007/opf");
	
	// metadata block
	xml::element* metadata(new xml::element("metadata"));
	metadata->set_attribute("xmlns:dc", "http://purl.org/dc/elements/1.1/");
	metadata->set_attribute("xmlns:opf", "http://www.idpf.org/2007/opf");
	opf->append(metadata);
	
	// the identifier
	xml::element* identifier(new xml::element("dc:identifier"));
	identifier->set_attribute("id", "BookID");
	identifier->set_attribute("opf:scheme", mDocumentIDScheme);
	identifier->content(mDocumentID);
	metadata->append(identifier);
	
	// other dublin core data
	for (map<string,string>::iterator dc = mDublinCore.begin(); dc != mDublinCore.end(); ++dc)
	{
		vector<string> values;
		ba::split(values, dc->second, ba::is_any_of("\n\r"));
		
		for (vector<string>::iterator value = values.begin(); value != values.end(); ++value)
		{
			xml::element* dcn(new xml::element(string("dc:") + dc->first));
			if (dc->first == "creator")
				dcn->set_attribute("opf:role", "aut");
			
			if (dc->first == "date")
			{
				string::size_type p;
				if ((p = value->find(':')) != string::npos)
				{
					dcn->set_attribute("opf:event", value->substr(0, p));

					string date = value->substr(p + 1);
					ba::trim(date);
					dcn->content(date);
				}
				else
					dcn->content(*value);
			}
			else
				dcn->content(*value);

			metadata->append(dcn);
		}
	}
	
	// manifest
	xml::element* manifest(new xml::element("manifest"));
	opf->append(manifest);

	for (MProjectGroup::iterator item = mRoot.begin(); item != mRoot.end(); ++item)
	{
		MePubItem* ePubItem = dynamic_cast<MePubItem*>(&*item);
		
		if (ePubItem == nil or ePubItem->GetID().empty())
			continue;
		
		xml::element* item_node(new xml::element("item"));
		item_node->set_attribute("id", ePubItem->GetID());
		item_node->set_attribute("href", relative_path(mRootFile.parent_path(), ePubItem->GetPath()).string());
		if (not ePubItem->GetMediaType().empty())
			item_node->set_attribute("media-type", ePubItem->GetMediaType());
		manifest->append(item_node);
	}
	
	// add the ncx to the manifest too
	
	xml::element* item_node(new xml::element("item"));
	item_node->set_attribute("id", "ncx");
	item_node->set_attribute("href", relative_path(mRootFile.parent_path(), mTOCFile).string());
	item_node->set_attribute("media-type", "application/x-dtbncx+xml");
	manifest->append(item_node);
	
	// spine. For now we write out all items having a file ID and a media-type application/xhtml+xml 

	xml::element* spine(new xml::element("spine"));
	spine->set_attribute("toc", "ncx");
	opf->append(spine);
	
	for (MProjectGroup::iterator item = mRoot.begin(); item != mRoot.end(); ++item)
	{
		MePubItem* ePubItem = dynamic_cast<MePubItem*>(&*item);
		
		if (ePubItem == nil or
			ePubItem->GetMediaType() != "application/xhtml+xml" or
			ePubItem->GetID().empty())
		{
			continue;
		}
		
		xml::element* itemref(new xml::element("itemref"));
		itemref->set_attribute("idref", ePubItem->GetID());
		if (ePubItem->IsLinear())
			itemref->set_attribute("linear", "yes");
		spine->append(itemref);
	}
	
	return opf;
}

xml::element* MePubDocument::CreateNCX()
{
	xml::element* ncx(new xml::element("ncx"));
	ncx->set_attribute("xmlns", "http://www.daisy.org/z3986/2005/ncx/");
	ncx->set_attribute("version", "2005-1");
	ncx->set_attribute("xml:lang", mDublinCore["language"]);

	// the required head values
	xml::element* head(new xml::element("head"));
	ncx->append(head);
	
	xml::element* meta(new xml::element("meta"));
	meta->set_attribute("name", "dtb:uid");
	meta->set_attribute("content", mDocumentID);
	head->append(meta);
	
	meta = new xml::element("meta");
	meta->set_attribute("name", "dtb:depth");
	meta->set_attribute("content", boost::lexical_cast<string>(mTOC.GetDepth() - 1));
	head->append(meta);
	
	meta = new xml::element("meta");
	meta->set_attribute("name", "dtb:totalPageCount");
	meta->set_attribute("content", "0");
	head->append(meta);
	
	meta = new xml::element("meta");
	meta->set_attribute("name", "dtb:maxPageNumber");
	meta->set_attribute("content", "0");
	head->append(meta);
	
	// the title
	
	xml::element* docTitle(new xml::element("docTitle"));
	xml::element* text(new xml::element("text"));
	text->content(mDublinCore["title"]);
	docTitle->append(text);
	ncx->append(docTitle);
	
	// the navMap
	
	xml::element* navMap(new xml::element("navMap"));
	ncx->append(navMap);
	
	uint32 id = 1;
	MPlayOrder playOrder;
	CreateNavMap(&mTOC, navMap, id, playOrder);
	
	return ncx;
}

void MePubDocument::CreateNavMap(
	MProjectGroup*		inGroup,
	xml::element*		inNavPoint,
	uint32&				ioID,
	MPlayOrder&			ioPlayOrder)
{
	for (int32 ix = 0; ix < inGroup->Count(); ++ix)
	{
		MePubTOCItem* tocItem = dynamic_cast<MePubTOCItem*>(inGroup->GetItem(ix));
		THROW_IF_NIL(tocItem);
		
		string src = tocItem->GetSrc();
		
		if (ioPlayOrder.find(src) == ioPlayOrder.end())
		{
			uint32 playOrder = ioPlayOrder.size() + 1;
			ioPlayOrder[src] = playOrder;
		}

		xml::element* navPoint(new xml::element("navPoint"));
		navPoint->set_attribute("id", string("id-") + boost::lexical_cast<string>(ioID));
		++ioID;

		if (not tocItem->GetId().empty())
			navPoint->set_attribute("id", tocItem->GetId());
		if (not tocItem->GetClass().empty())
			navPoint->set_attribute("class", tocItem->GetClass());
		navPoint->set_attribute("playOrder", boost::lexical_cast<string>(ioPlayOrder[src]));
		
		if (not tocItem->GetName().empty())
		{
			xml::element* navLabel(new xml::element("navLabel"));
			xml::element* text(new xml::element("text"));
			text->content(tocItem->GetName());
			navLabel->append(text);
			navPoint->append(navLabel);
		}
		
		xml::element* content(new xml::element("content"));
		content->set_attribute("src", src);
		navPoint->append(content);

		if (tocItem->Count() > 0)
			CreateNavMap(tocItem, navPoint, ioID, ioPlayOrder);
		
		inNavPoint->append(navPoint);
	}	
}	

MProjectGroup* MePubDocument::GetFiles() const
{
//	return dynamic_cast<MProjectGroup*>(mRoot.GetItem(0));
	return const_cast<MProjectGroup*>(&mRoot);
}

MProjectGroup* MePubDocument::GetTOC() const
{
	return const_cast<MProjectGroup*>(&mTOC);
}

void MePubDocument::ParseOPF(
	const fs::path&		inDirectory,
	xml::element*		inOPF,
	MMessageList&		outProblems)
{
	if (inOPF->name() != "package")
		THROW(("Not an OPF file, root item should be package"));
	
	// fetch the unique-identifier
	string uid = inOPF->get_attribute("unique-identifier");
	if (uid.empty())
		outProblems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("Unique Identifier is missing in OPF"));
	
	// fetch the meta data
	xml::element* metadata = inOPF->find_first("metadata");
	if (not metadata)
		outProblems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("Metadata is missing from OPF"));
	else
	{
		// now I've found a weird file containing a dc-metadata inside metadata
		
		bool oldDC = false;
		xml::element* n = metadata->find_first("dc-metadata");
		if (n)
		{
			oldDC = true;
			metadata = n;
		}
		
		// collect all the Dublin Core information
		foreach (xml::element* dc, metadata->children<xml::element>())
		{
			string name = dc->name();
			
			if (oldDC or dc->ns() == "http://purl.org/dc/elements/1.0/")
				ba::to_lower(name);
			else if (dc->ns() != "http://purl.org/dc/elements/1.1/" and
				dc->ns() != "http://www.idpf.org/2007/opf")
			{
				outProblems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("unsupported dublin core version: ") + dc->ns());
//				continue;
			}
			
			string content = dc->content();
			ba::trim(content);
			
			if (content.empty())
				continue;
			
			if (name == "identifier")
			{
				if (dc->get_attribute("id") == uid)
				{
					mDocumentID = content;
					mDocumentIDScheme = dc->get_attribute("opf:scheme");
					if (mDocumentIDScheme.empty() and dc->ns() == "http://purl.org/dc/elements/1.0/")
						mDocumentIDScheme = dc->get_attribute("scheme");
				}
			}
			else if (name == "date")
			{
				string date = mDublinCore[name];
				if (not date.empty())
					date += '\n';
				
				if (dc->get_attribute("opf:event").empty())
					date += content;
				else
					date += dc->get_attribute("opf:event") + ": " + content;
	
				mDublinCore[name] = date;
			}
			else if (mDublinCore[name].empty())
				mDublinCore[name] = content;
			else
			{
				string separator = "; ";
				if (name == "subject")
					separator = "\n";
				mDublinCore[name] = mDublinCore[name] + separator + content;
			}
		}
	}

	// collect the items from the manifest
	bool warnMediaType = true, warnInvalidID = true;
	
	xml::element* manifest = inOPF->find_first("manifest");
	if (not manifest)
		outProblems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("Manifest missing from OPF document"));
	else
	{
		foreach (xml::element* item, manifest->children<xml::element>())
		{
			if (item->name() != "item") // or item->ns() != "http://www.idpf.org/2007/opf")
				continue;
	
			fs::path href = inDirectory / item->get_attribute("href");
			
			if (mTOCFile.empty())
			{
				if (item->get_attribute("media-type") == "application/x-dtbncx+xml")
				{
					mTOCFile = href;
					continue;
				}

				if (FileNameMatches("*.ncx", href))
				{
					outProblems.AddMessage(kMsgKindError, MFile(), 0, 0, 0,
						_("TOC/NCX file should have mimetype application/x-dtbncx+xml"));
					mTOCFile = href;
					continue;
				}
			}
			
			MProjectGroup* group = mRoot.GetGroupForPath(href.parent_path());
			
			unique_ptr<MePubItem> eItem(new MePubItem(href.filename(), group));
			
			string id = item->get_attribute("id");
			if (id.empty() or not isalpha(id[0]))
			{
				if (warnInvalidID)
					outProblems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("ID's should start with a letter"));
				id = 'x' + id;
				warnInvalidID = false;
			}
			eItem->SetID(id);
			
			string mediaType = item->get_attribute("media-type");
			
			if (mediaType == "text/html")
			{
				mediaType = "application/xhtml+xml";
				if (warnMediaType)
					outProblems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("Use media-type application/xhtml+xml instead of text/html"));
				warnMediaType = false;
			}
				
			eItem->SetMediaType(mediaType);
			
			group->AddProjectItem(eItem.release());
		}
	}
	
	// the spine
	
	xml::element* spine = inOPF->find_first("spine");
	if (not spine)
		outProblems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("Spine missing from OPF document"));
	else
	{
		foreach (xml::element* item, spine->children<xml::element>())
		{
			if (item->name() != "itemref" or item->ns() != "http://www.idpf.org/2007/opf")
				continue;
			
			string idref = item->get_attribute("idref");

			if (item->get_attribute("linear") == "yes")
				mLinear.insert(idref);
		}
	}

	// sanity checks
	if (mDocumentID.empty())
		outProblems.AddMessage(kMsgKindError, MFile(), 0, 0, 0, _("Missing document identifier in metadata section"));
}

void MePubDocument::ParseNCX(
	xml::element*			inNCX)
{
	xml::element* navMap = inNCX->find_first("navMap");
	if (not navMap)
		THROW(("Missing navMap element in NCX file"));

	foreach (xml::element* n, navMap->children<xml::element>())
	{
		if (n->name() == "navPoint")
			ParseNavPoint(&mTOC, n);
	}
}

void MePubDocument::ParseNavPoint(
	MProjectGroup*		inGroup,
	xml::element*		inNavPoint)
{
	assert(inNavPoint->name() == "navPoint");
	
	unique_ptr<MePubTOCItem> np;

	xml::element* label = inNavPoint->find_first("navLabel");
	if (label != nil)
	{
		xml::element* name = label->find_first("text");
		if (not name)
			THROW(("Missing text in navLabel"));
		np.reset(new MePubTOCItem(name->content(), inGroup));
	}
	else
		np.reset(new MePubTOCItem("", inGroup));

	if (not inNavPoint->get_attribute("id").empty())
		np->SetId(inNavPoint->get_attribute("id"));

	if (not inNavPoint->get_attribute("playOrder").empty())
		np->SetPlayOrder(boost::lexical_cast<uint32>(inNavPoint->get_attribute("playOrder")));

	xml::element* content = inNavPoint->find_first("content");
	if (not content)
		THROW(("Missing content in navPoint"));
	
	np->SetSrc(content->get_attribute("src"));

	np->SetClass(inNavPoint->get_attribute("class"));
	
	foreach (xml::element* n, inNavPoint->children<xml::element>())
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

void MePubDocument::SetDocumentIDScheme(
	const string&	inScheme)
{
	mDocumentIDScheme = inScheme;
}

string MePubDocument::GetDocumentIDScheme() const
{
	return mDocumentIDScheme;
}

void MePubDocument::GenerateNewDocumentID()
{
	uuid_t id;
	uuid_generate(id);
	
	char b[32] = "";
	uuid_unparse_lower(id, b);
	mDocumentID = "uuid:";
	mDocumentID += b;
	mDocumentIDScheme = "uuid";	
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
	{
		if (DisplayAlert("epub-item-does-not-exist", mFile.GetFileName(), inFile.filename()) == 1)
		{
			MProjectGroup* folder = dynamic_cast<MProjectGroup*>(mRoot.GetItem(inFile.parent_path()));
			if (folder == nil)
				folder = dynamic_cast<MProjectGroup*>(mRoot.GetItem(0));
			THROW_IF_NIL(folder);
			
			MePubItem* item = new MePubItem(inFile.filename(), folder);
			item->GuessMediaType();
			item->SetID("main");
			item->SetOutOfDate(false);
			item->SetData(inText);
			folder->AddProjectItem(item);
			
			eFileItemInserted(item);
		}	
		else
			THROW(("ePub item %s does not exist", inFile.string().c_str()));
	}
	else
	{
		MePubItem* epi = dynamic_cast<MePubItem*>(pi);
		if (epi == nil)
			THROW(("Error, item is not an editable file"));
	
		epi->SetData(inText);
		epi->SetOutOfDate(false);
	}
	
	SetModified(true);
}

MFile MePubDocument::GetFileForSrc(
	const string&		inSrc)
{
	fs::path path = mRootFile.parent_path() / inSrc;

	MFile file(new MePubContentFile(this, path));
	return file;
}

void MePubDocument::SetModified(
	bool				inModified)
{
	if (inModified == false)
		mRoot.SetOutOfDate(false);
	
	MDocument::SetModified(inModified);
}

void MePubDocument::CreateItem(
	const string&		inFile,
	MProjectGroup*		inGroup,
	MProjectItem*&		outItem)
{
	fs::path path(MFile(inFile).GetPath());
	string name = path.filename();
	
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
		unique_ptr<MePubItem> item(new MePubItem(name, inGroup));
		
		item->GuessMediaType();
		
		vector<char> data;

		fs::ifstream in(path, ios::binary);
		io::filtering_ostream out(io::back_inserter(data));
		io::copy(in, out);

		item->SetData(string(&data[0], data.size()));

		item->SetID(fs::basename(name));

		outItem = item.release();
	}
}

void MePubDocument::ItemMoved()
{
	SetModified(true);
}

void MePubDocument::ItemRenamed(
	MProjectItem*		inItem,
	const string&		inOldName,
	const string&		inNewName)
{
	MePubItem* item = dynamic_cast<MePubItem*>(inItem);
	if (item != nil)
	{
		fs::path path = item->GetPath();
		path = path.parent_path() / inOldName;
		
		MFile file(new MePubContentFile(this, path));
		
		MDocument* doc = MDocument::GetDocumentForFile(file);
		if (doc != nil)
		{
			file = MFile(new MePubContentFile(this, item->GetPath()));
			doc->SetFile(file);
		}
	}
}
