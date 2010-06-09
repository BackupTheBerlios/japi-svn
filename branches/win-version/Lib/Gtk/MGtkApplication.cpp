
//// ----------------------------------------------------------------------------
////	Main routines, forking a client/server e.g.
//
//void my_signal_handler(int inSignal)
//{
//	switch (inSignal)
//	{
//		case SIGPIPE:
//			break;
//		
//		case SIGUSR1:
//			break;
//		
//		case SIGINT:
//			gQuit = true;
//			break;
//		
//		case SIGTERM:
//			gQuit = true;
//			break;
//	}
//}
//
//void error(const char* msg, ...)
//{
//	fprintf(stderr, "%s stopped with an error:\n", g_get_application_name());
//	va_list vl;
//	va_start(vl, msg);
//	vfprintf(stderr, msg, vl);
//	va_end(vl);
//	if (errno != 0)
//		fprintf(stderr, "\n%s\n", strerror(errno));
//	exit(1);
//}
//
//struct MSockMsg
//{
//	uint32		msg;
//	int32		length;
//};
//
//void MJapiApp::ProcessSocketMessages()
//{
//	int fd = accept(mSocketFD, nil, nil);
//	
//	if (fd >= 0)
//	{
//		MDocClosedNotifier notify(fd);		// takes care of closing fd
//		
//		bool readStdin = false;
//		
//		for (;;)
//		{
//			MSockMsg msg = {};
//			int r = read(fd, &msg, sizeof(msg));
//			
//			if (r == 0 or msg.msg == 'done' or msg.length > PATH_MAX)		// done
//				break;
//			
//			char buffer[PATH_MAX + 1];
//			if (msg.length > 0)
//			{
//				r = read(fd, buffer, msg.length);
//				if (r != msg.length)
//					break;
//				buffer[r] = 0;
//			}
//			
//			try
//			{
//				MDocument* doc = nil;
//				int32 lineNr = -1;
//
//				switch (msg.msg)
//				{
//					case 'open':
//						memcpy(&lineNr, buffer, sizeof(lineNr));
//						doc = gApp->OpenOneDocument(MFile(buffer + sizeof(lineNr)));
//						break;
//					
//					case 'new ':
//						doc = MDocument::Create<MTextDocument>(MFile());
//						break;
//					
//					case 'data':
//						readStdin = true;
//						doc = MDocument::Create<MTextDocument>(MFile());
//						break;
//				}
//				
//				if (doc != nil)
//				{
//					mInitialized = true;
//					
//					DisplayDocument(doc);
//					doc->AddNotifier(notify, readStdin);
//					
//					if (lineNr > 0 and dynamic_cast<MTextDocument*>(doc) != nil)
//						static_cast<MTextDocument*>(doc)->GoToLine(lineNr - 1);
//				}
//			}
//			catch (exception& e)
//			{
//				DisplayError(e);
//				readStdin = false;
//			}
//		}
//	}
//}
//
//namespace {
//
//int OpenSocket(
//	struct sockaddr_un&		addr)
//{
//	int sockfd = -1;
//	
//	if (fs::exists(fs::path(addr.sun_path)))
//	{
//		sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
//		if (sockfd < 0)
//			cerr << "creating socket failed: " << strerror(errno) << endl;
//		else
//		{
//			int err = connect(sockfd, (const sockaddr*)&addr, sizeof(addr));
//			if (err < 0)
//			{
//				close(sockfd);
//				sockfd = -1;
//
//				unlink(addr.sun_path);	// allowed to fail
//			}
//		}
//	}
//	
//	return sockfd;
//}
//	
//}

//bool MJapiApp::IsServer()
//{
//	bool isServer = false;
//
//	struct sockaddr_un addr = {};
//	addr.sun_family = AF_LOCAL;
//	snprintf(addr.sun_path, sizeof(addr.sun_path), kSocketName, getuid());
//	
//	mSocketFD = OpenSocket(addr);
//
//	if (mSocketFD == -1)
//	{
//		isServer = true;
//
//		// So we're supposed to become a server
//		// We will fork here and open a new socket
//		// We use a pipe to let the client wait for the server
//
//		int fd[2];
//		(void)pipe(fd);
//		
//		int pid = fork();
//		
//		if (pid == -1)
//		{
//			close(fd[0]);
//			close(fd[1]);
//			
//			cerr << _("Fork failed: ") << strerror(errno) << endl;
//			
//			return false;
//		}
//		
//		if (pid == 0)	// forked process (child which becomes the server)
//		{
//			// detach from the process group, create new
//			// to avoid being killed by a CNTRL-C in the shell
//			setpgid(0, 0);
//
//			// now setup the socket
//			mSocketFD = socket(AF_LOCAL, SOCK_STREAM, 0);
//			int err = ::bind(mSocketFD, (const sockaddr*)&addr, SUN_LEN(&addr));
//		
//			if (err < 0)
//				cerr << _("bind failed: ") << strerror(errno) << endl;
//			else
//			{
//				err = listen(mSocketFD, 5);
//				if (err < 0)
//					cerr << _("Failed to listen to socket: ") << strerror(errno) << endl;
//				else
//				{
//					int flags = fcntl(mSocketFD, F_GETFL, 0);
//					if (fcntl(mSocketFD, F_SETFL, flags | O_NONBLOCK))
//						cerr << _("Failed to set mSocketFD non blocking: ") << strerror(errno) << endl;
//				}
//			}
//			
//			write(fd[1], " ", 1);
//			close(fd[0]);
//			close(fd[1]);
//		}
//		else
//		{
//			// client (parent process). Wait until the server has finished setting up the socket.
//			isServer = false;
//			
//			char c;
//			(void)read(fd[0], &c, 1);
//
//			close(fd[1]);
//			close(fd[0]);
//			
//			// the socket should now really exist
//			mSocketFD = OpenSocket(addr);
//		}
//	}
//	
//	return isServer;
//}
//
//bool MJapiApp::IsClient()
//{
//	return mSocketFD >= 0;
//}
//
//void MJapiApp::ProcessArgv(
//	bool				inReadStdin,
//	mRecentFilesToOpenList&	inDocs)
//{
//	MSockMsg msg = { };
//
//	if (inReadStdin)
//	{
//		msg.msg = 'data';
//		(void)write(mSocketFD, &msg, sizeof(msg));
//	}
//
//	if (inDocs.size() > 0)
//	{
//		msg.msg = 'open';
//		for (MJapiApp::mRecentFilesToOpenList::const_iterator d = inDocs.begin(); d != inDocs.end(); ++d)
//		{
//			int32 lineNr = d->first;
//			string url = d->second.GetURI();
//			
//			msg.length = url.length() + sizeof(lineNr);
//			(void)write(mSocketFD, &msg, sizeof(msg));
//			(void)write(mSocketFD, &lineNr, sizeof(lineNr));
//			(void)write(mSocketFD, url.c_str(), url.length());
//		}
//	}
//	
//	if (not inReadStdin and inDocs.size() == 0)
//	{
//		msg.msg = 'new ';
//		(void)write(mSocketFD, &msg, sizeof(msg));
//	}
//	
//	msg.msg = 'done';
//	msg.length = 0;
//	(void)write(mSocketFD, &msg, sizeof(msg));
//
//	if (inReadStdin)
//	{
//		int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
//		if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK))
//			cerr << _("Failed to set fd non blocking: ") << strerror(errno) << endl;
//
//		for (;;)
//		{
//			char buffer[10240];
//			int r = read(STDIN_FILENO, buffer, sizeof(buffer));
//			
//			if (r == 0 or (r < 0 and errno != EAGAIN))
//				break;
//			
//			if (r > 0)
//				r = write(mSocketFD, buffer, r);
//		}
//	}
//
//	// now block until all windows are closed or server dies
//	char c;
//	read(mSocketFD, &c, 1);
//}
//
//void usage()
//{
//	cout << "usage: japi [options] [ [+line] files ]" << endl
//		 << "    available options: " << endl
//		 << endl
//		 << "    -i         Install japi at prefix where prefix path" << endl
//		 << "    -p prefix  Prefix path where to install japi, default is /usr/local" << endl
//		 << "               resulting in /usr/local/bin/japi" << endl
//		 << "    -h         This help message" << endl
//		 << "    -f         Don't fork into client/server mode" << endl
//		 << endl
//		 << "  One or more files may be specified, use - for reading from stdin" << endl
//		 << endl;
//	
//	exit(1);
//}
//
//void InstallJapi(
//	std::string		inPrefix)
//{
//	if (getuid() != 0)
//		error("You must be root to be able to install japi");
//	
//	// copy the executable to the appropriate destination
//	if (inPrefix.empty())
//		inPrefix = "/usr/local";
//	
//	fs::path prefix(inPrefix);
//	
//	if (not fs::exists(prefix / "bin"))
//	{
//		cout << "Creating directory " << (prefix / "bin") << endl;
//		fs::create_directories(prefix / "bin");
//	}
//
//	if (not fs::exists(gExecutablePath))
//		error("I don't seem to exist...[%s]?", gExecutablePath.string().c_str());
//
//	fs::path dest = prefix / "bin" / "japi";
//	cout << "copying " << gExecutablePath.string() << " to " << dest.string() << endl;
//	
//	if (fs::exists(dest))
//		fs::remove(dest);
//		
//	fs::copy_file(gExecutablePath, dest);
//	
//	// create desktop file
//		
//	mrsrc::rsrc rsrc("japi.desktop");
//	if (not rsrc)
//		error("japi.desktop file could not be created, missing data");
//
//	string desktop(rsrc.data(), rsrc.size());
//	ba::replace_first(desktop, "__EXE__", dest.string());
//	
//	// locate applications directory
//	// don't use glib here, 
//	
//	fs::path desktopFile, applicationsDir;
//	
//	const char* const* config_dirs = g_get_system_data_dirs();
//	for (const char* const* dir = config_dirs; *dir != nil; ++dir)
//	{
//		applicationsDir = fs::path(*dir) / "applications";
//		if (fs::exists(applicationsDir) and fs::is_directory(applicationsDir))
//			break;
//	}
//
//	if (not fs::exists(applicationsDir))
//	{
//		cout << "Creating directory " << applicationsDir << endl;
//		fs::create_directories(applicationsDir);
//	}
//
//	desktopFile = applicationsDir / "japi.desktop";
//	cout << "writing desktop file " << desktopFile << endl;
//
//	fs::ofstream df(desktopFile, ios::trunc);
//	df << desktop;
//	df.close();
//	
//	// write out all locale files
//	
//	mrsrc::rsrc_list loc_rsrc = mrsrc::rsrc("Locale").children();
//	for (mrsrc::rsrc_list::iterator l = loc_rsrc.begin(); l != loc_rsrc.end(); ++l)
//	{
//		mrsrc::rsrc_list loc_files = l->children();
//		if (loc_files.empty())
//			continue;
//		
//		rsrc = loc_files.front();
//		if (not rsrc or rsrc.name() != "japi.po")
//			continue;
//		
//		fs::path localeDir =
//			applicationsDir.parent_path() / "japi" / "locale" / l->name() / "LC_MESSAGES";
//		
//		if (not fs::exists(localeDir))
//		{
//			cout << "Creating directory " << localeDir << endl;
//			fs::create_directories(localeDir);
//		}
//		
//		stringstream cmd;
//		cmd << "msgfmt -o " << (localeDir / "japi.mo") << " -";
//		cout << "Installing locale file: `" << cmd.str() << '`' << endl;
//		
//		FILE* f = popen(cmd.str().c_str(), "w");
//		if (f != nil)
//		{
//			fwrite(rsrc.data(), rsrc.size(), 1, f);
//			pclose(f);
//		}
//		else
//			cout << "failed: " << strerror(errno) << endl;
//	}
//	
//	exit(0);
//}
//
//int main(int argc, char* argv[])
//{
//	try
//	{
//		fs::path::default_name_check(fs::no_check);
//
//		// First find out who we are. Uses proc filesystem to find out.
//		char exePath[PATH_MAX + 1];
//		
//		int r = readlink("/proc/self/exe", exePath, PATH_MAX);
//		if (r > 0)
//		{
//			exePath[r] = 0;
//			gExecutablePath = fs::system_complete(exePath);
//			gPrefixPath = gExecutablePath.parent_path();
//		}
//		
//		if (not fs::exists(gExecutablePath))
//			gExecutablePath = fs::system_complete(argv[0]);
//	
//		// Collect the options
//		int c;
//		bool fork = true, readStdin = false, install = false;
//		string target, prefix;
//
//		while ((c = getopt(argc, const_cast<char**>(argv), "h?fip:m:vt")) != -1)
//		{
//			switch (c)
//			{
//				case 'f':
//					fork = false;
//					break;
//				
//				case 'i':
//					install = true;
//					break;
//				
//				case 'p':
//					prefix = optarg;
//					break;
//				
//				case 'm':
//					target = optarg;
//					break;
//#if DEBUG
//				case 'v':
//					++VERBOSE;
//					break;
//#endif
//				default:
//					usage();
//					break;
//			}
//		}
//		
//		if (install)
//		{
//			gtk_init(&argc, &argv);
//			InstallJapi(prefix);
//		}
//		
//		// if the option was to build a target, try it and exit.
//		if (not target.empty())
//		{
//			if (optind >= argc)
//				THROW(("You should specify a project file to use for building"));
//			
//			MFile file(fs::system_complete(argv[optind]));
//			
//			unique_ptr<MProject> project(MDocument::Create<MProject>(file));
//			project->SelectTarget(target);
//			if (project->Make(false))
//				cout << "Build successful, " << target << " is up-to-date" << endl;
//			else
//				cout << "Building " << target << " Failed" << endl;
//			exit(0);
//		}
//
//		// setup locale, if we can find it.
//		fs::path localeDir = gPrefixPath / "share" / "japi" / "locale";
//		if (not fs::exists(localeDir))
//			localeDir = fs::path("/usr/local/share/japi/locale");
//		if (not fs::exists(localeDir))
//			localeDir = fs::path("/usr/share/japi/locale");
//		if (fs::exists(localeDir))
//		{
//			setlocale(LC_CTYPE, "");
//			setlocale(LC_MESSAGES, "");
//			bindtextdomain("japi", localeDir.string().c_str());
//			textdomain("japi");
//		}
//		
//		// see if we need to open any files from the commandline
//		int32 lineNr = -1;
//		MJapiApp::mRecentFilesToOpenList filesToOpen;
//		
//		for (int32 i = optind; i < argc; ++i)
//		{
//			string a(argv[i]);
//			
//			if (a == "-")
//				readStdin = true;
//			else if (a.substr(0, 1) == "+")
//				lineNr = atoi(a.c_str() + 1);
//			else
//			{
//				filesToOpen.push_back(make_pair(lineNr, MFile(a)));
//				lineNr = -1;
//			}
//		}
//		
//		MJapiApp app;
//		
//		if (fork == false or app.IsServer())
//		{
//			g_thread_init(nil);
//			gdk_threads_init();
//			gtk_init(&argc, &argv);
//	
//			InitGlobals();
//			
//			// now start up the normal executable		
//			gtk_window_set_default_icon_name ("accessories-text-editor");
//	
//			struct sigaction act, oact;
//			act.sa_handler = my_signal_handler;
//			sigemptyset(&act.sa_mask);
//			act.sa_flags = 0;
//			::sigaction(SIGTERM, &act, &oact);
//			::sigaction(SIGUSR1, &act, &oact);
//			::sigaction(SIGPIPE, &act, &oact);
//			::sigaction(SIGINT, &act, &oact);
//	
//			gdk_notify_startup_complete();
//
//			app.RunEventLoop();
//		
//			// we're done, clean up
//			MFindDialog::Instance().Close();
//			
//			SaveGlobals();
//	
//			if (fork)
//			{
//				char path[1024] = {};
//				snprintf(path, sizeof(path), kSocketName, getuid());
//				unlink(path);
//			}
//		}
//		else if (app.IsClient())
//			app.ProcessArgv(readStdin, filesToOpen);
//	}
//	catch (exception& e)
//	{
//		cerr << e.what() << endl;
//	}
//	catch (...)
//	{
//		cerr << "Exception caught" << endl;
//	}
//	
//	return 0;
//}
//

/*
int main(int argc, char* argv[])
{
	try
	{
		fs::path::default_name_check(fs::no_check);

		// First find out who we are. Uses proc filesystem to find out.
		char exePath[PATH_MAX + 1];
		
		int r = readlink("/proc/self/exe", exePath, PATH_MAX);
		if (r > 0)
		{
			exePath[r] = 0;
			gExecutablePath = fs::system_complete(exePath);
			gPrefixPath = gExecutablePath.parent_path();
		}
		
		if (not fs::exists(gExecutablePath))
			gExecutablePath = fs::system_complete(argv[0]);
	
		// Collect the options
		int c;
		bool fork = true, readStdin = false, install = false;
		string target, prefix;

		while ((c = getopt(argc, const_cast<char**>(argv), "h?fip:m:vt")) != -1)
		{
			switch (c)
			{
				case 'f':
					fork = false;
					break;
				
				case 'i':
					install = true;
					break;
				
				case 'p':
					prefix = optarg;
					break;
				
				case 'm':
					target = optarg;
					break;
#if DEBUG
				case 'v':
					++VERBOSE;
					break;
#endif
				default:
					usage();
					break;
			}
		}
		
		if (install)
		{
			gtk_init(&argc, &argv);
			InstallJapi(prefix);
		}
		
		// if the option was to build a target, try it and exit.
		if (not target.empty())
		{
			if (optind >= argc)
				THROW(("You should specify a project file to use for building"));
			
			MFile file(fs::system_complete(argv[optind]));
			
			unique_ptr<MProject> project(MDocument::Create<MProject>(file));
			project->SelectTarget(target);
			if (project->Make(false))
				cout << "Build successful, " << target << " is up-to-date" << endl;
			else
				cout << "Building " << target << " Failed" << endl;
			exit(0);
		}

		// setup locale, if we can find it.
		fs::path localeDir = gPrefixPath / "share" / "japi" / "locale";
		if (not fs::exists(localeDir))
			localeDir = fs::path("/usr/local/share/japi/locale");
		if (not fs::exists(localeDir))
			localeDir = fs::path("/usr/share/japi/locale");
		if (fs::exists(localeDir))
		{
			setlocale(LC_CTYPE, "");
			setlocale(LC_MESSAGES, "");
			bindtextdomain("japi", localeDir.string().c_str());
			textdomain("japi");
		}
		
		// see if we need to open any files from the commandline
		int32 lineNr = -1;
		MApplication::MFilesToOpenList filesToOpen;
		
		for (int32 i = optind; i < argc; ++i)
		{
			string a(argv[i]);
			
			if (a == "-")
				readStdin = true;
			else if (a.substr(0, 1) == "+")
				lineNr = atoi(a.c_str() + 1);
			else
			{
				filesToOpen.push_back(make_pair(lineNr, MFile(a)));
				lineNr = -1;
			}
		}
		
		MApplication app;
		
		if (fork == false or app.IsServer())
		{
			g_thread_init(nil);
			gdk_threads_init();
			gtk_init(&argc, &argv);
	
			InitGlobals();
			
			// now start up the normal executable		
			gtk_window_set_default_icon_name ("accessories-text-editor");
	
			struct sigaction act, oact;
			act.sa_handler = my_signal_handler;
			sigemptyset(&act.sa_mask);
			act.sa_flags = 0;
			::sigaction(SIGTERM, &act, &oact);
			::sigaction(SIGUSR1, &act, &oact);
			::sigaction(SIGPIPE, &act, &oact);
			::sigaction(SIGINT, &act, &oact);
	
			gdk_notify_startup_complete();

			app.RunEventLoop();
		
			// we're done, clean up
			MFindDialog::Instance().Close();
			
			SaveGlobals();
	
			if (fork)
			{
				char path[1024] = {};
				snprintf(path, sizeof(path), kSocketName, getuid());
				unlink(path);
			}
		}
		else if (app.IsClient())
			app.ProcessArgv(readStdin, filesToOpen);
	}
	catch (exception& e)
	{
		cerr << e.what() << endl;
	}
	catch (...)
	{
		cerr << "Exception caught" << endl;
	}
	
	return 0;
}
*/
