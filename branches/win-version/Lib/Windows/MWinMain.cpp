//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include <windows.h>
#include <ddeml.h>

#include "MApplication.h"
#include "MError.h"
#include "MWinUtils.h"
#include "MDocument.h"
#include "MDocClosedNotifier.h"

using namespace std;
namespace po = boost::program_options;

class MWinDocClosedNotifierImpl : public MDocClosedNotifierImpl
{
public:
							MWinDocClosedNotifierImpl(HCONV inConv)
								: mConv(inConv) {}

							~MWinDocClosedNotifierImpl()
							{
								eDocClosed(mConv);
							}

	MEventOut<void(HCONV)>	eDocClosed;

	virtual bool			ReadSome(string& outText)		{ return false; }

private:
	HCONV					mConv;
};

class MDDEImpl
{
public:
						MDDEImpl(uint32 inInst, HSZ inServer, HSZ inTopic)
							: mInst(inInst)
							, mServer(inServer)
							, mTopic(inTopic)
						{
							sInstance = this;
						}

						~MDDEImpl()
						{
							::DdeFreeStringHandle(mInst, mServer);
							::DdeFreeStringHandle(mInst, mTopic);
							::DdeUninitialize(mInst);
						}

	void				Send(HCONV inConversation, const wstring& inCommand);

	virtual HDDEDATA	Callback(UINT uType, UINT uFmt, HCONV hconv,
							HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
							DWORD dwData1, DWORD dwData2) = 0;

	static HDDEDATA CALLBACK
						DdeCallback(UINT uType, UINT uFmt, HCONV hconv,
							HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
							DWORD dwData1, DWORD dwData2)
						{
							HDDEDATA result = DDE_FNOTPROCESSED;
							if (sInstance != nil)
								 result = sInstance->Callback(uType, uFmt, hconv, hsz1, hsz2, hdata, dwData1, dwData2);
							return result;
						}

protected:
	static MDDEImpl*	sInstance;
	uint32				mInst;
	HSZ					mServer, mTopic;
};

MDDEImpl* MDDEImpl::sInstance;

void MDDEImpl::Send(HCONV inConversation, const wstring& inCommand)
{
	DWORD err = 0;
	HDDEDATA result = ::DdeClientTransaction(
		(LPBYTE)inCommand.c_str(), inCommand.length() * sizeof(wchar_t), inConversation, 0, CF_UNICODETEXT, XTYP_EXECUTE, 1000, &err);
	if (result)
		::DdeFreeDataHandle(result);
}



class MDDEServerImpl : public MDDEImpl
{
public:
						MDDEServerImpl(uint32 inInst, HSZ inServer, HSZ inTopic)
							: MDDEImpl(inInst, inServer, inTopic)
							, eDocClosed(this, &MDDEServerImpl::DocClosed)
						{
							if (not ::DdeNameService(mInst, mServer, 0, DNS_REGISTER))
								THROW(("Failed to register DDE name service"));
						}

	virtual HDDEDATA	Callback(UINT uType, UINT uFmt, HCONV hconv,
							HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
							DWORD dwData1, DWORD dwData2);

private:

	MEventIn<void(HCONV)>
						eDocClosed;

	void				DocClosed(HCONV inConversation);

	set<HCONV>			mConversations;
};

HDDEDATA MDDEServerImpl::Callback(UINT uType, UINT uFmt, HCONV hconv,
	HSZ hsz1, HSZ hsz2, HDDEDATA hdata, DWORD dwData1, DWORD dwData2)
{
	HDDEDATA result = DMLERR_NO_ERROR;
	switch (uType)
	{
		case XTYP_CONNECT:
			result = (HDDEDATA)mTopic;
			break;
		
		case XTYP_CONNECT_CONFIRM:
			mConversations.insert(hconv);
			break;
		
		case XTYP_DISCONNECT:
			mConversations.erase(hconv);
			break;

		case XTYP_EXECUTE:
		{
			DWORD len;
			wchar_t* cmd = reinterpret_cast<wchar_t*>(::DdeAccessData(hdata, &len));
			len /= sizeof(wchar_t);

			if (cmd != nil)
			{
				wstring text(cmd, len);
				static boost::wregex rx(L"\\[(open|new)(\\(\"(.+?)\"(,\\s*(\\d+))?\\))?\\]");

				boost::wsmatch match;
				if (boost::regex_match(text, match, rx))
				{
					MDocument* doc = nil;
					bool read = false;

					if (match.str(1) == L"open")
					{
						fs::path file(w2c(match.str(3)));
						doc = gApp->OpenOneDocument(MFile(file));
					}
					else if (match.str(1) == L"new")
						doc = gApp->CreateNewDocument();

					if (doc != nil)
					{
						MWinDocClosedNotifierImpl* notifier = new MWinDocClosedNotifierImpl(hconv);
						AddRoute(notifier->eDocClosed, eDocClosed);
						doc->AddNotifier(MDocClosedNotifier(notifier), read);
					}
				}

				::DdeUnaccessData(hdata);
			}
			break;
		}

		default:
			result = (HDDEDATA)DMLERR_NOTPROCESSED;
			break;
	}
	return result;
}

void MDDEServerImpl::DocClosed(HCONV inConversation)
{
	if (mConversations.count(inConversation))
	{
		::DdeDisconnect(inConversation);
		mConversations.erase(inConversation);
	}
}

class MDDEClientImpl : public MDDEImpl
{
public:
				MDDEClientImpl(DWORD inInst, HCONV inConv, HSZ inServer, HSZ inTopic)
					: MDDEImpl(inInst, inServer, inTopic)
					, mConv(inConv)
				{
				}

				~MDDEClientImpl()
				{
					::DdeDisconnect(mConv);
				}

	void		Open(const fs::path& inFile, int32 inLineNr);
	void		New();
	void		Wait();

private:
	virtual HDDEDATA	Callback(UINT uType, UINT uFmt, HCONV hconv,
							HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
							DWORD dwData1, DWORD dwData2);

	HCONV		mConv;
	bool		mDone;
};

void MDDEClientImpl::Open(const fs::path& inFile, int32 inLineNr)
{
	Send(mConv, (boost::wformat(L"[open(\"%1%\",%2%)]") % c2w(inFile.native_file_string()) % inLineNr).str());
}

void MDDEClientImpl::New()
{
	Send(mConv, L"[new]");
}

HDDEDATA MDDEClientImpl::Callback(UINT uType, UINT uFmt, HCONV hconv,
	HSZ hsz1, HSZ hsz2, HDDEDATA hdata, DWORD dwData1, DWORD dwData2)
{
	HDDEDATA result = DMLERR_NO_ERROR;
	switch (uType)
	{
		case XTYP_DISCONNECT:
			mDone = true;
			break;

		default:
			result = (HDDEDATA)DMLERR_NOTPROCESSED;
			break;
	}
	return result;
}

void MDDEClientImpl::Wait()
{
	mDone = false;
	while (not mDone)
	{
		MSG message;

		int result = ::GetMessageW (&message, NULL, 0, 0);
		if (result <= 0)
		{
			if (result < 0)
				result = message.wParam;
			break;
		}
		
		::TranslateMessage(&message);
		::DispatchMessageW(&message);
	}
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPreInst, LPSTR lpszCmdLine, int nCmdShow)
{
	vector<string> args = po::split_winmain(lpszCmdLine);

	bool server = false;
	DWORD inst = 0;
	UINT err = ::DdeInitialize(&inst, &MDDEImpl::DdeCallback, 0, 0);

	int result = 0;

	if (err == DMLERR_NO_ERROR)
	{
		HSZ srvr = ::DdeCreateStringHandle(inst, c2w(kAppName).c_str(), CP_WINUNICODE);
		HSZ topic = ::DdeCreateStringHandle(inst, L"System", CP_WINUNICODE);
		
		HCONV conn = ::DdeConnect(inst, srvr, topic, nil);
		if (conn != 0)
		{
			MDDEClientImpl client(inst, conn, srvr, topic);

			if (args.empty())
				client.New();
			else
			{
				foreach (string arg, args)
					client.Open(fs::system_complete(arg), 0);
			}

			client.Wait();
		}
		else
		{
			MDDEServerImpl server(inst, srvr, topic);
			result = MApplication::Main(args);
		}
	}
	else
		result = MApplication::Main(args);

	return result;
}
