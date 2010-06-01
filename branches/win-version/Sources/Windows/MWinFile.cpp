//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <windows.h>

#define STRICT_TYPED_ITEMIDS
#include <shlobj.h>
#include <objbase.h>      // For COM headers
#include <shobjidl.h>     // for IFileDialogEvents and IFileDialogControlEvents
#include <shlwapi.h>
//#include <knownfolders.h> // for KnownFolder APIs/datatypes/function headers
//#include <propvarutil.h>  // for PROPVAR-related functions
//#include <propkey.h>      // for the Property key APIs/datatypes
//#include <propidl.h>      // for the Property System APIs
//#include <strsafe.h>      // for StringCchPrintfW
//#include <shtypes.h>      // for COMDLG_FILTERSPEC

#include "MLib.h"
#include "MFile.h"
#include "MWinUtils.h"

using namespace std;

class CDialogEventHandler : public IFileDialogEvents,
                            public IFileDialogControlEvents
{
public:
    // IUnknown methods
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        static const QITAB qit[] = {
            QITABENT(CDialogEventHandler, IFileDialogEvents),
            QITABENT(CDialogEventHandler, IFileDialogControlEvents),
            { 0 },
        };
        return QISearch(this, qit, riid, ppv);
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cRef);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        long cRef = InterlockedDecrement(&_cRef);
        if (!cRef)
            delete this;
        return cRef;
    }

    // IFileDialogEvents methods
    IFACEMETHODIMP OnFileOk(IFileDialog *) { return S_OK; };
    IFACEMETHODIMP OnFolderChange(IFileDialog *) { return S_OK; };
    IFACEMETHODIMP OnFolderChanging(IFileDialog *, IShellItem *) { return S_OK; };
    IFACEMETHODIMP OnHelp(IFileDialog *) { return S_OK; };
    IFACEMETHODIMP OnSelectionChange(IFileDialog *) { return S_OK; };
    IFACEMETHODIMP OnShareViolation(IFileDialog *, IShellItem *, FDE_SHAREVIOLATION_RESPONSE *) { return S_OK; };
	IFACEMETHODIMP OnTypeChange(IFileDialog *pfd) { return S_OK; }
    IFACEMETHODIMP OnOverwrite(IFileDialog *, IShellItem *, FDE_OVERWRITE_RESPONSE *) { return S_OK; };

    // IFileDialogControlEvents methods
	IFACEMETHODIMP OnItemSelected(IFileDialogCustomize *pfdc, DWORD dwIDCtl, DWORD dwIDItem) { return S_OK; }
    IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize *, DWORD) { return S_OK; };
    IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize *, DWORD, BOOL) { return S_OK; };
    IFACEMETHODIMP OnControlActivating(IFileDialogCustomize *, DWORD) { return S_OK; };

    CDialogEventHandler() : _cRef(1) { };
private:
    ~CDialogEventHandler() { };
    long _cRef;
};

// Instance creation helper
HRESULT CDialogEventHandler_CreateInstance(REFIID riid, void **ppv)
{
    *ppv = NULL;
    CDialogEventHandler *pDialogEventHandler = new (std::nothrow) CDialogEventHandler();
    HRESULT hr = pDialogEventHandler ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        hr = pDialogEventHandler->QueryInterface(riid, ppv);
        pDialogEventHandler->Release();
    }
    return hr;
}


bool ChooseDirectory(
	fs::path&	outDirectory)
{
	return false;
}

//bool ChooseDirectory(
//	fs::path&			outDirectory)
//{
//	bool result = true; 
//	
//	MFile dir(outDirectory);
//
//	if (ChooseDirectory(dir))
//	{
//		outDirectory = dir.GetPath();
//		result = true;
//	}
//	
//	return result; 
//}

// This code snippet demonstrates how to work with the common file dialog interface
bool BasicFileOpen(MFile& outFile)
{
	//bool result = false;

	////IFileDialog *pfd = nil;
 ////   IFileDialogEvents *pfde = nil;
	////IShellItem *psiResult;

	//// CoCreate the File Open Dialog object.
	//MComPtr<IFileDialog> pfd;
	//THROW_IF_HRESULT_ERROR(::CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)));

	//// Create an event handling object, and hook it up to the dialog.
	//MComPtr<IFileDialogEvents> pfde;
	//THROW_IF_HRESULT_ERROR(CDialogEventHandler_CreateInstance(IID_PPV_ARGS(&pfde)));

	//// Hook up the event handler.
 //   DWORD dwCookie;
	//THROW_IF_HRESULT_ERROR(pfd->Advise(pfde, &dwCookie));

	//// Set the options on the dialog.
 //   DWORD dwFlags;

 //   // Before setting, always get the options first in order not to override existing options.
	//THROW_IF_HRESULT_ERROR(pfd->GetOptions(&dwFlags));

	//// In this case, get shell items only for file system items.
	//THROW_IF_HRESULT_ERROR(pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM));

	//const COMDLG_FILTERSPEC types[] =
	//{
	//	{L"All Documents (*.*)",         L"*.*"}
	//};

	//// Set the file types to display only. Notice that, this is a 1-based array.
 //   THROW_IF_HRESULT_ERROR(pfd->SetFileTypes(1, types));

	//// Set the selected file type index to Word Docs for this example.
 //   THROW_IF_HRESULT_ERROR(pfd->SetFileTypeIndex(0));

	//// Set the default extension to be ".doc" file.
 //   THROW_IF_HRESULT_ERROR(pfd->SetDefaultExtension(L"txt"));

	//// Show the dialog
 //   THROW_IF_HRESULT_ERROR(pfd->Show(NULL));

	//// Obtain the result, once the user clicks the 'Open' button.
 //   // The result is an IShellItem object.
	//MComPtr<IShellItem> psiResult;
	//THROW_IF_HRESULT_ERROR(pfd->GetResult(&psiResult));

	//// We are just going to print out the name of the file for sample sake.
 //   PWSTR pszFilePath = NULL;
 //   THROW_IF_HRESULT_ERROR(psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath));

	//string path = w2c(pszFilePath);
	//outFile = MFile(path);
	//result = true;

	//CoTaskMemFree(pszFilePath);

	//// Unhook the event handler.
 //   pfd->Unadvise(dwCookie);

	//return result;
	return false;
}

bool ChooseOneFile(
	MFile&	ioFile)
{
	BasicFileOpen(ioFile);

	return false;
}

bool ChooseFiles(
	bool				inLocalOnly,
	std::vector<MFile>&	outFiles)
{
	bool result = false;

	// CoCreate the File Open Dialog object.
	MComPtr<IFileOpenDialog> pfd;
	THROW_IF_HRESULT_ERROR(::CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)));

	// Create an event handling object, and hook it up to the dialog.
	MComPtr<IFileDialogEvents> pfde;
	THROW_IF_HRESULT_ERROR(CDialogEventHandler_CreateInstance(IID_PPV_ARGS(&pfde)));

	// Hook up the event handler.
    DWORD dwCookie;
	THROW_IF_HRESULT_ERROR(pfd->Advise(pfde, &dwCookie));

	// Set the options on the dialog.
    DWORD dwFlags;

    // Before setting, always get the options first in order not to override existing options.
	THROW_IF_HRESULT_ERROR(pfd->GetOptions(&dwFlags));

	// In this case, get shell items only for file system items.
	THROW_IF_HRESULT_ERROR(pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT));

	const COMDLG_FILTERSPEC types[] =
	{
		{L"All Documents (*.*)",         L"*.*"}
	};

	// Set the file types to display only. Notice that, this is a 1-based array.
    THROW_IF_HRESULT_ERROR(pfd->SetFileTypes(1, types));

	// Set the selected file type index to Word Docs for this example.
    THROW_IF_HRESULT_ERROR(pfd->SetFileTypeIndex(0));

	// Set the default extension to be ".doc" file.
    THROW_IF_HRESULT_ERROR(pfd->SetDefaultExtension(L"txt"));

	// Show the dialog
    THROW_IF_HRESULT_ERROR(pfd->Show(NULL));

	// Obtain the result, once the user clicks the 'Open' button.
    // The result is an IShellItem object.
	MComPtr<IShellItemArray> psiResult;
	THROW_IF_HRESULT_ERROR(pfd->GetResults(&psiResult));

	DWORD count;
	THROW_IF_HRESULT_ERROR(psiResult->GetCount(&count));

	for (int i = 0; i < count; ++i)
	{
		MComPtr<IShellItem> item;
		THROW_IF_HRESULT_ERROR(psiResult->GetItemAt(i, &item));

		PWSTR pszFilePath = NULL;
		THROW_IF_HRESULT_ERROR(item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath));

		string path = w2c(pszFilePath);
		outFiles.push_back(MFile(path));
		result = true;

		::CoTaskMemFree(pszFilePath);
	}

	// Unhook the event handler.
    pfd->Unadvise(dwCookie);

	return result;
}
