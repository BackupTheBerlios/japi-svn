/*
	Copyright (c) 2006, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*	$Id: MCallBacks.h 119 2007-04-25 08:26:21Z maarten $
	
	copyright Maarten L. Hekkelman
	
	Based on MP2PEvents but simplified. Callbacks do not disconnect automatically
	like P2PEvents do. There's also only one callback method that can be registered
	for a callback object.
*/

#ifndef MCALLBACKS_H
#define MCALLBACKS_H

#include <memory>

#include "MAlerts.h"

// shield implementation details in our namespace

namespace MCallBackNS
{

// HandlerBase is a templated pure virtual base class. This is needed to declare
// HCallBackIn objects without knowing the type of the object to deliver the callback to.

template<typename Function> struct HandlerBase;

template<typename R>
struct HandlerBase<R()>
{
	virtual 							~HandlerBase() {}
	virtual R							DoCallBack() = 0;
};

template<typename R, typename T1>
struct HandlerBase<R(T1)>
{
	virtual 							~HandlerBase() {}
	virtual R							DoCallBack(T1 a1) = 0;
};

template<typename R, typename T1, typename T2>
struct HandlerBase<R(T1,T2)>
{
	virtual 							~HandlerBase() {}
	virtual R							DoCallBack(T1 a1, T2 a2) = 0;
};

template<typename R, typename T1, typename T2, typename T3>
struct HandlerBase<R(T1,T2,T3)>
{
	virtual 							~HandlerBase() {}
	virtual R							DoCallBack(T1 a1, T2 a2, T3 a3) = 0;
};

template<typename R, typename T1, typename T2, typename T3, typename T4>
struct HandlerBase<R(T1,T2,T3,T4)>
{
	virtual 							~HandlerBase() {}
	virtual R							DoCallBack(T1 a1, T2 a2, T3 a3, T4 a4) = 0;
};

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
struct HandlerBase<R(T1,T2,T3,T4,T5)>
{
	virtual 							~HandlerBase() {}
	virtual R							DoCallBack(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5) = 0;
};

template<typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
struct HandlerBase<R(T1,T2,T3,T4,T5,T6)>
{
	virtual 							~HandlerBase() {}
	virtual R							DoCallBack(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6) = 0;
};

// Handler is a base class that delivers the actual callback to the owner of MCallBackIn
// as stated above, it is derived from HandlerBase.
// We use the Curiously Recurring Template Pattern to capture all the needed info from the
// MCallBackInHandler class. 

template<class Derived, class Owner, typename Function> struct Handler;

template<class Derived, class Owner, typename R>
struct Handler<Derived, Owner, R()> : public HandlerBase<R()>
{
	typedef R (Owner::*Callback)();
	
	virtual R							DoCallBack()
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;

											R result = R();	
											
											try
											{
												result = (owner->*func)();
											}
											catch (const std::exception& e)
											{
												DisplayError(e);
											}
											
											return result;
										}
};

template<class Derived, class Owner>
struct Handler<Derived, Owner, void()> : public HandlerBase<void()>
{
	typedef void (Owner::*Callback)();
	
	virtual void						DoCallBack()
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;

											try
											{
												(owner->*func)();
											}
											catch (const std::exception& e)
											{
												DisplayError(e);
											}
										}
};

template<class Derived, class Owner, typename R, typename T1>
struct Handler<Derived, Owner, R(T1)> : public HandlerBase<R(T1)>
{
	typedef R (Owner::*Callback)(T1);
	
	virtual R							DoCallBack(T1 a1)
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;

											R result = R();	
											
											try
											{
												result = (owner->*func)(a1);
											}
											catch (const std::exception& e)
											{
												DisplayError(e);
											}
											
											return result;
										}
};

template<class Derived, class Owner, typename T1>
struct Handler<Derived, Owner, void(T1)> : public HandlerBase<void(T1)>
{
	typedef void (Owner::*Callback)(T1);
	
	virtual void						DoCallBack(T1 a1)
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;

											try
											{
												(owner->*func)(a1);
											}
											catch (const std::exception& e)
											{
												DisplayError(e);
											}
										}
};

template<class Derived, class Owner, typename R, typename T1, typename T2>
struct Handler<Derived, Owner, R(T1, T2)> : public HandlerBase<R(T1, T2)>
{
	typedef R (Owner::*Callback)(T1, T2);
	
	virtual R							DoCallBack(T1 a1, T2 a2)
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;

											R result = R();	
											
											try
											{
												result = (owner->*func)(a1, a2);
											}
											catch (const std::exception& e)
											{
												DisplayError(e);
											}
											
											return result;
										}
};

template<class Derived, class Owner, typename T1, typename T2>
struct Handler<Derived, Owner, void(T1, T2)> : public HandlerBase<void(T1, T2)>
{
	typedef void (Owner::*Callback)(T1, T2);
	
	virtual void						DoCallBack(T1 a1, T2 a2)
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;
											
											try
											{
												(owner->*func)(a1, a2);
											}
											catch (const std::exception& e)
											{
												DisplayError(e);
											}
										}
};

template<class Derived, class Owner, typename R, typename T1, typename T2, typename T3>
struct Handler<Derived, Owner, R(T1, T2, T3)> : public HandlerBase<R(T1, T2, T3)>
{
	typedef R (Owner::*Callback)(T1, T2, T3);
	
	virtual R							DoCallBack(T1 a1, T2 a2, T3 a3)
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;

											R result = R();	
											
											try
											{
												result = (owner->*func)(a1, a2, a3);
											}
											catch (const std::exception& e)
											{
												DisplayError(e);
											}
											
											return result;
										}
};

template<class Derived, class Owner, typename T1, typename T2, typename T3>
struct Handler<Derived, Owner, void(T1, T2, T3)> : public HandlerBase<void(T1, T2, T3)>
{
	typedef void (Owner::*Callback)(T1, T2, T3);
	
	virtual void						DoCallBack(T1 a1, T2 a2, T3 a3)
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;
											
											try
											{
												(owner->*func)(a1, a2, a3);
											}
											catch (const std::exception& e)
											{
												DisplayError(e);
											}
										}
};

template<class Derived, class Owner, typename R, typename T1, typename T2, typename T3, typename T4>
struct Handler<Derived, Owner, R(T1, T2, T3, T4)> : public HandlerBase<R(T1, T2, T3, T4)>
{
	typedef R (Owner::*Callback)(T1, T2, T3, T4);
	
	virtual R							DoCallBack(T1 a1, T2 a2, T3 a3, T4 a4)
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;

											R result = R();	
											
											try
											{
												result = (owner->*func)(a1, a2, a3, a4);
											}
											catch (const std::exception& e)
											{
												DisplayError(e);
											}
											
											return result;
										}
};

template<class Derived, class Owner, typename T1, typename T2, typename T3, typename T4>
struct Handler<Derived, Owner, void(T1, T2, T3, T4)> : public HandlerBase<void(T1, T2, T3, T4)>
{
	typedef void (Owner::*Callback)(T1, T2, T3, T4);
	
	virtual void						DoCallBack(T1 a1, T2 a2, T3 a3, T4 a4)
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;
											
											try
											{
												(owner->*func)(a1, a2, a3, a4);
											}
											catch (const std::exception& e)
											{
												DisplayError(e);
											}
										}
};

template<class Derived, class Owner, typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
struct Handler<Derived, Owner, R(T1, T2, T3, T4, T5)> : public HandlerBase<R(T1, T2, T3, T4, T5)>
{
	typedef R (Owner::*Callback)(T1, T2, T3, T4, T5);
	
	virtual R							DoCallBack(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5)
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;

											R result = R();	
											
											try
											{
												result = (owner->*func)(a1, a2, a3, a4, a5);
											}
											catch (const std::exception& e)
											{
												DisplayError(e);
											}
											
											return result;
										}
};

template<class Derived, class Owner, typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
struct Handler<Derived, Owner, R(T1, T2, T3, T4, T5, T6)> : public HandlerBase<R(T1, T2, T3, T4, T5, T6)>
{
	typedef R (Owner::*Callback)(T1, T2, T3, T4, T5, T6);
	
	virtual R							DoCallBack(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6)
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;

											R result = R();	
											
											try
											{
												result = (owner->*func)(a1, a2, a3, a4, a5, a6);
											}
											catch (const std::exception& e)
											{
												DisplayError(e);
											}
											
											return result;
										}
};

template<class Derived, class Owner, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
struct Handler<Derived, Owner, void(T1, T2, T3, T4, T5, T6)> : public HandlerBase<void(T1, T2, T3, T4, T5, T6)>
{
	typedef void (Owner::*Callback)(T1, T2, T3, T4, T5, T6);
	
	virtual void						DoCallBack(T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6)
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;
											
											try
											{
												(owner->*func)(a1, a2, a3, a4, a5, a6);
											}
											catch (const std::exception& e)
											{
												DisplayError(e);
											}
										}
};

// MCallBackInHandler is the complete handler object that has all the type info
// needed to deliver the callback.

template<class C, typename Function>
struct MCallBackInHandler : public Handler<MCallBackInHandler<C, Function>, C, Function>
{
	typedef Handler<MCallBackInHandler,C,Function>	base;
	typedef typename base::Callback					CallBackProc;
	typedef C										Owner;
	
										MCallBackInHandler(Owner* inOwner, CallBackProc inHandler)
											: fOwner(inOwner)
											, fHandler(inHandler)
										{
										}
	
	C*									fOwner;
	CallBackProc						fHandler;
};

// MCallBackOutHandler objects. Again we use the Curiously Recurring Template Pattern
// to pull in type info we don't yet know.

template<class CallBackIn, typename Function>
struct MCallBackOutHandler {};

template<class CallBackIn, typename R>
struct MCallBackOutHandler<CallBackIn, R()>
{
	std::auto_ptr<CallBackIn>		mHandler;
	
	R			operator() ()
				{
					if (mHandler.get() != nil)
						return mHandler->DoCallBack();
					return R();
				}

	static R	GCallback(
					GObject*		inObject,
					gpointer		inData)
				{
					R result = R();
					
					MCallBackOutHandler& handler = *reinterpret_cast<MCallBackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						result = handler.mHandler->DoCallBack();
					
					return result;
				}
};

template<class CallBackIn>
struct MCallBackOutHandler<CallBackIn, void()>
{
	std::auto_ptr<CallBackIn>		mHandler;
	
	static void	GCallback(
					GObject*		inWidget,
					gpointer		inData)
				{
					MCallBackOutHandler& handler = *reinterpret_cast<MCallBackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						handler.mHandler->DoCallBack();
				}
};

template<class CallBackIn, typename R, typename T1>
struct MCallBackOutHandler<CallBackIn, R(T1)>
{
	std::auto_ptr<CallBackIn>		mHandler;

	R			operator() (T1 a1)
				{
					if (mHandler.get() != nil)
						return mHandler->DoCallBack(a1);
					return R();
				}

	static R	GCallback(
					GObject*		inWidget,
					T1				inArg1,
					gpointer		inData)
				{
					R result = R();
					
					MCallBackOutHandler& handler = *reinterpret_cast<MCallBackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						result = handler.mHandler->DoCallBack(inArg1);
					
					return result;
				}
};

template<class CallBackIn, typename T1>
struct MCallBackOutHandler<CallBackIn, void(T1)>
{
	std::auto_ptr<CallBackIn>		mHandler;

	void		operator() (T1 a1)
				{
					if (mHandler.get() != nil)
						mHandler->DoCallBack(a1);
				}

	static void	GCallback(
					GObject*		inWidget,
					T1				inArg1,
					gpointer		inData)
				{
					MCallBackOutHandler& handler = *reinterpret_cast<MCallBackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						handler.mHandler->DoCallBack(inArg1);
				}
};

template<class CallBackIn, typename R, typename T1, typename T2>
struct MCallBackOutHandler<CallBackIn, R(T1, T2)>
{
	std::auto_ptr<CallBackIn>		mHandler;

	R			operator() (T1 a1, T2 a2)
				{
					if (mHandler.get() != nil)
						return mHandler->DoCallBack(a1, a2);
					return R();
				}

	static R	GCallback(
					GObject*		inWidget,
					T1				inArg1,
					T2				inArg2,
					gpointer		inData)
				{
					R result = R();
					
					MCallBackOutHandler& handler = *reinterpret_cast<MCallBackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						result = handler.mHandler->DoCallBack(inArg1, inArg2);
					
					return result;
				}
};

template<class CallBackIn, typename T1, typename T2>
struct MCallBackOutHandler<CallBackIn, void(T1, T2)>
{
	std::auto_ptr<CallBackIn>		mHandler;

	void		operator() (T1 a1, T2 a2)
				{
					if (mHandler.get() != nil)
						mHandler->DoCallBack(a1, a2);
				}

	static void	GCallback(
					GObject*		inWidget,
					T1				inArg1,
					T2				inArg2,
					gpointer		inData)
				{
					MCallBackOutHandler& handler = *reinterpret_cast<MCallBackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						handler.mHandler->DoCallBack(inArg1, inArg2);
				}
};

template<class CallBackIn, typename R, typename T1, typename T2, typename T3>
struct MCallBackOutHandler<CallBackIn, R(T1, T2, T3)>
{
	std::auto_ptr<CallBackIn>		mHandler;

	R			operator() (T1 a1, T2 a2, T3 a3)
				{
					if (mHandler.get() != nil)
						return mHandler->DoCallBack(a1, a2, a3);
					return R();
				}
};

template<class CallBackIn, typename R, typename T1, typename T2, typename T3, typename T4>
struct MCallBackOutHandler<CallBackIn, R(T1, T2, T3, T4)>
{
	std::auto_ptr<CallBackIn>		mHandler;

	R			operator() (T1 a1, T2 a2, T3 a3, T4 a4)
				{
					if (mHandler.get() != nil)
						return mHandler->DoCallBack(a1, a2, a3, a4);
					return R();
				}

	static R	GCallback(
					GObject*		inWidget,
					T1				inArg1,
					T2				inArg2,
					T3				inArg3,
					T4				inArg4,
					gpointer		inData)
				{
					R result = R();
					
					MCallBackOutHandler& handler = *reinterpret_cast<MCallBackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						result = handler.mHandler->DoCallBack(inArg1, inArg2, inArg3, inArg4);
					
					return result;
				}
};

template<class CallBackIn, typename T1, typename T2, typename T3, typename T4>
struct MCallBackOutHandler<CallBackIn, void(T1, T2, T3, T4)>
{
	std::auto_ptr<CallBackIn>		mHandler;

	void		operator() (T1 a1, T2 a2, T3 a3, T4 a4)
				{
					if (mHandler.get() != nil)
						mHandler->DoCallBack(a1, a2, a3, a4);
				}

	static void	GCallback(
					GObject*		inWidget,
					T1				inArg1,
					T2				inArg2,
					T3				inArg3,
					T4				inArg4,
					gpointer		inData)
				{
					MCallBackOutHandler& handler = *reinterpret_cast<MCallBackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						handler.mHandler->DoCallBack(inArg1, inArg2, inArg3, inArg4);
				}
};

template<class CallBackIn, typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
struct MCallBackOutHandler<CallBackIn, R(T1, T2, T3, T4, T5)>
{
	std::auto_ptr<CallBackIn>		mHandler;

	R			operator() (T1 a1, T2 a2, T3 a3, T4 a4, T5 a5)
				{
					if (mHandler.get() != nil)
						return mHandler->DoCallBack(a1, a2, a3, a4, a5);
					return R();
				}
};

template<class CallBackIn, typename R, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
struct MCallBackOutHandler<CallBackIn, R(T1, T2, T3, T4, T5, T6)>
{
	std::auto_ptr<CallBackIn>		mHandler;

	R			operator() (T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6)
				{
					if (mHandler.get() != nil)
						return mHandler->DoCallBack(a1, a2, a3, a4, a5, a6);
					return R();
				}
};

template<class CallBackIn, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
struct MCallBackOutHandler<CallBackIn, void(T1, T2, T3, T4, T5, T6)>
{
	std::auto_ptr<CallBackIn>		mHandler;

	void		operator() (T1 a1, T2 a2, T3 a3, T4 a4, T5 a5, T6 a6)
				{
					if (mHandler.get() != nil)
						mHandler->DoCallBack(a1, a2, a3, a4, a5, a6);
				}

	static void	GCallback(
					GObject*		inWidget,
					T1				inArg1,
					T2				inArg2,
					T3				inArg3,
					T4				inArg4,
					T5				inArg5,
					T6				inArg6,
					gpointer		inData)
				{
					MCallBackOutHandler& handler = *reinterpret_cast<MCallBackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						handler.mHandler->DoCallBack(inArg1, inArg2, inArg3, inArg4, inArg5, inArg6);
				}
};

// a type factory

template<typename Function>
struct MakeCallBackHandler
{
	typedef MCallBackOutHandler<
		HandlerBase<Function>,
		Function> type;
};

} // namespace

template<typename Function>
class MCallBack : public MCallBackNS::MakeCallBackHandler<Function>::type
{
	typedef typename		MCallBackNS::MakeCallBackHandler<Function>::type	base_class;
  public:
							MCallBack()		{}
							~MCallBack()	{}
	
	template<class C>
	void					SetProc(C* inOwner,
								typename MCallBackNS::MCallBackInHandler<C, Function>::CallBackProc	inProc)
							{
								base_class* self = static_cast<base_class*>(this);
								typedef typename MCallBackNS::MCallBackInHandler<C,Function> Handler;
								
								if (inOwner != nil and inProc != nil)
									self->mHandler.reset(new Handler(inOwner, inProc));
								else
									self->mHandler.reset(nil);
							}

  private:
							MCallBack(const MCallBack&);
	MCallBack&				operator=(const MCallBack&);
};

template<typename Function, class C>
void SetCallBack(MCallBack<Function>& inCallBack, C* inObject,
	typename MCallBackNS::MCallBackInHandler<C,Function>::CallBackProc inProc)
{
	inCallBack.SetProc(inObject, inProc);
}

template<typename Function>
class MSlot : public MCallBackNS::MakeCallBackHandler<Function>::type
{
	typedef typename		MCallBackNS::MakeCallBackHandler<Function>::type	base_class;
  public:
	
	template<class C>
							MSlot(
								C*				inOwner,
								typename MCallBackNS::MCallBackInHandler<C, Function>::CallBackProc
												inProc)
							{
								base_class* self = static_cast<base_class*>(this);
								typedef typename MCallBackNS::MCallBackInHandler<C,Function> Handler;
								
								self->mHandler.reset(new Handler(inOwner, inProc));
							}

							~MSlot()		{}

	void					Connect(
								GtkWidget*		inObject,
								const char*		inSignalName)
							{
								g_signal_connect(G_OBJECT(inObject), inSignalName,
									G_CALLBACK(&base_class::GCallback), this);
							}
	
	void					Connect(
								GObject*		inObject,
								const char*		inSignalName)
							{
								g_signal_connect(inObject, inSignalName,
									G_CALLBACK(&base_class::GCallback), this);
							}
	
  private:

							MSlot(const MSlot&);
	MSlot&					operator=(const MSlot&);
};

#endif
