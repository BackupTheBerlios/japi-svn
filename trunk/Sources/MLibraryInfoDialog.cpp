//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <sstream>
#include <iterator>
#include <vector>

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>

#include "MFile.h"
#include "MProject.h"
#include "MPreferences.h"
#include "MView.h"
#include "MUnicode.h"
#include "MGlobals.h"
#include "MLibraryInfoDialog.h"
#include "MDevice.h"
#include "MGtkWrappers.h"
#include "MError.h"
#include "MPkgConfig.h"
#include "MStrings.h"

using namespace std;
namespace ba = boost::algorithm;

namespace {

enum {
	kNameEditTextID				= 'name',
	kIgnoreCheckboxID			= 'igno',
	kLinkStaticRadioButtonID	= 'file',
	kLinkSharedRadioButtonID	= 'link',
};

}

MLibraryInfoDialog::MLibraryInfoDialog()
	: MDialog("library-info-dialog")
	, mProject(nil)
{
}

MLibraryInfoDialog::~MLibraryInfoDialog()
{
}

void MLibraryInfoDialog::Initialize(
	MProject*		inProject)
{
	mProject = inProject;
	
}

bool MLibraryInfoDialog::OKClicked()
{
	return true;
}

void MLibraryInfoDialog::ValueChanged(
	uint32			inID)
{
	string s;

cout << "value changed: " << hex << inID << endl;
	
	switch (inID)
	{
//		case kTargetNameControlID:
//			GetText(inID, target.mName);
//			UpdateTargetPopup(targetPopup.GetActive());
//			break;

	}	
}

