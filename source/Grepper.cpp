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

#include <Directory.h>
#include <List.h>
#include <NodeInfo.h>
#include <Path.h>
#include <UTF8.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Grepper.h"


char *strdup_to_utf8(uint32 encode, const char *src, int32 length)
{
	int32 srcLen = length;
	int32 dstLen = srcLen + srcLen;
	char *dst = new char[dstLen+1];
	int32 cookie = 0;
	convert_to_utf8(encode, src, &srcLen, dst, &dstLen, &cookie);
	dst[dstLen] = '\0';
	char *dup = strdup(dst);
	delete[] dst;
	if (srcLen != length)
		fprintf(stderr, "strdup_to_utf8(%ld, %ld) dst allocate smoalled(%ld)\n",
						encode, length, dstLen);
	return dup;
}


char *strdup_from_utf8(uint32 encode, const char *src, int32 length)
{
	int32 srcLen = length;
	int32 dstLen = srcLen;
	char *dst = new char[dstLen+1];
	int32 cookie = 0;
	convert_from_utf8(encode, src, &srcLen, dst, &dstLen, &cookie);
	dst[dstLen] = '\0';
	char *dup = strdup(dst);
	delete[] dst;
	if (srcLen != length)
		fprintf(stderr, "strdup_from_utf8(%ld, %ld) dst allocate smoalled(%ld)\n",
						encode, length, dstLen);
	return dup;
}


Grepper::Grepper(const char *pattern, Model *model) 
{
	fModel = model;
	
	fThreadId = -1;
	fMustQuit = false;
	fCurrentRef = 0;

	if (fModel->fEncoding) {
		char *src = strdup_from_utf8(fModel->fEncoding, pattern, strlen(pattern));
		SetPattern(src);
		free(src);
	}
	else
		SetPattern(pattern);

	fCurrentDir = new BDirectory(&fModel->fDirectory);
	fCurrentDir->Rewind();

	fDirectories = new BList(10);
	fDirectories->AddItem(fCurrentDir);
}


Grepper::~Grepper()
{
	free(fPattern);

	// If the thread terminated normally, then there is only 
	// one object in the list: the initial directory. But if 
	// the user aborted the search, there may be more.

	for (int32 t = fDirectories->CountItems(); t > 0; --t) {
		delete static_cast<BDirectory*>(fDirectories->RemoveItem((int32) 0));
	}

	delete fDirectories;
}


void Grepper::Start()
{
	fThreadId = spawn_thread(
		SpawnThread, "GrepperThread", B_NORMAL_PRIORITY, this); 
                                
	resume_thread(fThreadId); 
}


void Grepper::Cancel()
{
	fMustQuit = true;
	int32 exitValue;
	wait_for_thread(fThreadId, &exitValue);
}


int32 Grepper::SpawnThread(void *arg) 
{ 
	return static_cast<Grepper*>(arg)->GrepperThread();
}
   

int32 Grepper::GrepperThread() 
{
	BMessage message;

	char fileName[B_PATH_NAME_LENGTH]; 
	char tempString[B_PATH_NAME_LENGTH];
	char command[B_PATH_NAME_LENGTH + 32];

	BPath tempFile;
	if (find_directory(B_SYSTEM_TEMP_DIRECTORY, &tempFile, true) != B_OK)
		return -1;
	sprintf(fileName, "TrackerGrep%ld", fThreadId);
	tempFile.Append(fileName);

	while (!fMustQuit && GetNextName(fileName)) {
		message.MakeEmpty();
		message.what = MSG_REPORT_FILE_NAME;
		message.AddString("filename", fileName);
		fModel->fTarget->PostMessage(&message);

		message.MakeEmpty();
		message.what = MSG_REPORT_RESULT;
		message.AddString("filename", fileName);
		
		BEntry entry(fileName);
		entry_ref ref;
		entry.GetRef(&ref);
		message.AddRef("ref", &ref);

		EscapeSpecialChars(fileName);

		//assume that grep is already in $PATH
		sprintf(
			command, "grep -hn %s %s \"%s\" > \"%s\"",
			fModel->fCaseSensitive ? "" : "-i", fPattern, fileName, 
			tempFile.Path());

		int res = system(command);

		if (res == 0 || res == 1) {
			FILE *results = fopen(tempFile.Path(), "r");

			if (results != NULL) {
				while (fgets(tempString, B_PATH_NAME_LENGTH, results) != 0)
				{
					if (fModel->fEncoding) {
						char *tempdup = strdup_to_utf8(fModel->fEncoding, 
							tempString, strlen(tempString));
						message.AddString("text", tempdup);
						free(tempdup);
					}
					else
						message.AddString("text", tempString);
				}
				
				if (message.HasString("text"))
					fModel->fTarget->PostMessage(&message);

				fclose(results);
				continue;
			}
		}

		sprintf(
			tempString, "%s: There was a problem running grep.",
			fileName);

		message.MakeEmpty();
		message.what = MSG_REPORT_ERROR;
		message.AddString("error", tempString);
		fModel->fTarget->PostMessage(&message);
	}

	// We wait with removing the temporary file until after the
	// entire search has finished, to prevent a lot of flickering
	// if the Tracker window for /boot/var/tmp/ might be open.

	remove(tempFile.Path());

	message.MakeEmpty();
	message.what = MSG_SEARCH_FINISHED;
	fModel->fTarget->PostMessage(&message);

	return 0;
}


void Grepper::SetPattern(const char *src)
{
	if (fModel->fEscapeText) {
		// We will simply guess the size of the memory buffer
		// that we need. This should always be large enough.
		fPattern = (char*) malloc((strlen(src) + 1) * 3 * sizeof(char));
		
		const char *srcPtr = src;
		char *dstPtr = fPattern;

		// Put double quotes around the pattern, so separate
		// words are considered to be part of a single string.
		*dstPtr++ = '"';

		while (*srcPtr != '\0') {
			char c = *srcPtr++;
			
			// Put a backslash in front of characters
			// that should be escaped. 
			if ((c == '.')  || (c == ',')
				||  (c == '[')  || (c == ']')
				||  (c == '?')  || (c == '*')
				||  (c == '+')  || (c == '-')
				||  (c == ':')  || (c == '^')
				||  (c == '\'') || (c == '"')) {
				*dstPtr++ = '\\';
			} else if ((c == '\\') || (c == '$')) {
				// Some characters need to be escaped
				// with *three* backslashes in a row.
				*dstPtr++ = '\\';
				*dstPtr++ = '\\';
				*dstPtr++ = '\\';
			}

			// Note: we do not have to escape the
			// { } ( ) < > and | characters.

			*dstPtr++ = c;
		}

		*dstPtr++ = '"';
		*dstPtr = '\0';
	} else
		fPattern = strdup(src);
}


void Grepper::EscapeSpecialChars(char *buffer)
{
	char *copy = strdup(buffer);
	uint32 len = strlen(copy);
	for (uint32 count = 0; count < len; ++count) {
		if (copy[count] == '"' || copy[count] == '$')
			*buffer++ = '\\';      
		*buffer++ = copy[count];
	}
	*buffer = '\0';
	free(copy);
}


bool Grepper::GetNextName(char *buffer)
{
	BEntry entry;
	struct stat fileStat;

	while (true) {
		// Traverse the directory to get a new BEntry.
		// GetNextEntry returns false if there are no
		// more entries, and we exit the loop.

		if (!GetNextEntry(entry))
			return false;

		// If the entry is a subdir, then add it to the
		// list of directories and continue the loop. 
		// If the entry is a file and we can grep it
		// (i.e. it is a text file), then we're done
		// here. Otherwise, continue with the next entry.

		if (entry.GetStat(&fileStat) == B_OK) {
			if (S_ISDIR(fileStat.st_mode)) {
				// subdir
				ExamineSubdir(entry);
			} else {
				// file or a (non-traversed) symbolic link
				if (ExamineFile(entry, buffer))
					return true;
			}
		}
	}
}


bool Grepper::GetNextEntry(BEntry &entry)
{
	if (fDirectories->CountItems() == 1)
		return GetTopEntry(entry);
	else
		return GetSubEntry(entry);
}


bool Grepper::GetTopEntry(BEntry &entry)
{
	// If the user selected one or more files, we must look 
	// at the "refs" inside the message that was passed into 
	// our add-on's process_refs(). If the user didn't select 
	// any files, we will simply read all the entries from the 
	// current working directory.
	
	entry_ref fileRef;

	if (fModel->fSelectedFiles.FindRef("refs",
		fCurrentRef, &fileRef) == B_OK) {
		entry.SetTo(&fileRef, fModel->fRecurseLinks);
		++fCurrentRef;
		return true;
	} else if (fCurrentRef > 0) {
		// when we get here, we have processed
		// all the refs from the message
		return false;
	} else {
		// examine the whole directory
		return fCurrentDir->GetNextEntry(&entry,
			fModel->fRecurseLinks) == B_OK;
	}
}


bool Grepper::GetSubEntry(BEntry &entry)
{
	if (fCurrentDir->GetNextEntry(&entry, fModel->fRecurseLinks) != B_OK) {
		// If we get here, there are no more entries in 
		// this subdir, so return to the parent directory.

		fDirectories->RemoveItem(fCurrentDir);
		delete fCurrentDir;
		fCurrentDir = (BDirectory*) fDirectories->LastItem();
	
		return GetNextEntry(entry);
	}

	return true;
}


void Grepper::ExamineSubdir(BEntry &entry)
{
	if (fModel->fRecurseDirs) {
		if (fModel->fSkipDotDirs) {
			char nameBuf[B_FILE_NAME_LENGTH];
			if (entry.GetName(nameBuf) == B_OK) {
				if (*nameBuf == '.')
					return;
			}
		}

		BDirectory *dir = new BDirectory(&entry);

		if (dir->InitCheck() == B_OK) {
			// Add new directory to the list 
			// and make it the current one.
			fDirectories->AddItem(dir);  
			fCurrentDir = dir;
		} else {
			delete dir;
				// clean up
		}
	}
}


bool Grepper::ExamineFile(BEntry &entry, char *buffer)
{
	BPath path;
	if (entry.GetPath(&path) == B_OK) { 
		strcpy(buffer, path.Path());

		if (!fModel->fTextOnly)
			return true;

		BNode node(&entry);
		BNodeInfo nodeInfo(&node);
		char mimeTypeString[B_MIME_TYPE_LENGTH];

		if (nodeInfo.GetType(mimeTypeString) == B_OK) {
			BMimeType mimeType(mimeTypeString);
			BMimeType superType;

			if (mimeType.GetSupertype(&superType) == B_OK) {
				if ((strcmp("text", superType.Type()) == 0) 
					|| (strcmp("message", superType.Type()) == 0)) {							
					return true;
				} 
			}
		}
	}

	return false;
}

