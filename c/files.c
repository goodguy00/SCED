/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/files.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: files.c,v $
 * Revision 1.0  1997/05/06 20:30:22  schenney
 * Initial revision
 *
 */
#define PATCHLEVEL 0
/*
**    ScEd: A Constraint Based Scene Editor.
**    Copyright (C) 1994-1998  Stephen Chenney (schenney@cs.berkeley.edu)
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
**    This program is distributed in the hope that it will be useful,
**    but WITHOUT ANY WARRANTY; without even the implied warranty of
**    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**    GNU General Public License for more details.
**
**    You should have received a copy of the GNU General Public License
**    along with this program; if not, write to the Free Software
**    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
**  Sced: A Constraint Based Object Scene Editor
**
**  files.c : file selection stuff.
*/

#include <sced.h>
#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Viewport.h>

#if HAVE_DIRENT_H
#include <dirent.h>
#else
#define dirent direct
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

#if HAVE_STRING_H
#include <string.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif


#if ( ! defined(MAXPATHLEN) )
#define MAXPATHLEN 1024
#endif

#define FILE_CONT 0
#define FILE_DONE 1
#define FILE_CANCEL 2

static Boolean  File_Get_Name(char*, char*);

static Widget   file_shell = NULL;
static Widget   list_widget;
static Widget   name_widget;
static Widget   label_widget;
static char     name_buf[MAXPATHLEN];
static char     working_dir[MAXPATHLEN];
static int      finished;
static String   *file_list = NULL;
static int      num_files = 0;
static int      max_num_files = 0;
static int      old_length = 0;


/* Open a file and return the file pointer. NULL will be returned
** if the file could not be opened, and an error message popped up.
*/
FILE*
File_Open(char *filename, char *perms)
{
    FILE    *res = fopen(filename, perms);

    if ( ! res )
#if HAVE_STRERROR
        Popup_Error(strerror(errno), main_window.shell, "Error");
#else 
        Popup_Error(StrError(errno), main_window.shell, "Error");
#endif  

    return res;
}


/* Select a filename and open it. New_name is filled with a pointer to
** the name of the opened file. It is newly allocated memory.
*/
FILE*
File_Select_Open(char *default_name, char *label, char *perms, char **new_name)
{
    if ( File_Get_Name(default_name, label) )
    {
        *new_name = Strdup(name_buf);
        return File_Open(*new_name, perms);
    }

    return NULL;
}


/* Select a filename and return the new name.
*/
char*
File_Select(char *default_name, char *label)
{
    if ( File_Get_Name(default_name, label) )
        return Strdup(name_buf);

    return NULL;
}


static void
File_Clean_Filename(char *filename)
{
    int i, j;
    int length;

    /* Remove double slashes. */
    length = strlen(filename);
    for ( i = 0, j = 0 ; i < length ; i++ )
    {
        if ( filename[i] != '/' || filename[i + 1] != '/' )
            filename[j++] = filename[i];
    }
    filename[j] = '\0';

    /* Remove .'s by themselves. */
    length = strlen(filename);
    i = 0;
    j = 0;
    while ( i < length )
    {
        if ( ! strncmp(filename + i, "/./", 3) ||
             ! strcmp(filename + i, "/.") )
            i += 2;
        else
            filename[j++] = filename[i++];
    }
    filename[j] = '\0';

    /* Remove ..'s. */
    length = strlen(filename);
    i = 0;
    j = 0;
    while ( i < length )
    {
        if ( ! strncmp(filename + i, "/../", 4) ||
             ! strcmp(filename + i, "/..") )
        {
            for ( j-- ; j >= 0 && filename[j] != '/' ; j-- );
            if ( j < 0 )
                filename[j++] = '/';
            i += 3;
        }
        else
            filename[j++] = filename[i++];
    }
    filename[j] = '\0';

}


static int
File_Compare(const void *a, const void *b) 
{
    return strcmp(*(char**)a, *(char**)b);
}


static Boolean
File_Set_List(char *dir_name)
{
    DIR             *this_dir;
    struct dirent   *entry;
    struct stat     stat_buf;
    char            filename[MAXPATHLEN];
    int             i;

    for ( i = 0 ; i < num_files ; i++ )
        free(file_list[i]);

    if ( ! ( this_dir = opendir(dir_name) ) )
    {
        XBell(XtDisplay(main_window.shell), 0);
#if HAVE_STRERROR
        Popup_Error(strerror(errno), main_window.shell, "Error");
#else 
        Popup_Error(StrError(errno), main_window.shell, "Error");
#endif  
        return FALSE;
    }

    num_files = 0;
    while ( ( entry = readdir(this_dir) ) )
    {
        strcpy(filename, dir_name);
        strcat(filename, "/");
        strcat(filename, entry->d_name);
        File_Clean_Filename(filename);

        if ( stat(filename, &stat_buf) )
            continue;

        if ( num_files >= max_num_files )
        {
            max_num_files += 10;
            file_list = More(file_list, String, max_num_files);
        }

        if ( stat_buf.st_mode & S_IFDIR )
        {
            file_list[num_files] = New(char, strlen(entry->d_name) + 4);
            strcpy(file_list[num_files], entry->d_name);
            strcat(file_list[num_files], "/");
            num_files++;
        }
        else
            file_list[num_files++] = Strdup(entry->d_name);
    }

    free(this_dir);

    qsort((void*)(file_list + 2), num_files - 2, sizeof(String), File_Compare);

    XawListChange(list_widget, file_list, num_files, 0, TRUE);

    return TRUE;
}


static void
File_Done_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    struct stat     stat_buf;

    /* If it's a directory, refuse to return. */
    if ( ! stat(name_buf, &stat_buf) && stat_buf.st_mode & S_IFDIR )
    {
        XBell(XtDisplay(main_window.shell), 0);
        return;
    }

    XtPopdown(file_shell);
    old_length = strlen(name_buf);
    finished = FILE_DONE;
}


void
File_Action_Function(Widget w, XEvent *e, String *s, Cardinal *n)
{
    File_Done_Callback(w, NULL, NULL);
}


static void
File_Cancel_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(file_shell);
    old_length = strlen(name_buf);
    finished = FILE_CANCEL;
}


static void
File_Reset_Text()
{
    XawTextBlock    text_block;

    text_block.firstPos = 0;
    text_block.format = FMT8BIT;
    text_block.length = strlen(name_buf);
    text_block.ptr = name_buf;
    XawTextReplace(name_widget, 0, old_length, &text_block);
    old_length = strlen(name_buf);
}


static void
File_List_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    String  entry_name = ((XawListReturnStruct*)ca)->string;

    strcpy(name_buf, working_dir);
    strcat(name_buf, "/");
    strcat(name_buf, entry_name);
    File_Clean_Filename(name_buf);
    if ( entry_name[strlen(entry_name) - 1] == '/' )
    {
        /* Directory. */
        if ( File_Set_List(name_buf) )
            strcpy(working_dir, name_buf);
    }

    /* Change the text string. */
    File_Reset_Text();
}


static void
File_Create_Dialog()
{
    Widget      form;
    Widget      viewport;
    Widget      done, cancel;
    String      geometry;
    unsigned    shell_width, shell_height;
    int         gap;
    int         junk;
    Arg         args[15];
    int         n, m;
    String      name_trans = ":<Key>Return: File_Action()";

    n = 0;
    XtSetArg(args[n], XtNtitle, "File Select");     n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    file_shell = XtCreatePopupShell("fileShell", transientShellWidgetClass,
                                    main_window.shell, args, n);

    /* Get geometry. */
    XtVaGetValues(file_shell, XtNgeometry, &geometry, NULL);
    XParseGeometry(geometry, &junk, &junk, &shell_width, &shell_height);

    /* Create the form. */
    n = 0;
    form =
        XtCreateManagedWidget("fileForm", formWidgetClass, file_shell, args, n);

    /* Get its spacing. */
    XtVaGetValues(form, XtNdefaultDistance, &gap, NULL);

    m = 0;
    XtSetArg(args[m], XtNresizable, TRUE);      m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);    m++;
    XtSetArg(args[m], XtNright, XtChainRight);  m++;
    XtSetArg(args[m], XtNtop, XtChainTop);      m++;
    XtSetArg(args[m], XtNbottom, XtChainTop);   m++;
    XtSetArg(args[m], XtNwidth, shell_width - 2 * gap - 2); m++;

    /* Create all the other stuff. */
    n = m;
    XtSetArg(args[n], XtNborderWidth, 0);   n++;
    label_widget = XtCreateManagedWidget("fileLabel", labelWidgetClass,
                                         form, args, n);

    n = m;
    XtSetArg(args[n], XtNfromVert, label_widget);       n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);        n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);       n++;
    XtSetArg(args[n], XtNstring, name_buf);             n++;
    XtSetArg(args[n], XtNlength, MAXPATHLEN);           n++;
    XtSetArg(args[n], XtNresize, XawtextResizeWidth);   n++;
    name_widget = XtCreateManagedWidget("fileText", asciiTextWidgetClass,
                                        form, args, n);
    XtOverrideTranslations(name_widget, XtParseTranslationTable(name_trans));

    XtSetArg(args[0], XtNresizable, FALSE);
    XtSetArg(args[m - 2], XtNbottom, XtChainBottom);

    n = m;
    XtSetArg(args[n], XtNfromVert, name_widget);    n++;
    XtSetArg(args[n], XtNallowVert, TRUE);          n++;
    viewport = XtCreateManagedWidget("fileViewport", viewportWidgetClass,
                                     form, args, n);

    n = 0;
    XtSetArg(args[n], XtNdefaultColumns, 1);    n++;
    XtSetArg(args[n], XtNforceColumns, TRUE);   n++;
    XtSetArg(args[n], XtNverticalList, TRUE);   n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);    n++;
    XtSetArg(args[n], XtNright, XtChainRight);  n++;
    XtSetArg(args[n], XtNtop, XtChainTop);      n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);n++;
    XtSetArg(args[n], XtNwidth, shell_width - 2 * gap - 2); n++;
    list_widget = XtCreateManagedWidget("fileList", listWidgetClass, viewport,
                                        args, n);
    XtAddCallback(list_widget, XtNcallback, File_List_Callback, NULL);

    n = 0;
    XtSetArg(args[n], XtNresizable, TRUE);          n++;
    XtSetArg(args[n], XtNtop, XtChainBottom);       n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);    n++;
    XtSetArg(args[n], XtNright, XtChainLeft);       n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);        n++;
    XtSetArg(args[n], XtNfromVert, viewport);       n++;
    m = n;
    XtSetArg(args[n], XtNlabel, "Done");            n++;
    done = XtCreateManagedWidget("fileDone", commandWidgetClass, form, args, n);
    XtAddCallback(done, XtNcallback, File_Done_Callback, NULL);

    n = m;
    XtSetArg(args[n], XtNfromHoriz, done);      n++;
    XtSetArg(args[n], XtNlabel, "Cancel");      n++;
    cancel = XtCreateManagedWidget("fileCancel", commandWidgetClass, form,
                                   args, n);
    XtAddCallback(cancel, XtNcallback, File_Cancel_Callback, NULL);

    XtRealizeWidget(file_shell);
}


static void
File_Set_Name_Buf(char *def_name)
{
    if ( def_name[0] == '/' )
    {
        strncpy(name_buf, def_name, 1024);
        strcpy(working_dir, name_buf);
        strrchr(working_dir, '/')[0] = '\0';
    }
    else
    {
#if HAVE_GETCWD
        getcwd(name_buf, MAXPATHLEN);
#else
        getwd(name_buf);
#endif
        strcat(name_buf, "/");
        if ( def_name && def_name[0] != '\0' )
            strcat(name_buf, def_name);
    }

    File_Clean_Filename(name_buf);
}


static Boolean
File_Get_Name(char *default_name, char *label)
{
    XtAppContext    context;
    XEvent          event;

    if ( ! file_shell )
    {
        File_Create_Dialog();
#if HAVE_GETCWD
        getcwd(working_dir, MAXPATHLEN);
#else
        getwd(working_dir);
#endif
    }

    /* Sort out names and labels and stuff. */
    XtVaSetValues(label_widget, XtNlabel, label, NULL);
    File_Set_Name_Buf(default_name);
    File_Reset_Text();
    XawTextSetInsertionPoint(name_widget, strlen(name_buf) + 1);

    /* Reset the directory information. */
    File_Set_List(working_dir);

    /* Position and pop up the shell. */
    Position_Shell(file_shell, FALSE);
    XtPopup(file_shell, XtGrabExclusive);

    /* Loop until done. */
    finished = FILE_CONT;
    context = XtWidgetToApplicationContext(main_window.shell);
    while ( finished == FILE_CONT )
    {
        XtAppNextEvent(context, &event);
        XtDispatchEvent(&event);
    }

    return ( finished == FILE_DONE );
}


