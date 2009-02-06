//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <iostream>
#include <iomanip>
#include <archive.h>
#include <archive_entry.h>
#include <cstring>

#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>

#include "document.hpp"

#include "MError.h"
#include "MePubDocument.h"
#include "MePubItem.h"

using namespace std;
namespace ba = boost::algorithm;

// --------------------------------------------------------------------

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
	const char kMagicMT[] = "mimetypeapplication/epub+zip";
	
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
			
			if (path == "mimetype")
			{
				if (not ba::starts_with(s, "application/epub+zip"))
					THROW(("Invalid ePub file, mimetype is not correct"));
			}
			else if (path == "META-INF/container.xml")
			{
				xml::document container(s);
				xml::node_ptr root = container.root();
				
				if (root->name() != "container")
					THROW(("Invalid container.xml file, root should be <container>"));
				
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
