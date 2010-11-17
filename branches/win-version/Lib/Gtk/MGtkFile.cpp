bool ChooseDirectory(
	fs::path&	outDirectory)
{
	GtkWidget* dialog = nil;
	bool result = false;

	try
	{
		dialog = 
			gtk_file_chooser_dialog_new(_("Select Folder"), nil,
				GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
		
		THROW_IF_NIL(dialog);
	
		string currentFolder = gApp->GetCurrentFolder();
	
		if (currentFolder.length() > 0)
		{
			gtk_file_chooser_set_current_folder_uri(
				GTK_FILE_CHOOSER(dialog), currentFolder.c_str());
		}
		
		if (fs::exists(outDirectory) and outDirectory != fs::path())
		{
			gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(dialog),
				outDirectory.string().c_str());
		}
		
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		{
			char* uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
			if (uri != nil)
			{
				MFile url(uri, true);
				outDirectory = url.GetPath();
	
				g_free(uri);
	
				result = true;
			}
//
//			gApp->SetCurrentFolder(
//				gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog)));
		}
	}
	catch (exception& e)
	{
		if (dialog)
			gtk_widget_destroy(dialog);
		
		throw;
	}
	
	gtk_widget_destroy(dialog);

	return result;
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

bool ChooseOneFile(
	MFile&	ioFile)
{
	GtkWidget* dialog = nil;
	bool result = false;
	
	try
	{
		dialog = 
			gtk_file_chooser_dialog_new(_("Select File"), nil,
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
		
		THROW_IF_NIL(dialog);
	
		gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), false);
		gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), false);
		
		if (ioFile.IsValid())
		{
			gtk_file_chooser_select_uri(GTK_FILE_CHOOSER(dialog),
				ioFile.GetURI().c_str());
		}
		else if (gApp->GetCurrentFolder().length() > 0)
		{
			gtk_file_chooser_set_current_folder_uri(
				GTK_FILE_CHOOSER(dialog), gApp->GetCurrentFolder().c_str());
		}
		
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		{
			char* uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
			if (uri != nil)
			{
				ioFile = MFile(uri, true);
				g_free(uri);
	
				gApp->SetCurrentFolder(
					gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog)));
	
				result = true;
			}
		}
	}
	catch (exception& e)
	{
		if (dialog)
			gtk_widget_destroy(dialog);
		
		throw;
	}
	
	gtk_widget_destroy(dialog);
	
	return result;
}

bool ChooseFiles(
	bool				inLocalOnly,
	std::vector<MFile>&	outFiles)
{
	GtkWidget* dialog = nil;
	
	try
	{
		dialog = 
			gtk_file_chooser_dialog_new(_("Open"), nil,
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
		
		THROW_IF_NIL(dialog);
	
		gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), true);
		gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), inLocalOnly);
		
		if (gApp->GetCurrentFolder().length() > 0)
		{
			gtk_file_chooser_set_current_folder_uri(
				GTK_FILE_CHOOSER(dialog), gApp->GetCurrentFolder().c_str());
		}
		
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		{
			GSList* uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));	
			
			GSList* file = uris;	
			
			while (file != nil)
			{
				MFile url(reinterpret_cast<char*>(file->data), true);

				g_free(file->data);
				file->data = nil;

				outFiles.push_back(url);

				file = file->next;
			}
			
			g_slist_free(uris);
		}
		
		char* cwd = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
		if (cwd != nil)
		{
			gApp->SetCurrentFolder(cwd);
			g_free(cwd);
		}
	}
	catch (exception& e)
	{
		if (dialog)
			gtk_widget_destroy(dialog);
		
		throw;
	}
	
	gtk_widget_destroy(dialog);
	
	return outFiles.size() > 0;
}



//
//
//bool ChooseDirectory(
//	fs::path&	outDirectory)
//{
//	GtkWidget* dialog = nil;
//	bool result = false;
//
//	try
//	{
//		dialog = 
//			gtk_file_chooser_dialog_new(_("Select Folder"), nil,
//				GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
//				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
//				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
//				NULL);
//		
//		THROW_IF_NIL(dialog);
//	
//		string currentFolder = gApp->GetCurrentFolder();
//	
//		if (currentFolder.length() > 0)
//		{
//			gtk_file_chooser_set_current_folder_uri(
//				GTK_FILE_CHOOSER(dialog), currentFolder.c_str());
//		}
//		
//		if (fs::exists(outDirectory) and outDirectory != fs::path())
//		{
//			gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(dialog),
//				outDirectory.string().c_str());
//		}
//		
//		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
//		{
//			char* uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
//			if (uri != nil)
//			{
//				MFile url(uri, true);
//				outDirectory = url.GetPath();
//	
//				g_free(uri);
//	
//				result = true;
//			}
////
////			gApp->SetCurrentFolder(
////				gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog)));
//		}
//	}
//	catch (exception& e)
//	{
//		if (dialog)
//			gtk_widget_destroy(dialog);
//		
//		throw;
//	}
//	
//	gtk_widget_destroy(dialog);
//
//	return result;
//}
//
////bool ChooseDirectory(
////	fs::path&			outDirectory)
////{
////	bool result = true; 
////	
////	MFile dir(outDirectory);
////
////	if (ChooseDirectory(dir))
////	{
////		outDirectory = dir.GetPath();
////		result = true;
////	}
////	
////	return result; 
////}
//
//bool ChooseOneFile(
//	MFile&	ioFile)
//{
//	GtkWidget* dialog = nil;
//	bool result = false;
//	
//	try
//	{
//		dialog = 
//			gtk_file_chooser_dialog_new(_("Select File"), nil,
//				GTK_FILE_CHOOSER_ACTION_OPEN,
//				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
//				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
//				NULL);
//		
//		THROW_IF_NIL(dialog);
//	
//		gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), false);
//		gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), false);
//		
//		if (ioFile.IsValid())
//		{
//			gtk_file_chooser_select_uri(GTK_FILE_CHOOSER(dialog),
//				ioFile.GetURI().c_str());
//		}
//		else if (gApp->GetCurrentFolder().length() > 0)
//		{
//			gtk_file_chooser_set_current_folder_uri(
//				GTK_FILE_CHOOSER(dialog), gApp->GetCurrentFolder().c_str());
//		}
//		
//		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
//		{
//			char* uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
//			if (uri != nil)
//			{
//				ioFile = MFile(uri, true);
//				g_free(uri);
//	
//				gApp->SetCurrentFolder(
//					gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog)));
//	
//				result = true;
//			}
//		}
//	}
//	catch (exception& e)
//	{
//		if (dialog)
//			gtk_widget_destroy(dialog);
//		
//		throw;
//	}
//	
//	gtk_widget_destroy(dialog);
//	
//	return result;
//}
//
//bool ChooseFiles(
//	bool				inLocalOnly,
//	std::vector<MFile>&	outFiles)
//{
//	GtkWidget* dialog = nil;
//	
//	try
//	{
//		dialog = 
//			gtk_file_chooser_dialog_new(_("Open"), nil,
//				GTK_FILE_CHOOSER_ACTION_OPEN,
//				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
//				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
//				NULL);
//		
//		THROW_IF_NIL(dialog);
//	
//		gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), true);
//		gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), inLocalOnly);
//		
//		if (gApp->GetCurrentFolder().length() > 0)
//		{
//			gtk_file_chooser_set_current_folder_uri(
//				GTK_FILE_CHOOSER(dialog), gApp->GetCurrentFolder().c_str());
//		}
//		
//		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
//		{
//			GSList* uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));	
//			
//			GSList* file = uris;	
//			
//			while (file != nil)
//			{
//				MFile url(reinterpret_cast<char*>(file->data), true);
//
//				g_free(file->data);
//				file->data = nil;
//
//				outFiles.push_back(url);
//
//				file = file->next;
//			}
//			
//			g_slist_free(uris);
//		}
//		
//		char* cwd = gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
//		if (cwd != nil)
//		{
//			gApp->SetCurrentFolder(cwd);
//			g_free(cwd);
//		}
//	}
//	catch (exception& e)
//	{
//		if (dialog)
//			gtk_widget_destroy(dialog);
//		
//		throw;
//	}
//	
//	gtk_widget_destroy(dialog);
//	
//	return outFiles.size() > 0;
//}
