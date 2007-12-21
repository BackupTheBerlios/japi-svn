/*	$Id: MSftpPutDialog.cpp,v 1.12 2004/02/07 13:10:09 maarten Exp $
	Copyright Hekkelman Programmatuur b.v.
	Created Tuesday August 29 2000 13:01:50
*/

#include "MJapieG.h"

#include "MSftpPutDialog.h"
#include "MDocument.h"
#include "MDocWindow.h"
#include "MPreferences.h"

using namespace std;

MSftpPutDialog::MSftpPutDialog(
	MDocument*		inDocument)
	: MDialog("sftp-dialog")
	, fChannel(nil)
	, fDoc(nil)
	, fOffset(0)
	, fComplete(false)
{
	fUrl = inDocument->GetURL();
	
	MTextBuffer& text = inDocument->GetTextBuffer();

	text.GetText(0, text.GetSize(), fText);

	fOffset = 0;
	fSize = text.GetSize();

	SetText('lbl1', string("Storing ") + fUrl.GetFileName());
	SetText('lbl2', "connecting");

	RestorePosition("putdialog");
	
	fChannel =
		new MSftpChannel(fUrl.GetHost(), fUrl.GetUser(), fUrl.GetPort());

	SetCallBack(fChannel->eChannelEvent, this, &MSftpPutDialog::ChannelEvent);
	SetCallBack(fChannel->eChannelMessage, this, &MSftpPutDialog::ChannelMessage);
	
	eNotifyPut(true);

	Show(MDocWindow::DisplayDocument(inDocument));
}

void MSftpPutDialog::Close()
{
	eNotifyPut(false);
	
	if (not fComplete)
		fDoc->SetModified(true);
	
	SavePosition("putdialog");
	MDialog::Close();
}

void MSftpPutDialog::ChannelEvent(
	int		inMessage)
{
	const uint32 kBufferSize = 1024;
	
	switch (inMessage)
	{
		case SFTP_INIT_DONE:
			eConnected();
			fChannel->WriteFile(fUrl.GetPath().string(),
				Preferences::GetInteger("text transfer", true));
			SetText('lbl2', "Connected");
//			SetNodeText('stat',
//				HStrings::GetFormattedIndString(4007, 1, fUrl.GetFileName()));
			break;
		
		case SFTP_CAN_SEND_DATA:
			if (fOffset < fSize)
			{
				uint32 k = fSize - fOffset;
				if (k > kBufferSize)
					k = kBufferSize;

				SetText('lbl2', "Sending data");
				fChannel->SendData(fText.substr(fOffset, k));
				
				fOffset += k;

				SetProgressFraction('prog', float(fOffset) / fSize);
			}
			else
			{
				SetText('lbl2', "Closing file");
				fChannel->CloseFile();
				fComplete = true;

				if (Preferences::GetInteger("loguploads", false) != 0)
					LogUpload();

				MDialog::Close();
			}
			break;

		case SSH_CHANNEL_TIMEOUT:
//			if (HAlerts::Alert(this, 504, fUrl.GetHost().c_str()) == 1)
//				fChannel->ResetTimer();
			SetText('lbl2', "Timeout, saving failed");
			break;
		
	}
}

void MSftpPutDialog::ChannelMessage(
	string		inMessage)
{
	SetText('lbl2', inMessage);
}

void MSftpPutDialog::LogUpload()
{
//	HUrl log(gPrefsDir / "Ftp History.txt");
//
//	int fd = HFile::Open(log, O_RDWR | O_CREAT);
//	
//	if (fd != -1)
//	{
//		std::time_t now;
//		char s[1200];
//
//		std::time(&now);
//		
//		HUrl url(fUrl);
//		url.SetPassword(kEmptyString);
//		std::snprintf(s, 1200, "%24.24s - %s\n", std::ctime(&now), url.GetURL().c_str());
//		
//		HFile::Seek(fd, 0, SEEK_END);
//		(void)HFile::Write(fd, s, std::strlen(s));
//		HFile::Close(fd);
//	}
}
