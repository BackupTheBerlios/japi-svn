/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 28-09-07 22:05:43
*/

#include "MJapieG.h"

#include "MVScrollbar.h"

using namespace std;

MVScrollbar::MVScrollbar(
	GtkObject*		inAdjustment)
	: MView(gtk_vscrollbar_new(GTK_ADJUSTMENT(inAdjustment)))
{
}
