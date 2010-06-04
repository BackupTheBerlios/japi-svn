//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include "zeep/xml/document.hpp"

#include <sstream>
#include <list>
#include <limits>
#include <map>

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH
#include <boost/lexical_cast.hpp>

#include "MObjectFile.h"
#include "MResources.h"
#include "MPatriciaTree.h"
#include "MError.h"

using namespace std;
using namespace zeep;
namespace io = boost::iostreams;

namespace mrsrc
{

struct rsrc_imp
{
	HRSRC		rsrc;
	HGLOBAL		hmem;
	const char*	data;
	uint32		size;
	string		name;
};

class index
{
public:
	rsrc_imp*		load(const string& name);

	static index&	instance();

private:
					index();

	

	map<string,int>	m_index;
};

index& index::instance()
{
	static index sInstance;
	return sInstance;
}

index::index()
{
	auto_ptr<rsrc_imp> impl(new rsrc_imp);

	impl->rsrc = ::FindResourceW(nil, MAKEINTRESOURCEW(1), L"MRSRCIX");
	if (impl->rsrc == nil)
		throw rsrc_not_found_exception();
	
	impl->hmem = ::LoadResource(nil, impl->rsrc);
	THROW_IF_NIL(impl->hmem);

	impl->size = ::SizeofResource(nil, impl->rsrc);

	impl->data = reinterpret_cast<char*>(::LockResource(impl->hmem));
	THROW_IF_NIL(impl->data);

	io::stream<io::array_source> data(impl->data, impl->size);
	xml::document doc(data);

	foreach (xml::element* rsrc, doc.find("//rsrc"))
		m_index[rsrc->get_attribute("name")] = boost::lexical_cast<int>(rsrc->get_attribute("nr"));
}

rsrc_imp* index::load(const string& name)
{
	rsrc_imp* result = nil;

	try
	{
		int nr = m_index[name];

		auto_ptr<rsrc_imp> impl(new rsrc_imp);

		impl->rsrc = ::FindResourceW(nil, MAKEINTRESOURCEW(nr), L"MRSRC");
		if (impl->rsrc == nil)
			throw rsrc_not_found_exception();
	
		impl->hmem = ::LoadResource(nil, impl->rsrc);
		THROW_IF_NIL(impl->hmem);

		impl->size = ::SizeofResource(nil, impl->rsrc);

		impl->data = reinterpret_cast<char*>(::LockResource(impl->hmem));
		THROW_IF_NIL(impl->data);

		result = impl.release();
	}
	catch (...)
	{
		throw rsrc_not_found_exception();
	}

	return result;
}

rsrc::rsrc()
	: m_impl(nil)
{
}

rsrc::rsrc(const rsrc& o)
	: m_impl(o.m_impl)
{
}

rsrc& rsrc::operator=(const rsrc& other)
{
	m_impl = other.m_impl;
	return *this;
}

rsrc::rsrc(const string& name)
	: m_impl(index::instance().load(name))
{
}

string rsrc::name() const
{
	return m_impl->name;
}

const char* rsrc::data() const
{
	return m_impl->data;
}
	
unsigned long rsrc::size() const
{
	return m_impl->size;
}

rsrc::operator bool () const
{
	return m_impl != nil and m_impl->size > 0;
}

rsrc_list rsrc::children() const
{
	return list<rsrc>();
}

}