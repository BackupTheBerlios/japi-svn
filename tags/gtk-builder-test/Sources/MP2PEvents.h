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

/*	$Id: MP2PEvents.h 119 2007-04-25 08:26:21Z maarten $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten Hekkelman
	
	Created: Friday April 28 2000 15:57:00
	Completely rewritten in July 2005
	
	This is an implementation of point to point events. You can deliver messages from one
	object to another using the EventIn and EventOut objects. One of the huge advantages
	of using p2p events instead of simply calling another objects's method is that dangling
	pointers are avoided, the event objects know how to detach from each other.
	
	For now this implementation allows the passing of 0 to 4 arguments. If needed more
	can be added very easily.
	
	Usage:
	
	struct foo
	{
		MEventOut<void(int)>	eBeep;
	};
	
	struct bar
	{
		MEventIn<void(int)>		eBeep;
		
		void					Beep(int inBeeps);
		
								bar();
	};
	
	bar::bar()
		: eBeep(this, &bar::Beep)
	{
	}
	
	void bar::Beep(int inBeeps)
	{
		cout << "beep " << inBeeps << " times" << endl;
	}

	...
	
	foo f;
	bar b;
	
	AddRoute(f.eBeep, b.eBeep);
	
	f.eBeep(2);
	
*/

#ifndef MP2PEVENTS_H
#define MP2PEVENTS_H

#include <memory>
#include <list>
#include <functional>
#include <algorithm>

// forward declarations of our event types

template<typename Function> class MEventIn;
template<typename Function> class MEventOut;

// shield implementation details in our namespace

namespace MP2PEvent
{

// HandlerBase is a templated pure virtual base class. This is needed to declare
// HEventIn objects without knowing the type of the object to deliver the event to.

template<typename Function> struct HandlerBase;

template<>
struct HandlerBase<void()>
{
	virtual 							~HandlerBase() {}
	virtual void						HandleEvent() = 0;
};

template<typename T1>
struct HandlerBase<void(T1)>
{
	virtual 							~HandlerBase() {}
	virtual void						HandleEvent(T1 a1) = 0;
};

template<typename T1, typename T2>
struct HandlerBase<void(T1,T2)>
{
	virtual 							~HandlerBase() {}
	virtual void						HandleEvent(T1 a1, T2 a2) = 0;
};

template<typename T1, typename T2, typename T3>
struct HandlerBase<void(T1,T2,T3)>
{
	virtual 							~HandlerBase() {}
	virtual void						HandleEvent(T1 a1, T2 a2, T3 a3) = 0;
};

template<typename T1, typename T2, typename T3, typename T4>
struct HandlerBase<void(T1,T2,T3,T4)>
{
	virtual 							~HandlerBase() {}
	virtual void						HandleEvent(T1 a1, T2 a2, T3 a3, T4 a4) = 0;
};

// Handler is a base class that delivers the actual event to the owner of MEventIn
// as stated above, it is derived from HandlerBase.
// We use the Curiously Recurring Template Pattern to capture all the needed info from the
// MEventInHandler class. 

template<class Derived, class Owner, typename Function> struct Handler;

template<class Derived, class Owner>
struct Handler<Derived, Owner, void()> : public HandlerBase<void()>
{
	typedef void (Owner::*Callback)();
	
	virtual void						HandleEvent()
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;
											(owner->*func)();
										}
};

template<class Derived, class Owner, typename T1>
struct Handler<Derived, Owner, void(T1)> : public HandlerBase<void(T1)>
{
	typedef void (Owner::*Callback)(T1);
	
	virtual void						HandleEvent(T1 a1)
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;
											(owner->*func)(a1);
										}
};

template<class Derived, class Owner, typename T1, typename T2>
struct Handler<Derived, Owner, void(T1, T2)> : public HandlerBase<void(T1, T2)>
{
	typedef void (Owner::*Callback)(T1, T2);
	
	virtual void						HandleEvent(T1 a1, T2 a2)
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;
											(owner->*func)(a1, a2);
										}
};

template<class Derived, class Owner, typename T1, typename T2, typename T3>
struct Handler<Derived, Owner, void(T1, T2, T3)> : public HandlerBase<void(T1, T2, T3)>
{
	typedef void (Owner::*Callback)(T1, T2, T3);
	
	virtual void						HandleEvent(T1 a1, T2 a2, T3 a3)
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;
											(owner->*func)(a1, a2, a3);
										}
};

template<class Derived, class Owner, typename T1, typename T2, typename T3, typename T4>
struct Handler<Derived, Owner, void(T1, T2, T3, T4)> : public HandlerBase<void(T1, T2, T3, T4)>
{
	typedef void (Owner::*Callback)(T1, T2, T3, T4);
	
	virtual void						HandleEvent(T1 a1, T2 a2, T3 a3, T4 a4)
										{
											Derived* self = static_cast<Derived*>(this);
											Owner* owner = self->fOwner;
											Callback func = self->fHandler;
											(owner->*func)(a1, a2, a3, a4);
										}
};

// MEventInHandler is the complete handler object that has all the type info
// needed to deliver the event.

template<class C, typename Function>
struct MEventInHandler : public Handler<MEventInHandler<C, Function>, C, Function>
{
	typedef Handler<MEventInHandler, C, Function>	base;
	typedef typename base::Callback					Callback;
	typedef C										Owner;
	
										MEventInHandler(Owner* inOwner, Callback inHandler)
											: fOwner(inOwner)
											, fHandler(inHandler)
										{
										}
	
	C*									fOwner;
	Callback							fHandler;
};

// MEventOutHandler objects. Again we use the Curiously Recurring Template Pattern
// to pull in type info we don't yet know.

template<class EventOut, class EventIn, class EventInList, typename Function>
struct MEventOutHandler {};

template<class EventOut, class EventIn, class EventInList>
struct MEventOutHandler<EventOut, EventIn, EventInList, void()>
{
	void		operator() ()
				{
					typedef typename EventInList::iterator iterator;
					EventInList events = static_cast<EventOut*>(this)->GetInEvents();
					for (iterator iter = events.begin(); iter != events.end(); ++iter)
						(*iter)->GetHandler()->HandleEvent();
				}
};

template<class EventOut, class EventIn, class EventInList, typename T1>
struct MEventOutHandler<EventOut, EventIn, EventInList, void(T1)>
{
	void		operator() (T1 a1)
				{
					typedef typename EventInList::iterator iterator;
					EventInList events = static_cast<EventOut*>(this)->GetInEvents();
					for (iterator iter = events.begin(); iter != events.end(); ++iter)
						(*iter)->GetHandler()->HandleEvent(a1);
				}
};

template<class EventOut, class EventIn, class EventInList, typename T1, typename T2>
struct MEventOutHandler<EventOut, EventIn, EventInList, void(T1, T2)>
{
	void		operator() (T1 a1, T2 a2)
				{
					typedef typename EventInList::iterator iterator;
					EventInList events = static_cast<EventOut*>(this)->GetInEvents();
					for (iterator iter = events.begin(); iter != events.end(); ++iter)
						(*iter)->GetHandler()->HandleEvent(a1, a2);
				}
};

template<class EventOut, class EventIn, class EventInList, typename T1, typename T2, typename T3>
struct MEventOutHandler<EventOut, EventIn, EventInList, void(T1, T2, T3)>
{
	void		operator() (T1 a1, T2 a2, T3 a3)
				{
					typedef typename EventInList::iterator iterator;
					EventInList events = static_cast<EventOut*>(this)->GetInEvents();
					for (iterator iter = events.begin(); iter != events.end(); ++iter)
						(*iter)->GetHandler()->HandleEvent(a1, a2, a3);
				}
};

template<class EventOut, class EventIn, class EventInList, typename T1, typename T2, typename T3, typename T4>
struct MEventOutHandler<EventOut, EventIn, EventInList, void(T1, T2, T3, T4)>
{
	void		operator() (T1 a1, T2 a2, T3 a3, T4 a4)
				{
					typedef typename EventInList::iterator iterator;
					EventInList events = static_cast<EventOut*>(this)->GetInEvents();
					for (iterator iter = events.begin(); iter != events.end(); ++iter)
						(*iter)->GetHandler()->HandleEvent(a1, a2, a3, a4);
				}
};

// a type factory

template<typename Function>
struct make_eventouthandler
{
	typedef MEventOutHandler<
		MEventOut<Function>,
		MEventIn<Function>,
		std::list<MEventIn<Function>*>,
		Function> type;
};

} // namespace

// the actual MEventIn class

template<typename Function>
class MEventIn
{
	typedef MEventOut<Function>							MEventOut;
	typedef std::list<MEventOut*>						MEventOutList;
	typedef typename MP2PEvent::HandlerBase<Function>	HandlerBase;
	
  public:
							template<class C>
							MEventIn(C* inOwner, typename MP2PEvent::MEventInHandler<C, Function>::Callback inHandler)
								: fHandler(new MP2PEvent::MEventInHandler<C, Function>(inOwner, inHandler))
							{
							}
	virtual					~MEventIn();
							
	HandlerBase*			GetHandler() const					{ return fHandler.get(); }

	void					AddEventOut(MEventOut* inEventOut);
	void					RemoveEventOut(MEventOut* inEventOut);

  private:
							MEventIn(const MEventIn&);
	MEventIn&				operator=(const MEventIn&);


	std::auto_ptr<HandlerBase>
							fHandler;
	MEventOutList			fOutEvents;
};

// and the MEventOut class

template<typename Function>
class MEventOut : public MP2PEvent::make_eventouthandler<Function>::type
{
	typedef MEventIn<Function>							MEventIn;
	typedef std::list<MEventIn*>						MEventInList;

  public:
							MEventOut();
	virtual					~MEventOut();

	void					AddEventIn(MEventIn* inEventIn);
	void					RemoveEventIn(MEventIn* inEventIn);
	
	MEventInList			GetInEvents() const							{ return fInEvents; }
	
	unsigned int			CountRoutes() const							{ return fInEvents.size(); }
	
  private:
							MEventOut(const MEventOut&);
	MEventOut&				operator=(const MEventOut&);

	MEventInList			fInEvents;
};

// Use these functions to create and delete routes.

template <typename Function>
void AddRoute(MEventIn<Function>& inEventIn, MEventOut<Function>& inEventOut)
{
	inEventOut.AddEventIn(&inEventIn);
}

template <typename Function>
void AddRoute(MEventOut<Function>& inEventOut, MEventIn<Function>& inEventIn)
{
	inEventOut.AddEventIn(&inEventIn);
}

template <class Function>
void RemoveRoute(MEventIn<Function>& inEventIn, MEventOut<Function>& inEventOut)
{
	inEventOut.RemoveEventIn(&inEventIn);
}

template <class Function>
void RemoveRoute(MEventOut<Function>& inEventOut, MEventIn<Function>& inEventIn)
{
	inEventOut.RemoveEventIn(&inEventIn);
}

// implementation of the MEventIn methods

template<typename Function>
MEventIn<Function>::~MEventIn()
{
	MEventOutList events;
	std::swap(events, fOutEvents);

	std::for_each(events.begin(), events.end(),
		std::bind2nd(std::mem_fun(&MEventOut::RemoveEventIn), this));
}

template<typename Function>
void MEventIn<Function>::AddEventOut(MEventOut* inEventOut)
{
	if (std::find(fOutEvents.begin(), fOutEvents.end(), inEventOut) == fOutEvents.end())
		fOutEvents.push_back(inEventOut);
}

template<typename Function>
void MEventIn<Function>::RemoveEventOut(MEventOut* inEventOut)
{
	if (fOutEvents.size())
		fOutEvents.erase(std::remove(fOutEvents.begin(), fOutEvents.end(), inEventOut), fOutEvents.end());
}

// implementation of the MEventOut methods

template<typename Function>
MEventOut<Function>::MEventOut()
{
}

template<typename Function>
MEventOut<Function>::~MEventOut()
{
	std::for_each(fInEvents.begin(), fInEvents.end(),
		std::bind2nd(std::mem_fun(&MEventIn::RemoveEventOut), this));
}

template<typename Function>
void MEventOut<Function>::AddEventIn(MEventIn* inEventIn)
{
	if (std::find(fInEvents.begin(), fInEvents.end(), inEventIn) == fInEvents.end())
	{
		inEventIn->AddEventOut(this);
		fInEvents.push_back(inEventIn);
	}
}

template<typename Function>
void MEventOut<Function>::RemoveEventIn(MEventIn* inEventIn)
{
	inEventIn->RemoveEventOut(this);
	fInEvents.erase(std::remove(fInEvents.begin(), fInEvents.end(), inEventIn), fInEvents.end());
}

#endif
