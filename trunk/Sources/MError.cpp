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

#include "MJapieG.h"

#include <iostream>
#include <cstdarg>

#include "MError.h"
#include "MTypes.h"
#include "MUtils.h"
#include "MSound.h"
#include "MAlerts.h"

using namespace std;

#ifndef NDEBUG
int StOKToThrow::sOkToThrow = 0;
#endif

MException::MException(
	int					inErr)
{
	snprintf(mMessage, sizeof(mMessage), "OS error %d", inErr);
}

MException::MException(
	const char*			inMsg,
	...)
{
	va_list vl;
	va_start(vl, inMsg);
	vsnprintf(mMessage, sizeof(mMessage), inMsg, vl);
	va_end(vl);

cerr << endl << endl << "Throwing with msg: " << mMessage << endl << endl;
}

const char*	MException::what() const throw()
{
	return mMessage;
}

namespace MError
{

void DisplayError(
	const exception&	inErr)
{
	PlaySound("error");
	DisplayAlert("exception-alert", inErr.what());
}

}

#ifndef NDEBUG

void __m_debug_str(const char* inStr, ...)
{
	char msg[1024];
	
	va_list vl;
	va_start(vl, inStr);
	vsnprintf(msg, sizeof(msg), inStr, vl);
	va_end(vl);
	
	cerr << msg << endl;
}

void __signal_throw(
	const char*		inCode,
	const char*		inFunction,
	const char*		inFile,
	int				inLine)
{
	cerr << "Throwing in file " << inFile << " line " << inLine
		<< " \"" << inFunction << "\": " << endl << inCode << endl;
	
	if (StOKToThrow::IsOK())
		return;

	GtkWidget* dlg = gtk_message_dialog_new(nil, GTK_DIALOG_MODAL,
		GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
		"Exception thrown in file '%s', line %d, function: '%s'\n\n"
		"code: %s", inFile, inLine, inFunction, inCode);
	
	PlaySound("error");
	(void)gtk_dialog_run(GTK_DIALOG(dlg));
	
	gtk_widget_destroy(dlg);
}

#endif
