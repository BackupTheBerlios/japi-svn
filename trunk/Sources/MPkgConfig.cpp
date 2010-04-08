//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <cerrno>
#include <sys/stat.h>
#include <sys/wait.h>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <sstream>
#include <fcntl.h>

#include "MFile.h"
#include "MPkgConfig.h"
#include "MPreferences.h"
#include "MError.h"

#define foreach BOOST_FOREACH

using namespace std;
namespace ba = boost::algorithm;

extern char** environ;

namespace {

class MArgv
{
  public:
					MArgv();
					~MArgv();

	void			push_back(const char* s);
	void			push_back(const string& s);
	
					operator char**();

  private:
	vector<char*>	mArgs;
};

MArgv::MArgv()
{
}

MArgv::~MArgv()
{
	foreach (char* p, mArgs)
		delete[] p;
}

void MArgv::push_back(const char* p)
{
	assert(p);
	if (p != nil)
	{
		char* n = new char[strlen(p) + 1];
		strcpy(n, p);
		mArgs.push_back(n);
	}
}

void MArgv::push_back(const string& p)
{
	char* n = new char[p.length() + 1];
	strcpy(n, p.c_str());
	mArgs.push_back(n);
}

MArgv::operator char**()
{
	mArgs.push_back(nil);
	return &mArgs[0];
}

void LocateCommand(
	const string&		inCommand,
	fs::path&			outPath)
{
	// Try to locate the executable
	
//	outPath = fs::path(Preferences::GetString(inCommand.c_str(), inCommand));
	outPath = inCommand;
	bool found = true;
	
	// If the path contains slashes we don't bother
	const char* PATH = getenv("PATH");
	
	if (outPath.string().find('/') == string::npos and PATH != nil)
	{
		found = false;
		
		string b(PATH);
		char* last;
		char* d;
		
		for (d = strtok_r(const_cast<char*>(b.c_str()), ":", &last);
			 d != nil and found == false;
			 d = strtok_r(nil, ":", &last))
		{
			fs::path p(d);
			
			if (fs::exists(p / outPath))
			{
				outPath = fs::system_complete(p / outPath);
				found = true;
			}
		}
	}
	
	if (not found)
		THROW(("command not found: %s", outPath.string().c_str()));
}

static void RunCommand(
	const fs::path&		cmd,
	char*				argv[],
	string&				outResult)
{
	// OK, now start it.

	int ofd[2];
	
	pipe(ofd);
	
	int pid = fork();
	
	if (pid == -1)
	{
		close(ofd[0]);
		close(ofd[1]);
		
		THROW(("fork failed: %s", strerror(errno)));
	}
	
	if (pid == 0)	// the child
	{
		setpgid(0, 0);		// detach from the process group, create new

		dup2(ofd[1], STDOUT_FILENO);
		close(ofd[1]);
		
		// redirect stderr to /dev/null
		int fd = open("/dev/null", O_RDWR);
		dup2(fd, STDERR_FILENO);
		close(fd);

		close(ofd[0]);
		close(STDIN_FILENO);
		
		// redirect errors to /dev/null
		int sink = open("/dev/null", O_RDWR);
		if (sink >= 0)
		{
			dup2(sink, STDERR_FILENO);
			close(sink);
		}
		
		(void)execve(cmd.string().c_str(), argv, environ);

		cerr << "execution of " << argv[0] << " failed: " << strerror(errno) << endl;
		exit(-1);
	}
	
	close(ofd[1]);

	outResult.clear();
	for (;;)
	{
		char b[1024];

		int r = read(ofd[0], b, sizeof(b));
		
		if (r == 0)
			break;
		
		if (r < 0 and errno == EAGAIN)
			continue;
		
		if (r > 0)
			outResult.append(b, b + r);
		else
			THROW(("error calling read: %s", strerror(errno)));
	}
	
	int status;
	waitpid(pid, &status, 0);	// avoid zombies
}

void ParseString(
	const string&		inString,
	vector<string>&		outFlags)
{
	// OK, so s now contains the result from the pkg-config command
	// parse it, and extract the various parts
	
	vector<char*> argv;
	bool esc = false, squot = false, dquot = false;
	char* ss = const_cast<char*>(inString.c_str());
	
	while (isspace(*ss))
		++ss;
	
	argv.push_back(ss);
	
	for (char* c = ss; *c != 0; ++c)
	{
		if (esc)
			esc = false;
		else if (*c == '\\')
			esc = true;
		else if (squot)
		{
			if (*c == '\'')
				squot = false;
		}
		else if (dquot)
		{
			if (*c == '"')
				dquot = false;
		}
		else if (*c == '\'')
			squot = true;
		else if (*c == '"')
			dquot = true;
		else if (isspace(*c))
		{
			*c = 0;
			
			while (isspace(*(c + 1)))
				++c;
			
			if (c > ss and *(c + 1) != 0)
				argv.push_back(c + 1);

			ss = c + 1;
		}
	}
	
	for (vector<char*>::iterator arg = argv.begin(); arg != argv.end(); ++arg)
	{
		string a(*arg);
		ba::trim(a);
		if (not a.empty())
			outFlags.push_back(a);
	}
}

}

void GetPkgConfigResult(
	const string&		inPackage,
	const char*			inInfo,
	vector<string>&		outFlags)
{
	fs::path cmd;
	LocateCommand("pkg-config", cmd);
	
	MArgv args;
	args.push_back(cmd.filename());
	args.push_back(inInfo);
	args.push_back(inPackage);

	string s;
	RunCommand(cmd, args, s);

	ParseString(s, outFlags);

//cout << "pkg-config: " << path << endl
//	 << "pkg: " << inPackage << endl
//	 << "info: " << inInfo << endl
//	 << "flags:" << endl;
//	copy(outFlags.begin(), outFlags.end(), ostream_iterator<string>(cout, "\n"));
}

void GetCompilerPaths(
	const string&	inCompiler,
	string&			outCppIncludeDir,
	vector<fs::path>&	outLibDirs)
{
	fs::path cmd;
	LocateCommand(inCompiler, cmd);
	
	// get the list of libraries search dirs
	{
		MArgv args;
		args.push_back(cmd.filename());
		args.push_back("-v");
	
		string s;
		RunCommand(cmd, args, s);
		
		stringstream ss(s);
		
		for (;;)
		{
			string line;
			getline(ss, line);
			
			if (ss.eof())
				break;
	
			if (ba::starts_with(line, "Configured with: "))
			{
				string::size_type p = line.find("--with-gxx-include-dir");
				if (p != string::npos)
				{
					p += sizeof("--with-gxx-include-dir"); // - \0 + '='
					outCppIncludeDir = line.substr(p);
					p = outCppIncludeDir.find(' ');
					if (p != string::npos)
						outCppIncludeDir.erase(p, string::npos);
				}

				break;
			}
		}
	}

	// get the list of libraries search dirs
	{
		MArgv args;
		args.push_back(cmd.filename());
		args.push_back("-print-search-dirs");
	
		string s;
		RunCommand(cmd, args, s);
		
		stringstream ss(s);
		
		for (;;)
		{
			string line;
			getline(ss, line);
			
			if (ss.eof())
				break;
	
	//		if (ba::starts_with(line, "install: "))
	//			outInstallDir = line.substr(9);
	//		else
			if (ba::starts_with(line, "libraries: ="))
			{
				ba::erase_first(line, "libraries: =");
				
				vector<string> l;
				split(l, line, ba::is_any_of(":"));
				copy(l.begin(), l.end(), back_inserter(outLibDirs));
			}
		}
	}
}

void GetToolConfigResult(
	const std::string&			inTool,
	const char*					inArgs[],
	std::vector<std::string>&	outFlags)
{
	fs::path cmd;
	LocateCommand(inTool, cmd);
	
	MArgv argv;
	argv.push_back(inTool);
	for (const char*const* a = inArgs; *a != nil; ++a)
		argv.push_back(*a);

	string s;
	RunCommand(cmd, argv, s);
	ParseString(s, outFlags);
//
//cout << "tool: " << inTool << endl
//	 << "flags:" << endl;
//	copy(outFlags.begin(), outFlags.end(), ostream_iterator<string>(cout, "\n"));
}

void GetPkgConfigPackagesList(
	vector<pair<string,string> >&	outPackages)
{
	fs::path cmd;
	LocateCommand("pkg-config", cmd);
	
	MArgv argv;
	argv.push_back("pkg-config");
	argv.push_back("--list-all");

	string s;
	RunCommand(cmd, argv, s);
	
	stringstream ss(s);
	while (not ss.eof())
	{
		string line;
		getline(ss, line);
		
		string::size_type p = line.find(' ');
		if (p == string::npos)
			continue;
		
		pair<string,string> pkg;
		pkg.first = line.substr(0, p);

		while (p != line.length() and line[p] == ' ')
			++p;
		
		pkg.second = line.substr(p);

		outPackages.push_back(pkg);
	}
	
	sort(outPackages.begin(), outPackages.end());
}
