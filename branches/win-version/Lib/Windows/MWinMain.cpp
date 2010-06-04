//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include <boost/program_options.hpp>

#include <windows.h>

#include "MApplication.h"

using namespace std;
namespace po = boost::program_options;

int WINAPI WinMain( HINSTANCE /*hInst*/, 	/*Win32 entry-point routine */
					HINSTANCE /*hPreInst*/, 
					LPSTR lpszCmdLine, 
					int /*nCmdShow*/ )
{
	vector<string> args = po::split_winmain(lpszCmdLine);
	return MApplication::Main(args);
}
