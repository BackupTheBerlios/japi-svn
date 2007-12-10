#ifndef MDIALOG2_H
#define MDIALOG2_H

#include <glade/glade-xml.h>
#include <vector>

#include "MWindow.h"

class MDialog2 : public MWindow
{
  public:
					~MDialog2();

	void			Show(
						MWindow*			inParent);

	template<class DLG>
	static DLG*		Create(
						const char*			inResource);

	void			SavePosition(
						const char*			inName);

	void			RestorePosition(
						const char*			inName);

	MWindow*		GetParentWindow() const				{ return mParentWindow; }
	
	void			SetCloseImmediatelyFlag(
						bool				inCloseImmediately);

	void			SetFocus(
						uint32				inID);
	
	void			GetText(
						uint32				inID,
						std::string&		outText) const;

	void			SetText(
						uint32				inID,
						const std::string&	inText);

	int32			GetValue(
						uint32				inID) const;

	void			SetValue(
						uint32				inID,
						int32				inValue);

	// for comboboxes
	void			GetValues(
						uint32				inID,
						std::vector<std::string>& 
											outValues) const;

	void			SetValues(
						uint32				inID,
						const std::vector<std::string>&
											inValues);

	bool			IsChecked(
						uint32				inID) const;

	void			SetChecked(
						uint32				inID,
						bool				inOn);

	bool			IsVisible(
						uint32				inID) const;

	void			SetVisible(
						uint32				inID,
						bool				inVisible);

	bool			IsEnabled(
						uint32				inID) const;
	
	void			SetEnabled(
						uint32				inID,
						bool				inEnabled);

	bool			IsExpanded(
						uint32				inID) const;
	
	void			SetExpanded(
						uint32				inID,
						bool				inExpanded);

	virtual void	ValueChanged(
						uint32				inID);

  protected:

					MDialog2(
						GladeXML*			inGlade,
						GtkWidget*			inRoot);

	virtual void	Init();

	GtkWidget*		GetWidget(
						uint32				inID) const;

  private:

	static void		CreateGladeAndWidgets(
						const char*			inResource,
						GladeXML*&			outGlade,
						GtkWidget*&			outWidget);

	const char*		IDToName(
						uint32				inID,
						char				inName[5]) const;

	static void		ChangedCallBack(
						GtkWidget*			inWidget,
						gpointer			inUserData);

	static void		DoForEachCallBack(
						GtkWidget*			inWidget,
						gpointer			inUserData);

	void			DoForEach(
						GtkWidget*			inWidget);

	bool			ChildFocus(
						GdkEventFocus*		inEvent);
	
	MSlot<bool(GdkEventFocus*)>				mChildFocus;

	GladeXML*		mGlade;
	MWindow*		mParentWindow;
	MDialog2*		mNext;						// for the close all
	static MDialog2*	sFirst;
	bool			mCloseImmediatelyOnOK;
};

template<class DLG>
DLG* MDialog2::Create(
	const char* inResource)
{
	GladeXML* glade;
	GtkWidget* widget;
	CreateGladeAndWidgets(inResource, glade, widget);
	
	std::auto_ptr<DLG> dialog(new DLG(glade, widget));
	dialog->Init();
	return dialog.release();
}

#endif
