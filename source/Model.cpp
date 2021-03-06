/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include <FindDirectory.h>
#include <Entry.h>
#include <Path.h>
#include <Directory.h>
#include <File.h>
#include <List.h>
#include <MenuItem.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Model.h"


Model::Model()
{
	fRecurseDirs = true;
	fRecurseLinks = false;
	fCaseSensitive = false;
	fEscapeText = true;
	fTextOnly = true;
	fInvokePe = false;
	fShowContents = false;
	fSkipDotDirs = true;

	fTarget = NULL;
	fState = STATE_IDLE;

	fFrame.left   = 100;
	fFrame.right  = 500;
	fFrame.top    = 100;
	fFrame.bottom = 400;
	
	BPath path;
	status_t status = find_directory(B_USER_DIRECTORY, &path);
	if (status == B_OK)
		fFilePanelPath = path.Path();
	else
		fFilePanelPath = "/boot/home";
	
	fEncoding = 0;
}


status_t Model::LoadPrefs()
{
	BFile file;
	status_t status = OpenFile(&file, PREFS_FILE, B_READ_ONLY, 
		B_USER_SETTINGS_DIRECTORY, NULL);
	
	if (status != B_OK)
		return status;
	
	status = file.Lock();
	if (status != B_OK)
		return status;
	
	int32 value;
	
	if (file.ReadAttr("RecurseDirs", B_INT32_TYPE, 0, &value, sizeof(int32)) > 0)
		fRecurseDirs = (value != 0);

	if (file.ReadAttr("RecurseLinks", B_INT32_TYPE, 0, &value, sizeof(int32)) > 0)
		fRecurseLinks = (value != 0);

	if (file.ReadAttr("SkipDotDirs", B_INT32_TYPE, 0, &value, sizeof(int32)) > 0)
		fSkipDotDirs = (value != 0);

	if (file.ReadAttr("CaseSensitive", B_INT32_TYPE, 0, &value, sizeof(int32)) > 0)
		fCaseSensitive = (value != 0);

	if (file.ReadAttr("EscapeText", B_INT32_TYPE, 0, &value, sizeof(int32)) > 0)
		fEscapeText = (value != 0);

	if (file.ReadAttr("TextOnly", B_INT32_TYPE, 0, &value, sizeof(int32)) > 0)
		fTextOnly = (value != 0);

	if (file.ReadAttr("InvokePe", B_INT32_TYPE, 0, &value, sizeof(int32)) > 0)
		fInvokePe = (value != 0);

	if (file.ReadAttr("ShowContents", B_INT32_TYPE, 0, &value, sizeof(int32)) > 0)
		fShowContents = (value != 0);

	char buffer [B_PATH_NAME_LENGTH+1];
	int32 length = file.ReadAttr("FilePanelPath", B_STRING_TYPE, 0, &buffer, sizeof(buffer));
	if (length > 0) {
		buffer[length] = '\0';
		fFilePanelPath = buffer;
	}
	
	file.ReadAttr("WindowFrame", B_RECT_TYPE, 0, &fFrame, sizeof(BRect));

	if (file.ReadAttr("Encoding", B_INT32_TYPE, 0, &value, sizeof(int32)) > 0)
		fEncoding = value;

	file.Unlock();
	
	return B_OK;
}


status_t Model::SavePrefs()
{
	BFile file;
	status_t status = OpenFile(&file, PREFS_FILE, 
		B_CREATE_FILE | B_WRITE_ONLY, B_USER_SETTINGS_DIRECTORY, NULL);
	
	if (status != B_OK)
		return status;

	status = file.Lock();
	if (status != B_OK)
		return status;

	int32 value = 2;
	file.WriteAttr("Version", B_INT32_TYPE, 0, &value, sizeof(int32));

	value = fRecurseDirs ? 1 : 0;
	file.WriteAttr("RecurseDirs", B_INT32_TYPE, 0, &value, sizeof(int32));

	value = fRecurseLinks ? 1 : 0;
	file.WriteAttr("RecurseLinks", B_INT32_TYPE, 0, &value, sizeof(int32));

	value = fSkipDotDirs ? 1 : 0;
	file.WriteAttr("SkipDotDirs", B_INT32_TYPE, 0, &value, sizeof(int32));

	value = fCaseSensitive ? 1 : 0;
	file.WriteAttr("CaseSensitive", B_INT32_TYPE, 0, &value, sizeof(int32));

	value = fEscapeText ? 1 : 0;
	file.WriteAttr("EscapeText", B_INT32_TYPE, 0, &value, sizeof(int32));

	value = fTextOnly ? 1 : 0;
	file.WriteAttr("TextOnly", B_INT32_TYPE, 0, &value, sizeof(int32));
	
	value = fInvokePe ? 1 : 0;
	file.WriteAttr("InvokePe", B_INT32_TYPE, 0, &value, sizeof(int32));
	
	value = fShowContents ? 1 : 0;
	file.WriteAttr("ShowContents", B_INT32_TYPE, 0, &value, sizeof(int32));
	
	file.WriteAttr("WindowFrame", B_RECT_TYPE, 0, &fFrame, sizeof(BRect));
	
	file.WriteAttr("FilePanelPath", B_STRING_TYPE, 0, fFilePanelPath.String(), fFilePanelPath.Length()+1);

	file.WriteAttr("Encoding", B_INT32_TYPE, 0, &fEncoding, sizeof(int32));

	file.Sync();
	file.Unlock();

	return B_OK;
}


void Model::AddToHistory(const char *text) 
{
	BList *items = LoadHistory();

	for (int32 t = 0; t < items->CountItems(); ++t) {
		// If the same text is already in the list,
		// then remove it first. Case-sensitive.
		
		BString *string = static_cast<BString*>(items->ItemAt(t));
		if (*string == text) {
			delete static_cast<BString*>(items->RemoveItem(t));
			break;
		}
	}

	if (items->CountItems() == HISTORY_LIMIT)
		delete static_cast<BString*>(items->RemoveItem((int32) 0));

	items->AddItem(new BString(text));

	SaveHistory(items);
	FreeHistory(items);
}


void Model::FillHistoryMenu(BMenu *menu)
{
	BList *items = LoadHistory();

	for (int32 t = items->CountItems(); t > 0; --t) {
		BString *item = static_cast<BString*>(items->ItemAt(t - 1));
		BMessage *message = new BMessage(MSG_SELECT_HISTORY);
		message->AddString("text", item->String());
		menu->AddItem(new BMenuItem(item->String(), message));
	}

	FreeHistory(items);
}


BList *Model::LoadHistory()
{
	BList *items = new BList();

	BFile file;
	status_t status = OpenFile(&file, PREFS_FILE, B_READ_ONLY, 
		B_USER_SETTINGS_DIRECTORY, NULL);
	
	if (status != B_OK)
		return items;
	
	status = file.Lock();
	if (status != B_OK)
		return items;
	
	BMessage message;
	status = message.Unflatten(&file);
	if (status != B_OK)
		return items;
	
	file.Unlock();
	
	BString string;
	for (int32 x = 0; ; x++) {
		status = message.FindString("string", x, &string);
		if (status != B_OK)
			break;
		
		items->AddItem(new BString(string.String()));
	}

	return items;
}


status_t Model::SaveHistory(BList *items)
{
	BFile file;
	status_t status = OpenFile(&file, PREFS_FILE, 
		B_CREATE_FILE | B_WRITE_ONLY, 
		B_USER_SETTINGS_DIRECTORY, NULL);
	
	if (status != B_OK)
		return status;

	status = file.Lock();
	if (status != B_OK)
		return status;
	
	BMessage message;
	for (int32 x = 0; ; x++) {
		BString *string = static_cast<BString*>(items->ItemAt(x));
		if (string == NULL)
			break;
		
		message.AddString("string", string->String());
	}

	message.Flatten(&file);
	file.SetSize(message.FlattenedSize());
	file.Sync();
	file.Unlock();
	
	return B_OK;
}


void Model::FreeHistory(BList *items)
{
	for (int32 t = items->CountItems(); t > 0; --t) {
		delete static_cast<BString*>((items->RemoveItem(t - 1)));
	}

	delete items;
}


status_t Model::OpenFile(BFile *file, char *name, uint32 openMode, 
	directory_which which, BVolume *volume)
{
	if (file == NULL)
		return B_ERROR;

	BPath path;
	status_t status = find_directory(which, &path, true, volume);
	
	if (status != B_OK)
		return status;
		
	status = path.Append(PREFS_FILE);
	if (status != B_OK)
		return status;
		
	status = file->SetTo(path.Path(), openMode);
	if (status != B_OK)
		return status;
	
	status = file->InitCheck();
	if (status != B_OK)
		return status;
		
	return B_OK;
}
