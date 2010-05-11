//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINPROCMIXIN_H
#define MWINPROCMIXIN_H

#include <string>
#include <map>

class MWinProcMixin
{
  public:
					MWinProcMixin();
	virtual			~MWinProcMixin();

	HWND			GetHandle() const						{ return mHandle; }
	void			SetHandle(HWND inHandle);

	static MWinProcMixin*
					Fetch(HWND inHandle);
	
	virtual void	Create(const MRect& inBounds, const std::string& inTitle);

//	void			SubClass();

	typedef bool (MWinProcMixin::*MWMCall)(HWND inHWnd, UINT inUMsg, WPARAM inWParam,
						LPARAM inLParam, int& outResult);
	
	void			AddHandler(UINT inMessage, MWMCall inHandler)
						{ mHandlers[inMessage] = inHandler; }

  protected:

	virtual void	CreateParams(DWORD& outStyle, DWORD& outExStyle,
						std::wstring& outClassName, HMENU& outMenu);
	virtual void	RegisterParams(UINT& outStyle, HCURSOR& outCursor,
						HICON& outIcon, HICON& outSmallIcon, HBRUSH& outBackground);

	  //virtual void	CreateHandle(MCreateParams& inParam);
	
	virtual bool	WMDestroy(HWND inHWnd, UINT inUMsg, WPARAM inWParam,
						LPARAM inLParam, int& outResult);
	virtual bool	WMKeydown(HWND inHWnd, UINT inUMsg, WPARAM inWParam,
						LPARAM inLParam, int& outResult);
	virtual bool	WMChar(HWND inHWnd, UINT inUMsg, WPARAM inWParam,
						LPARAM inLParam, int& outResult);

//	virtual bool	DispatchKeyDown(const MKeyDown& inKeyDown);

	virtual INT		WinProc(HWND inHWnd, UINT inUMsg,
						WPARAM inWParam, LPARAM inLParam);
	virtual int		DefProc(HWND inHWnd, UINT inUMsg, 
						WPARAM inWParam, LPARAM inLParam);
	
  private:
	typedef std::map<UINT,MWMCall>				MHandlerTable;

	HWND			mHandle;
	WNDPROC			mOldWinProc;
	MHandlerTable	mHandlers;
	bool			mSuppressNextWMChar;

	static LRESULT CALLBACK
					WinProcCallBack(HWND inHWnd, UINT inUMsg,
						WPARAM inWParam, LPARAM inLParam);
};

#endif