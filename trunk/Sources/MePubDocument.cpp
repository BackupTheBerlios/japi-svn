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

#include <openssl/pem.h>
#include <openssl/aes.h>

#include "document.hpp"

#include "MError.h"
#include "MePubDocument.h"
#include "MePubItem.h"
#include "MTextBuffer.h"
#include "MFile.h"
#include "MResources.h"
#include "MStrings.h"
#include "MGlobals.h"
#include "MAlerts.h"
#include "MUtils.h"

#include "MXHTMLTools.h"

using namespace std;
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
	
	if (s.readsome(b, 4) != 4)
		THROW(("Truncated ePub file"));

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

	if (s.readsome(b + 4, sizeof(b) - 4) != sizeof(b) - 4)
		THROW(("Truncated ePub file"));

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
	if (s.readsome(&fn[0], fileNameLength) != fileNameLength)
		THROW(("Truncated ePub file"));
	
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
			if (s.readsome(&b[0], fh.compressed_size) != fh.compressed_size)
				THROW(("Truncated ePub file"));
			
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
			if (s.readsome(b2, sizeof(b2)) != sizeof(b2))
				THROW(("Truncated ePub file"));

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
	xml::node_ptr		inXML,
	ZIPLocalFileHeader&	outFileHeader)
{
	xml::document doc(inXML);
	
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
	, mRoot("", nil)
	, mTOC("", nil)
{
}

MePubDocument::MePubDocument()
	: MDocument(MFile())
	, eCreateItem(this, &MePubDocument::CreateItem)
	, eItemMoved(this, &MePubDocument::ItemMoved)
	, eItemRemoved(this, &MePubDocument::ItemMoved)
	, mRoot("", nil)
	, mTOC("", nil)
{
}

MePubDocument::~MePubDocument()
{
}

void MePubDocument::InitializeNew()
{
	MResource rsrc = MResource::root().find("empty_xhtml_file.xhtml");
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
	vector<string> problems;
	ParseOPF(fs::path("OEBPS"), *opf.root(), problems);

	fs::path dir = inOEB.GetPath().branch_path();
	
	for (MProjectGroup::iterator item = mRoot.begin(); item != mRoot.end(); ++item)
	{
		MePubItem* epi = dynamic_cast<MePubItem*>(&*item);
		
		if (epi == nil)
			continue;
		
		epi->GuessMediaType();
		
		fs::path path = dir / relative_path("OEBPS", epi->GetPath());
		
		if (not fs::exists(path))
			problems.push_back(_("Could not locate file ") + path.string());
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

					problems.push_back(p->message);
				}
				
				MePubTOCItem* toc = new MePubTOCItem(epi->GetID(), &mTOC);
				toc->SetSrc((relative_path("OEBPS", epi->GetPath())).string());
				mTOC.AddProjectItem(toc);
			}

			epi->SetData(data);
		}
	}
	
	if (not problems.empty())
		DisplayAlert("problems-in-epub", ba::join(problems, "\n"));
	
}

void MePubDocument::ReadFile(
	std::istream&		inFile)
{
	ZIPLocalFileHeader fh;
	
	vector<string> problems;
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
				problems.push_back(_("Invalid ePub file, mimetype should be first file"));
			firstItem = false;
			continue;
		}
		
		if (ba::ends_with(fh.filename, "/") or fh.uncompressed_size == 0)
			continue;
		
		fs::path path(fh.filename);

		if (path == "META-INF/container.xml")
		{
			xml::document container(fh.data);
			xml::node_ptr root = container.root();
			
			if (root->name() != "container" or root->ns() != kContainerNS)
				problems.push_back(_("Invalid or unsupported container.xml file"));
			
			xml::node_ptr n = root->find_first_child("rootfiles");
			if (not n)
				THROW(("Invalid container.xml file, <rootfiles> missing."));
			
			n = n->find_first_child("rootfile");
			if (not n)
				THROW(("Invalid container.xml file, <rootfile> missing."));
			
			if (n->get_attribute("media-type") != "application/oebps-package+xml")
				problems.push_back(_("Invalid container.xml file, first rootfile should be of type \"application/oebps-package+xml\""));
			
			mRootFile = n->get_attribute("full-path");
		}
		else if (path == "META-INF/encryption.xml")
		{
			xml::document encryption(fh.data);
			xml::node_ptr root = encryption.root();
			
			if (root->name() != "encryption" or root->ns() != kContainerNS)
				problems.push_back(_("Invalid or unsupported encryption.xml file"));
			
			for (xml::node_ptr n = root->children(); n; n = n->next())
			{
				if (n->name() != "EncryptedData")
					continue;
				
				xml::node_ptr cd = n->find_first_child("CipherData");
				if (not cd)
					continue;
				
				xml::node_ptr cr = cd->find_first_child("CipherReference");
				if (not cr)
					continue;
				
				fs::path uri = cr->get_attribute("URI");
				encrypted.insert(uri);
			}
		}
		else if (path == "META-INF/rights.xml")
		{
			xml::document rights(fh.data);
			xml::node_ptr root = rights.root();
			
			if (root->name() != "rights" or root->ns() != kAdobeAdeptNS)
			{
				problems.push_back(_("Invalid or unsupported rights.xml file"));
				continue;
			}
			
			xml::node_ptr licenseToken = root->find_first_child("licenseToken");
			if (not licenseToken)
			{
				problems.push_back(_("licenseToken not found in rights file"));
				continue;
			}
			
			xml::node_ptr encryptedKey = licenseToken->find_first_child("encryptedKey");
			if (not encryptedKey)
			{
				problems.push_back(_("encryptedKey not found in rights file"));
				continue;
			}
			
			try
			{
				DecryptBookKey(encryptedKey->content(), mKey);
				keyDecrypted = true;
			}
			catch (exception& e)
			{
				problems.push_back(_("This book is encrypted and something went wrong trying to decrypt it:"));
				problems.push_back(e.what());
			}
		}
		else
			content[path] = fh.data;
	}

	if (not hasMimeType)
		problems.push_back(_("Invalid ePub file, mimetype file is missing"));

	xml::document opf(content[mRootFile]);
	
	ParseOPF(mRootFile.branch_path(), *opf.root(), problems);
	
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
			ParseNCX(*ncx.root());
		}
		catch (exception& e)
		{
			problems.push_back(_(e.what()));
		}
	}
	else
		mTOCFile = mRootFile.branch_path() / "book.ncx";
	
	// and now fill in the data for the items we've found
	
	for (map<fs::path,string>::iterator item = content.begin(); item != content.end(); ++item)
	{
		if (item->first == mRootFile or item->first == mTOCFile)
			continue;
		
		MProjectItem* pi = mRoot.GetItem(item->first);
		MePubItem* epi;
		
		if (pi == nil)
		{
			if (item->first.leaf() == ".DS_Store")
				continue;
			
			// this item was not mentioned in the spine, add it here
			MProjectGroup* group = mRoot.GetGroupForPath(item->first.branch_path());
			epi = new MePubItem(item->first.leaf(), group);
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
		DisplayAlert("problems-in-epub", ba::join(problems, "\n"));
	
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

	cd.offset = offset;
	offset += write_next_file(inFile, fh);
	cd.file = fh;
	dir.push_back(cd);
	
	// the OPF...
	
	fh.filename = mRootFile.string();
	xml::node_ptr opf = CreateOPF(oebpsPath);
	deflate(opf, fh);

	cd.offset = offset;
	offset += write_next_file(inFile, fh);
	cd.file = fh;
	dir.push_back(cd);
	
	// the NCX...
	
	fh.filename = mTOCFile.string();
	xml::node_ptr ncx = CreateNCX();
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
		vector<string> values;
		ba::split(values, dc->second, ba::is_any_of("\n\r"));
		
		for (vector<string>::iterator value = values.begin(); value != values.end(); ++value)
		{
			xml::node_ptr dcn(new xml::node(dc->first, "dc"));
			if (dc->first == "creator")
				dcn->add_attribute("opf:role", "aut");
			
			if (dc->first == "date")
			{
				string::size_type p;
				if ((p = value->find(':')) != string::npos)
				{
					dcn->add_attribute("opf:event", value->substr(0, p));

					string date = value->substr(p + 1);
					ba::trim(date);
					dcn->content(date);
				}
				else
					dcn->content(*value);
			}
			else
				dcn->content(*value);

			metadata->add_child(dcn);
		}
	}
	
	// manifest
	xml::node_ptr manifest(new xml::node("manifest"));
	opf->add_child(manifest);

	for (MProjectGroup::iterator item = mRoot.begin(); item != mRoot.end(); ++item)
	{
		MePubItem* ePubItem = dynamic_cast<MePubItem*>(&*item);
		
		if (ePubItem == nil or ePubItem->GetID().empty())
			continue;
		
		xml::node_ptr item_node(new xml::node("item"));
		item_node->add_attribute("id", ePubItem->GetID());
		item_node->add_attribute("href", relative_path(mRootFile.branch_path(), ePubItem->GetPath()).string());
		if (not ePubItem->GetMediaType().empty())
			item_node->add_attribute("media-type", ePubItem->GetMediaType());
		manifest->add_child(item_node);
	}
	
	// add the ncx to the manifest too
	
	xml::node_ptr item_node(new xml::node("item"));
	item_node->add_attribute("id", "ncx");
	item_node->add_attribute("href", relative_path(mRootFile.branch_path(), mTOCFile).string());
	item_node->add_attribute("media-type", "application/x-dtbncx+xml");
	manifest->add_child(item_node);
	
	// spine. For now we write out all items having a file ID and a media-type application/xhtml+xml 

	xml::node_ptr spine(new xml::node("spine"));
	spine->add_attribute("toc", "ncx");
	opf->add_child(spine);
	
	for (MProjectGroup::iterator item = mRoot.begin(); item != mRoot.end(); ++item)
	{
		MePubItem* ePubItem = dynamic_cast<MePubItem*>(&*item);
		
		if (ePubItem == nil or
			ePubItem->GetMediaType() != "application/xhtml+xml" or
			ePubItem->GetID().empty())
		{
			continue;
		}
		
		xml::node_ptr itemref(new xml::node("itemref"));
		itemref->add_attribute("idref", ePubItem->GetID());
		if (ePubItem->IsLinear())
			itemref->add_attribute("linear", "yes");
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
	
	uint32 id = 1;
	MPlayOrder playOrder;
	CreateNavMap(&mTOC, navMap, id, playOrder);
	
	return ncx;
}

void MePubDocument::CreateNavMap(
	MProjectGroup*		inGroup,
	xml::node_ptr		inNavPoint,
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

		xml::node_ptr navPoint(new xml::node("navPoint"));
		navPoint->add_attribute("id", string("id-") + boost::lexical_cast<string>(ioID));
		++ioID;

		navPoint->add_attribute("class", tocItem->GetClass());
		navPoint->add_attribute("playOrder", boost::lexical_cast<string>(ioPlayOrder[src]));
		
		xml::node_ptr navLabel(new xml::node("navLabel"));
		xml::node_ptr text(new xml::node("text"));
		text->content(tocItem->GetName());
		navLabel->add_child(text);
		navPoint->add_child(navLabel);
		
		xml::node_ptr content(new xml::node("content"));
		content->add_attribute("src", src);
		navPoint->add_child(content);

		if (tocItem->Count() > 0)
			CreateNavMap(tocItem, navPoint, ioID, ioPlayOrder);
		
		inNavPoint->add_child(navPoint);
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
	xml::node&			inOPF,
	vector<string>&		outProblems)
{
	if (inOPF.name() != "package")
		THROW(("Not an OPF file, root item should be package"));
	
	// fetch the unique-identifier
	string uid = inOPF.get_attribute("unique-identifier");
	if (uid.empty())
		outProblems.push_back(_("Unique Identifier is missing in OPF"));
	
	// fetch the meta data
	xml::node_ptr metadata = inOPF.find_first_child("metadata");
	if (not metadata)
		outProblems.push_back(_("Metadata is missing from OPF"));
	else
	{
		// now I've found a weird file containing a dc-metadata inside metadata
		
		bool oldDC = false;
		xml::node_ptr n = metadata->find_first_child("dc-metadata");
		if (n)
		{
			oldDC = true;
			metadata = n;
		}
		
		// collect all the Dublin Core information
		for (xml::node_ptr dc = metadata->children(); dc; dc = dc->next())
		{
			string name = dc->name();
			
			if (oldDC or dc->ns() == "http://purl.org/dc/elements/1.0/")
				ba::to_lower(name);
			else if (dc->ns() != "http://purl.org/dc/elements/1.1/")
			{
				outProblems.push_back(_("unsupported dublin core version"));
				continue;
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
	
	xml::node_ptr manifest = inOPF.find_first_child("manifest");
	if (not manifest)
		outProblems.push_back(_("Manifest missing from OPF document"));
	else
	{
		for (xml::node_ptr item = manifest->children(); item; item = item->next())
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
					outProblems.push_back(_("TOC/NCX file should have mimetype application/x-dtbncx+xml"));
					mTOCFile = href;
					continue;
				}
			}
			
			MProjectGroup* group = mRoot.GetGroupForPath(href.branch_path());
			
			auto_ptr<MePubItem> eItem(new MePubItem(href.leaf(), group));
			
			string id = item->get_attribute("id");
			if (id.empty() or not isalpha(id[0]))
			{
				if (warnInvalidID)
					outProblems.push_back(_("ID's should start with a letter"));
				id = 'x' + id;
				warnInvalidID = false;
			}
			eItem->SetID(id);
			
			string mediaType = item->get_attribute("media-type");
			
			if (mediaType == "text/html")
			{
				mediaType = "application/xhtml+xml";
				if (warnMediaType)
					outProblems.push_back(_("Use media-type application/xhtml+xml instead of text/html"));
				warnMediaType = false;
			}
				
			eItem->SetMediaType(mediaType);
			
			group->AddProjectItem(eItem.release());
		}
	}
	
	// the spine
	
	xml::node_ptr spine = inOPF.find_first_child("spine");
	if (not spine)
		outProblems.push_back(_("Spine missing from OPF document"));
	else
	{
		for (xml::node_ptr item = spine->children(); item; item = item->next())
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
		outProblems.push_back(_("Missing document identifier in metadata section"));
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

void MePubDocument::CreateItem(
	const string&		inFile,
	MProjectGroup*		inGroup,
	MProjectItem*&		outItem)
{
	fs::path path(MFile(inFile).GetPath());
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
		
		item->GuessMediaType();
		
		fs::ifstream file(path);

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

		item->SetID(fs::basename(name));

		outItem = item.release();
	}
}

void MePubDocument::ItemMoved()
{
	SetModified(true);
}
