/* 
   Created by: Maarten L. Hekkelman
   Date: donderdag 05 februari, 2009
*/

#ifndef MEPUBDOCUMENT_H
#define MEPUBDOCUMENT_H

#include <map>
#include "MDocument.h"

class MePubDocument : public MDocument
{
  public:

	explicit			MePubDocument(
							const fs::path&		inProjectFile);

						MePubDocument(
							const fs::path&		inParentDir,
							const std::string&	inName);

	virtual				~MePubDocument();

	virtual bool		UpdateCommandStatus(
							uint32			inCommand,
							MMenu*			inMenu,
							uint32			inItemIndex,
							bool&			outEnabled,
							bool&			outChecked);

	virtual bool		ProcessCommand(
							uint32			inCommand,
							const MMenu*	inMenu,
							uint32			inItemIndex,
							uint32			inModifiers);

	virtual void		ReadFile(
							std::istream&		inFile);

	virtual void		WriteFile(
							std::ostream&		inFile);

  private:

	static ssize_t		archive_read_callback_cb(
							struct archive*		inArchive,
							void*				inClientData,
							const void**		_buffer);

	static ssize_t		archive_write_callback_cb(
							struct archive*		inArchive,
							void*				inClientData,
							const void*			_buffer,
							size_t				_length);

	static int			archive_open_callback_cb(
							struct archive*		inArchive,
							void*				inClientData);

	static int			archive_close_callback_cb(
							struct archive*		inArchive,
							void*				inClientData);

	ssize_t				archive_read_callback(
							struct archive*		inArchive,
							const void**		_buffer);

	ssize_t				archive_write_callback(
							struct archive*		inArchive,
							const void*			_buffer,
							size_t				_length);

	int					archive_open_callback(
							struct archive*		inArchive);

	int					archive_close_callback(
							struct archive*		inArchive);
	
	std::istream*		mInputFileStream;
	char				mBuffer[1024];

	std::string			mRootFile;
	std::map<fs::path,std::string>
						mContent;
};

#endif
