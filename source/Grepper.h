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

#ifndef __GREPPER_H__
#define __GREPPER_H__

#include "Model.h"

// Executes "grep" in a background thread.
class Grepper {
	public:
	
		Grepper(const char *pattern, Model *model);
		virtual ~Grepper();
	
		void Start();
		void Cancel(); 
	
	private:
	
		// Spawns the real grepper thread.
		static int32 SpawnThread(void *arg);
	
		// The thread function that does the actual grepping.
		int32 GrepperThread(); 
	
		// Remembers, and possibly escapes, the search pattern.
		void SetPattern(const char *src);
	
		// Prepends all quotes, dollars and backslashes with at backslash
		// to prevent the shell from misinterpreting them.
		void EscapeSpecialChars(char *buffer);
	
		// Returns the full path name of the next file.
		bool GetNextName(char *buffer);
	
		// Looks for the next entry.
		bool GetNextEntry(BEntry &entry);
	
		// Looks for the next entry in the top-level dir.
		bool GetTopEntry(BEntry &entry);
	
		// Looks for the next entry in a subdir.
		bool GetSubEntry(BEntry &entry);
	
		// Determines whether we can add a subdir.
		void ExamineSubdir(BEntry &entry);
	
		// Determines whether we can grep a file.
		bool ExamineFile(BEntry &entry, char *buffer);
	
		// Contains pointers to BDirectory objects.
		BList *fDirectories;
	
		// The directory we are currently looking at.
		BDirectory *fCurrentDir;
	
		// The ref number we are currently looking at.
		int32 fCurrentRef;
	
		// The (escaped) search pattern.
		char *fPattern;
		
		// The directory or files to grep on.
		Model *fModel;
	    
		// Our thread's ID.
		thread_id fThreadId;
	
		// Whether our thread must quit.
		bool fMustQuit;
};

#endif // __GREPPER_H__
