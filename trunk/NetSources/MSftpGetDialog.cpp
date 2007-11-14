/*	$Id: MSftpGetDialog.cpp,v 1.16 2004/02/07 13:10:09 maarten Exp $
	Copyright Maarten
	Created Monday September 29 2003 20:22:06
*/

#include "MJapieG.h"

#include "MSftpChannel.h"

#include "MSftpGetDialog.h"
#include "MPreferences.h"
#include "MDocWindow.h"

MSftpGetDialog::MSftpGetDialog(
	const MUrlList&	inUrls)
	: eChannelEvent(this, &MSftpGetDialog::ChannelEvent)
	, eChannelMessage(this, &MSftpGetDialog::ChannelMessage)
	, fChannel(nil)
	, fUrls(inUrls)
{
	SetTitle("Fetching file");
	
	AddStaticText('lbl1', "x");
	AddStaticText('lbl2', "y");
	
	RestorePosition("fetchdialog");

	fUrls = inUrls;

	if (fChannel.get() == nil)
	{
		fUrl = fUrls.front();
		
		unsigned short port = fUrl.GetPort();
		if (port == 0)
			port = 22;
		fChannel.reset(
			new MSftpChannel(fUrl.GetHost(), fUrl.GetUser(), port));
		AddRoute(fChannel->eChannelEvent, eChannelEvent);
		AddRoute(fChannel->eChannelMessage, eChannelMessage);
	}

	Show(nil);
}

//void MSftpGetDialog::InitSelf()
//{
//	FindNode<HControlNode>('prog')->SetMaxValue(1000);
//
//	Show();
//}
//

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
		SetText('lbl1', fUrl.GetFileName());

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
			fSize = fChannel->GetFileSize();
//			SetNodeText('stat',
//				HStrings::GetFormattedIndString(4007, 0, fUrl.GetFileName()));
			break;
		
		case SFTP_DATA_AVAILABLE:
			GotData();
			break;
		
		case SFTP_DATA_DONE:
			GotFile();
			break;
		
		case SFTP_FILE_CLOSED:
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
	{
		double v = 1000.0 * fFetched / fSize;
		if (v < 0.0 || v > 1000.0)
			v = 1000;
//		SetNodeValue('prog', static_cast<long>(v));
	}
}

void MSftpGetDialog::GotFile()
{
	fUrl.SetPassword("");
	
	MDocument* doc = new MDocument(fText, fUrl.GetFileName());
	
	MDocWindow::DisplayDocument(doc);
	
	fChannel->CloseFile();
}
