#ifndef MGTKWRAPPERS_H
#define MGTKWRAPPERS_H

class MGtkWidget
{
  public:
				MGtkWidget(
					GtkWidget*		inWidget)
					: mGtkWidget(inWidget)
				{
//					g_object_ref(G_OBJECT(mGtkWidget));
				}

	virtual		~MGtkWidget()
				{
//					g_object_unref(G_OBJECT(mGtkWidget));
				}
				
				operator GtkWidget*()		{ return mGtkWidget; }

  protected:
	GtkWidget*	mGtkWidget;

  private:
				MGtkWidget(
					const MGtkWidget&	rhs);
				
	MGtkWidget&	operator=(
					const MGtkWidget&	rhs);
};

class MGtkNotebook : public MGtkWidget
{
  public:
				MGtkNotebook(
					GtkWidget*		inWidget)
					: MGtkWidget(inWidget)
				{
					assert(GTK_IS_NOTEBOOK(mGtkWidget));
				}
	
				operator GtkNotebook*()		{ return GTK_NOTEBOOK(mGtkWidget); }
	
	int32		GetPage() const
				{
					return gtk_notebook_get_current_page(GTK_NOTEBOOK(mGtkWidget));
				}

	void		SetPage(
					int32			inPage)
				{
					return gtk_notebook_set_current_page(GTK_NOTEBOOK(mGtkWidget), inPage);
				}
};

class MGtkComboBox : public MGtkWidget
{
  public:
				MGtkComboBox(
					GtkWidget*			inWidget)
					: MGtkWidget(inWidget)
				{
					assert(GTK_IS_COMBO_BOX(mGtkWidget));
				}
	
	int32		GetActive() const
				{
					return gtk_combo_box_get_active(GTK_COMBO_BOX(mGtkWidget));
				}
	
	void		SetActive(
					int32				inActive)
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(mGtkWidget), inActive);
				}
	
	void		RemoveAll()
				{
					GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(mGtkWidget));
					int32 count = gtk_tree_model_iter_n_children(model, nil);
				
					while (count-- > 0)
						gtk_combo_box_remove_text(GTK_COMBO_BOX(mGtkWidget), count);
				}

	void		Append(
					const std::string&	inLabel)
				{
					gtk_combo_box_append_text(GTK_COMBO_BOX(mGtkWidget), inLabel.c_str());
				}
};

#endif
