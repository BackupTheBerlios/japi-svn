//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*	$Id: MPreferences.cpp 85 2006-09-14 08:23:20Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday August 01 2004 13:33:22
*/

#include "MLib.h"

#include <sstream>
#include <cerrno>
#include <cstring>
#include <map>

#include <boost/bind.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/serialization/serialization.hpp>
#include <cstring>

#include "MTypes.h"
#include "MPreferences.h"
#include "MGlobals.h"

#include <zeep/xml/document.hpp>
#include <zeep/xml/node.hpp>
#include <zeep/xml/serialize.hpp>
#include <zeep/xml/writer.hpp>

using namespace std;
namespace xml = zeep::xml;
namespace fs = boost::filesystem;

namespace Preferences
{
	
struct preference
{
	string				name;
	vector<string>		value;
	
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version)
	{
		ar & BOOST_SERIALIZATION_NVP(name)
		   & BOOST_SERIALIZATION_NVP(value);
	}
};

class IniFile
{
  public:

	static IniFile&	Instance();
	
	void			SetString(
						const char*				inName,
						const string&			inValue);

	string			GetString(
						const char*				inName,
						const string&			inDefault);

	void			SetStrings(
						const char*				inName,
						const vector<string>&	inValues);

	void			GetStrings(
						const char*				inName,
						vector<string>&			outStrings);
	
  private:
					IniFile();
					~IniFile();
	
	fs::path		mPrefsFile;
	map<string,vector<string> >
					mPrefs;
};

IniFile::IniFile()
{
	try
	{
		SOAP_XML_SET_STRUCT_NAME(preference);
		
		if (not fs::exists(gPrefsDir))
			fs::create_directories(gPrefsDir);
		
		mPrefsFile = gPrefsDir / "settings.xml";
		
		if (fs::exists(mPrefsFile))
		{
			fs::ifstream data(mPrefsFile);
			
			if (data.is_open())
			{
				vector<preference> pref;
				
				xml::document doc(data);
				xml::deserializer d(doc.child());
				d & BOOST_SERIALIZATION_NVP(pref);

				for (vector<preference>::iterator p = pref.begin(); p != pref.end(); ++p)
					mPrefs[p->name] = p->value;
			}
		}
	}
	catch (exception& e)
	{
		cerr << "Exception reading preferences: " << e.what() << endl;		
	}
}

IniFile::~IniFile()
{
	try
	{
		if (not fs::exists(gPrefsDir))
			fs::create_directories(gPrefsDir);
		
		mPrefsFile = gPrefsDir / "settings.xml";
		
		fs::ofstream data(mPrefsFile);
		
		if (data.is_open())
		{
			xml::writer w(data);
			
			w.xml_decl(false);
			
			w.start_element("japi-preferences");
			
			for (map<string,vector<string> >::iterator p = mPrefs.begin(); p != mPrefs.end(); ++p)
			{
				w.start_element("pref");

				w.element("name", p->first);
				
				for (vector<string>::iterator v = p->second.begin(); v != p->second.end(); ++v)
					w.element("value", *v);
				
				w.end_element();
			}
			
			w.end_element();
		}
	}
	catch (exception& e)
	{
		PRINT(("Exception writing prefs file: %s", e.what()));
	}
	catch (...) {}
}

IniFile& IniFile::Instance()
{
	static IniFile sInstance;
	return sInstance;
}

void IniFile::SetString(
	const char*		inName,
	const string&	inValue)
{
	vector<string> values;
	values.push_back(inValue);
	mPrefs[inName] = values;
}

string IniFile::GetString(
	const char*		inName,
	const string&	inDefault)
{
	if (mPrefs.find(inName) == mPrefs.end() or mPrefs[inName].size() == 0)
		SetString(inName, inDefault);
	vector<string>& values = mPrefs[inName];
	assert(values.size() == 1);
	if (values.size() != 1)
		cerr << "Inconsistent use of preference array/value" << endl;
	return mPrefs[inName].front();
}

void IniFile::SetStrings(
	const char*				inName,
	const vector<string>&	inValues)
{
	mPrefs[inName] = inValues;
}

void IniFile::GetStrings(
	const char*				inName,
	vector<string>&			outStrings)
{
	outStrings = mPrefs[inName];
}

int32
GetInteger(
	const char*	inName,
	int32		inDefaultValue)
{
	return boost::lexical_cast<int32>(GetString(
		inName, boost::lexical_cast<string>(inDefaultValue)));
}

void
SetInteger(
	const char*	inName,
	int32		inValue)
{
	SetString(inName, boost::lexical_cast<string>(inValue));
}

string GetString(
	const char*	inName,
	string		inDefaultValue)
{
	return IniFile::Instance().GetString(inName, inDefaultValue);
}

void SetString(
	const char*	inName,
	string		inValue)
{
	IniFile::Instance().SetString(inName, inValue);
}

void
GetArray(
	const char*		inName,
	vector<string>&	outArray)
{
	IniFile::Instance().GetStrings(inName, outArray);
}

void
SetArray(
	const char*				inName,
	const vector<string>&	inArray)
{
	IniFile::Instance().SetStrings(inName, inArray);
}

MColor GetColor(const char* inName, MColor inDefaultValue)
{
	inDefaultValue.hex(GetString(inName, inDefaultValue.hex()));
	return inDefaultValue;
}

void SetColor(const char* inName, MColor inValue)
{
	SetString(inName, inValue.hex());
}

MRect GetRect(const char* inName, MRect inDefault)
{
	stringstream s;
	s << inDefault.x << ' ' << inDefault.y << ' ' << inDefault.width << ' ' << inDefault.height;
	s.str(GetString(inName, s.str()));
	
	s >> inDefault.x >> inDefault.y >> inDefault.width >> inDefault.height;
	return inDefault;
}

void SetRect(const char* inName, MRect inValue)
{
	ostringstream s;
	s << inValue.x << ' ' << inValue.y << ' ' << inValue.width << ' ' << inValue.height;
	SetString(inName, s.str());
}

}
