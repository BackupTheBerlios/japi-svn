/*	$Id: MSftpGetDialog.h,v 1.6 2004/02/07 13:10:09 maarten Exp $
	Copyright Maarten
	Created Monday September 29 2003 20:21:49
*/

#ifndef MSFTPGETDIALOG_H
#define MSFTPGETDIALOG_H

#include <vector>

#include "MDialog.h"
#include "MUrl.h"
#include "MSftpChannel.h"

typedef std::vector<MUrl>	MUrlList;

class MSftpGetDialog : public MDialog
{
  public:
						MSftpGetDialog(
							const MUrlList&	inUrls);

	virtual void		Close();

//	HEventOut<bool>		eConnected;

  private:

//	void				SetURL(
//							const MUrlList&	inUrls);

	void				FetchNext();

	void 				GotData();
	void				GotFile();

	void				ChannelEvent(
							int				inMessage);

	void				ChannelMessage(
							std::string 	inMessage);

	std::auto_ptr<MSftpChannel>
						fChannel;
	MUrl				fUrl;
	std::string			fPath;
	MUrlList			fUrls;
	std::string			fText;
	uint32				fSize, fFetched;
};

#endif // MSFTPGETDIALOG_H
