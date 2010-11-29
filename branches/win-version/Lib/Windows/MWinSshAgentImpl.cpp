//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include <wincrypt.h>

#include "MWinSshAgentImpl.h"
#include "..\Ssh\MSshPacket.h"

#include <wincrypt.h>

using namespace CryptoPP;
using namespace std;

class MCertificateStore
{
  public:
	
	static MCertificateStore&	Instance();
	
				operator HCERTSTORE ()			{ return mCertificateStore; }

  private:
				MCertificateStore();
				~MCertificateStore();

	HCERTSTORE	mCertificateStore;
};

MCertificateStore::MCertificateStore()
{
	mCertificateStore = ::CertOpenSystemStoreW(nil, L"MY");
}

MCertificateStore::~MCertificateStore()
{
	if (mCertificateStore != nil)
		::CertCloseStore(mCertificateStore, 0);
}

MCertificateStore& MCertificateStore::Instance()
{
	static MCertificateStore sInstance;
	return sInstance;
}

// --------------------------------------------------------------------

MWinSshAgentImpl::MWinSshAgentImpl()
	: mCertificateContext(nil)
{
}

MWinSshAgentImpl::~MWinSshAgentImpl()
{
	if (mCertificateContext != nil)
		::CertFreeCertificateContext(mCertificateContext);
}

bool MWinSshAgentImpl::GetFirstIdentity(
	Integer&		e,
	Integer&		n,
	string&			outComment)
{
	if (mCertificateContext != nil)
		::CertFreeCertificateContext(mCertificateContext);
	
	mCertificateContext = ::CertEnumCertificatesInStore(MCertificateStore::Instance(), nil);

	bool result = false;
	if (mCertificateContext != nil)
		result = GetIdentity(e, n, outComment);
	return result;
}

bool MWinSshAgentImpl::GetNextIdentity(
	Integer&		e,
	Integer&		n,
	string&			outComment)
{
	mCertificateContext = ::CertEnumCertificatesInStore(
		MCertificateStore::Instance(), mCertificateContext);

	bool result = false;
	if (mCertificateContext != nil)
		result = GetIdentity(e, n, outComment);
	return result;
}

bool MWinSshAgentImpl::GetIdentity(
	Integer&		e,
	Integer&		n,
	string&			outComment)
{
	DWORD cbPublicKeyStruc = 0;
	bool result = false;

	PCERT_INFO certInfo = nil;

	if (mCertificateContext->pCertInfo != nil)
	{
		PCERT_PUBLIC_KEY_INFO pk = &mCertificateContext->pCertInfo->SubjectPublicKeyInfo;
		
		if (::CryptDecodeObject(
			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
			RSA_CSP_PUBLICKEYBLOB,
			pk->PublicKey.pbData,
		    pk->PublicKey.cbData,
		    0,
		    nil,
		    &cbPublicKeyStruc))
		{
			vector<uint8> b(cbPublicKeyStruc);
			
			if (::CryptDecodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
				RSA_CSP_PUBLICKEYBLOB,
				pk->PublicKey.pbData,
			    pk->PublicKey.cbData,
			    0,
			    &b[0],
			    &cbPublicKeyStruc))
			{
				PUBLICKEYSTRUC* pks = reinterpret_cast<PUBLICKEYSTRUC*>(&b[0]);

#pragma message("uitbreiden error check")
				RSAPUBKEY* pkd = reinterpret_cast<RSAPUBKEY*>(&b[0] + sizeof(PUBLICKEYSTRUC));
				byte* data = reinterpret_cast<byte*>(&b[0] + sizeof(RSAPUBKEY) + sizeof(PUBLICKEYSTRUC));
				
				e = pkd->pubexp;
				
				// public key is in little endian format
				uint32 len = pkd->bitlen / 8;
				
				reverse(data, data + len);
				n = Integer(data, len);
				
				result = true;
			}
		}
	}

	return result;
}

bool MWinSshAgentImpl::SignData(
	const MSshPacket&	inBlob,
	const MSshPacket&	inData,
	vector<uint8>&		outSignature)
{
	bool result = false;
	BOOL freeKey = false;
	DWORD keySpec, cb;
	HCRYPTPROV key;
	
	if (::CryptAcquireCertificatePrivateKey(mCertificateContext, 0, nil, &key, &keySpec, &freeKey))
	{
		HCRYPTHASH hash;

		if (::CryptCreateHash(key, CALG_SHA1, nil, 0, &hash))
		{
			if (::CryptHashData(hash, inData.peek(), inData.size(), 0))
			{
				cb = 0;
				::CryptSignHash(hash, keySpec, nil, 0, nil, &cb);
				
				if (cb > 0)
				{
					vector<uint8> b(cb);
					if (::CryptSignHash(hash, keySpec, nil, 0, &b[0], &cb))
					{
						// data is in little endian format
						reverse(&b[0], &b[0] + cb);
						swap(outSignature, b);
						result = true;
					}
				}
			}
			
			::CryptDestroyHash(hash);
		}
		
		if (freeKey)
			::CryptReleaseContext(key, 0);
	}
	
	return result;
}

MSshAgentImpl* MSshAgentImpl::Create()
{
	return new MWinSshAgentImpl();
}

//const int kMaxSecondsForDeletedCertificate = 5;
//
//class MCertificateStore
//{
//  public:
//	static MCertificateStore&		Instance();
//	
//	operator HCERTSTORE() const		{ return mStore; }
//	
//	MEventOut<void()>				eStoreChanged;
//	
//  private:
//				MCertificateStore();
//				~MCertificateStore();
//
//	void		Reopen();
//
//	HEventIn<void(double)>			eIdle;
//	void		Idle(double);
//	
//	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message,
//				WPARAM wParam, LPARAM lParam);
//	
//	HCERTSTORE	mStore;
//	HANDLE		mEvent;
//	HWND		mPageant;
//};
//
//HCertStore&	HCertStore::Instance()
//{
//	static MCertificateStore sInstance;
//	return sInstance;
//}
//
//MCertificateStore::MCertificateStore()
//	: eIdle(this, &MCertificateStore::Idle)
//	, fStore(nil)
//	, fEvent(::CreateEvent(nil, false, false, nil))
//	, fPageant(nil)
//{
//	Reopen();
//
//	::CertControlStore(fStore, 0, CERT_STORE_CTRL_NOTIFY_CHANGE, &fEvent);
//	
//	AddRoute(gApp->ePulse, eIdle);
//	
//	HINSTANCE inst = gApp->impl()->GetInstanceHandle();
//
//	if (not HRegisterWindowClassEx(0, &MCertificateStore::WndProc, 0, 0,
//		LoadIconA(inst, MAKEINTRESOURCEA(ID_DEF_DOC_ICON)),
//		LoadCursorA(NULL, IDC_ARROW), (HBRUSH)::GetStockObject(BLACK_BRUSH),
//		nil, kPageantName, LoadIconA(inst,
//		MAKEINTRESOURCEA(ID_DEF_DOC_ICON))))
//	{
//		ThrowIfOSErr(::GetLastError());
//	}
//	
//	fPageant = HCreateWindowEx(0, kPageantName, kPageantName,
//		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
//		100, 100, NULL, NULL, NULL);
//		
//	::ShowWindow(fPageant, SW_HIDE);
//}
//
//MCertificateStore::~MCertificateStore()
//{
//	::CloseHandle(fPageant);
//	::CertCloseStore(fStore, 0);
//}
//
//void MCertificateStore::Reopen()
//{
//	if (fStore != nil)
//		::CertCloseStore(fStore, 0);
//	
//	fStore = ::CertOpenSystemStoreW(nil, L"MY");
//}
//
//void MCertificateStore::Idle(const double&, HEventSender*)
//{
//	if (::WaitForSingleObjectEx(fEvent, 0, false) == WAIT_OBJECT_0)
//	{
//		Reopen();
//
//		eStoreChanged(true, this);
//
//		::CertControlStore(fStore, 0, CERT_STORE_CTRL_NOTIFY_CHANGE, &fEvent);
//	}
//}
//
//LRESULT CALLBACK MCertificateStore::WndProc(HWND hwnd, UINT message,
//				WPARAM wParam, LPARAM lParam)
//{
//	bool result = 0;
//	
//	try
//	{
//		switch (message)
//		{
//			case WM_COPYDATA:
//			{
//				char* p = nil;
//				HANDLE mapFile = nil;
//				
//				do
//				{
//					COPYDATASTRUCT *cds = reinterpret_cast<COPYDATASTRUCT*>(lParam);
//					if (cds == nil or cds->dwData != AGENT_COPYDATA_ID)
//						break;
//					
//					char* fileName = reinterpret_cast<char*>(cds->lpData);
//					if (fileName == nil or fileName[cds->cbData - 1] != 0)
//						break;
//					
//					mapFile = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, false,
//						fileName);
//					if (mapFile == nil or mapFile == INVALID_HANDLE_VALUE)
//						break;
//					
//					p = reinterpret_cast<char*>(::MapViewOfFile(mapFile, FILE_MAP_WRITE, 0, 0, 0));
//					if (p == nil)
//						break;
//					
//					HANDLE proc = ::OpenProcess(MAXIMUM_ALLOWED, false, ::GetCurrentProcessId());
//					if (proc == nil)
//						break;
//					
//					PSECURITY_DESCRIPTOR procSD = nil, mapSD = nil;
//					PSID mapOwner, procOwner;
//					
//					if (::GetSecurityInfo(proc, SE_KERNEL_OBJECT, OWNER_SECURITY_INFORMATION,
//							&procOwner, nil, nil, nil, &procSD) != ERROR_SUCCESS)
//					{
//						if (procSD != nil)
//							::LocalFree(procSD);
//						procSD = nil;
//					}
//					::CloseHandle(proc);
//					
//					if (::GetSecurityInfo(mapFile, SE_KERNEL_OBJECT, OWNER_SECURITY_INFORMATION,
//						&mapOwner, nil, nil, nil, &mapSD) != ERROR_SUCCESS)
//					{
//						if (mapSD != nil)
//							::LocalFree(mapSD);
//						mapSD = nil;
//					}
//					
//					if (::EqualSid(mapOwner, procOwner))
//					{
//						uint32* len = reinterpret_cast<uint32*>(p);
//						
//						HSshPacket in, out;
//						in.data.assign(p + 4, byte_swapper::swap(*len));
//						
//						uint8 msg;
//						
//						in >> msg;
//						
//						HCertificate::HandleAuthMessage(msg, in, out);
//						
//						*len = byte_swapper::swap(static_cast<uint32>(out.data.length()));
//						out.data.copy(p + 4, out.data.length());
//						result = 1;
//					}
//					
//					if (procSD != nil)
//						::LocalFree(procSD);
//					
//					if (mapSD != nil)
//						::LocalFree(mapSD);
//				}
//				while (false);
//
//				if (p != nil)
//					::UnmapViewOfFile(p);
//				
//				if (mapFile != nil and mapFile != INVALID_HANDLE_VALUE)
//					::CloseHandle(mapFile);
//
//				break;
//			}
//
//			default:
//				result = ::HDefWindowProc(hwnd, message, wParam, lParam);
//				break;
//		}
//	}
//	catch (...)
//	{
//	}
//	
//	return result;
//}
//
//struct HCertificateImp
//{
//  public:
//
//	static HCertificateImp*	Create(string inSignature,
//								HCertificate* inObject);
//	static HCertificateImp*	Create(HCertificate* inObject);
//
//							~HCertificateImp();
//
//	bool					Next();
//	string					GetFriendlyName() const;
//	string					GetComment() const;
//	string					GetSignature() const;	// SHA1 hash
//	string					GetPublicKeyFileContent() const;
//	
//	bool					GetPublicRSAKey(CryptoPP::Integer& outExp,
//								CryptoPP::Integer& outN) const;
//	
//	bool					SignData(string inData, string& outSignature);
//
//	HEventIn<HCertificateImp,bool>		eStoreChanged;
//	void					StoreChanged(const bool&, HEventSender*);
//	
//	HEventIn<HCertificateImp,double>	eIdle;
//	void					Idle(const double&, HEventSender*);
//
//  private:
//							HCertificateImp(PCCERT_CONTEXT inCert,
//								string inSignature, HCertificate* inObject);
//
//	bool					Reload();
//	
//	PCCERT_CONTEXT			fCert;
//	string					fSignature;
//	long					fRefCount;
//	double					fDeletedAt, fCheckedAt;
//	static bool				sInhibitIdle;
//	HCertificate*			fCertificateObj;
//};
//
//bool HCertificateImp::sInhibitIdle = false;
//
//HCertificateImp* HCertificateImp::Create(
//	const std::string&	inSignature,
//	HCertificate*		inObject)
//{
//	string sig;
//
//	Base64Decoder d(new StringSink(sig));
//	d.Put(reinterpret_cast<const byte*>(inSignature.c_str()), inSignature.length());
//	d.MessageEnd();
//	
//	CRYPT_HASH_BLOB k;
//	k.cbData = sig.length();
//	k.pbData = const_cast<byte*>(reinterpret_cast<const byte*>(sig.c_str()));
//	
//	PCCERT_CONTEXT cert = ::CertFindCertificateInStore(
//		MCertificateStore::Instance(), X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
//		0, CERT_FIND_SHA1_HASH, &k, nil);
//	
//	return new HCertificateImp(cert, sig, inObject);
//}
//
//HCertificateImp* HCertificateImp::Create(HCertificate* inObject)
//{
//	return new HCertificateImp(nil, "", inObject);
//}
//
//HCertificateImp::HCertificateImp(
//	PCCERT_CONTEXT		inCert,
//	const string&		inSignature,
//	HCertificate*		inObject)
//	: fCert(inCert)
//	, fRefCount(1)
//	, fCertificateObj(inObject)
//	, eStoreChanged(this, &HCertificateImp::StoreChanged)
//	, eIdle(this, &HCertificateImp::Idle)
//{
//	AddRoute(eStoreChanged, MCertificateStore::Instance().eStoreChanged);
//}
//
//HCertificateImp::~HCertificateImp()
//{
////	assert(fRefCount == 0);
//	if (fCert != nil)
//		::CertFreeCertificateContext(fCert);
//	fCert = nil;
//}
//
//bool HCertificateImp::Reload()
//{
//	HValueChanger<bool> save(sInhibitIdle, true);
//	
//	if (fCert != nil)
//		::CertFreeCertificateContext(fCert);
//	
//	fCert = nil;
//
//	CRYPT_HASH_BLOB k;
//	k.cbData = fSignature.length();
//	k.pbData = const_cast<byte*>(reinterpret_cast<const byte*>(fSignature.c_str()));
//	
//	fCert = ::CertFindCertificateInStore(
//		MCertificateStore::Instance(), X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
//		0, CERT_FIND_SHA1_HASH, &k, nil);
//	
//	bool result = fCert != nil;
//	
//	if (result)
//		RemoveRoute(eIdle, gApp->ePulse);
//	
//	return result;
//}
//
//bool HCertificateImp::Next()
//{
//	HValueChanger<bool> save(sInhibitIdle, true);
//
//	fCert = ::CertEnumCertificatesInStore(
//		MCertificateStore::Instance(), fCert);
//	
//	return fCert != nil;
//}
//
//string HCertificateImp::GetFriendlyName() const
//{
//	HValueChanger<bool> save(sInhibitIdle, true);
//
//	string result;
//	::HCertGetNameString(fCert, CERT_NAME_FRIENDLY_DISPLAY_TYPE,
//		0, nil, result);
//	return result;
//}
//
//string HCertificateImp::GetComment() const
//{
//	HValueChanger<bool> save(sInhibitIdle, true);
//
//	string result;
//	::HCertGetNameString(fCert, CERT_NAME_EMAIL_TYPE, 0, nil, result);
//	return result;
//}
//
//string HCertificateImp::GetSignature() const
//{
//	HValueChanger<bool> save(sInhibitIdle, true);
//
//	byte hash[20];	// SHA1 hash is always 20 bytes
//	DWORD cbHash = sizeof(hash);
//	
//	if (not ::CertGetCertificateContextProperty(
//		fCert, CERT_HASH_PROP_ID, hash, &cbHash))
//	{
//		throw HError(::GetLastError());
//	}
//
//	string s;
//	Base64Encoder enc(new StringSink(s));
//	
//	enc.Put(hash, cbHash);
//	enc.MessageEnd(true);
//	
//	return s;
//}
//
//bool HCertificateImp::GetPublicRSAKey(CryptoPP::Integer& outExp,
//	CryptoPP::Integer& outN) const
//{
//	HValueChanger<bool> save(sInhibitIdle, true);
//
//	DWORD cbPublicKeyStruc = 0;
//	bool result = false;
//
//	PCERT_INFO certInfo = nil;
//
//	if (fCert != nil and fCert->pCertInfo != nil)
//	{
//		PCERT_PUBLIC_KEY_INFO pk = &fCert->pCertInfo->SubjectPublicKeyInfo;
//		
//		if (::CryptDecodeObject(
//			X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
//			RSA_CSP_PUBLICKEYBLOB,
//			pk->PublicKey.pbData,
//		    pk->PublicKey.cbData,
//		    0,
//		    nil,
//		    &cbPublicKeyStruc))
//		{
//			HAutoBuf<char> b(new char[cbPublicKeyStruc]);
//			
//			if (::CryptDecodeObject(X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
//				RSA_CSP_PUBLICKEYBLOB,
//				pk->PublicKey.pbData,
//			    pk->PublicKey.cbData,
//			    0,
//			    &b[0],
//			    &cbPublicKeyStruc))
//			{
//				PUBLICKEYSTRUC* pks = reinterpret_cast<PUBLICKEYSTRUC*>(&b[0]);
//
//#pragma message("uitbreiden error check")
//				RSAPUBKEY* pkd = reinterpret_cast<RSAPUBKEY*>(&b[0] + sizeof(PUBLICKEYSTRUC));
//				byte* data = reinterpret_cast<byte*>(&b[0] + sizeof(RSAPUBKEY) + sizeof(PUBLICKEYSTRUC));
//				
//				outExp = pkd->pubexp;
//				
//				// public key is in little endian format
//				
//				uint32 len = pkd->bitlen / 8;
//				
//				reverse(data, data + len);
//				outN = Integer(data, len);
//				
//				result = true;
//			}
//		}
//	}
//
//	return result;
//}
//
//string HCertificateImp::GetPublicKeyFileContent() const
//{
//	HValueChanger<bool> save(sInhibitIdle, true);
//
//	ThrowIfNil(fCert);
//	
//	Integer e, n;
//	GetPublicRSAKey(e, n);
//	
//	HSshPacket p;
//	p << "ssh-rsa" << e << n;
//	
//	string blob;
//	Base64Encoder enc(new StringSink(blob));
//	
//	enc.Put(reinterpret_cast<const byte*>(p.data.c_str()), p.data.length());
//	enc.MessageEnd(true);
//	
//	string owner;
//	::HCertGetNameString(fCert, CERT_NAME_EMAIL_TYPE, 0, nil, owner);
//	
//	string blob2;
//	
//	for (string::iterator i = blob.begin(); i != blob.end(); ++i)
//	{
//		if (not isspace(*i))
//			blob2 += *i;
//	}
//
//	return HStrings::GetFormattedIndString(4009, 0, owner, blob, blob2, owner);
//}
//
//bool HCertificateImp::SignData(string inData, string& outSignature)
//{
//	HValueChanger<bool> save(sInhibitIdle, true);
//
//	bool result = false;
//	BOOL freeKey = false;
//	DWORD keySpec, cb;
//	HCRYPTPROV key;
//	
//	if (::CryptAcquireCertificatePrivateKey(fCert, 0, nil, &key, &keySpec, &freeKey))
//	{
//		HCRYPTHASH hash;
//
//		if (::CryptCreateHash(key, CALG_SHA1, nil, 0, &hash))
//		{
//			if (::CryptHashData(hash, reinterpret_cast<const byte*>(inData.c_str()), inData.length(), 0))
//			{
//				cb = 0;
//				::CryptSignHash(hash, keySpec, nil, 0, nil, &cb);
//				
//				if (cb > 0)
//				{
//					HAutoBuf<byte> b(new byte[cb]);
//					if (::CryptSignHash(hash, keySpec, nil, 0, &b[0], &cb))
//					{
//						reverse(&b[0], &b[0] + cb);
//						outSignature.assign(reinterpret_cast<char*>(&b[0]), cb);
//						result = true;
//					}
//					else
//					{
//						throw HError(::GetLastError(), false, false);
//					}
//				}
//			}
//			
//			::CryptDestroyHash(hash);
//		}
//		
//		if (freeKey)
//			::CryptReleaseContext(key, 0);
//	}
//	
//	return result;
//}
//
//void HCertificateImp::StoreChanged(const bool&, HEventSender*)
//{
//	HValueChanger<bool> save(sInhibitIdle, true);
//
//	if (not Reload())
//	{
//		fDeletedAt = ::system_time();
//		fCheckedAt = ::system_time();
//		AddRoute(eIdle, gApp->ePulse);
//	}
//}
//
//void HCertificateImp::Idle(const double& inTime, HEventSender*)
//{
//	if (not sInhibitIdle and fCheckedAt + 1 < ::system_time())
//	{
//		fCheckedAt = ::system_time();
//
//		if (fCert != nil or Reload())
//			RemoveRoute(eIdle, gApp->ePulse);
//		else if (inTime > fDeletedAt + kMaxSecondsForDeletedCertificate)
//		{
//			RemoveRoute(eIdle, gApp->ePulse);
//			HCertificate::eCertificateDeleted(true, fCertificateObj);
//		}
//	}
//}
//
////--------------------
//
//HEventOut<bool>	HCertificate::eCertificateDeleted;
//
//HCertificate::HCertificate()
//	: fImpl(HCertificateImp::Create(this))
//{
//}
//
//HCertificate::HCertificate(string inSignature)
//	: fImpl(HCertificateImp::Create(inSignature, this))
//{
////	if (not fImpl->Load())
////		THROW((pErrCertificateNotFound));
//}
//
//HCertificate::~HCertificate()
//{
//	delete fImpl;
//}
//
//bool HCertificate::Next(HCertificate& ioCertificate)
//{
//	ThrowIfNil(ioCertificate.fImpl);
//	return ioCertificate.fImpl->Next();
//}
//
//HCertificate* HCertificate::Select(HWindow* inParent)
//{
//	HCertificate* result = nil;
//
//	const char* sig = gPrefs->GetPrefString("auth-certificate", nil);
//	
//	if (sig != nil)
//	{
//		try
//		{
//			result = new HCertificate(sig);
//		}
//		catch (...)
//		{
//			result = nil;
//		}
//	}
//
//	return result;
//}
//
//string HCertificate::GetFriendlyName() const
//{
//	ThrowIfNil(fImpl);
//	return fImpl->GetFriendlyName();
//}
//
//string HCertificate::GetComment() const
//{
//	ThrowIfNil(fImpl);
//	return fImpl->GetComment();
//}
//
//string HCertificate::GetSignature() const
//{
//	ThrowIfNil(fImpl);
//	return fImpl->GetSignature();
//}
//
//bool HCertificate::GetPublicRSAKey(CryptoPP::Integer& outExp,
//	CryptoPP::Integer& outN) const
//{
//	ThrowIfNil(fImpl);
//	return fImpl->GetPublicRSAKey(outExp, outN);
//}
//
//string HCertificate::GetPublicKeyFileContent() const
//{
//	ThrowIfNil(fImpl);
//	return fImpl->GetPublicKeyFileContent();
//}
//
//bool HCertificate::SignData(string inData, string& outSignature)
//{
//	ThrowIfNil(fImpl);
//	return fImpl->SignData(inData, outSignature);
//}
//
//enum kSshAgentMsgs {
//	
///*
// * SSH1 agent messages.
// */
//	SSH_AGENTC_REQUEST_RSA_IDENTITIES = 1,
//	SSH_AGENT_RSA_IDENTITIES_ANSWER,
//	SSH_AGENTC_RSA_CHALLENGE,
//	SSH_AGENT_RSA_RESPONSE,
///*
// * Messages common to SSH1 and OpenSSH's SSH2.
// */
//	SSH_AGENT_FAILURE,
//	SSH_AGENT_SUCCESS,
//
///*
// * SSH1 agent messages.
// */
//
//	SSH_AGENTC_ADD_RSA_IDENTITY,
//	SSH_AGENTC_REMOVE_RSA_IDENTITY,
//	SSH_AGENTC_REMOVE_ALL_RSA_IDENTITIES,	/* openssh private? */
//
///*
// * OpenSSH's SSH2 agent messages.
// */
//	SSH_AGENTC_REQUEST_IDENTITIES = 11,
//	SSH_AGENT_IDENTITIES_ANSWER,
//	SSH_AGENTC_SIGN_REQUEST,
//	SSH_AGENT_SIGN_RESPONSE,
//	SSH_AGENTC_ADD_IDENTITY = 17,
//	SSH_AGENTC_REMOVE_IDENTITY,
//	SSH_AGENTC_REMOVE_ALL_IDENTITIES
//};
//
//void HCertificate::HandleAuthMessage(uint8 inMessage, HSshPacket& in, HSshPacket& out)
//{
//	switch (inMessage)
//	{
//		case SSH_AGENTC_REQUEST_IDENTITIES:
//		{
//			HCertificate cert;
//			uint32 cnt = 0;
//			
//			while (HCertificate::Next(cert))
//				++cnt;
//			
//			out << uint8(SSH_AGENT_IDENTITIES_ANSWER) << cnt;
//			
//			while (HCertificate::Next(cert))
//			{
//				CryptoPP::Integer e, n;
//				
//				cert.GetPublicRSAKey(e, n);
//
//				HSshPacket blob;
//				blob << "ssh-rsa" << e << n;
//				
//				out << blob.data << cert.GetComment();
//			}
//			break;
//		}
//		
//		case SSH_AGENTC_SIGN_REQUEST:
//		{
//			string blob, data;
//			uint32 flags;
//			
//			in >> blob >> data;
//
//			HCertificate cert;
//
//			while (HCertificate::Next(cert))
//			{
//				CryptoPP::Integer e, n;
//				cert.GetPublicRSAKey(e, n);
//
//				HSshPacket p;
//				p << "ssh-rsa" << e << n;
//				
//				if (p.data == blob)
//				{
//					std::string sig;
//						// no matter what, send a bogus sig if needed
//					cert.SignData(data, sig);
//					
//					HSshPacket ps;
//					ps << "ssh-rsa" << sig;
//
//					out << uint8(SSH_AGENT_SIGN_RESPONSE) << ps.data;
//					break;
//				}
//			}
//
//			if (out.data.length() == 0)
//				out << uint8(SSH_AGENT_FAILURE);
//			break;
//		}
//		
//		default:
//			out << uint8(SSH_AGENT_FAILURE);
//			break;
//	}
//}
//
//
//#if 0
//#include <sys/un.h>
//#include <sys/socket.h>
//#endif
//
//#include <cerrno>
//#include <fcntl.h>
//
//#include <cryptopp/integer.h>
//
//#include "MSsh.h"
//#include "MSshAgent.h"
//
//using namespace std;
//using namespace CryptoPP;
//
//enum {
//	
//	/* Messages for the authentication agent connection. */
//	SSH_AGENTC_REQUEST_RSA_IDENTITIES =	1,
//	SSH_AGENT_RSA_IDENTITIES_ANSWER,
//	SSH_AGENTC_RSA_CHALLENGE,
//	SSH_AGENT_RSA_RESPONSE,
//	SSH_AGENT_FAILURE,
//	SSH_AGENT_SUCCESS,
//	SSH_AGENTC_ADD_RSA_IDENTITY,
//	SSH_AGENTC_REMOVE_RSA_IDENTITY,
//	SSH_AGENTC_REMOVE_ALL_RSA_IDENTITIES,
//	
//	/* private OpenSSH extensions for SSH2 */
//	SSH2_AGENTC_REQUEST_IDENTITIES = 11,
//	SSH2_AGENT_IDENTITIES_ANSWER,
//	SSH2_AGENTC_SIGN_REQUEST,
//	SSH2_AGENT_SIGN_RESPONSE,
//	SSH2_AGENTC_ADD_IDENTITY = 17,
//	SSH2_AGENTC_REMOVE_IDENTITY,
//	SSH2_AGENTC_REMOVE_ALL_IDENTITIES,
//	
//	/* smartcard */
//	SSH_AGENTC_ADD_SMARTCARD_KEY,
//	SSH_AGENTC_REMOVE_SMARTCARD_KEY,
//	
//	/* lock/unlock the agent */
//	SSH_AGENTC_LOCK,
//	SSH_AGENTC_UNLOCK,
//	
//	/* add key with constraints */
//	SSH_AGENTC_ADD_RSA_ID_CONSTRAINED,
//	SSH2_AGENTC_ADD_ID_CONSTRAINED,
//	SSH_AGENTC_ADD_SMARTCARD_KEY_CONSTRAINED,
//	
//	SSH_AGENT_CONSTRAIN_LIFETIME = 1,
//	SSH_AGENT_CONSTRAIN_CONFIRM,
//	
//	/* extended failure messages */
//	SSH2_AGENT_FAILURE = 30,
//	
//	/* additional error code for ssh.com's ssh-agent2 */
//	SSH_COM_AGENT2_FAILURE = 102,
//	
//	SSH_AGENT_OLD_SIGNATURE = 0x01
//};
//
//MSshAgent* MSshAgent::Create()
//{
//	unique_ptr<MSshAgent> result;
//
//	const char* authSock = getenv("SSH_AUTH_SOCK");
//	
//	//if (authSock != nil)
//	//{
//	//	struct sockaddr_un addr = {};
//	//	addr.sun_family = AF_LOCAL;
//	//	strcpy(addr.sun_path, authSock);
//	//	
//	//	int sock = socket(AF_LOCAL, SOCK_STREAM, 0);
//	//	if (sock >= 0)
//	//	{
//	//		if (fcntl(sock, F_SETFD, 1) < 0)
//	//			close(sock);
//	//		else if (connect(sock, (const sockaddr*)&addr, sizeof(addr)) < 0)
//	//			close(sock);
//	//		else
//	//			result.reset(new MSshAgent(sock));
//	//	}
//	//}
//
//	return result.release();
//}
//
//MSshAgent::MSshAgent(
//	int			inSock)
//	: mSock(inSock)
//	, mCount(0)
//{
//}
//
//MSshAgent::~MSshAgent()
//{
//	//close(mSock);
//}
//
//bool MSshAgent::GetFirstIdentity(
//	Integer&	e,
//	Integer&	n,
//	string&		outComment)
//{
//	bool result = false;
//	
//	mCount = 0;
//
//	MSshPacket out;
//	uint8 msg = SSH2_AGENTC_REQUEST_IDENTITIES;
//	out << msg;
//	
//	if (RequestReply(out, mIdentities))
//	{
//		mIdentities >> msg;
//		
//		if (msg == SSH2_AGENT_IDENTITIES_ANSWER)
//		{
//			mIdentities >> mCount;
//
//			if (mCount > 0 and mCount < 1024)
//				result = GetNextIdentity(e, n, outComment);
//		}
//	}
//	
//	return result;
//}
//
//bool MSshAgent::GetNextIdentity(
//	Integer&	e,
//	Integer&	n,
//	string&		outComment)
//{
//	bool result = false;
//	
//	while (result == false and mCount-- > 0 and not mIdentities.empty())
//	{
//		MSshPacket blob;
//
//		mIdentities >> blob >> outComment;
//		
//		string type;
//		blob >> type;
//		
//		if (type != "ssh-rsa")
//			continue;
//
//		blob >> e >> n;
//
//		result = true;
//	}
//	
//	return result;
//}
//
//bool MSshAgent::RequestReply(
//	MSshPacket&		out,
//	MSshPacket&		in)
//{
//	bool result = false;
//	
////	net_swapper swap;
////	
////	uint32 l = out.size();
////	l = swap(l);
////	
////	if (write(mSock, &l, sizeof(l)) == sizeof(l) and
////		write(mSock, out.peek(), out.size()) == int32(out.size()) and
////		read(mSock, &l, sizeof(l)) == sizeof(l))
////	{
////		l = swap(l);
////		
////		if (l < 256 * 1024)
////		{
////			char b[1024];
////
////			uint32 k = l;
////			if (k > sizeof(b))
////				k = sizeof(b);
////			
////			while (l > 0)
////			{
////				if (read(mSock, b, k) != k)
////					break;
////				
////				in.data.append(b, k);
////				
////				l -= k;
////			}
////			
////			result = (l == 0);
////		}
////	}
////	
//	return result;
//}
//
//void MSshAgent::SignData(
//	const string&	inBlob,
//	const string&	inData,
//	string&			outSignature)
//{
//	MSshPacket out;
//	uint8 msg = SSH2_AGENTC_SIGN_REQUEST;
//
//	uint32 flags = 0;
//	
//	out << msg << inBlob << inData << flags;
//	
//	MSshPacket in;
//	if (RequestReply(out, in))
//	{
//		in >> msg;
//		
//		if (msg == SSH2_AGENT_SIGN_RESPONSE)
//			in >> outSignature;
//	}
//}
