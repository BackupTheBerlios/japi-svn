/*	$Id$
	Copyright Drs M.L. Hekkelman
	Created 28-09-07 11:18:30
*/

#include "MJapieG.h"

#include <iostream>

#include "MView.h"

using namespace std;

MView::MView(
	GtkWidget*		inWidget)
	: mGtkWidget(inWidget)
{
}

MView::~MView()
{
}

