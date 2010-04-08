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

// we're using the boost iterating macro's to build the specialisations
// for our handler objects
#if not defined(BOOST_PP_IS_ITERATING)

#include <iostream>
#include <boost/preprocessor.hpp>
#include <memory>
#include <string>

#include "MAlerts.h"

// shield implementation details in our namespace

namespace MCallbackNS
{

// HandlerBase is a templated pure virtual base class. This is needed to declare
// HCallbackIn objects without knowing the type of the object to deliver the callback to.

template<typename Function> struct HandlerBase;
// (specialisations are at the bottom of the file)

// Handler is a base class that delivers the actual callback to the owner of MCallbackIn
// as stated above, it is derived from HandlerBase.
// We use the Curiously Recurring Template Pattern to capture all the needed info from the
// MCallbackInHandler class. 

template<class Derived, class Owner, typename Function> struct Handler;
// (again, specialisations are at the bottom of the file)

// MCallbackInHandler is the complete handler object that has all the type info
// needed to deliver the callback.

template<class C, typename Function>
struct MCallbackInHandler : public Handler<MCallbackInHandler<C, Function>, C, Function>
{
	typedef Handler<MCallbackInHandler,C,Function>	base;
	typedef typename base::Callback					CallbackProc;
	typedef C										Owner;
	
										MCallbackInHandler(Owner* inOwner, CallbackProc inHandler)
											: fOwner(inOwner)
											, fHandler(inHandler)
										{
										}

	C*									fOwner;
	CallbackProc						fHandler;
};

// MCallbackOutHandler objects. Again we use the Curiously Recurring Template Pattern
// to pull in type info we don't yet know.

template<class CallbackIn, typename Function>
struct MCallbackOutHandler {};
// (and yet again, specialisations are at the bottom of the file)

// Now include the specialisations using the boost preprocessor macro's
#define  BOOST_PP_FILENAME_1 "MCallbacks.h"
#define  BOOST_PP_ITERATION_LIMITS (0, 9)
#include BOOST_PP_ITERATE()

// a type factory

template<typename Function>
struct MakeCallbackHandler
{
	typedef MCallbackOutHandler<
		HandlerBase<Function>,
		Function> type;
};

} // namespace

template<typename Function>
class MCallback : public MCallbackNS::MakeCallbackHandler<Function>::type
{
	typedef typename		MCallbackNS::MakeCallbackHandler<Function>::type	base_class;
  public:
							MCallback()		{}
							~MCallback()	{}
	
	template<class C>
	void					SetProc(C* inOwner,
								typename MCallbackNS::MCallbackInHandler<C, Function>::CallbackProc	inProc)
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
void SetCallback(MCallback<Function>& inCallback, C* inObject,
	typename MCallbackNS::MCallbackInHandler<C,Function>::CallbackProc inProc)
{
	inCallback.SetProc(inObject, inProc);
}

// MSlot is used to connect to g_object signals.
// A special case is the support for GtkBuilder handlers, a kludge
// makes it possible to connect to all 'handler's in one call.

typedef std::vector<std::pair<GObject*,std::string> > MSignalHandlerArray;
class MSlotProviderMixin
{
  public:
	virtual void			GetSlotsForHandler(
								const char*		inHandler,
								MSignalHandlerArray&
												outSlots) = 0;
};

template<typename Function>
class MSlot : public MCallbackNS::MakeCallbackHandler<Function>::type
{
	typedef typename		MCallbackNS::MakeCallbackHandler<Function>::type	base_class;
  public:
	
	template<class C>
							MSlot(
								C*				inOwner,
								typename MCallbackNS::MCallbackInHandler<C, Function>::CallbackProc
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
								MSlotProviderMixin*
												inWindow,
								const char*		inHandlerName)
							{
								MSignalHandlerArray slots;
								inWindow->GetSlotsForHandler(inHandlerName, slots);
								for (MSignalHandlerArray::iterator slot = slots.begin(); slot != slots.end(); ++slot)
								{
									g_signal_connect(slot->first, slot->second.c_str(),
										G_CALLBACK(&base_class::GCallback), this);
								}
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

	GObject*				GetSourceGObject() const			{ return base_class::mSendingGObject; }

  private:

							MSlot(const MSlot&);
	MSlot&					operator=(const MSlot&);
};

#define MCALLBACKS_H

#else

#define N BOOST_PP_ITERATION()

//
//	Specializations for the handlers for a range of parameters
//
//	First the HandlerBase, which is a pure virtual base class
//
template<typename R BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM_PARAMS(N,typename T)>
struct HandlerBase<R(BOOST_PP_ENUM_PARAMS(N,T))>
{
	virtual 							~HandlerBase() {}
	virtual R							DoCallback(BOOST_PP_ENUM_BINARY_PARAMS(N,T,a)) = 0;
};

//
//	Next is the Handler which derives from HandlerBase
//
template<class Derived, class Owner, typename R BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM_PARAMS(N,typename T)>
struct Handler<Derived, Owner, R(BOOST_PP_ENUM_PARAMS(N,T))> : public HandlerBase<R(BOOST_PP_ENUM_PARAMS(N,T))>
{
	typedef R (Owner::*Callback)(BOOST_PP_ENUM_PARAMS(N,T));
	
	virtual R							DoCallback(BOOST_PP_ENUM_BINARY_PARAMS(N,T,a))
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;

											R result = R();	
											
											try
											{
												result = (owner->*func)(BOOST_PP_ENUM_PARAMS(N,a));
											}
											catch (const std::exception& e)
											{
												DisplayError(e);
											}
											
											return result;
										}
};

template<class Derived, class Owner BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM_PARAMS(N,typename T)>
struct Handler<Derived, Owner, void(BOOST_PP_ENUM_PARAMS(N,T))> : public HandlerBase<void(BOOST_PP_ENUM_PARAMS(N,T))>
{
	typedef void (Owner::*Callback)(BOOST_PP_ENUM_PARAMS(N,T));
	
	virtual void						DoCallback(BOOST_PP_ENUM_BINARY_PARAMS(N,T,a))
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;

											try
											{
												(owner->*func)(BOOST_PP_ENUM_PARAMS(N,a));
											}
											catch (const std::exception& e)
											{
												DisplayError(e);
											}
										}
};

//
//	And now the callback handlers
//
template<class CallbackIn, typename R BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM_PARAMS(N,typename T)>
struct MCallbackOutHandler<CallbackIn, R(BOOST_PP_ENUM_PARAMS(N,T))>
{
	std::unique_ptr<CallbackIn>	mHandler;
	GObject*					mSendingGObject;

	R			operator() (BOOST_PP_ENUM_BINARY_PARAMS(N,T,a))
				{
					if (mHandler.get() != nil)
						return mHandler->DoCallback(BOOST_PP_ENUM_PARAMS(N,a));
					return R();
				}

	static R	GCallback(
					GObject*		inObject,
					BOOST_PP_ENUM_BINARY_PARAMS(N,T,a) BOOST_PP_COMMA_IF(N)
					gpointer		inData)
				{
					R result = R();

					try
					{
						MCallbackOutHandler& handler = *reinterpret_cast<MCallbackOutHandler*>(inData);
						
						if (handler.mHandler.get() != nil)
						{
							handler.mSendingGObject = inObject;
							result = handler.mHandler->DoCallback(BOOST_PP_ENUM_PARAMS(N,a));
						}
					}
					catch (...)
					{
						std::cerr << "caught exception in GCallback" << std::endl;
					}
					
					return result;
				}
};

template<class CallbackIn BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM_PARAMS(N,typename T)>
struct MCallbackOutHandler<CallbackIn, void(BOOST_PP_ENUM_PARAMS(N,T))>
{
	std::unique_ptr<CallbackIn>	mHandler;
	GObject*					mSendingGObject;

	void		operator() (BOOST_PP_ENUM_BINARY_PARAMS(N,T,a))
				{
					if (mHandler.get() != nil)
						mHandler->DoCallback(BOOST_PP_ENUM_PARAMS(N,a));
				}

	static void	GCallback(
					GObject*		inObject,
					BOOST_PP_ENUM_BINARY_PARAMS(N,T,a) BOOST_PP_COMMA_IF(N)
					gpointer		inData)
				{
					try
					{
						MCallbackOutHandler& handler = *reinterpret_cast<MCallbackOutHandler*>(inData);
						
						if (handler.mHandler.get() != nil)
						{
							handler.mSendingGObject = inObject;
							handler.mHandler->DoCallback(BOOST_PP_ENUM_PARAMS(N,a));
						}
					}
					catch (...)
					{
						std::cerr << "caught exception in GCallback" << std::endl;
					}
				}
};

#endif

#endif
