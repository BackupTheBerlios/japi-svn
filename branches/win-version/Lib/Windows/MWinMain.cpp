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

#include "MWinApplicationImpl.h"
#include "MError.h"
#include "MWinUtils.h"
#include "MDocument.h"
#include "MDocClosedNotifier.h"
#include "MCommands.h"

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
						MDDEImpl(uint32 inInst);
						~MDDEImpl();

	static uint32		Init();

	bool				IsServer() const;
	void				Send(HCONV inConversation, const wstring& inCommand);
	void				Open(const string& inFile, int32 inLineNr);
	void				New();
	void				Wait();

protected:

	MEventIn<void(HCONV)>
						eDocClosed;

	void				DocClosed(HCONV inConversation);

	HDDEDATA			Callback(UINT uType, UINT uFmt, HCONV hconv,
							HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
							DWORD dwData1, DWORD dwData2);

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

	static MDDEImpl*	sInstance;
	uint32				mInst;
	HSZ					mServer, mTopic;
	HCONV				mConv;				// 0 for server, defined for client
	set<HCONV>			mConversations;		// only for servers
	bool				mIsServer;
};

MDDEImpl* MDDEImpl::sInstance;

MDDEImpl::MDDEImpl(uint32 inInst)
	: eDocClosed(this, &MDDEImpl::DocClosed)
	, mInst(inInst)
	, mServer(::DdeCreateStringHandle(inInst, c2w(kAppName).c_str(), CP_WINUNICODE))
	, mTopic(::DdeCreateStringHandle(inInst, L"System", CP_WINUNICODE))
	, mConv(::DdeConnect(inInst, mServer, mTopic, nil))
	, mIsServer(false)
{
	sInstance = this;

	if (mConv == 0)
	{
		mIsServer = true;

		if (not ::DdeNameService(mInst, mServer, 0, DNS_REGISTER))
			THROW(("Failed to register DDE name service"));
		mConv = ::DdeConnect(inInst, mServer, mTopic, nil);
	}
}

MDDEImpl::~MDDEImpl()
{
	if (mConv != 0)
		::DdeDisconnect(mConv);

	::DdeFreeStringHandle(mInst, mServer);
	::DdeFreeStringHandle(mInst, mTopic);
	::DdeUninitialize(mInst);
}

uint32 MDDEImpl::Init()
{
	DWORD inst = 0;
	UINT err = ::DdeInitialize(&inst, &MDDEImpl::DdeCallback, 0, 0);
	if (err != DMLERR_NO_ERROR)
		inst = 0;
	return inst;
}

bool MDDEImpl::IsServer() const
{
	return mIsServer;
}

void MDDEImpl::Send(HCONV inConversation, const wstring& inCommand)
{
	DWORD err = 0;
	HDDEDATA result = ::DdeClientTransaction(
		(LPBYTE)inCommand.c_str(), inCommand.length() * sizeof(wchar_t), inConversation, 0, CF_UNICODETEXT, XTYP_EXECUTE, 1000, &err);
	if (result)
		::DdeFreeDataHandle(result);
}

HDDEDATA MDDEImpl::Callback(UINT uType, UINT uFmt, HCONV hconv,
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
			if (hconv == mConv)
				mConv = 0;
			else
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
						MFile file(w2c(match.str(3)));
						doc = gApp->OpenOneDocument(file);
					}
					else if (match.str(1) == L"new")
						doc = gApp->CreateNewDocument();

					if (doc != nil)
					{
						MWinDocClosedNotifierImpl* notifier = new MWinDocClosedNotifierImpl(hconv);
						AddRoute(notifier->eDocClosed, eDocClosed);
						doc->AddNotifier(MDocClosedNotifier(notifier), read);

						gApp->DisplayDocument(doc);
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

void MDDEImpl::DocClosed(HCONV inConversation)
{
	if (mConversations.count(inConversation))
	{
		::DdeDisconnect(inConversation);
		mConversations.erase(inConversation);
	}
}

void MDDEImpl::Open(const string& inFile, int32 inLineNr)
{
	Send(mConv, (boost::wformat(L"[open(\"%1%\",%2%)]") % c2w(inFile) % inLineNr).str());
}

void MDDEImpl::New()
{
	Send(mConv, L"[new]");
}

void MDDEImpl::Wait()
{
	while (mConv != 0)
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
	int result = 0;

	PRINT(("Hello, world!"));

	vector<string> args = po::split_winmain(lpszCmdLine);
	uint32 inst = MDDEImpl::Init();

	unique_ptr<MApplication> app(MApplication::Create(new MWinApplicationImpl(hInst)));

	if (inst != 0)
	{
		MDDEImpl dde(inst);

		if (args.empty())
			dde.New();
		else
		{
			foreach (string arg, args)
			{
				MFile file(arg);
				if (file.IsLocal())
					dde.Open(fs::system_complete(arg).native_file_string(), 0);
				else
					dde.Open(file.GetURI(), 0);
			}
		}

		if (dde.IsServer())
			result = app->RunEventLoop();
		else
			dde.Wait();
	}
	else
	{
		app->ProcessCommand(cmd_New, nil, 0, 0);
		result = app->RunEventLoop();
	}

	return result;
}
