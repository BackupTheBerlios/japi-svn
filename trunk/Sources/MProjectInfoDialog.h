//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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

	void				AddTarget();
	
	void				DeleteTarget();

	void				UpdateTargetPopup(
							uint32			inActive);

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

	void				PkgToggled(
							const std::string&	inPkg,
							bool				inSelected);
	MEventIn<void(const std::string&,bool)>		ePkgToggled;

	MProject*			mProject;
	MProjectInfo		mProjectInfo;
};

#endif // MFINDANDOPENDIALOG_H
