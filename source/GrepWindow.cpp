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

#include <Application.h>
#include <AppFileInfo.h>
#include <Alert.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "TranslZeta.h"
#include "Grepper.h"
#include "GrepWindow.h"


class ResultItem : public BStringItem {
	public:

		ResultItem(entry_ref &ref) : BStringItem("", 0, false)
		{
			this->ref = ref;
			BEntry entry(&ref);
			BPath path(&entry);
			SetText(path.Path()); 
		}
	
		entry_ref ref;
};


GrepWindow::GrepWindow(BMessage *message)
	: BWindow(BRect(0, 0, 1, 1), NULL, B_DOCUMENT_WINDOW, 0)
{
	entry_ref directory;
	InitRefsReceived(&directory, message);

	fFilePanel = NULL;
	fGrepper = NULL;

	fModel = new Model();
	fModel->fDirectory = directory;
	fModel->fSelectedFiles = *message;
	fModel->fTarget = this;

	SetWindowTitle();
	CreateMenus();
	CreateViews();
	LayoutViews();
	LoadPrefs();
	TileIfMultipleWindows();
	Show(); 	
}


void GrepWindow::InitRefsReceived(entry_ref *directory, BMessage *message)
{
	// HACK-HACK-HACK:
	// 	If the user selected a single folder and invoked TrackerGrep on it,
	//		but recurse directories is switched off, TrackerGrep would do nothing.
	//		In that special case, we'd like it to recurse into that folder (but
	//		not go any deeper after that).

	type_code code;
	int32 count;
	message->GetInfo("refs", &code, &count);
	
	if (count == 0) {
		if (message->FindRef("dir_ref", 0, directory) == B_OK)
			message->MakeEmpty();
	}
	
	if (count == 1) {
		entry_ref ref;
		if (message->FindRef("refs", 0, &ref) == B_OK) {
			BEntry entry(&ref, true);
			if (entry.IsDirectory()) {
				// ok, special case, we use this folder as base directory
				// and pretend nothing had been selected:
				*directory = ref;
				message->MakeEmpty();
			}
		}
	}
}

GrepWindow::~GrepWindow()
{
	if (fModel->fState == STATE_SEARCH) {
		fGrepper->Cancel();
	}
	
	delete fModel;
}


void GrepWindow::FrameResized(float width, float height)
{
	BWindow::FrameResized(width, height);
	fModel->fFrame = Frame();
	SavePrefs();
}


void GrepWindow::FrameMoved(BPoint origin)
{
	BWindow::FrameMoved(origin);
	fModel->fFrame = Frame();
	SavePrefs();
}


void GrepWindow::MenusBeginning()
{
	fModel->FillHistoryMenu(fHistoryMenu);
	BWindow::MenusBeginning();
}


void GrepWindow::MenusEnded()
{
	for (int32 t = fHistoryMenu->CountItems(); t > 0; --t)
		delete fHistoryMenu->RemoveItem(t - 1);

	BWindow::MenusEnded();
}


void GrepWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_ABOUT_REQUESTED:
			OnAboutRequested();
			break;
			
		case MSG_NEW_WINDOW:
			OnNewWindow();
			break;
		
		case B_SIMPLE_DATA:
			OnFileDrop(message);
			break;
			
		case MSG_OPEN_PANEL:
			OnOpenPanel();
			break;
			
		case MSG_REFS_RECEIVED:
			OnRefsReceived(message);
			break;
			
		case B_CANCEL:
			OnOpenPanelCancel();
			break;
			
		case MSG_RECURSE_LINKS:
			OnRecurseLinks();
			break;
			
		case MSG_RECURSE_DIRS:
			OnRecurseDirs();
			break;
			
		case MSG_SKIP_DOT_DIRS:
			OnSkipDotDirs();
			break;
			
		case MSG_CASE_SENSITIVE:
			OnCaseSensitive();
			break;
			
		case MSG_ESCAPE_TEXT:
			OnEscapeText();
			break;
			
		case MSG_TEXT_ONLY:
			OnTextOnly();
			break;
			
		case MSG_INVOKE_PE:
			OnInvokePe();
			break;
			
		case MSG_SEARCH_TEXT:
			OnSearchText();
			break;
			
		case MSG_SELECT_HISTORY:
			OnHistoryItem(message);
			break;
			
		case MSG_START_CANCEL:
			OnStartCancel();
			break;
			
		case MSG_SEARCH_FINISHED:
			OnSearchFinished();
			break;
			
		case MSG_REPORT_FILE_NAME:
			OnReportFileName(message);
			break;
			
		case MSG_REPORT_RESULT:
			OnReportResult(message);
			break;
			
		case MSG_REPORT_ERROR:
			OnReportError(message);
			break;
			
		case MSG_SELECT_ALL:
			OnSelectAll(message);
			break;
			
		case MSG_TRIM_SELECTION:
			OnTrimSelection();
			break;
			
		case MSG_COPY_TEXT:
			OnCopyText();
			break;
			
		case MSG_SELECT_IN_TRACKER:
			OnSelectInTracker();
			break;
			
		case MSG_MENU_SHOW_LINES:
			OnMenuShowLines();
			break;
			
		case MSG_CHECKBOX_SHOW_LINES:
			OnCheckboxShowLines();
			break;
		
		case MSG_OPEN_SELECTION:
			// fall through
		case MSG_INVOKE_ITEM:
			OnInvokeItem();
			break;
			
		case MSG_QUIT_NOW:
			OnQuitNow();
			break;
			
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void GrepWindow::Quit()
{
	SavePrefs();

	if (be_app->Lock()) {
		be_app->PostMessage(MSG_TRY_QUIT);
		be_app->Unlock();
		BWindow::Quit();
	}
}


void GrepWindow::SetWindowTitle()
{
	BEntry entry(&fModel->fDirectory, true);
	BString title;
	if (entry.InitCheck() == B_OK) {
		BPath path;
		if (entry.GetPath(&path) == B_OK)
			title << "Tracker Grep: " << path.Path();
	}

	if (!title.Length())
		title = "Tracker Grep";
		
	SetTitle(title.String());
}


void GrepWindow::CreateMenus()
{
	fMenuBar = new BMenuBar(BRect(0,0,1,1), "menubar");

	fFileMenu = new BMenu(TranslZeta("File"));
	fActionMenu = new BMenu(TranslZeta("Actions"));
	fPreferencesMenu = new BMenu(TranslZeta("Preferences"));
	fHistoryMenu = new BMenu(TranslZeta("History"));
	
	fNew = new BMenuItem(
		TranslZeta("New Window"), new BMessage(MSG_NEW_WINDOW), 'N');
		
	fOpen = new BMenuItem(
		TranslZeta("Set Which Files to Search"), new BMessage(MSG_OPEN_PANEL), 'F');

	fClose = new BMenuItem(
		TranslZeta("Close"), new BMessage(B_QUIT_REQUESTED), 'W');
	
	fAbout = new BMenuItem(
		TranslZeta("About"), new BMessage(B_ABOUT_REQUESTED));

	fQuit = new BMenuItem(
		TranslZeta("Quit"), new BMessage(MSG_QUIT_NOW), 'Q');
	
	fSearch = new BMenuItem(
		TranslZeta("Search"), new BMessage(MSG_START_CANCEL), 'S');
		
	fSelectAll = new BMenuItem(
		TranslZeta("Select All"), new BMessage(MSG_SELECT_ALL), 'A');
		
	fTrimSelection = new BMenuItem(
		TranslZeta("Trim to Selection"), new BMessage(MSG_TRIM_SELECTION), 'T');
		
	fOpenSelection = new BMenuItem(
		TranslZeta("Open Selection"), new BMessage(MSG_OPEN_SELECTION), 'O');
		
	fSelectInTracker = new BMenuItem(
		TranslZeta("Show Files in Tracker"), new BMessage(MSG_SELECT_IN_TRACKER), 'K');

	fCopyText = new BMenuItem(
		TranslZeta("Copy Text to Clipboard"), new BMessage(MSG_COPY_TEXT), 'B');

	fRecurseLinks = new BMenuItem(
		TranslZeta("Follow symbolic links"), new BMessage(MSG_RECURSE_LINKS));

	fRecurseDirs = new BMenuItem(
		TranslZeta("Look in sub-directories"), new BMessage(MSG_RECURSE_DIRS));

	fSkipDotDirs = new BMenuItem(
		TranslZeta("Skip sub-directories starting with a dot"), new BMessage(MSG_SKIP_DOT_DIRS));

	fCaseSensitive = new BMenuItem(
		TranslZeta("Case sensitive"), new BMessage(MSG_CASE_SENSITIVE));

	fEscapeText = new BMenuItem(
		TranslZeta("Escape search text"), new BMessage(MSG_ESCAPE_TEXT));

	fTextOnly = new BMenuItem(
		TranslZeta("Text files only"), new BMessage(MSG_TEXT_ONLY));

	fInvokePe = new BMenuItem(
		TranslZeta("Open files in Pe"), new BMessage(MSG_INVOKE_PE));

	fShowLinesMenuitem = new BMenuItem(
		TranslZeta("Show Lines"), new BMessage(MSG_MENU_SHOW_LINES), 'L');
	fShowLinesMenuitem->SetMarked(true);
		
	fFileMenu->AddItem(fNew);
	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem(fOpen);
	fFileMenu->AddItem(fClose);
	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem(fAbout);
	fFileMenu->AddSeparatorItem();
	fFileMenu->AddItem(fQuit);
	
	fActionMenu->AddItem(fSearch);
	fActionMenu->AddSeparatorItem();
	fActionMenu->AddItem(fSelectAll);
	fActionMenu->AddItem(fTrimSelection);
	fActionMenu->AddSeparatorItem();
	fActionMenu->AddItem(fOpenSelection);
	fActionMenu->AddItem(fSelectInTracker);
	fActionMenu->AddItem(fCopyText);
	
	fPreferencesMenu->AddItem(fRecurseLinks);
	fPreferencesMenu->AddItem(fRecurseDirs);
	fPreferencesMenu->AddItem(fSkipDotDirs);
	fPreferencesMenu->AddItem(fCaseSensitive);
	fPreferencesMenu->AddItem(fEscapeText);
	fPreferencesMenu->AddItem(fTextOnly);
	fPreferencesMenu->AddItem(fInvokePe);
	fPreferencesMenu->AddSeparatorItem();
	fPreferencesMenu->AddItem(fShowLinesMenuitem);
	
	fMenuBar->AddItem(fFileMenu);
	fMenuBar->AddItem(fActionMenu);
	fMenuBar->AddItem(fPreferencesMenu);
	fMenuBar->AddItem(fHistoryMenu);
	
	AddChild(fMenuBar);
	SetKeyMenuBar(fMenuBar);
	
	fSearch->SetEnabled(false);
}


void GrepWindow::CreateViews()
{
	// The search pattern entry field does not send a message when 
	// <Enter> is pressed, because the "Search/Cancel" button already 
	// does this and we don't want to send the same message twice.

	fSearchText = new BTextControl(
		BRect(0, 0, 0, 1), "SearchText", NULL, NULL, NULL,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE); 
	
	fSearchText->TextView()->SetMaxBytes(1000); 
	fSearchText->ResizeToPreferred();
		// because it doesn't have a label

	fSearchText->SetModificationMessage(new BMessage(MSG_SEARCH_TEXT));

	fButton = new BButton(
		BRect(0, 1, 80, 1), "Button", TranslZeta("Search"),
		new BMessage(MSG_START_CANCEL), B_FOLLOW_RIGHT);
	
	fButton->MakeDefault(true);
	fButton->ResizeToPreferred();
	fButton->SetEnabled(false);
	
	fShowLinesCheckbox = new BCheckBox(
		BRect(0, 0, 1, 1), "ShowLines", TranslZeta("Show Lines"),
		new BMessage(MSG_CHECKBOX_SHOW_LINES), B_FOLLOW_LEFT);
	
	fShowLinesCheckbox->SetValue(B_CONTROL_ON);
	fShowLinesCheckbox->ResizeToPreferred();
	
	fSearchResults = new GrepListView(); 

	fSearchResults->SetInvocationMessage(new BMessage(MSG_INVOKE_ITEM));
	fSearchResults->ResizeToPreferred();
}	


void GrepWindow::LayoutViews()
{
	float menubarWidth, menubarHeight = 20;
	fMenuBar->GetPreferredSize(&menubarWidth, &menubarHeight);

	BBox *background = new BBox(
		BRect(0, menubarHeight + 1, 2, menubarHeight + 2), B_EMPTY_STRING, 
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, 
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, 
		B_PLAIN_BORDER); 

	BScrollView *scroller = new BScrollView(
		"ScrollSearchResults", fSearchResults, B_FOLLOW_ALL_SIDES, 
		B_FULL_UPDATE_ON_RESIZE, true, true, B_NO_BORDER);

	scroller->ResizeToPreferred();

	float width = 8 + fShowLinesCheckbox->Frame().Width()
		+ 8 + fButton->Frame().Width() + 8;

	float height = 8 + fSearchText->Frame().Height() + 8
		+ fButton->Frame().Height() + 8 + scroller->Frame().Height();

	float backgroundHeight = 8 + fSearchText->Frame().Height()
		+ 8 + fButton->Frame().Height() + 8;

	ResizeTo(width, height);  

	AddChild(background);
	background->ResizeTo(width,	backgroundHeight);
	background->AddChild(fSearchText);
	background->AddChild(fShowLinesCheckbox);
	background->AddChild(fButton);

	fSearchText->MoveTo(8, 8);
	fSearchText->ResizeBy(width - 16, 0);
	fSearchText->MakeFocus(true);

	fShowLinesCheckbox->MoveTo(
		8, 8 + fSearchText->Frame().Height() + 8
			+ (fButton->Frame().Height() - fShowLinesCheckbox->Frame().Height())/2);

	fButton->MoveTo(
		width - fButton->Frame().Width() - 8,
		8 + fSearchText->Frame().Height() + 8);

	AddChild(scroller);
	scroller->MoveTo(0, menubarHeight + 1 + backgroundHeight + 1);
	scroller->ResizeTo(width + 1, height - backgroundHeight - menubarHeight - 1);

	BRect screenRect = BScreen(this).Frame();

	MoveTo(
		(screenRect.Width() - width) / 2,
		(screenRect.Height() - height) / 2);

	SetSizeLimits(width, 10000, height, 10000);
}


void GrepWindow::LoadPrefs()
{
	Lock();

	fModel->LoadPrefs();

	fRecurseDirs->SetMarked(fModel->fRecurseDirs);
	fRecurseLinks->SetMarked(fModel->fRecurseLinks);
	fSkipDotDirs->SetMarked(fModel->fSkipDotDirs);
	fCaseSensitive->SetMarked(fModel->fCaseSensitive);
	fEscapeText->SetMarked(fModel->fEscapeText);
	fTextOnly->SetMarked(fModel->fTextOnly);
	fInvokePe->SetMarked(fModel->fInvokePe);

	fShowLinesCheckbox->SetValue(
		fModel->fShowContents ? B_CONTROL_ON : B_CONTROL_OFF);
	fShowLinesMenuitem->SetMarked(
		fModel->fShowContents ? true : false);

	MoveTo(fModel->fFrame.left, fModel->fFrame.top);
	ResizeTo(fModel->fFrame.Width(), fModel->fFrame.Height());

	Unlock();
}


void GrepWindow::SavePrefs()
{
	fModel->SavePrefs();
}


void GrepWindow::OnStartCancel()
{
	if (fModel->fState == STATE_IDLE) {
		fModel->fState = STATE_SEARCH;

		fSearchResults->MakeEmpty();
		
		if (fSearchText->TextView()->TextLength() == 0)
			return;

		fModel->AddToHistory(fSearchText->Text());

		// From now on, we don't want to be notified when the
		// search pattern changes, because the control will be
		// displaying the names of the files we are grepping.

		fSearchText->SetModificationMessage(NULL);

		fFileMenu->SetEnabled(false);
		fActionMenu->SetEnabled(false);
		fPreferencesMenu->SetEnabled(false);
		fHistoryMenu->SetEnabled(false);
		
		fSearchText->SetEnabled(false);

		fButton->MakeFocus(true);
		fButton->SetLabel(TranslZeta("Cancel"));
		fSearch->SetEnabled(false);

		// We need to remember the search pattern, because during 
		// the grepping, the text control's text will be replaced 
		// by the name of the file that's currently being grepped. 
		// When the grepping finishes, we need to restore the old
		// search pattern.

		fOldPattern = fSearchText->Text();

		fGrepper = new Grepper(fOldPattern.String(), fModel);
		fGrepper->Start();
	} else if (fModel->fState == STATE_SEARCH) {
		fModel->fState = STATE_CANCEL;
		fGrepper->Cancel();
	}
}


void GrepWindow::OnSearchFinished()
{
	fModel->fState = STATE_IDLE;

	delete fGrepper;
	fGrepper = NULL;

	fFileMenu->SetEnabled(true);
	fActionMenu->SetEnabled(true);
	fPreferencesMenu->SetEnabled(true);
	fHistoryMenu->SetEnabled(true);
	
	fButton->SetLabel(TranslZeta("Search"));
	fButton->SetEnabled(true);
	fSearch->SetEnabled(true);
	
	fSearchText->SetEnabled(true);
	fSearchText->MakeFocus(true);
	fSearchText->SetText(fOldPattern.String());
	fSearchText->TextView()->SelectAll();
	fSearchText->SetModificationMessage(new BMessage(MSG_SEARCH_TEXT));
}


void GrepWindow::OnReportFileName(BMessage *message)
{
	fSearchText->SetText(message->FindString("filename"));
}

		
void GrepWindow::OnReportResult(BMessage *message)
{
	entry_ref ref;
	if (message->FindRef("ref", &ref) != B_OK)
		return;

	BStringItem *item = new ResultItem(ref);
	fSearchResults->AddItem(item);
	item->SetExpanded(fModel->fShowContents);

	type_code type;
	int32 count;
	message->GetInfo("text", &type, &count);

	const char *buf;
	while (message->FindString("text", --count, &buf) == B_OK) {
		uchar *temp = (uchar*) strdup(buf);
		uchar *ptr  = temp;

		while (true) {
			// replace all non-printable characters by spaces
			uchar c = *ptr;

			if (c == '\0')
				break;

			if (!(c & 0x80) && iscntrl(c))
				*ptr = ' ';

			++ptr;
		}

		fSearchResults->AddUnder(
			new BStringItem((const char*) temp), item);

		free(temp);
	}
}


void GrepWindow::OnReportError(BMessage *message)
{
	const char *buf;
	if (message->FindString("error", &buf) == B_OK)
		fSearchResults->AddItem(new BStringItem(buf));
}


void GrepWindow::OnRecurseLinks()
{
	fModel->fRecurseLinks = !fModel->fRecurseLinks;
	fRecurseLinks->SetMarked(fModel->fRecurseLinks);
	SavePrefs();
}

		
void GrepWindow::OnRecurseDirs()
{
	fModel->fRecurseDirs = !fModel->fRecurseDirs;
	fRecurseDirs->SetMarked(fModel->fRecurseDirs);
	SavePrefs();
}

		
void GrepWindow::OnSkipDotDirs()
{
	fModel->fSkipDotDirs = !fModel->fSkipDotDirs;
	fSkipDotDirs->SetMarked(fModel->fSkipDotDirs);
	SavePrefs();
}


void GrepWindow::OnEscapeText()
{
	fModel->fEscapeText = !fModel->fEscapeText;
	fEscapeText->SetMarked(fModel->fEscapeText);
	SavePrefs();
}

	
void GrepWindow::OnCaseSensitive()
{
	fModel->fCaseSensitive = !fModel->fCaseSensitive;
	fCaseSensitive->SetMarked(fModel->fCaseSensitive);
	SavePrefs();
}

	
void GrepWindow::OnTextOnly()
{
	fModel->fTextOnly = !fModel->fTextOnly;
	fTextOnly->SetMarked(fModel->fTextOnly);
	SavePrefs();
}


void GrepWindow::OnInvokePe()
{
	fModel->fInvokePe = !fModel->fInvokePe;
	fInvokePe->SetMarked(fModel->fInvokePe);
	SavePrefs();
}


void GrepWindow::OnCheckboxShowLines()
{
	// toggle checkbox and menuitem
	fModel->fShowContents = !fModel->fShowContents;
	fShowLinesMenuitem->SetMarked(!fShowLinesMenuitem->IsMarked());
	
	// Selection in BOutlineListView in multiple selection mode
	// gets weird when collapsing. I've tried all sorts of things.
	// It seems impossible to make it behave just right.
	
	// Going from collapsed to expande mode, the superitems
	// keep their selection, the subitems don't (yet) have 
	// a selection. This works as expected, AFAIK.
	
	// Going from expanded to collapsed mode, I would like
	// for a selected subitem (line) to select its superitem,
	// (its file) and the subitem be unselected. 
	
	// I've successfully tried code patches that apply the
	// selection pattern that I want, but with weird effects
	// on subsequent manual selection. 
	// Lines stay selected while the user tries to select
	// some other line. It just gets weird. 
	
	// It's as though listItem->Select() and Deselect() 
	// put the items in some semi-selected state.
	// Or maybe I've got it all wrong.
	
	// So, here's the plain basic collapse/expand. 
	// I think it's the least bad of what's possible on BeOS R5,
	// but perhaps someone comes along with a patch of magic.

	int32 numItems = fSearchResults->FullListCountItems();
	BListItem *listItem = NULL;

	for (int32 x = 0; x < numItems; ++x) {
	
		listItem = fSearchResults->FullListItemAt(x);
		
		if (listItem->OutlineLevel() == 0)
		{
			if (fModel->fShowContents) {
				if (!fSearchResults->IsExpanded(x))
					fSearchResults->Expand(listItem);
			} else {
				if (fSearchResults->IsExpanded(x))
					fSearchResults->Collapse(listItem);
			}
		}
	}
	
	fSearchResults->Invalidate();

	SavePrefs();
}


void GrepWindow::OnMenuShowLines()
{
	// toggle companion checkbox
	fShowLinesCheckbox->SetValue(!fShowLinesCheckbox->Value());
	OnCheckboxShowLines();
}


bool GrepWindow::OpenInPe(const entry_ref &ref, int32 lineNum)
{
	BMessage message('Cmdl');
	message.AddRef("refs", &ref);
	
	if (lineNum != -1)
		message.AddInt32("line", lineNum);

	entry_ref pe;
	if (be_roster->FindApp(PE_SIGNATURE, &pe) != B_OK)
		return false;
		
	if (be_roster->IsRunning(&pe)) {
		BMessenger msngr(NULL, be_roster->TeamFor(&pe));
		if (msngr.SendMessage(&message) != B_OK)
			return false;
	} else {
		if (be_roster->Launch(&pe, &message) != B_OK)
			return false;
	}
	return true;
}


void GrepWindow::OnInvokeItem()
{
	BListItem *item = NULL;
	int32 selectionIndex = -1;

	for (int32 itemIndex = 0; ; itemIndex++) {
		selectionIndex = fSearchResults->CurrentSelection(itemIndex); 
	
		if (selectionIndex >= 0)
			item = fSearchResults->ItemAt(selectionIndex);
		else
			break;
		
		if (item != NULL) {
			int32 level = item->OutlineLevel();
			int32 lineNum = -1;
	
			// Get the line number.
			// only this level has line numbers
			if (level == 1) {
				BStringItem *str = dynamic_cast<BStringItem*>(item);
				if (str != NULL) {
					lineNum = atol(str->Text());
						// fortunately, atol knows when to stop the conversion
				}
			}
	
			// Get the top-most item and launch its entry_ref.
			while (level != 0) {
				item = fSearchResults->Superitem(item);
				if (!item)
					break;
				level = item->OutlineLevel();
			}
	
			ResultItem *entry = dynamic_cast<ResultItem*>(item);
			if (entry != NULL) {
				bool done = false;
	
				if (fModel->fInvokePe)
					done = OpenInPe(entry->ref, lineNum);
	
				if (!done)
					be_roster->Launch(&entry->ref);
			}
		}
	}
}


void GrepWindow::OnSearchText()
{
	fButton->SetEnabled(fSearchText->TextView()->TextLength() != 0);
	fSearch->SetEnabled(fSearchText->TextView()->TextLength() != 0);
}


void GrepWindow::OnHistoryItem(BMessage *message)
{
	const char *buf;
	if (message->FindString("text", &buf) == B_OK)
		fSearchText->SetText(buf);
}


void GrepWindow::OnTrimSelection()
{
	if (fSearchResults->CurrentSelection() < 0) {
		BAlert *alert = new BAlert(NULL,
			TranslZeta("Please select the files you wish to keep searching. "
			"The unselected files will be removed from the list.\n"),
			TranslZeta("Okay"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go(NULL);
		return;
	}
	
	BStringItem *item = NULL;
	BStringItem *fileItem = NULL;
	
	BMessage message;
	BString path;

	for (int32 index = 0; ; index++) {
		item = dynamic_cast<BStringItem*>(fSearchResults->ItemAt(index));
		if (item == NULL)
			break;
		
		if (item->OutlineLevel() == 0)
			fileItem = item;
		
		if (item->IsSelected())	{
			if (path != fileItem->Text()) {
				path = fileItem->Text();
				entry_ref ref;
				
				if (get_ref_for_path(path.String(), &ref) == B_OK)
					message.AddRef("refs", &ref);
			}
		}
	}
	
	entry_ref directory;
	fModel->fDirectory = directory;
		// invalidated on purpose
		
	fModel->fSelectedFiles.MakeEmpty();
	fModel->fSelectedFiles = message;
	
	PostMessage(MSG_START_CANCEL);
	
	SetWindowTitle();
}


void GrepWindow::OnCopyText()
{
	bool onlyCopySelection = true;
	
	if (fSearchResults->CurrentSelection() < 0)
		onlyCopySelection = false;
	
	BStringItem *item = NULL;
	BString buffer;

	for (int32 index = 0; ; index++) {
		item = dynamic_cast<BStringItem*>(fSearchResults->ItemAt(index));
		if (item == NULL)
			break;
		
		if (onlyCopySelection) {
			if (item->IsSelected())
				buffer << item->Text() << "\n";
		} else
			buffer << item->Text() << "\n";
	}
	
	status_t status = B_OK;
	
	BMessage *clip = NULL;
	
	if (be_clipboard->Lock()) {
		be_clipboard->Clear();
		
		clip = be_clipboard->Data();

		clip->AddData("text/plain", B_MIME_TYPE, buffer.String(), buffer.Length());			
		
		status = be_clipboard->Commit();
		
		if (status != B_OK) {
			be_clipboard->Unlock();
			return;
		}
	
		be_clipboard->Unlock();
	} else
		return;
}


void GrepWindow::OnSelectInTracker()
{
	if (fSearchResults->CurrentSelection() < 0) {
		BAlert *alert = new BAlert(NULL,
			TranslZeta("Please select the files you wish to have selected for you in Tracker."),
			TranslZeta("Okay"), NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		alert->Go(NULL);
		return;
	}

	BStringItem *item = NULL;
	BStringItem *fileItem = NULL;
	
	BMessage message;
	BString filePath;
	BPath folderPath;
	BList folderList;
	BString lastFolderAddedToList;

	for (int32 index = 0; ; index++) {
		item = dynamic_cast<BStringItem*>(fSearchResults->ItemAt(index));
		if (item == NULL)
			break;
		
		if (item->OutlineLevel() == 0)
			fileItem = item;
		
		if (item->IsSelected()) {
			if (filePath != fileItem->Text()) {
				filePath = fileItem->Text();
				entry_ref file_ref;
				
				if (get_ref_for_path(filePath.String(), &file_ref) == B_OK) {
					message.AddRef("refs", &file_ref);
					
					// add parent folder to list of folders to open
					folderPath.SetTo(filePath.String());
					if (folderPath.GetParent(&folderPath) == B_OK) {
						BPath *path = new BPath(folderPath);
						if (path->Path() != lastFolderAddedToList) {
							// catches some duplicates
							folderList.AddItem(static_cast<void*>(path));
							lastFolderAddedToList = path->Path();
								
						}
					}
				}
			}
		}
	}
	
	RemoveFolderListDuplicates(&folderList);
	OpenFoldersInTracker(&folderList);
	
	int32 aShortWhile = 100000;
	snooze(aShortWhile);
	
	if (!AreAllFoldersOpenInTracker(&folderList)) {
		for (int32 x = 0; x < 5; x++) {
			aShortWhile += 100000;
			snooze(aShortWhile);
			OpenFoldersInTracker(&folderList);
		}
	}	
	
	if (!AreAllFoldersOpenInTracker(&folderList)) {
		BAlert *alert = new BAlert(NULL,
			TranslZeta("Tracker Grep couldn't open one or more folders, and it's very sorry about it."),
			TranslZeta("Forgive and forget!"), NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go(NULL);
		return;
	}
	
	SelectFilesInTracker(&folderList, &message);
	
	// delete folderList contents
	int32 folderCount = folderList.CountItems();
	BPath *path = NULL;
	
	for (int32 x = 0; x < folderCount; x++) {
		path = static_cast<BPath*>(folderList.ItemAt(x));
		delete path;
	}
	folderList.MakeEmpty();
}


status_t GrepWindow::OpenFoldersInTracker(BList *folderList)
{
	status_t status = B_OK;
	BMessage refsMsg(B_REFS_RECEIVED);
	
	int32 folderCount = folderList->CountItems();
	
	for (int32 index = 0; index < folderCount; index++) {
		BPath *path = static_cast<BPath*>(folderList->ItemAt(index));
		
		entry_ref folderRef;
		status = get_ref_for_path(path->Path(), &folderRef);
		if (status != B_OK)
			return status;

		status = refsMsg.AddRef("refs", &folderRef);
		if (status != B_OK)
			return status;
	} 

	status = be_roster->Launch(TRACKER_SIGNATURE, &refsMsg);
	if (status != B_OK && status != B_ALREADY_RUNNING)
		return status;

	return B_OK;
}


void GrepWindow::OnQuitNow()
{
	if (be_app->Lock()) {
		be_app->PostMessage(B_QUIT_REQUESTED);
		be_app->Unlock();
	}
}


void GrepWindow::OnAboutRequested()
{
	app_info appInfo;
	version_info vInfo;
	BFile appFile;
	BAppFileInfo appFileInfo;
	BString fAppVersion;
	
	if (be_app->Lock()) {
		be_app->GetAppInfo(&appInfo); 
		appFile.SetTo(&appInfo.ref, B_READ_ONLY);
		appFileInfo.SetTo(&appFile);
		if (appFileInfo.GetVersionInfo(&vInfo, B_APP_VERSION_KIND) == B_OK)
			fAppVersion = BString("") << vInfo.major << '.' << vInfo.middle;
		be_app->Unlock();
	}

	BString text 
		= BString("Tracker Grep ") << fAppVersion
			<< TranslZeta("\nHow to use grep from Tracker.\n\n"
				"Created by Matthijs Hollemans\n"
				"mahlzeit@users.sf.net\n\n"
				"Maintained by Jonas Sundström\n"
				"jonas@kirilla.com\n\n"
				"Thanks to Peter Hinely, Serge Fantino, "
				"Hideki Naito, Oscar Lesta, Oliver Tappe, "
				"Luc Schrijvers and momoziro.\n");

	(new BAlert(NULL, text.String(), TranslZeta("Okay"), NULL, NULL,
		B_WIDTH_AS_USUAL, B_INFO_ALERT))->Go(NULL);
}


void GrepWindow::OnFileDrop(BMessage *message)
{
	if (fModel->fState == STATE_IDLE) {
		entry_ref directory;
		InitRefsReceived(&directory, message);

		fModel->fDirectory = directory;
		fModel->fSelectedFiles.MakeEmpty();
		fModel->fSelectedFiles = *message;
		
		fSearchResults->MakeEmpty();
		
		SetWindowTitle();
	}
}


void GrepWindow::TileIfMultipleWindows()
{
	if (be_app->Lock()) {
		int32 windowCount = be_app->CountWindows();
		be_app->Unlock();
		
		if (windowCount > 1)
			MoveBy(20,20); 
	}
	
	BScreen screen(this);
	BRect screenFrame = screen.Frame();
	BRect windowFrame = Frame();
	
	if (windowFrame.left > screenFrame.right
		|| windowFrame.top > screenFrame.bottom
		|| windowFrame.right < screenFrame.left
		|| windowFrame.bottom < screenFrame.top)
		MoveTo(50,50);
}


void GrepWindow::OnOpenPanel()
{
	if (fFilePanel == NULL) {
		entry_ref path;
		if (get_ref_for_path(fModel->fFilePanelPath.String(), &path) == B_OK) {
			fFilePanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(NULL, this),
				&path, B_FILE_NODE|B_DIRECTORY_NODE|B_SYMLINK_NODE, 
				true, new BMessage(MSG_REFS_RECEIVED), NULL, true, true);
		
			fFilePanel->Show();
		}
	}
}


void GrepWindow::OnRefsReceived(BMessage *message)
{
	OnFileDrop(message);
	// It seems a B_CANCEL always follows a B_REFS_RECEIVED
	// from a BFilePanel in Open mode.
	//
	// OnOpenPanelCancel() is called on B_CANCEL.
	// That's where saving the current dir of the file panel occurs, for now,
	// and also the neccesary deletion of the file panel object.
	// A hidden file panel would otherwise jam the shutdown process.
}


void GrepWindow::OnOpenPanelCancel()
{
	entry_ref panelDirRef;
	fFilePanel->GetPanelDirectory(&panelDirRef);
	BPath path(&panelDirRef);
	fModel->fFilePanelPath = path.Path();
	delete fFilePanel;
	fFilePanel = NULL;
}


void GrepWindow::OnSelectAll(BMessage *message)
{
	BMessenger *messenger = new BMessenger(fSearchResults);
	messenger->SendMessage(new BMessage(B_SELECT_ALL));
}


void GrepWindow::RemoveFolderListDuplicates(BList *folderList)
{
	if (folderList == NULL)
		return;
	
	int32 folderCount = folderList->CountItems();
	BString folderX;
	BString folderY;
	
	for (int32 x = 0; x < folderCount; x++) {
		
		BPath *path = static_cast<BPath*>(folderList->ItemAt(x));
		folderX = path->Path();

		for (int32 y = x+1; y < folderCount; y++) {
		
			path = static_cast<BPath*>(folderList->ItemAt(y));
			folderY = path->Path();
			
			if (folderX == folderY) {
				path = static_cast<BPath*>(folderList->RemoveItem(y));
				delete path;
				folderCount--;
				y--;
			}
		}
	} 
}


bool GrepWindow::AreAllFoldersOpenInTracker(BList *folderList)
{
	// Compare the folders we want open in Tracker to 
	// the actual Tracker windows currently open.

	// We build a list of open Tracker windows, and compare
	// it to the list of folders we want open in Tracker.

	// If all folders exists in the list of Tracker windows
	// return true

	status_t status = B_OK;
	BMessenger trackerMessenger(TRACKER_SIGNATURE);
	BMessage sendMessage;
	BMessage replyMessage;
	BList windowList;
	
	if (!trackerMessenger.IsValid())
		return false; 
	
	for (int32 count = 1; ; count++) {
		sendMessage.MakeEmpty();
		replyMessage.MakeEmpty();
		
		sendMessage.what = B_GET_PROPERTY;
		sendMessage.AddSpecifier("Path");
		sendMessage.AddSpecifier("Poses");
		sendMessage.AddSpecifier("Window", count);

		status = trackerMessenger.SendMessage(&sendMessage, &replyMessage);

		if (status != B_OK)
			return false;
	
		entry_ref *tracker_ref = new entry_ref;
		status = replyMessage.FindRef("result", tracker_ref);
		
		if (status == B_OK)
			windowList.AddItem(static_cast<void*>(tracker_ref));
		
		if (status != B_OK)
			break;
	}
	
	int32 folderCount = folderList->CountItems();
	int32 windowCount = windowList.CountItems();
	
	int32 found = 0;
	BPath *folderPath;
	entry_ref *windowRef;
	BString folderString;
	BString windowString;
	
	if (folderCount > windowCount) 
		// at least one folder is not open in Tracker
		return false;
	
	// Loop over the two lists and see if all folders exist as window
	
	for (int32 x = 0; x < folderCount; x++) {
		for (int32 y = 0; y < windowCount; y++) {
			
			folderPath = static_cast<BPath*>(folderList->ItemAt(x));
			windowRef = static_cast<entry_ref*>(windowList.ItemAt(y));
			
			if (folderPath == NULL)
				break;
			
			if (windowRef == NULL)
				break;
			
			folderString = folderPath->Path();
			
			BEntry entry;
			BPath path;

			if (entry.SetTo(windowRef) == B_OK && path.SetTo(&entry) == B_OK) {

				windowString = path.Path();

				if (folderString == windowString) {
					found++;
					break;
				}
			}
		}
	}
	
	// delete list of window entry_refs
	for (int32 x = 0; x < windowCount; x++) {
		windowRef = static_cast<entry_ref*>(windowList.ItemAt(x));
		delete windowRef;
	}
	windowList.MakeEmpty();
	
	if (found == folderCount)
		return true;

	return false;
}


status_t GrepWindow::SelectFilesInTracker(BList *folderList, BMessage *refsMessage)
{
	// loops over Tracker windows, find each windowRef,
	// extract the refs that are children of windowRef,
	// add refs to selection-message

	status_t status = B_OK;
	BMessenger trackerMessenger(TRACKER_SIGNATURE);
	BMessage windowSendMessage;
	BMessage windowReplyMessage;
	BMessage selectionSendMessage;
	BMessage selectionReplyMessage;
	
	if (!trackerMessenger.IsValid())
		return status; 
	
	// loop over Tracker windows
	for (int32 windowCount = 1; ; windowCount++) {
	
		windowSendMessage.MakeEmpty();
		windowReplyMessage.MakeEmpty();
		
		windowSendMessage.what = B_GET_PROPERTY;
		windowSendMessage.AddSpecifier("Path");
		windowSendMessage.AddSpecifier("Poses");
		windowSendMessage.AddSpecifier("Window", windowCount);

		status = trackerMessenger.SendMessage(&windowSendMessage, &windowReplyMessage);

		if (status != B_OK)
			return status;
	
		entry_ref *windowRef = new entry_ref;
		status = windowReplyMessage.FindRef("result", windowRef);
		
		if (status != B_OK)
			break;
		
		int32 folderCount = folderList->CountItems();

		// loop over folders in folderList
		for (int32 x = 0; x < folderCount; x++) {
			
			BPath *folderPath = static_cast<BPath*>(folderList->ItemAt(x));
				
			if (folderPath == NULL)
				break;
			
			BString folderString = folderPath->Path();
					
			BEntry windowEntry;
			BPath windowPath;
			BString windowString;
			
			status = windowEntry.SetTo(windowRef);
			if (status != B_OK)
				break;
			
			status = windowPath.SetTo(&windowEntry);
			if (status != B_OK)
				break;
			
			windowString = windowPath.Path();
			
			// if match, loop over items in refsMessage
			// and add those that live in window/folder
			// to a selection message
			
			if (windowString == folderString) {
				
				selectionSendMessage.MakeEmpty();
				selectionSendMessage.what = B_SET_PROPERTY;
				selectionReplyMessage.MakeEmpty();
				
				// loop over refs and add to message
				entry_ref ref;
				for (int32 index = 0; ; index++) {
				
					status = refsMessage->FindRef("refs", index, &ref);
					if (status != B_OK)
						break;
				
					BDirectory directory(windowRef);
					BEntry entry(&ref);
					if (directory.Contains(&entry))
						selectionSendMessage.AddRef("data", &ref);
				}

				// finish selection message
				selectionSendMessage.AddSpecifier("Selection");
				selectionSendMessage.AddSpecifier("Poses");	
				selectionSendMessage.AddSpecifier("Window", windowCount);				
				
				trackerMessenger.SendMessage(&selectionSendMessage, &selectionReplyMessage);	
			}
		}
	}

	return B_OK;
}


void GrepWindow::OnNewWindow()
{
	BMessage cloneRefs;
		// we don't want GrepWindow::InitRefsReceived()
		// to mess with the refs of the current window

	cloneRefs = fModel->fSelectedFiles;
	cloneRefs.AddRef("dir_ref", &(fModel->fDirectory));

	new GrepWindow(& cloneRefs);	
}
