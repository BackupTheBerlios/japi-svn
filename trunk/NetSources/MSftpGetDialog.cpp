/*	$Id: MSftpGetDialog.cpp,v 1.16 2004/02/07 13:10:09 maarten Exp $
	Copyright Maarten
	Created Monday September 29 2003 20:22:06
*/

#include "MJapieG.h"

#include "MSftpChannel.h"

#include "MSftpGetDialog.h"
#include "MPreferences.h"
#include "MDocWindow.h"

using namespace std;

MSftpGetDialog::MSftpGetDialog(
	const MUrlList&	inUrls)
	: MDialog("sftp-dialog")
	, eChannelEvent(this, &MSftpGetDialog::ChannelEvent)
	, eChannelMessage(this, &MSftpGetDialog::ChannelMessage)
	, fChannel(nil)
{
	RestorePosition("fetchdialog");

	fUrls = inUrls;
	fUrl = fUrls.front();
	
	fChannel.reset(
		new MSftpChannel(fUrl.GetHost(), fUrl.GetUser(), fUrl.GetPort()));

	AddRoute(fChannel->eChannelEvent, eChannelEvent);
	AddRoute(fChannel->eChannelMessage, eChannelMessage);

	Show(nil);
}

void MSftpGetDialog::Close()
{
	SavePosition("fetchdialog");
	MDialog::Close();
}

void MSftpGetDialog::FetchNext()
{
	if (fUrls.size() == 0)
		Close();
	else
	{
		fUrl = fUrls.front();
		fUrls.erase(fUrls.begin());

//		SetTitle(HStrings::GetFormattedIndString(5002, 2,
//			fUrl.GetFileName()));
		SetText('lbl1', string("Fetching ") + fUrl.GetFileName());

		fChannel->ReadFile(fUrl.GetPath().string(),
			Preferences::GetInteger("text transfer", true));
		
		fFetched = 0;
//		SetNodeValue('prog', 0);
	}
}

void MSftpGetDialog::ChannelEvent(
	int		inMessage)
{
	switch (inMessage)
	{
		case SFTP_INIT_DONE:
//			eConnected(true, this);
			FetchNext();
			break;
		
		case SFTP_FILE_SIZE_KNOWN:
			SetText('lbl2', "File size known");
			fSize = fChannel->GetFileSize();
//			SetNodeText('stat',
//				HStrings::GetFormattedIndString(4007, 0, fUrl.GetFileName()));
			break;
		
		case SFTP_DATA_AVAILABLE:
			SetText('lbl2', "Receiving data");
			GotData();
			break;
		
		case SFTP_DATA_DONE:
			SetText('lbl2', "File received");
			GotFile();
			break;
		
		case SFTP_FILE_CLOSED:
			SetText('lbl2', "File closed");
			FetchNext();
			break;

		case SSH_CHANNEL_TIMEOUT:
//			if (HAlerts::Alert(this, 504, fUrl.GetHost().c_str()) == 1)
//				fChannel->ResetTimer();
			break;
		
	}
}

void MSftpGetDialog::ChannelMessage(
	std::string		inMessage)
{
//	SetNodeText('stat', inMessage);
	SetText('lbl2', inMessage);
}

void MSftpGetDialog::GotData()
{
	std::string data = fChannel->GetData();

	fText.append(data);
	fFetched += data.length();
	
	PRINT(("Fetched: %d", fFetched));
	
	if (fSize > 0)
		SetProgressFraction('prog', float(fFetched) / fSize);
}

void MSftpGetDialog::GotFile()
{
	PRINT(("Got file %s", fUrl.str().c_str()));
	
	fUrl.SetPassword("");
	
	MDocument* doc = new MDocument(&fUrl);
	doc->Type(fText.c_str(), fText.length());
	
	gApp->AddToRecentMenu(fUrl);
	
	MDocWindow::DisplayDocument(doc);
	
	fChannel->CloseFile();
}
