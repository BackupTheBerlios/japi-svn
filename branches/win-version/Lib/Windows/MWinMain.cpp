//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include <boost/program_options.hpp>
#include <boost/regex.hpp>

#include <windows.h>
#include <ddeml.h>

#include "MApplication.h"
#include "MError.h"

using namespace std;
namespace po = boost::program_options;

class MDDE;

class MDDEImpl
{
public:
						MDDEImpl(MDDE* inDDE, uint32 inInst, HSZ inServer, HSZ inTopic, bool inServer)
							: mDDE(inDDE)
							, mInst(inInst)
							, mServer(inServer)
							, mTopic(inTopic)
							, mIsServer(inIsServer)
						{
							sInstance = this;
						}

						~MDDEImpl()
						{
							::DdeFreeStringHandle(mInst, mServer);
							::DdeFreeStringHandle(mInst, mTopic);
							::DdeUninitialize(mInst);
						}

	static MDDEImpl*	Create(MDDE* inWinDDE);

	HSZ					GetTopic() const			{ return mTopic; }

protected:
	static MDDEImpl*	sInstance;

	MDDE*				mDDE;
	uint32				mInst;
	HSZ					mServer, mTopic;
	bool				mIsServer;
};

MDDEImpl* MDDEImpl::sInstance;

class MDDEServerImpl : public MDDEImpl
{
public:
				MDDEServerImpl(MDDE* inDDE, uint32 inInst, HSZ inServer, HSZ inTopic)
					: MDDEImpl(inDDE, inInst, inServer, inTopic)
				{
					if (not ::DdeNameService(mInst, mServer, 0, DNS_REGISTER))
						THROW(("Failed to register DDE name service"));
				}

private:
	static HDDEDATA CALLBACK
				DdeCallback(UINT uType, UINT uFmt, HCONV hconv,
					HSZ hsz1, HSZ hsz2, HDDEDATA hdata,
					DWORD dwData1, DWORD dwData2);
};

HDDEDATA CALLBACK MDDEServerImpl::DdeCallback(UINT uType, UINT uFmt, HCONV hconv,
	HSZ hsz1, HSZ hsz2, HDDEDATA hdata, DWORD dwData1, DWORD dwData2)
{
	HDDEDATA result = (HDDEDATA)DDE_FNOTPROCESSED;
	switch (uType)
	{
		case XTYP_CONNECT:
			result = (HDDEDATA)(hsz1 == sInstance->GetTopic());
			break;
		
//		case XTYP_CONNECT_CONFIRM:
//			break;
			
		case XTYP_EXECUTE:
		{
			DWORD len;
			wchar_t* cmd = reinterpret_cast<wchar_t*>(::DdeAccessData(hdata, &len));

			if (cmd != nil)
			{
				wstring text(cmd, len);
				static boost::wregex rx(L"\[(open|new)(\(\"(.+?)\"(,\s*(\d+))\))?\]");

				boost::wsmatch match;
				if (boost::regex_match(text, match, rx))
				{

				}

				::DdeUnaccessData(hdata);
			}
			break;
		}
	}
	return result;
}

HWinServerDDEImp::HDDECmdParser::HDDECmdParser(wchar_t* inText)
	: text(inText)
	, start(inText)
{
}

inline
bool HWinServerDDEImp::HDDECmdParser::isident(wchar_t inChar)
{
	return
		(inChar >= 'a' && inChar <= 'z') ||
		(inChar >= '0' && inChar <= '9') ||
		inChar == '_';
}

void HWinServerDDEImp::HDDECmdParser::GetNextToken()
{
	lookahead = TokenUndefined;
	while (lookahead == TokenUndefined)
	{
		start = text;
		switch (*text)
		{
			case 0:
				lookahead = TokenEOF;
				break;
			case '[':
			case ']':
			case '(':
			case ')':
				lookahead = *text;
				++text;
				break;
			case '-':
				if (isdigit(text[1]))
				{
					value = wcstol(text, &text, 10);
					lookahead = TokenNumber;
				}
				else
					lookahead = *text;
				break;
			case '"':
				++text;
				for (;;)
				{
					if (*text == 0)
						break;

					if (*text == '"' && text[1] == '"')
					{
						text += 2;
						continue;
					}
					
					if (*text == '"')
					{
						lookahead = TokenString;
						++text;
						break;
					}
					++text;
				}
				break;
			default:
				if (isdigit(*text))
				{
					value = wcstol(text, &text, 10);
					lookahead = TokenNumber;
				}
				else if (isident(*text))
				{
					do	++text;
					while (isident(*text));
					
					if (/*std::*/wcsncmp(start, L"open", 4) == 0)
						lookahead = TokenOpenCmd;
					else if (/*std::*/wcsncmp(start, L"new", 3) == 0)
						lookahead = TokenNewCmd;
					else
						lookahead = TokenIdent;
				}
				else
				{
					lookahead = *text;
					++text;
				}
				break;	
		}
	}
}

void HWinServerDDEImp::HDDECmdParser::Match(long inToken)
{
	if (lookahead == inToken)
		GetNextToken();
	else
	{
//		StOKToThrow ok;
		THROW((pErrStatus));
	}
}

void HWinServerDDEImp::HDDECmdParser::ParseCommands()
{
	GetNextToken();
	while (lookahead != TokenEOF)
		HandleNextCommand();
}

void HWinServerDDEImp::HDDECmdParser::HandleNextCommand()
{
	Match('[');
	if (lookahead == TokenOpenCmd)
	{
		int32 lineNr = -1;
		
		Match(TokenOpenCmd);
		Match('(');
		
		unsigned long l = static_cast<unsigned long>(text - start);
		HAutoBuf<wchar_t> path(new wchar_t[l + 1]);
		std::memcpy(path.get(), start, l * 2);
		path.get()[l] = 0;
		
		wchar_t* p = path.get();
		
		if (p[l - 1] == '"')
			p[l - 1] = 0;
		
		if (p[0] == '"')
			++p;
		
		HUrl url;
		url.SetSpecifier(HFileSpec(p));
		
		Match(TokenString);
		
		if (lookahead == ',')
		{
			Match(',');
			lineNr = value;
			Match(TokenNumber);
		}
		
		Match(')');

		gApp->Open(url, false);
		
		if (lineNr != -1)
			gApp->DocGoToLine(url, lineNr);
	}
	else if (lookahead == TokenNewCmd)
	{
		Match(TokenNewCmd);
		gApp->StartUp();
	}
	else
	{
		Match(TokenIdent);

		if (lookahead == '(')
		{
			Match('(');
			while (lookahead != TokenEOF && lookahead != ')')
				GetNextToken();
			Match(')');
		}
	}

	Match(']');
}

#pragma mark -

struct HWinClientDDEImp : public HWinDDEImp
{
				HWinClientDDEImp(HWinDDE* inWinDDE, DWORD inInst,
					HCONV inConv, HSZ inServer, HSZ inTopic);
				~HWinClientDDEImp();

	void		SendOpen(const HUrl& inURL, int32 inLineNr);
	void		SendNew();
	
	HCONV		fConv;
};

HWinClientDDEImp::HWinClientDDEImp(HWinDDE* inWinDDE, DWORD inInst,
		HCONV inConv, HSZ inServer, HSZ inTopic)
	: HWinDDEImp(inWinDDE, inInst, false, inServer, inTopic)
	, fConv(inConv)
{
}

HWinClientDDEImp::~HWinClientDDEImp()
{
	::DdeDisconnect(fConv);
}

void HWinClientDDEImp::SendOpen(const HUrl& inURL, int32 inLineNr)
{
	HFileSpec spec;
	if (inURL.GetSpecifier(spec) == kNoError)
	{
		std::wstring cmd = L"[open(\"";
		cmd += spec.GetWCharPath();
		cmd += L"\",";
		cmd += HEncoder::EncodeFromUTF8(NumToString(inLineNr).c_str());
		cmd += L")]";
		
		DWORD len = 2UL * (cmd.length() + 1);
		DWORD err = 0;
		HDDEDATA result = ::DdeClientTransaction(
			(LPBYTE)cmd.c_str(), len, fConv, 0, CF_UNICODETEXT, XTYP_EXECUTE,
			1000, &err);
		if (result)
			::DdeFreeDataHandle(result);
//		if (err)
//			DisplayError(HError(err, true, true));
	}
}

void HWinClientDDEImp::SendNew()
{
	const wchar_t kCmd[] = L"[new]";
	
	DWORD len = 2UL * (/*std::*/wcslen(kCmd) + 1);
	DWORD err = 0;
	HDDEDATA result = ::DdeClientTransaction(
		(LPBYTE)kCmd, len, fConv, 0, CF_UNICODETEXT, XTYP_EXECUTE,
		1000, &err);
	if (result)
		::DdeFreeDataHandle(result);
}

#pragma mark -

HWinDDEImp* HWinDDEImp::Create(HWinDDE* inWinDDE, const char* inAppName)
{
	HWinDDEImp* result = nil;
	
	DWORD inst = 0;
	UINT err;
	if (HasUnicode())
		err = ::DdeInitializeW(&inst, &HWinServerDDEImp::DdeCallback,
			CBF_FAIL_SELFCONNECTIONS | CBF_SKIP_ALLNOTIFICATIONS, 0);
	else
		err = ::DdeInitializeA(&inst, &HWinServerDDEImp::DdeCallback,
			CBF_FAIL_SELFCONNECTIONS | CBF_SKIP_ALLNOTIFICATIONS, 0);

	if (err == DMLERR_NO_ERROR)
	{
		HSZ srvr = ::DdeCreateStringHandleA(inst, const_cast<char*>(inAppName), CP_WINANSI);
		HSZ topic = ::DdeCreateStringHandleA(inst, "System", CP_WINANSI);
		
		HCONV conn = ::DdeConnect(inst, srvr, topic, nil);
		if (conn == 0)
			result = new HWinServerDDEImp(inWinDDE, inst, srvr, topic);
		else
			result = new HWinClientDDEImp(inWinDDE, inst, conn, srvr, topic);
	}

	return result;
}

#pragma mark -

HWinDDE& HWinDDE::Instance(const char* inAppName)
{
	static HWinDDE sInstance(inAppName);
	return sInstance;
}

HWinDDE::HWinDDE(const char* inAppName)
	: fImpl(HWinDDEImp::Create(this, inAppName))
{
}

HWinDDE::~HWinDDE()
{
	delete fImpl;
}

bool HWinDDE::IsServer() const
{
	return fImpl == nil || fImpl->fIsServer;
}

void HWinDDE::OpenURL(const HUrl& inUrl, int32 inLineNr)
{
	assert(not IsServer());
	static_cast<HWinClientDDEImp*>(fImpl)->SendOpen(inUrl, inLineNr);
}

void HWinDDE::OpenNew()
{
	assert(not IsServer());
	static_cast<HWinClientDDEImp*>(fImpl)->SendNew();
}





int WINAPI WinMain( HINSTANCE /*hInst*/, 	/*Win32 entry-point routine */
					HINSTANCE /*hPreInst*/, 
					LPSTR lpszCmdLine, 
					int /*nCmdShow*/ )
{
	vector<string> args = po::split_winmain(lpszCmdLine);
	return MApplication::Main(args);
}
