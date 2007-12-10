#include "MJapieG.h"

#include <cerrno>
#include <sys/stat.h>
#include <sys/wait.h>

#include "MPkgConfig.h"
#include "MPreferences.h"

using namespace std;

extern char** environ;

void GetPkgConfigResult(
	const string&		inPackage,
	const char*			inInfo,
	vector<string>&		outFlags)
{
	// Try to locate the pkg-config executable first
	
	string path = Preferences::GetString("pkg-config", "pkg-config");
	bool found = true;
	
	// If the path contains slashes we don't bother
	const char* PATH = getenv("PATH");
	
	if (path.find('/') == string::npos and PATH != nil)
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
			
			if (fs::exists(p / path))
			{
				path = fs::system_complete(p / path).string();
				found = true;
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
			inInfo,
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
	char* ss = const_cast<char*>(s.c_str());
	
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
