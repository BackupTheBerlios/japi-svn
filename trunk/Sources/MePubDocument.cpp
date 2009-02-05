/* 
   Created by: Maarten L. Hekkelman
   Date: donderdag 05 februari, 2009
*/

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

using namespace std;
namespace ba = boost::algorithm;


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
			
			if (path == "mimetype" and not ba::starts_with(s, "application/epub+zip"))
				THROW(("Invalid ePub file, mimetype is not correct"));
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
			
			if (S_ISREG(archive_entry_filetype(entry)))
				mContent[path] = s;
		}

		for (map<fs::path,string>::iterator file = mContent.begin(); file != mContent.end(); ++file)
			cout << file->first << ' ' << file->second.length() << endl;
		
		xml::document opf(mContent[mRootFile]);
		
		cout << opf << endl;
		
		cout << mContent[mRootFile] << endl;
		
//		cout << endl;
//		cout << mContent["META-INF/encryption.xml"] << endl;
//
//		cout << endl;
//		cout << mContent["OEBPS/page-template.xpgt"] << endl;
//
//		cout << endl;
//		cout << mContent["OEBPS/toc.ncx"] << endl;

		
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
