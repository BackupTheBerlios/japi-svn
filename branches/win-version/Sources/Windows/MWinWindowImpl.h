//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINWINDOWIMPL_H
#define MWINWINDOWIMPL_H

#include <map>

#include "MWindowImpl.h"

class MWinWindowImpl : public MWindowImpl
{
  public:
					MWinWindowImpl(MWindow* inWindow);
	virtual			~MWinWindowImpl();

	HWND			GetHandle() const						{ return mHandle; }
	void			SetHandle(HWND inHandle);

	static MWinWindowImpl*
					FetchWindow(HWND inHandle);
	
	virtual void	Create(MRect inBounds, std::string inTitle);
	//virtual void	CreateParams(MCreateParams& ioParams);
	//virtual void	RegisterParams(MRegisterParams& ioParams);

//	void			SubClass();

	typedef bool (MWinWindowImpl::*MWMCall)(HWND inHWnd, UINT inUMsg, WPARAM inWParam,
						LPARAM inLParam, int& outResult);
	
	void			AddHandler(UINT inMessage, MWMCall inHandler)
						{ mHandlers[inMessage] = inHandler; }

  protected:

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