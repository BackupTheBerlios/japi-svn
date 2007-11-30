#ifndef MDIALOG2_H
#define MDIALOG2_H

#include <glade/glade-xml.h>

#include "MWindow.h"

class MDialog2 : public MWindow
{
  public:
					~MDialog2();

	template<class DLG>
	static DLG*		Create(
						const char*		inResource);

	bool				IsChecked(
							uint32				inID) const;

	void				SetChecked(
							uint32				inID,
							bool				inOn);

  protected:

	virtual void	Init();

  private:

					MDialog2(
						GladeXML*		inGlade,
						GtkWidget*		inRoot);

	static void		CreateGladeAndWidgets(
						const char*		inResource,
						GladeXML*&		outGlade,
						GtkWidget*&		outWidget);

	const char*		IDToName(
						uint32			inID,
						char			inName[5]) const;

	GladeXML*		mGlade;
};

template<class DLG>
DLG* MDialog2::Create(
	const char* inResource)
{
	GladeXML* glade;
	GtkWidget* widget;
	CreateGladeAndWidgets(inResource, glade, widget);
	
	std::auto_ptr<DLG> dialog(new MDialog2(glade, widget));
	dialog->Init();
	return dialog.release();
}

#endif
