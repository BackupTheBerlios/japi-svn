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

#ifndef MPROJECTINFODIALOG_H
#define MPROJECTINFODIALOG_H

#include "MDialog.h"

class MProject;
class MProjectTarget;

class MProjectInfoDialog : public MDialog
{
  public:
						MProjectInfoDialog();

						~MProjectInfoDialog();

	void				Initialize(
							MProject*		inProject);

	virtual bool		OKClicked();
	
  private:

	virtual void		ValueChanged(
							uint32			inID);

	void				TargetChanged();
	MSlot<void()>		eTargetChanged;

	void				DefinesChanged();
	MSlot<void()>		eDefinesChanged;

	void				WarningsChanged();
	MSlot<void()>		eWarningsChanged;

	void				SysPathsChanged();
	MSlot<void()>		eSysPathsChanged;

	void				UserPathsChanged();
	MSlot<void()>		eUserPathsChanged;

	void				LibPathsChanged();
	MSlot<void()>		eLibPathsChanged;

	MProject*			mProject;
	MProjectInfo		mProjectInfo;
};

#endif // MFINDANDOPENDIALOG_H
