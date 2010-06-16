//          Copyright Maarten L. Hekkelman 2006-2010
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MWinLib.h"

#include "MFile.h"
#include "MWinUtils.h"
#include "MWinWindowImpl.h"

using namespace std;

int32 read_attribute(const fs::path& inPath, const char* inName, void* outData, size_t inDataSize)
{
	int32 result = -1;

	wstring path = c2w(inPath.string());
	path += L":japi";

	unsigned long access = GENERIC_READ;
	unsigned long shareMode = 0;
	unsigned long create = OPEN_EXISTING;
	unsigned long flags = FILE_ATTRIBUTE_NORMAL;
	
	HANDLE fd = ::CreateFileW(path.c_str(),
		access, shareMode, nil, create, flags, nil);

	if (fd >= 0)
	{
		DWORD read = 0;
		if (::ReadFile(fd, outData, inDataSize, &read, nil))
			result = read;
		::CloseHandle(fd);
	}

	return result;
}

int32 write_attribute(const fs::path& inPath, const char* inName, const void* inData, size_t inDataSize)
{
	int32 result = -1;

	wstring path = c2w(inPath.string());
	path += L":japi";

	unsigned long access = GENERIC_READ | GENERIC_WRITE;
	unsigned long shareMode = 0;
	unsigned long create = OPEN_ALWAYS;
	unsigned long flags = FILE_ATTRIBUTE_NORMAL;
	
	HANDLE fd = ::CreateFileW(path.c_str(),
		access, shareMode, nil, create, flags, nil);

	if (fd >= 0)
	{
		FILETIME t = {};
		
		::GetFileTime(fd, nil, nil, &t);
			
		DWORD written = 0;
		if (::WriteFile(fd, inData, inDataSize, &written, nil))
			result = written;
		
		if (t.dwHighDateTime or t.dwLowDateTime)
			::SetFileTime(fd, nil, nil, &t);
		
		::CloseHandle(fd);
	}

	return result;
}

namespace MFileDialogs
{

class CDialogEventHandler : public IFileDialogEvents,
                            public IFileDialogControlEvents
{
public:
    // IUnknown methods
    IFACEMETHODIMP	QueryInterface(REFIID riid, void** ppv);

    IFACEMETHODIMP_(ULONG)
					AddRef();

    IFACEMETHODIMP_(ULONG)
					Release();

    // IFileDialogEvents methods
    IFACEMETHODIMP	OnFileOk(IFileDialog *)							{ return S_OK; };
    IFACEMETHODIMP	OnFolderChange(IFileDialog *)					{ return S_OK; };
    IFACEMETHODIMP	OnFolderChanging(IFileDialog *, IShellItem *)	{ return S_OK; };
    IFACEMETHODIMP	OnHelp(IFileDialog *)							{ return S_OK; };
    IFACEMETHODIMP	OnSelectionChange(IFileDialog *)				{ return S_OK; };
    IFACEMETHODIMP	OnShareViolation(IFileDialog *, IShellItem *, FDE_SHAREVIOLATION_RESPONSE *)
																	{ return S_OK; };
	IFACEMETHODIMP	OnTypeChange(IFileDialog *pfd)					{ return S_OK; }
    IFACEMETHODIMP	OnOverwrite(IFileDialog *, IShellItem *, FDE_OVERWRITE_RESPONSE *)
																	{ return S_OK; };

    // IFileDialogControlEvents methods
	IFACEMETHODIMP	OnItemSelected(IFileDialogCustomize *pfdc, DWORD dwIDCtl, DWORD dwIDItem)
																	{ return S_OK; }
    IFACEMETHODIMP	OnButtonClicked(IFileDialogCustomize *, DWORD)	{ return S_OK; };
    IFACEMETHODIMP	OnCheckButtonToggled(IFileDialogCustomize *, DWORD, BOOL)
																	{ return S_OK; };
    IFACEMETHODIMP	OnControlActivating(IFileDialogCustomize *, DWORD)
																	{ return S_OK; };

    CDialogEventHandler() : mRefCount(1) { };

	static HRESULT	Create(REFIID riid, void **ppv);

private:
					~CDialogEventHandler() { };
    long			mRefCount;
};

IFACEMETHODIMP CDialogEventHandler::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDialogEventHandler, IFileDialogEvents),
        QITABENT(CDialogEventHandler, IFileDialogControlEvents),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) CDialogEventHandler::AddRef()
{
    return InterlockedIncrement(&mRefCount);
}

IFACEMETHODIMP_(ULONG) CDialogEventHandler::Release()
{
    long cRef = InterlockedDecrement(&mRefCount);
    if (!cRef)
        delete this;
    return cRef;
}

// Instance creation helper
HRESULT CDialogEventHandler::Create(REFIID riid, void **ppv)
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

// --------------------------------------------------------------------

bool Choose(
	HWND				inParent,
	//const fs::path&		inDirectory,
	bool				inLocalOnly,
	bool				inSelectMultiple,
	bool				inSelectDirectory,
	std::vector<MFile>&	outSelected)
{
	bool result = false;

	// CoCreate the File Open Dialog object.
	MComPtr<IFileOpenDialog> pfd;
	THROW_IF_HRESULT_ERROR(::CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)));

	// Create an event handling object, and hook it up to the dialog.
	MComPtr<IFileDialogEvents> pfde;
	THROW_IF_HRESULT_ERROR(CDialogEventHandler::Create(IID_PPV_ARGS(&pfde)));

	// Hook up the event handler.
    DWORD dwCookie;
	THROW_IF_HRESULT_ERROR(pfd->Advise(pfde, &dwCookie));

	// Set the options on the dialog.
    DWORD dwFlags;

    // Before setting, always get the options first in order not to override existing options.
	THROW_IF_HRESULT_ERROR(pfd->GetOptions(&dwFlags));

	dwFlags |= FOS_FORCEFILESYSTEM;
	if (inSelectDirectory)
		dwFlags |= FOS_PICKFOLDERS;
	else if (inSelectMultiple)
		dwFlags |= FOS_ALLOWMULTISELECT;

	THROW_IF_HRESULT_ERROR(pfd->SetOptions(dwFlags));

	const COMDLG_FILTERSPEC types[] =
	{
		{L"All Documents (*.*)",         L"*.*"}
	};

	// Set the file types to display only. Notice that, this is a 1-based array.
    THROW_IF_HRESULT_ERROR(pfd->SetFileTypes(1, types));

	// Set the selected file type index to Word Docs for this example.
    THROW_IF_HRESULT_ERROR(pfd->SetFileTypeIndex(0));

	//if (fs::exists(inDirectory))
	//{
	//	MComPtr<IShellItem> dir();
	//	pfd->SetFolder(inDirectory);
	//}

	//// Set the default extension to be ".doc" file.
 //   THROW_IF_HRESULT_ERROR(pfd->SetDefaultExtension(L"txt"));

	// Show the dialog
    if (pfd->Show(inParent) == S_OK)
	{
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

			try
			{
				string path = w2c(pszFilePath);
				outSelected.push_back(MFile(path));
				result = true;
			}
			catch (exception& e)
			{
				DisplayError(e);
			}

			::CoTaskMemFree(pszFilePath);
		}
	}

	// Unhook the event handler.
    pfd->Unadvise(dwCookie);

	return result;
}

// --------------------------------------------------------------------

bool ChooseDirectory(
	MWindow*			inParent,
	fs::path&			outDirectory)
{
	HWND hwnd = nil;
	if (inParent != nil)
		hwnd = static_cast<MWinWindowImpl*>(inParent->GetImpl())->GetHandle();

	vector<MFile> selected;
	bool result = Choose(hwnd, true, false, true, selected);
	if (result)
		outDirectory = selected.front().GetPath();
	return result;
}

bool ChooseOneFile(
	MWindow*			inParent,
	MFile&				outFile)
{
	HWND hwnd = nil;
	if (inParent != nil)
		hwnd = static_cast<MWinWindowImpl*>(inParent->GetImpl())->GetHandle();

	vector<MFile> selected;
	bool result = Choose(hwnd, true, false, false, selected);
	if (result)
		outFile = MFile(selected.front());
	return result;
}

bool ChooseFiles(
	MWindow*			inParent,
	bool				inLocalOnly,
	std::vector<MFile>&	outFiles)
{
	HWND hwnd = nil;
	if (inParent != nil)
		hwnd = static_cast<MWinWindowImpl*>(inParent->GetImpl())->GetHandle();

	return Choose(hwnd, true, false, false, outFiles);
}

bool SaveFileAs(
	MWindow*			inParent,
	fs::path&			ioFile)
{
	bool result = false;

	HWND hwnd = nil;
	if (inParent != nil)
		hwnd = static_cast<MWinWindowImpl*>(inParent->GetImpl())->GetHandle();

	// similar to the Choose code
	MComPtr<IFileSaveDialog> pfd;
	THROW_IF_HRESULT_ERROR(::CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd)));

	// Create an event handling object, and hook it up to the dialog.
	MComPtr<IFileDialogEvents> pfde;
	THROW_IF_HRESULT_ERROR(CDialogEventHandler::Create(IID_PPV_ARGS(&pfde)));

	// Hook up the event handler.
    DWORD dwCookie;
	THROW_IF_HRESULT_ERROR(pfd->Advise(pfde, &dwCookie));

	// Set the options on the dialog.
    DWORD dwFlags;

    // Before setting, always get the options first in order not to override existing options.
	THROW_IF_HRESULT_ERROR(pfd->GetOptions(&dwFlags));

	dwFlags |= FOS_FORCEFILESYSTEM;

	THROW_IF_HRESULT_ERROR(pfd->SetOptions(dwFlags));

	const COMDLG_FILTERSPEC types[] =
	{
		{L"All Documents (*.*)",         L"*.*"}
	};

	// Set the file types to display only. Notice that, this is a 1-based array.
    THROW_IF_HRESULT_ERROR(pfd->SetFileTypes(1, types));

	// Set the selected file type index to Word Docs for this example.
    THROW_IF_HRESULT_ERROR(pfd->SetFileTypeIndex(0));

	fs::path folder(ioFile.parent_path());
	if (fs::exists(folder))
	{
		wstring dir(c2w(folder.native_directory_string()));
		MComPtr<IShellItem> psiFolder;
		HRESULT hr = ::SHCreateItemFromParsingName(dir.c_str(), nil, IID_PPV_ARGS(&psiFolder));
	    if (SUCCEEDED(hr))
			pfd->SetFolder(psiFolder);
	}

	if (not ioFile.filename().empty())
	{
		wstring name(c2w(ioFile.filename()));
		pfd->SetFileName(name.c_str());
	}

	//// Set the default extension to be ".doc" file.
 //   THROW_IF_HRESULT_ERROR(pfd->SetDefaultExtension(L"txt"));

	// Show the dialog
    if (pfd->Show(hwnd) == S_OK)
	{
		result = true;

		MComPtr<IShellItem> psiResult;
		THROW_IF_HRESULT_ERROR(pfd->GetResult(&psiResult));

		PWSTR pszFilePath = NULL;

		try
		{
			THROW_IF_HRESULT_ERROR(psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath));
			string path = w2c(pszFilePath);
			ioFile = path;
			result = true;
		}
		catch (exception& e)
		{
			DisplayError(e);
			result = false;
		}

		::CoTaskMemFree(pszFilePath);
	}

	// Unhook the event handler.
    pfd->Unadvise(dwCookie);

	return result;
}

}
