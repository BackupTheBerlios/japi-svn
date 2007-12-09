#include "MJapieG.h"

#include <cerrno>
#include <sys/stat.h>

#include "MPkgConfig.h"
#include "MPreferences.h"

using namespace std;

extern char** environ;

namespace
{
	
string NextPath(string& ioPathVar, string inName)
{
	string result = inName;
	
	if (ioPathVar.length())
	{
		if (ioPathVar[0] == ':')
			ioPathVar.erase(0, 1);
	
		string::size_type n = ioPathVar.find(':');
		if (n != string::npos)
		{
			result = ioPathVar.substr(0, n) + '/' + inName;
			ioPathVar.erase(0, n + 1);
		}
		else if (ioPathVar.length() > 0)
		{
			result = ioPathVar + '/' + inName;
			ioPathVar.clear();
		}
	}
	
	return result;
}
	
}


void PkgConfigGetCFlags(
	const string&		inPackage,
	vector<string>&		outCFlags,
	vector<string>&		outIncludeDirs)
{
	// Try to locate the pkg-config executable first
	
	string path = Preferences::GetString("pkg-config", "pkg-config");
	bool found = true;
	
	// If the path contains slashes we don't bother
	if (path.find('/') == string::npos)
	{
		found = false;
		
		struct stat statb;
		
		string PATH = getenv("PATH");
		
		while ((path = NextPath(PATH, path)) != path)
		{
			if (stat(path.c_str(), &statb) >= 0 and S_ISREG(statb.st_mode))
			{
				found = true;
				break;
			}
		}
	}
	
	if (not found)
		THROW(("pkg-config command not found: %s", path.c_str()));

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
		
		const char* argv[] = {
			"pkg-config",
			"--cflags",
			"libglade-2.0",
			NULL
		};

		(void)execve(path.c_str(),
			const_cast<char*const*>(argv), environ);

		cerr << "execution of pkg-config failed: " << strerror(errno) << endl;
		exit(-1);
	}
	
	close(fd[1]);

	string s;
	
	for (;;)
	{
		char b[1024];

		int r = read(fd[0], b, sizeof(b));
		
		if (r == 0)
			break;
		
		if (r < 0 and errno == EAGAIN)
			continue;
		
		if (r > 0)
			s.append(b, b + r);
		else
			THROW(("error calling read: %s", strerror(errno)));
	}
	
	int status;
	waitpid(pid, &status, WNOHANG);	// avoid zombies
	
	// OK, so s now contains the result from the pkg-config command
	// parse it, and extract the various parts
	
	vector<char*> argv;
	bool esc = false, squot = false, dquot = false;
	
	for (char* c = const_cast<char*>(s.c_str()); *c != 0; ++c)
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
		else if (*c == ' ')
		{
			*c = 0;
			argv.push_back(c + 1);
		}
	}
	
	optind = 0;
	optreset = 1;
	int c;
	while ((c = getopt(argv.size(), &argv[0], "I:D:")) != -1)
	{
		switch (c)
		{
			case 'I':
				outIncludeDirs.push_back(optarg);
				break;
			
			default:
				break;
		}
	}
	
	cout << "Include dirs according to pkg-dir: " << endl;
	copy(outIncludeDirs.begin(), outIncludeDirs.end(),
		ostream_iterator<string>(cout, "\n"));
	
}
