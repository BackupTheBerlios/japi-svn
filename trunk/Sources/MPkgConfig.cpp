/*
	Copyright (c) 2007, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "MJapi.h"

#include <cerrno>
#include <sys/stat.h>
#include <sys/wait.h>
#include <boost/algorithm/string.hpp>
#include <sstream>

#include "MFile.h"
#include "MPkgConfig.h"
#include "MPreferences.h"
#include "MError.h"

using namespace std;

extern char** environ;

namespace {

void LocateCommand(
	const string&		inCommand,
	fs::path&			outPath)
{
	// Try to locate the executable
	
	outPath = fs::path(Preferences::GetString(inCommand.c_str(), inCommand));
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
	const char*			argv[],
	string&				outResult)
{
	// OK, now start it.

	int fd[2];
	
	pipe(fd);
	
	int pid = fork();
	
	if (pid == -1)
	{
		close(fd[0]);
		close(fd[1]);
		
		THROW(("fork failed: %s", strerror(errno)));
	}
	
	if (pid == 0)	// the child
	{
		setpgid(0, 0);		// detach from the process group, create new

		dup2(fd[1], STDOUT_FILENO);
		dup2(fd[1], STDERR_FILENO);
		close(fd[0]);
		close(fd[1]);

		close(STDIN_FILENO);
		
		(void)execve(cmd.string().c_str(),
			const_cast<char*const*>(argv), environ);

		cerr << "execution of " << argv[0] << " failed: " << strerror(errno) << endl;
		exit(-1);
	}
	
	close(fd[1]);

	outResult.clear();
	for (;;)
	{
		char b[1024];

		int r = read(fd[0], b, sizeof(b));
		
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
	
	copy(argv.begin(), argv.end(), back_inserter(outFlags));
}

}

void GetPkgConfigResult(
	const string&		inPackage,
	const char*			inInfo,
	vector<string>&		outFlags)
{
	fs::path cmd;
	LocateCommand("pkg-config", cmd);
	
	const char* args[] = {
		cmd.leaf().c_str(),
		inInfo,
		inPackage.c_str(),
		NULL
	};

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
		const char* args[] = {
			cmd.leaf().c_str(),
			"-v",
			NULL
		};
	
		string s;
		RunCommand(cmd, args, s);
		
		stringstream ss(s);
		
		for (;;)
		{
			string line;
			getline(ss, line);
			
			if (ss.eof())
				break;
	
			if (boost::algorithm::starts_with(line, "Configured with: "))
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
		const char* args[] = {
			cmd.leaf().c_str(),
			"-print-search-dirs",
			NULL
		};
	
		string s;
		RunCommand(cmd, args, s);
		
		stringstream ss(s);
		
		for (;;)
		{
			string line;
			getline(ss, line);
			
			if (ss.eof())
				break;
	
	//		if (boost::algorithm::starts_with(line, "install: "))
	//			outInstallDir = line.substr(9);
	//		else
			if (boost::algorithm::starts_with(line, "libraries: ="))
			{
				boost::algorithm::erase_first(line, "libraries: =");
				
				vector<string> l;
				split(l, line, boost::algorithm::is_any_of(":"));
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
	
	vector<const char*> argv;
	argv.push_back(inTool.c_str());
	for (const char*const* a = inArgs; *a != nil; ++a)
		argv.push_back(*a);
	argv.push_back(nil);

	string s;
	RunCommand(cmd, &argv[0], s);
	ParseString(s, outFlags);
//
//cout << "tool: " << inTool << endl
//	 << "flags:" << endl;
//	copy(outFlags.begin(), outFlags.end(), ostream_iterator<string>(cout, "\n"));
}
