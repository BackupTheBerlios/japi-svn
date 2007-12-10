/*	$Id: MSftpPutDialog.h,v 1.4 2004/02/07 13:10:09 maarten Exp $
	Copyright Hekkelman Programmatuur b.v.
	Created Tuesday August 29 2000 12:57:04
*/

#ifndef MSFTPPUTDIALOG_H
#define MSFTPPUTDIALOG_H

#include "MDialog.h"
#include "MUrl.h"
#include "MSftpChannel.h"

class MDocument;

class MSftpPutDialog : public MDialog
{
  public:
						MSftpPutDialog(
							GladeXML*		inGlade,
							GtkWidget*		inRoot);
	
	void				Initialize(
							MDocument*		inDocument);

	static const char*	GetResourceName()	{ return "sftp-dialog"; }

	virtual void		Close();

	MEventOut<void()>	eConnected;
	MEventOut<void(bool)>
						eNotifyPut;

  private:

	MEventIn<void(std::string)>	eChannelMessage;
	MEventIn<void(int)>			eChannelEvent;

	void				ChannelMessage(
							std::string		inData);

	void				ChannelEvent(
							int				inMessage);

	void				LogUpload();

	MSftpChannel*		fChannel;
	MDocument*			fDoc;
	std::string			fText;
	uint32				fOffset, fSize;
	MUrl				fUrl;
	std::string			fPath;
	bool				fComplete;
};

#endif // PPUTDIALOG_H
