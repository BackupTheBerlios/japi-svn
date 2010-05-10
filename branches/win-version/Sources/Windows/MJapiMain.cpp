#include <iostream>
#include <vector>

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
