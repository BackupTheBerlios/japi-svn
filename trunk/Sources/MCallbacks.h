//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MCallbacks.h 119 2007-04-25 08:26:21Z maarten $
	
	copyright Maarten L. Hekkelman
	
	Based on MP2PEvents but simplified. Callbacks do not disconnect automatically
	like P2PEvents do. There's also only one callback method that can be registered
	for a callback object.
*/

#ifndef MCALLBACKS_H
#define MCALLBACKS_H

#include <memory>

typedef struct _GladeXML GladeXML;

#include "MAlerts.h"

// shield implementation details in our namespace

namespace MCallbackNS
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

// Handler is a base class that delivers the actual callback to the owner of MCallbackIn
// as stated above, it is derived from HandlerBase.
// We use the Curiously Recurring Template Pattern to capture all the needed info from the
// MCallbackInHandler class. 

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

// MCallbackInHandler is the complete handler object that has all the type info
// needed to deliver the callback.

template<class C, typename Function>
struct MCallbackInHandler : public Handler<MCallbackInHandler<C, Function>, C, Function>
{
	typedef Handler<MCallbackInHandler,C,Function>	base;
	typedef typename base::Callback					CallBackProc;
	typedef C										Owner;
	
										MCallbackInHandler(Owner* inOwner, CallBackProc inHandler)
											: fOwner(inOwner)
											, fHandler(inHandler)
										{
										}
	
	C*									fOwner;
	CallBackProc						fHandler;
};

// MCallbackOutHandler objects. Again we use the Curiously Recurring Template Pattern
// to pull in type info we don't yet know.

template<class CallBackIn, typename Function>
struct MCallbackOutHandler {};

template<class CallBackIn, typename R>
struct MCallbackOutHandler<CallBackIn, R()>
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
					
					MCallbackOutHandler& handler = *reinterpret_cast<MCallbackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						result = handler.mHandler->DoCallBack();
					
					return result;
				}
};

template<class CallBackIn>
struct MCallbackOutHandler<CallBackIn, void()>
{
	std::auto_ptr<CallBackIn>		mHandler;
	
	static void	GCallback(
					GObject*		inWidget,
					gpointer		inData)
				{
					MCallbackOutHandler& handler = *reinterpret_cast<MCallbackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						handler.mHandler->DoCallBack();
				}
};

template<class CallBackIn, typename R, typename T1>
struct MCallbackOutHandler<CallBackIn, R(T1)>
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
					
					MCallbackOutHandler& handler = *reinterpret_cast<MCallbackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						result = handler.mHandler->DoCallBack(inArg1);
					
					return result;
				}
};

template<class CallBackIn, typename T1>
struct MCallbackOutHandler<CallBackIn, void(T1)>
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
					MCallbackOutHandler& handler = *reinterpret_cast<MCallbackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						handler.mHandler->DoCallBack(inArg1);
				}
};

template<class CallBackIn, typename R, typename T1, typename T2>
struct MCallbackOutHandler<CallBackIn, R(T1, T2)>
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
					
					MCallbackOutHandler& handler = *reinterpret_cast<MCallbackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						result = handler.mHandler->DoCallBack(inArg1, inArg2);
					
					return result;
				}
};

template<class CallBackIn, typename T1, typename T2>
struct MCallbackOutHandler<CallBackIn, void(T1, T2)>
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
					MCallbackOutHandler& handler = *reinterpret_cast<MCallbackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						handler.mHandler->DoCallBack(inArg1, inArg2);
				}
};

template<class CallBackIn, typename R, typename T1, typename T2, typename T3>
struct MCallbackOutHandler<CallBackIn, R(T1, T2, T3)>
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
struct MCallbackOutHandler<CallBackIn, R(T1, T2, T3, T4)>
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
					
					MCallbackOutHandler& handler = *reinterpret_cast<MCallbackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						result = handler.mHandler->DoCallBack(inArg1, inArg2, inArg3, inArg4);
					
					return result;
				}
};

template<class CallBackIn, typename T1, typename T2, typename T3, typename T4>
struct MCallbackOutHandler<CallBackIn, void(T1, T2, T3, T4)>
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
					MCallbackOutHandler& handler = *reinterpret_cast<MCallbackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						handler.mHandler->DoCallBack(inArg1, inArg2, inArg3, inArg4);
				}
};

template<class CallBackIn, typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
struct MCallbackOutHandler<CallBackIn, R(T1, T2, T3, T4, T5)>
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
struct MCallbackOutHandler<CallBackIn, R(T1, T2, T3, T4, T5, T6)>
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
struct MCallbackOutHandler<CallBackIn, void(T1, T2, T3, T4, T5, T6)>
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
					MCallbackOutHandler& handler = *reinterpret_cast<MCallbackOutHandler*>(inData);
					
					if (handler.mHandler.get() != nil)
						handler.mHandler->DoCallBack(inArg1, inArg2, inArg3, inArg4, inArg5, inArg6);
				}
};

// a type factory

template<typename Function>
struct MakeCallBackHandler
{
	typedef MCallbackOutHandler<
		HandlerBase<Function>,
		Function> type;
};

} // namespace

template<typename Function>
class MCallback : public MCallbackNS::MakeCallBackHandler<Function>::type
{
	typedef typename		MCallbackNS::MakeCallBackHandler<Function>::type	base_class;
  public:
							MCallback()		{}
							~MCallback()	{}
	
	template<class C>
	void					SetProc(C* inOwner,
								typename MCallbackNS::MCallbackInHandler<C, Function>::CallBackProc	inProc)
							{
								base_class* self = static_cast<base_class*>(this);
								typedef typename MCallbackNS::MCallbackInHandler<C,Function> Handler;
								
								if (inOwner != nil and inProc != nil)
									self->mHandler.reset(new Handler(inOwner, inProc));
								else
									self->mHandler.reset(nil);
							}

  private:
							MCallback(const MCallback&);
	MCallback&				operator=(const MCallback&);
};

template<typename Function, class C>
void SetCallback(MCallback<Function>& inCallBack, C* inObject,
	typename MCallbackNS::MCallbackInHandler<C,Function>::CallBackProc inProc)
{
	inCallBack.SetProc(inObject, inProc);
}

template<typename Function>
class MSlot : public MCallbackNS::MakeCallBackHandler<Function>::type
{
	typedef typename		MCallbackNS::MakeCallBackHandler<Function>::type	base_class;
  public:
	
	template<class C>
							MSlot(
								C*				inOwner,
								typename MCallbackNS::MCallbackInHandler<C, Function>::CallBackProc
												inProc)
							{
								base_class* self = static_cast<base_class*>(this);
								typedef typename MCallbackNS::MCallbackInHandler<C,Function> Handler;
								
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

	void					Connect(
								GladeXML*		inGladeXML,
								const char*		inSignalName)
							{
								glade_xml_signal_connect_data(inGladeXML, inSignalName, 
									G_CALLBACK(&base_class::GCallback), this);
							}

	void					Block(
								GtkWidget*		inObject,
								const char*		inSignalName)
							{
								g_signal_handlers_block_by_func(G_OBJECT(inObject),
									(void*)G_CALLBACK(&base_class::GCallback), this);
							}
	
	void					Unblock(
								GtkWidget*		inObject,
								const char*		inSignalName)
							{
								g_signal_handlers_unblock_by_func(G_OBJECT(inObject),
									(void*)G_CALLBACK(&base_class::GCallback), this);
							}
	
  private:

							MSlot(const MSlot&);
	MSlot&					operator=(const MSlot&);
};

#endif
