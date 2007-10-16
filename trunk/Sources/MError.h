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

#ifndef MERROR_H
#define MERROR_H

#include <exception>
#include <iostream>

class MException : public std::exception
{
  public:
				MException(int inErr);
				MException(const char* inErrMsg, ...);

	virtual const char*	what() const throw();

  protected:
				MException() {}

	char		mMessage[1024];
};

#ifdef NDEBUG

class StOKToThrow
{
  public:
	StOKToThrow() {}
	~StOKToThrow() {}
};

#define SIGNAL_THROW(a)

#define PRINT(x)

#else

class StOKToThrow
{
  public:
	StOKToThrow()	{ ++sOkToThrow; }
	~StOKToThrow()	{ --sOkToThrow; }
	
	static bool		IsOK()				{ return sOkToThrow; }
	
  private:
	static int		sOkToThrow;
};

void __signal_throw(const char* inCode, const char* inFunction, const char* inFile, int inLine);
void __m_debug_str(const char* inStr, ...);
//void __report_mach_error(const char* func, mach_error_t e);

#define PRINT(x)	__m_debug_str x

#define	SIGNAL_THROW(a)	__signal_throw(a, __func__, __FILE__, __LINE__);

#endif

#define THROW_IF_OSERROR(x) \
	do { OSStatus __err = (x); if (__err != noErr) { SIGNAL_THROW(#x) throw MException(x); } } while (false)

#define THROW_IF_CFCREATE_FAILED(x) \
	do { if ((x) == nil) { SIGNAL_THROW(#x) throw MException("CFCreate failed for variable %s", #x); } } while (false)

#define THROW_IF_POSIX_ERROR(x) \
	do { if ((int)(x) == -1) { SIGNAL_THROW(#x) throw MException("POSIX error %d: '%s'", errno, strerror(errno)); } } while (false)

#define THROW(x) \
	do { SIGNAL_THROW(#x) throw MException x; } while (false)

#define THROW_IF_NIL(x) \
	do { if ((x) == nil) { SIGNAL_THROW(#x) throw MException("Nil pointer %s", #x); } } while (false)

#define THROW_ON_MACH_ERR(x)		\
	do { kern_return_t err = (x); if (err != KERN_SUCCESS) { SIGNAL_THROW(#x) throw MException("mach error %d: ('%s') calling %s", err, mach_error_string(err), #x); } } while (false)

#define ASSERT(x, m)		if (not (x)) { SIGNAL_THROW(#x " => " #m); throw MException m; }

namespace MError
{

void DisplayError(const std::exception& inErr);

}

#endif
