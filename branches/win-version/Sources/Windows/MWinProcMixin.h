//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MWINPROCMIXIN_H
#define MWINPROCMIXIN_H

#include <boost/function.hpp>

class MWinProcMixin
{
  public:
	struct MCreateParams
	{
		DWORD		exStyle;
		std::string	className;
		std::string	windowName;
		DWORD		style;
		int			x;
		int			y;
		int			width;
		int			height;
		HWND		parent;
		HMENU		menu;
	};
	
	struct MRegisterParams
	{
		DWORD		style;
		int			wndExtra;
		HICON		icon;
		HICON		smallIcon;
		HCURSOR		cursor;
		HBRUSH		background;
	};
	  
					MWinProcMixin();
	virtual			~MWinProcMixin();

	static MWinProcMixin*
					FetchMixin(HWND inHandle);
	
	virtual void	CreateWindow(MRect inBounds, std::string inTitle);
	virtual void	CreateParams(MCreateParams& ioParams);
	virtual void	RegisterParams(MRegisterParams& ioParams);

	HWND			GetHandle() const						{ return mHandle; }
	void			SetHandle(HWND inHandle);

	void			SubClass();

	typedef boost::function<void(HWND inHWnd, UINT inUMsg, WPARAM inWParam,
						LPARAM inLParam, int& outResult)>	MWinProcCallbackType;
	
	void			AddHandler(UINT inMessage, MWinProcCallbackType inHandler)
						{ mHandlers[inMessage] = inHandler; }

  protected:

	virtual void	CreateHandle(MCreateParams& inParam);
	
	virtual bool	WMDestroy(HWND inHWnd, UINT inUMsg, WPARAM inWParam,
						LPARAM inLParam, int& outResult);
	virtual bool	WMKeydown(HWND inHWnd, UINT inUMsg, WPARAM inWParam,
						LPARAM inLParam, int& outResult);
	virtual bool	WMChar(HWND inHWnd, UINT inUMsg, WPARAM inWParam,
						LPARAM inLParam, int& outResult);

	virtual bool	DispatchKeyDown(const MKeyDown& inKeyDown);

	virtual INT		WinProc(HWND inHWnd, UINT inUMsg,
						WPARAM inWParam, LPARAM inLParam);
	virtual int		DefProc(HWND inHWnd, UINT inUMsg, 
						WPARAM inWParam, LPARAM inLParam);
	
  private:
	typedef std::map<UINT,MWinProcCallbackType>				MHandlerTable;

	HWND			mHandle;
	WNDPROC			mOldWinProc;
	MHandlerTable	mHandlers;
	bool			mSuppressNextWMChar;

	static LRESULT CALLBACK
					WinProcCallBack(HWND inHWnd, UINT inUMsg,
						WPARAM inWParam, LPARAM inLParam);

};

#endif