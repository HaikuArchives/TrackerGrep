TrackerGrep Release History
=====================================

*Version 5.1 (19 June 2007)*

 * Zeta localization. Dutch translation included.
 * Japanese encoding support added.

*Version 5.0 (21 March 2007)*

 * You can select multiple items in the list view.
 * You can trim away non-selected search hits, to expedite further searching.
 * You can have Tracker Grep show your selected search hits in Tracker.
 * It's now possible to copy a selection of search hits as text.
 * A menu bar has been added.
 * Lots of keyboard shortcuts added.
 * Tracker Grep is now a stand-alone application as well as a Tracker add-on.
 * Running Tracker Grep as an application will open an empty, untargetted window.
 * Windows can be retargetted to search some other folder or set of files.
 * Windows additionally accept file drops, to set search target.
 * Multiple windows can be open at the same time, and tile as necessary.
 * Invoking Tracker Grep as Tracker add-on will open a new window.
 * The application will accept search targets as command-line arguments, when invoked from Terminal. Odd case. Mostly good for testing.
 * Two settings files reduced to a single one. Your history is now history.

*Version 4.3 (17 January 2006)*

 * Corrected version in window title and About-dialog.
 * If Tracker Grep is being invoked with exactly one directory selected (like you do if you right-click a folder on the desktop), it will recurse into that directory even if the 'look in sub-directories'-setting is disabled. This has been suggested by Oscar Lesta.
 * Applied Oscar Lesta's patch that causes Tracker Grep to check if opening a file in Pe actually succeeded and if that isn't the case, opens the file with its default app instead. Thanks, Oscar!
 * Applied Oscar Lesta's patch which changes the behaviour of Tracker Grep for the case that you have checked 'Open with Pe': If you click on the line containing the file name (the superitem), Tracker Grep will invoke the default application (aka: view the file), the file will only be opened in Pe if you click on one of the result lines (aka: edit the file). Thanks again, Oscar!
 * Before invoking grep, Tracker Grep now makes sure to escape dollars and backslashes contained in the filename, too (not only quotes).

*Version 4.2 (15 January 2006)*

 * Tracker Grep now has a new maintainer, as Matthijs is not using BeOS anymore. If you have any questions and/or patches, please contact Oliver Tappe.
 * In case you are asking Tracker Grep to recurse into sub-directories, you can now request it to ignore any directories that start with a dot. This can be quite useful if you apply Tracker Grep on a svn repository (as svn keeps its own files in directories named '.svn').

*Version 4.1 (24 February 2004)*

 * Thanks to Oscar Lesta (BiPolar), Tracker Grep now has an "Open files in Pe" option. If this is enabled, double-clicking on a line from the search results will open the file in Pe, at the correct line number. Thanks Oscar, for this handy feature!

*Version 4.0a (9 July 2003)*

 * The meaning of the "Case sensitive" option was reversed. Whoops.

*Version 4.0 (13 February 2003)*

 * Cleaned up the source code.
 * The search history now removes duplicate items.
 * Renamed the "Do not escape search text" option to "Escape search text".
 * Added "Text files only" option. If you deselect it, Tracker Grep will also look into binary files.
 * The window title is now prefixed by "Tracker Grep", which makes it easier to find open Tracker Grep windows from the Deskbar.

*Version 3.2 (25 November 2000)*

 * Now the Tracker Grep window is named after what directory the user is searching in.
 * Reversed the order of the items in the history pop-up menu. It now shows the things you last searched for at the top, since that makes more sense.

*Version 3.1 (11 October 2000)*

 * When replacing non-printable characters, Tracker Grep was a little bit too enthousiastic and wiped out characters from non-Latin languages (such as Japanese) as well. Thanks to Hideki Naito for finding - and fixing - this bug.

*Version 3.0 (24 September 2000)*

 * Cleaned up the source code.
 * There is no more PowerPC version.
 * The "Show contents" check box is not disabled any more during a search, so you can toggle it on-the-fly.
 * Moved the other options into a separate "Options" menu.
 * Added an option for case sensitive searches.
 * By default, Tracker Grep now escapes special characters in the search text before it is sent to grep. For advanced users of grep, there is a "Do not escape search text" option.
 * Tracker Grep now keeps a history of the last twenty-or-so search patterns.
 * Non-printable characters (such as linefeeds) are removed from the search results.
 * Tracker Grep no longer crashes when grep is given an invalid search pattern, taking the Tracker with it.

*Version 2.2.0 (23 January 1999)*

 * Changes by Serge Fantino include:
   * Now you can run multiple copies of Tracker Grep at the same time without crashing the machine :-)
   * Only the files whose contents match the search pattern are displayed.
   * The results are now displayed in an outline list.
   * Double-clicking the name of a matching file opens that file.
 * Added the "Show Contents" option.
 * The window shows the name of the file that is being searched.
 * Tracker Grep remembers its most recent settings.

*Version 2.1.1 (12 January 1999)*

 * Ported to BeOS Intel R4. The PowerPC version is unchanged.

*Version 2.1.0 (24 August 1998)*

 * Changes by Peter Hinely include:
   * Ported to ppc.
   * Changed grep options to "grep -hin".
   * Improved display of matches.

*Version 2.0.0 (31 July 1998)*

 * This is a total rewrite (hence the major version number bump).
 * The user interface has been improved (and is more responsive).
 * Added a button to start and cancel the search.
 * The results display now uses a fixed-width font.
 * Added an option for subdirectory crawling.
 * Added an option for traversing symbolic links.

*Version 1.1.0 (Not released)*

 * Tracker Grep now looks at all the files in the current directory if nothing is selected.
 * Files may also have MIME supertype "message".

*Version 1.0.0 (4 July 1998)*

 * First version.
