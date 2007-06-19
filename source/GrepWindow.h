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

#ifndef __GREP_WINDOW_H__
#define __GREP_WINDOW_H__

#include <InterfaceKit.h>
#include <FilePanel.h>

#include "Model.h"
#include "GrepListView.h"

class Grepper;

class GrepWindow : public BWindow {
	public:
	
		GrepWindow(BMessage *message);
		virtual ~GrepWindow();
	
		virtual void FrameResized(float width, float height);
		virtual void FrameMoved(BPoint origin);
		virtual void MenusBeginning();
		virtual void MenusEnded();
		virtual void MessageReceived(BMessage *message);
		virtual void Quit();
	
	private:
	
		void InitRefsReceived(entry_ref *directory, BMessage *message);
		void SetWindowTitle();
		void CreateMenus();
		void CreateViews();
		void LayoutViews();
		void TileIfMultipleWindows();
	
		void LoadPrefs();
		void SavePrefs();
	
		void OnStartCancel();
		void OnSearchFinished();
		void OnReportFileName(BMessage *message);
		void OnReportResult(BMessage *message);
		void OnReportError(BMessage *message);
		void OnRecurseLinks();
		void OnRecurseDirs();
		void OnSkipDotDirs();
		void OnCaseSensitive();
		void OnEscapeText();
		void OnTextOnly();
		void OnInvokePe();
		void OnCheckboxShowLines();
		void OnMenuShowLines();
		void OnInvokeItem();
		void OnSearchText();
		void OnHistoryItem(BMessage *message);
		void OnTrimSelection();
		void OnCopyText();
		void OnSelectInTracker();
		void OnQuitNow();
		void OnAboutRequested();
		void OnFileDrop(BMessage *message);
		void OnRefsReceived(BMessage *message);
		void OnOpenPanel();
		void OnOpenPanelCancel();
		void OnSelectAll(BMessage *message);
		void OnNewWindow();
		
		bool OpenInPe(const entry_ref &ref, int32 lineNum);
		void RemoveFolderListDuplicates(BList *folderList);
		status_t OpenFoldersInTracker(BList *folderList);
		bool AreAllFoldersOpenInTracker(BList *folderList);
		status_t SelectFilesInTracker(BList *folderList, BMessage *refsMessage);
	
		BTextControl *fSearchText;
		GrepListView *fSearchResults;
		
		BMenuBar *fMenuBar;
		BMenu *fFileMenu;
		BMenuItem *fNew;
		BMenuItem *fOpen;
		BMenuItem *fClose;
		BMenuItem *fAbout;
		BMenuItem *fQuit;
		BMenu *fActionMenu;
		BMenuItem *fSelectAll;
		BMenuItem *fSearch;
		BMenuItem *fTrimSelection;
		BMenuItem *fCopyText;
		BMenuItem *fSelectInTracker;
		BMenuItem *fOpenSelection;
		BMenu *fPreferencesMenu;
		BMenuItem *fRecurseLinks;
		BMenuItem *fRecurseDirs;
		BMenuItem *fSkipDotDirs;
		BMenuItem *fCaseSensitive;
		BMenuItem *fEscapeText;
		BMenuItem *fTextOnly;
		BMenuItem *fInvokePe;
		BMenuItem *fShowLinesMenuitem;
		BMenu *fHistoryMenu;
		BMenu *fEncodingMenu;
		BMenuItem *fUTF8;
		BMenuItem *fShiftJIS;
		BMenuItem *fEUC;
		BMenuItem *fJIS;
		
		BCheckBox *fShowLinesCheckbox;
		BButton *fButton;
	
		Grepper *fGrepper;
		BString fOldPattern;
		
		Model *fModel;
		
		BFilePanel *fFilePanel;
};

#endif // __GREP_WINDOW_H__
