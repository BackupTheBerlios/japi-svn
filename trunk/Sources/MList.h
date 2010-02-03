//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef MLIST_H
#define MLIST_H

#include <string>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>

#include "MView.h"
#include "MP2PEvents.h"

template<class R>
class MList;
class MListBase;

template<typename T>
struct g_type_mapped
{
	static const GType g_type = G_TYPE_STRING;
	static std::pair<GtkCellRenderer*,const char*>	GetRenderer()
		{ return std::make_pair(gtk_cell_renderer_text_new(), "text"); }
	static void to_g_value(GValue& gv, T v)
	{
		g_value_set_string(&gv, boost::lexical_cast<std::string>(v).c_str());
	}
};

template<> struct g_type_mapped<std::string>
{
	static const GType g_type = G_TYPE_STRING;
	static std::pair<GtkCellRenderer*,const char*>	GetRenderer()
		{ return std::make_pair(gtk_cell_renderer_text_new(), "text"); }
	static void to_g_value(GValue& gv, const std::string& s)
	{
		g_value_set_string(&gv, s.c_str());
	}
};
template<> struct g_type_mapped<GdkPixbuf*>
{
	static const GType g_type = G_TYPE_OBJECT;
	static std::pair<GtkCellRenderer*,const char*>	GetRenderer()
		{ return std::make_pair(gtk_cell_renderer_pixbuf_new(), "pixbuf"); }
	static void to_g_value(GValue& gv, GdkPixbuf* pixbuf)
	{
		g_value_set_object(&gv, pixbuf);
	}
};

//---------------------------------------------------------------------
// MListRow

class MListRowBase : public boost::noncopyable
{
	friend class MListBase;

  public:
						MListRowBase();
	virtual				~MListRowBase();
	
	virtual bool		RowDropPossible() const		{ return false; }

  protected:

	bool				GetModelAndIter(
							GtkTreeStore*&	outTreeStore,
							GtkTreeIter&	outTreeIter);

	virtual void		UpdateDataInTreeStore() = 0;

  private:

	GtkTreePath*		GetTreePath() const;

	GtkTreeRowReference*
						mRowReference;
};

template<class I, typename... Args>
class MListRow : public MListRowBase
{
  private:
	friend class MList<MListRow>;

	template<class base_type, class N, class T, int P>
	struct column_type
	{
		typedef typename base_type::template type<P-1>	base_column_type;
		typedef typename base_column_type::name_type	name_type;
		typedef typename base_column_type::value_type	value_type;
	};
	
	template<class base_type, class N, class T>
	struct column_type<base_type,N,T,0>
	{
		typedef N										name_type;
		typedef T										value_type;
	};
	
	template<class... Args1>
	struct column_type_traits
	{
		static GType	GetGType(
							int			inColumnNr)					{ throw "error"; }

		static std::pair<GtkCellRenderer*,const char*>
						GetRenderer(
							int			inColumnNr)					{ throw "error"; }

		template<class G>
		static void		GetGValue(
							G			inProvider,
							int			inColumnNr,
							GValue&		outValue)					{ throw "error"; }
	};
	
	template<class N, class T, class... Args1>
	struct column_type_traits<N, T, Args1...>
	{
		static const int count = 1 + sizeof...(Args1) / 2;
		typedef column_type_traits<Args1...>	base_type;
		
		template<int P>
		struct type : public column_type<base_type,N,T,P>	{};
		
		static GType	GetGType(
							int			inColumnNr)
						{
							GType result;
							if (inColumnNr == 0)
								result = g_type_mapped<T>::g_type;
							else
								result = base_type::GetGType(inColumnNr - 1);
							return result;
						}

		static std::pair<GtkCellRenderer*,const char*>
						GetRenderer(
							int			inColumnNr)
						{
							std::pair<GtkCellRenderer*,const char*> result;
							if (inColumnNr == 0)
								result = g_type_mapped<T>::GetRenderer();
							else
								result = base_type::GetRenderer(inColumnNr - 1);
							return result;
						}

		template<class G>
		static void		GetGValue(
							G			inProvider,
							int			inColumnNr,
							GValue&		outValue)
						{
							if (inColumnNr == 0)
							{
								T v;
								inProvider->GetData(N(), v);
								g_value_init(&outValue, g_type_mapped<T>::g_type);
								g_type_mapped<T>::to_g_value(outValue, v);
							}
							else
								base_type::GetGValue(inProvider, inColumnNr - 1, outValue);
						}
	};

  protected:

	virtual void		UpdateDataInTreeStore()
						{
							GtkTreeStore* treeStore;
							GtkTreeIter treeIter;
							
							if (GetModelAndIter(treeStore, treeIter))
							{
								std::vector<GValue> v(column_count + 1);
								std::vector<gint> c(column_count + 1);
							
								for (int j = 0; j < column_count; ++j)
								{
									c[j] = j;
									traits::GetGValue(static_cast<impl_type*>(this), j, v[j]);
								}
								c[column_count] = column_count;
								
								g_value_init(&v[column_count], G_TYPE_POINTER);
								g_value_set_pointer(&v[column_count], this);
								
								gtk_tree_store_set_valuesv(treeStore, &treeIter, &c[0], &v[0], column_count + 1);
							}
						}
					
  public:

	typedef I							impl_type;
	typedef column_type_traits<Args...>	traits;
	static const int					column_count = traits::count;
};

//---------------------------------------------------------------------
// MList

class MListBase : public MView
{
  public:
					MListBase(
						GtkWidget*	inTreeView);

	virtual			~MListBase();

	void			SetColumnTitle(
						int					inColumnNr,
						const std::string&	inTitle);

	void			SelectRow(
						MListRowBase*		inRow);

	void			CollapseRow(
						MListRowBase*		inRow);
	
	void			ExpandRow(
						MListRowBase*		inRow,
						bool				inExpandAll);

	void			CollapseAll();
	
	void			ExpandAll();

	virtual int		GetColumnCount() const = 0;

  protected:

	void			CreateTreeStore(
						std::vector<GType>&	inTypes,
						std::vector<std::pair<GtkCellRenderer*,const char*>>&
											inRenderers);

	void			AppendRowInt(
						MListRowBase*		inRow,
						MListRowBase*		inParent);

	GtkTreePath*	GetTreePathForRow(
						MListRowBase*		inRow);

	bool			GetTreeIterForRow(
						MListRowBase*		inRow,
						GtkTreeIter*		outIter);

	MListRowBase*	GetRowForPath(
						GtkTreePath*		inPath);

	MListRowBase*	GetCursorRow();

	virtual void	CursorChanged();
	MSlot<void()>	mCursorChanged;
	virtual void	RowSelected(
						MListRowBase*		inRow) = 0;

	virtual void	RowActivated(
						GtkTreePath*		inTreePath,
						GtkTreeViewColumn*	inColumn);
	MSlot<void(GtkTreePath*,GtkTreeViewColumn*)>
					mRowActivated;
	virtual void	RowActivated(
						MListRowBase*		inRow) = 0;

	virtual void	RowChanged(
						GtkTreePath*		inTreePath,
						GtkTreeIter*		inTreeIter);
	MSlot<void(GtkTreePath*,GtkTreeIter*)>
					mRowChanged;

	virtual void	RowDeleted(
						GtkTreePath*		inTreePath);
	MSlot<void(GtkTreePath*)>
					mRowDeleted;

	virtual void	RowInserted(
						GtkTreePath*		inTreePath,
						GtkTreeIter*		inTreeIter);
	MSlot<void(GtkTreePath*,GtkTreeIter*)>
					mRowInserted;
	
	virtual void	RowsReordered(
						GtkTreePath*		inTreePath,
						GtkTreeIter*		inTreeIter,
						gint*				inNewOrder);
	MSlot<void(GtkTreePath*,GtkTreeIter*,gint*)>
					mRowsReordered;
	
	// tree store overrides
	
	typedef gboolean (*RowDropPossibleFunc)(GtkTreeDragDest*, GtkTreePath*, GtkSelectionData*);
	static gboolean RowDropPossibleCallback(GtkTreeDragDest*, GtkTreePath*, GtkSelectionData*);

	RowDropPossibleFunc
					mSavedRowDropPossible;

	virtual bool	RowDropPossible(
						GtkTreePath*		inTreePath,
						GtkSelectionData*	inSelectionData);

	GtkTreeStore*	mTreeStore;
};

template<class R>
class MList : public MListBase
{
  private:
	typedef R								row_type;
	typedef typename row_type::traits		column_type_traits;
	
  public:

					MList(
						GtkWidget*			inWidget);

	void			AppendRow(
						row_type*			inRow,
						row_type*			inParentRow = nil)
														{ AppendRowInt(inRow, inParentRow); }

	virtual int		GetColumnCount() const				{ return column_type_traits::count; }

	MEventOut<void(row_type*)>				eRowSelected;
	MEventOut<void(row_type*)>				eRowInvoked;
	
  protected:
	
	virtual void	RowSelected(
						MListRowBase*		inRow)		{ eRowSelected(static_cast<row_type*>(inRow)); }

	virtual void	RowActivated(
						MListRowBase*		inRow)		{ eRowInvoked(static_cast<row_type*>(inRow)); }
};

template<typename R>
MList<R>::MList(
	GtkWidget*		inTreeView)
	: MListBase(inTreeView)
{
    std::vector<GType> types(GetColumnCount() + 1);
    std::vector<std::pair<GtkCellRenderer*,const char*>> renderers(GetColumnCount());
    
    for (int i = 0; i < GetColumnCount(); ++i)
    {
    	types[i] = column_type_traits::GetGType(i);
    	renderers[i] = column_type_traits::GetRenderer(i);
    }
    types[GetColumnCount()] = G_TYPE_POINTER;
    
    CreateTreeStore(types, renderers);
}

#endif
