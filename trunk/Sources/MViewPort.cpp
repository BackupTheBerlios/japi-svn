/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 28-09-07 21:25:10
*/

#include "MJapieG.h"

#include "MViewPort.h"

using namespace std;

MViewPort::MViewPort(
	GtkObject*		inHAdjustment,
	GtkObject*		inVAdjustment)
	: MView(
		gtk_viewport_new(GTK_ADJUSTMENT(inHAdjustment), GTK_ADJUSTMENT(inVAdjustment)),
		false)
{
}

void MViewPort::SetShadowType(
	GtkShadowType	inShadowType)
{
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(GetGtkWidget()), inShadowType);
}

void MViewPort::Add(
	MView*			inView)
{
	gtk_container_add(GTK_CONTAINER(GetGtkWidget()), inView->GetGtkWidget());
}
