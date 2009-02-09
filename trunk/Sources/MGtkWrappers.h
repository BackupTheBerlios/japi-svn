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

class MGtkTreeView : public MGtkWidget
{
  public:
				MGtkTreeView(
					GtkWidget*		inWidget)
					: MGtkWidget(inWidget)
				{
					assert(GTK_IS_TREE_VIEW(mGtkWidget));
				}	

				operator GtkTreeView*()		{ return GTK_TREE_VIEW(mGtkWidget); }

	void		SetModel(
					GtkTreeModel*	inModel)
				{
					gtk_tree_view_set_model(GTK_TREE_VIEW(mGtkWidget), inModel);
				}

	void		ExpandAll()
				{
					gtk_tree_view_expand_all(GTK_TREE_VIEW(mGtkWidget));
				}

	bool		GetFirstSelectedRow(
					GtkTreePath*&	outRow)
				{
					bool result = false;
					GList* rows = gtk_tree_selection_get_selected_rows(
						gtk_tree_view_get_selection(GTK_TREE_VIEW(mGtkWidget)),
						nil);
					
					if (rows != nil)
					{
						outRow = gtk_tree_path_copy((GtkTreePath*)rows->data);
						result = outRow != nil;
						
						g_list_foreach(rows, (GFunc)(gtk_tree_path_free), nil);
						g_list_free(rows);
					}
					
					return result;
				}

	void		GetSelectedRows(
					std::vector<GtkTreePath*>&	outRows)
				{
					GList* rows = gtk_tree_selection_get_selected_rows(
						gtk_tree_view_get_selection(GTK_TREE_VIEW(mGtkWidget)),
						nil);
					
					if (rows != nil)
					{
						GList* row = rows;
						while (row != nil)
						{
							outRows.push_back(gtk_tree_path_copy((GtkTreePath*)row->data));
							row = row->next;
						}
						
						g_list_foreach(rows, (GFunc)(gtk_tree_path_free), nil);
						g_list_free(rows);
					}
				}
	
//	int32		GetScrollPosition() const
//				{
//					GdkRectangle r;
//					gtk_tree_view_get_visible_rect(GTK_TREE_VIEW(mGtkWidget), &r);
//					return r.y;
//				}
//	
//	void		SetScrollPosition(
//					int32			inPosition)
//				{
//					gtk_tree_view_scroll_to_point(GTK_TREE_VIEW(mGtkWidget), -1, inPosition);
//				}
//	
//	void		ScrollToCell(
//					GtkTreePath*		inPath,
//					GtkTreeViewColumn*	inColumn)
//				{
//				}

	void		SetCursor(
					GtkTreePath*		inPath,
					GtkTreeViewColumn*	inColumn,
					bool				inEdit = true)
				{
					gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(mGtkWidget),
						nil, inColumn, true, 0, 0);
					gtk_tree_view_set_cursor(GTK_TREE_VIEW(mGtkWidget),
						inPath, inColumn, inEdit);
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
