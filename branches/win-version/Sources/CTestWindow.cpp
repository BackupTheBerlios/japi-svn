#include "MJapi.h"
#include "CTestWindow.h"
//#include "MListView.h"
#include "MControls.h"

//#include <boost/fusion/support/pair.hpp>
//#include <boost/fusion/include/pair.hpp>
//#include <boost/fusion/sequence.hpp>
//#include <boost/fusion/container/map.hpp>
//#include <boost/fusion/algorithm/iteration/for_each.hpp>
//#include <boost/fusion/include/for_each.hpp>
//
//using namespace std;
//namespace f = boost::fusion;
//
//struct list_row_base {};
//
//template<typename C>
//struct list_row : public list_row_base
//{
//	typedef typename C		columns_type;
//
//	static const uint32		size = f::result_of::size<columns_type>::value;
//};
//
//namespace fields
//{
//	struct name;
//	struct text;
//	struct data;
//};
//
//class MListRow : public list_row<
//	f::map<
//		f::pair<fields::name, std::string>,
//		f::pair<fields::text, uint32>,
//		f::pair<fields::data, uint32>
//	>
//>
//{
//  public:
//
//	template<typename k>
//	struct value {
//		typedef typename f::result_of::at_key<columns_type, k>::type	type;
//	};
//
//  	template<typename k>
//	void					GetValue(typename value<k>::type& outValue);
//
//};
//
//template<>
//void MListRow::GetValue<fields::name>(string& outValue)
//{
//	outValue = "xxx";
//}
//
//template<>
//void MListRow::GetValue<fields::text>(uint32& outValue)
//{
//	outValue = 1;
//}
//
//template<>
//void MListRow::GetValue<fields::data>(uint32& outValue)
//{
//	outValue = 2;
//}
//
//void draw(const string& text)
//{
//	cout << text << endl;
//}
//
//void draw(uint32 value)
//{
//	cout << value << endl;
//}


CTestWindow::CTestWindow()
	: MWindow("Test", MRect(0, 0, 400, 400), kMPostionDefault, "")
{
//	MListRow row;
//	
//	uint32 size = row.size;



//	AddChild(new MListView("lijst", MRect(10, 10, 380, 380)));

	MNotebook* noteBook = new MNotebook("notebook", MRect(10, 10, 380, 380));
	AddChild(noteBook);
	noteBook->SetBindings(true, true, true, true);

	MView* page = new MView("page-1", MRect(4, 34, 372, 342));
	page->SetBindings(true, true, true, true);
	noteBook->AddPage("Files", page);

	MListHeader* kop = new MListHeader("kop-lijst", MRect(0, 0, 370, 24));
	kop->SetBindings(true, true, true, false);
	page->AddChild(kop);
	
	kop->AppendColumn("Files", 100);
	kop->AppendColumn("Text", 60);
	kop->AppendColumn("Data", 60);

	Show();
	Select();
}

