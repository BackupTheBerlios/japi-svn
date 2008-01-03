/*
cd $HOME/projects/jg
/usr/bin/g++-4.1 -o RSRCFileCreator -DPNG_NO_MMX_CODE -I/usr/include/gtk-2.0 -I/usr/lib64/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -I/usr/include/freetype2 -I/usr/include/libpng12  -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lpangocairo-1.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0   Sources/MRSRCFileCreator.cpp -lboost_filesystem Obj.Debug/MResources.o Obj.Debug/MError.o Obj.Debug/MObjectFile.o Obj.Debug/MObjectFileImp_elf.o -lxml2

pkg-config --cflags --libs 'gtk+-2.0'
-DPNG_NO_MMX_CODE -I/usr/include/gtk-2.0 -I/usr/lib64/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -I/usr/include/freetype2 -I/usr/include/libpng12  -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lpangocairo-1.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -ldl -lglib-2.0  

*/

#include "MJapieG.h"

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>

#include <getopt.h>
#include <string>
#include <vector>

#include "MResources.h"

using namespace std;

void* gResourceData;
void* gResourceIndex;

bool verbose;

void usage()
{
	cerr << "RSRCFileCreator can take the following options:" << endl
		 << "    -o file            Specifies output file" << endl
		 << "    -r dir             Specifies directory containing data files" << endl
		 << "    -a arch            Specifies target architecture, should be one of" << endl
		 << "                         i386, amd64, ppc, ppc64 or native" << endl
		 << "    -d datafile        Specifies a data file relative to the -r dir" << endl;
	
	exit(1);
}

void PlaySound(
	const string&		inSoundName)
{
}

void CreateRSRSFile(
	MTargetCPU			inTargetCPU,
	fs::path&			inRSRCFile,
	fs::path&			inRSRCDir,
	vector<fs::path>&	inDataFiles)
{
	MResourceFile rsrcFile(inTargetCPU);
	
	for (vector<fs::path>::iterator p = inDataFiles.begin(); p != inDataFiles.end(); ++p)
	{
		fs::ifstream f(inRSRCDir / *p);

		string name = p->string();

		if (verbose)
			cout << "adding " << name << endl;
		
		if (not f.is_open())
			THROW(("Could not open resource data file %s", p->string().c_str()));
		
		filebuf* b = f.rdbuf();
	
		uint32 size = b->pubseekoff(0, ios::end);
		b->pubseekpos(0);
	
		char* data = new char[size + 1];
		
		try
		{
			b->sgetn(data, size);
			data[size] = 0;
		
			f.close();
			
			rsrcFile.Add(name, data, size);
		}
		catch (...)
		{
			delete[] data;
			throw;
		}
		
		delete[] data;
	}
	
	rsrcFile.Write(inRSRCFile);
}

int main(int argc, const char* argv[])
{
	int c;
	vector<fs::path> dataFiles;
	fs::path rsrcFile;
	fs::path rsrcDir;
	MTargetCPU arch;
	
#if defined(__amd64)
	arch = eCPU_x86_64;
#elif defined(__i386__)
	arch = eCPU_386;
#elif defined(__powerpc64__) or defined(__PPC64__) or defined(__ppc64__)
	arch = eCPU_PowerPC_64;
#elif defined(__powerpc__) or defined(__PPC__) or defined(__ppc__)
	arch = eCPU_PowerPC_32;
#else
#	error("Undefined processor")
#endif				

	while ((c = getopt(argc, const_cast<char**>(argv), "o:d:vr:a:")) != -1)
	{
		switch (c)
		{
			case 'o':
				rsrcFile = fs::system_complete(optarg);
				break;

			case 'r':
				rsrcDir = fs::system_complete(optarg);
				break;
			
			case 'd':
				dataFiles.push_back(optarg);
				break;
			
			case 'v':
				verbose = true;
				break;
			
			case 'a':
				if (strcmp(optarg, "ppc") == 0)
					arch = eCPU_PowerPC_32;
				else if (strcmp(optarg, "ppc64") == 0)
					arch = eCPU_PowerPC_64;
				else if (strcmp(optarg, "i386") == 0)
					arch = eCPU_386;
				else if (strcmp(optarg, "amd64") == 0)
					arch = eCPU_x86_64;
				else if (strcmp(optarg, "native") != 0)
				{
					cerr << "Unsupported architecture" << endl;
					exit(1);
				}
				break;
			
			default:
				usage();
		}
	}
	
	try
	{
		if (dataFiles.size() > 0)
			CreateRSRSFile(arch, rsrcFile, rsrcDir, dataFiles);
		else
			usage();
	}
	catch (...)
	{
		cerr << "Exception" << endl;
		exit(1);
	}
	
	return 0;
}
