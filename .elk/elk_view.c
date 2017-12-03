/*
 * $Header$
 *
 * $Log$
 */

/*
 *    ScEd: A Constraint Based Scene Editor.
 *    Copyright (C) 1994-1998  Stephen Chenney (schenney@cs.berkeley.edu)
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *	Sced: A Constraint Based Object Scene Editor
 *
 *      elk_view.c: Code for the elk interaction window.
 */

#include <ctype.h>
#include <sced.h>
#include "elk_private.h"
#include <X11/cursorfont.h>
#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>

Widget	elk_shell = NULL;
static Widget	text_widget;
static String	translations = "Shift <Key>Return: elk_eval_action()";
static Cursor   time_cursor = 0;

int		elk_active = ELK_MAIN_WINDOW;
WindowInfoPtr   elk_window; /* The current elk window. */

extern Pixmap	icon_bitmap;


/* Response function for a Delete message from the window manager.
*/
void
Elk_Window_Delete_Message()
{
    XtDestroyWidget(elk_shell);
    elk_shell = NULL;
}


static void
Elk_Close_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XUnmapWindow(XtDisplay(elk_shell), XtWindow(elk_shell));
}


void
Elk_Create_Window()
{
    Arg         args[15];
    int         n;
    Widget      form;
    Widget      label;
    Widget      close;
    Dimension   height;
    String      shell_geometry;
    unsigned    shell_width, shell_height;
    int         gap;
    int         junk;
    Atom    	delete_atom = XInternAtom(XtDisplay(main_window.shell),
                                      	  "WM_DELETE_WINDOW", FALSE);

    n = 0;
    XtSetArg(args[n], XtNtitle, "Elk Interpreter");	n++;
    XtSetArg(args[n], XtNiconPixmap, icon_bitmap);  	n++;
    elk_shell = XtCreatePopupShell("elkShell", topLevelShellWidgetClass,
				   main_window.shell, args, n);

    XtVaGetValues(elk_shell, XtNgeometry, &shell_geometry, NULL);
    XParseGeometry(shell_geometry, &junk, &junk, &shell_width, &shell_height);

    n = 0;
    XtSetArg(args[n], XtNwidth, &shell_width);    n++;
    XtSetArg(args[n], XtNheight, &shell_height);  n++;
    form = XtCreateManagedWidget("elkForm", formWidgetClass, elk_shell, args,n);

    XtVaGetValues(form, XtNdefaultDistance, &gap, NULL);

    n = 0;
    XtSetArg(args[n], XtNtop, XtChainTop);              n++;
    XtSetArg(args[n], XtNbottom, XtChainTop);           n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);            n++;
    XtSetArg(args[n], XtNright, XtChainLeft);           n++;
    XtSetArg(args[n], XtNresizable, TRUE);              n++;
    XtSetArg(args[n], XtNlabel, "Elk Interpreter:");    n++;
    XtSetArg(args[n], XtNborderWidth, 0);               n++;
    label = XtCreateManagedWidget("elkLabel", labelWidgetClass, form, args, n);

    XtVaGetValues(label, XtNheight, &height, NULL);

    n = 0;
    XtSetArg(args[n], XtNtop, XtChainTop);          			n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);    			n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);        			n++;
    XtSetArg(args[n], XtNright, XtChainRight);      			n++;
    XtSetArg(args[n], XtNresizable, TRUE);          			n++;
    XtSetArg(args[n], XtNwidth, (int)shell_height - 2 - 2  * (int)gap);	n++;
    XtSetArg(args[n], XtNheight,
             (int)shell_height - (int)height * 4 - 8 - 6 * (int)gap);   n++;
    XtSetArg(args[n], XtNstring, "");               			n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    			n++;
    XtSetArg(args[n], XtNscrollVertical, XawtextScrollWhenNeeded);	n++;
    XtSetArg(args[n], XtNscrollHorizontal, XawtextScrollWhenNeeded);	n++;
    XtSetArg(args[n], XtNfromVert, label);          			n++;
    text_widget = XtCreateManagedWidget("elkText", asciiTextWidgetClass,
					form, args, n);
    XtOverrideTranslations(text_widget,
                           XtParseTranslationTable(translations));

    n = 0;
    XtSetArg(args[n], XtNtop, XtChainBottom);       n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);    n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);        n++;
    XtSetArg(args[n], XtNright, XtChainLeft);       n++;
    XtSetArg(args[n], XtNresizable, TRUE);          n++;
    XtSetArg(args[n], XtNlabel, "Close");           n++;
    XtSetArg(args[n], XtNfromVert, text_widget);    n++;
    close = XtCreateManagedWidget("elkClose", commandWidgetClass, form, args,n);
    XtAddCallback(close, XtNcallback, Elk_Close_Callback, NULL);

    XtRealizeWidget(elk_shell);

    XSetWMProtocols(XtDisplay(elk_shell), XtWindow(elk_shell), &delete_atom, 1);
}


/* Function called to pop-up elk window. */
void
Elk_Window_Popup(Widget w, XtPointer cl, XtPointer ca)
{
    if ( ! elk_shell )
        Elk_Create_Window();

    XMapRaised(XtDisplay(elk_shell), XtWindow(elk_shell));
}


void
Elk_Echo_Output(char *text)
{
    int		    insert_pos;
    char 	    c;
    XawTextBlock    block;

    XtVaGetValues(text_widget, XtNinsertPosition, &insert_pos, NULL);

    /*
     * Now place the result of the scheme expression in the text widget.
     */
    /* Newline before the result. */
    c = '\n';
    block.firstPos = 0;
    block.length = 1;
    block.ptr = &c;
    XawTextReplace(text_widget, insert_pos, insert_pos, &block);
    /* The string. */
    insert_pos++;
    block.firstPos = 0;
    block.length = strlen(text);
    block.ptr = text;
    XawTextReplace(text_widget, insert_pos, insert_pos, &block);
    /* Another newline. */
    insert_pos += block.length;
    block.firstPos = 0;
    block.length = 1;
    block.ptr = &c;
    XawTextReplace(text_widget, insert_pos, insert_pos, &block);
    /* Reset the insert posn. */
    insert_pos++;
    XawTextSetInsertionPoint(text_widget, insert_pos);
}


/***********************************************************************
 *
 * Description: Elk_Eval_Action() is the callback for when the users presses
 *      Shift-Return in the scheme command window. This function
 *      scans backward from the current insertion point to either
 *      the beginning of the previous word or the beginning of
 *      the previous scheme expression (i.e. matching parenthesis).
 *      The found text is then pased to Elk_Eval() for evaluation
 *      by the scheme interpreter. NOTE: This function is used
 *      by both the csg_window and the main_window.
 *
 * Return value: None
 *
 ***********************************************************************/
void
Elk_Eval_Action(Widget w, XEvent *event, String *str, Cardinal *num)
{
    String s;
    int epos, spos, ipos, len;
    char c;
    char *p;
    
    /*
     * Here we look at the preceeding character (from the current
     * insertion point) and if it is a ')' then we look for its
     * match otherwise just go back as far as the previous word.
     */
    XtVaGetValues(w,
              XtNinsertPosition, &epos,
              XtNstring, &s,
              NULL);
    ipos = epos;
    if (epos == 0)
        return;
    epos--;
    while ((epos >= 0) && isspace(s[epos]))
        epos--;
    if (epos < 0)
        return;
    if (s[epos] == ')') {
        int count = 1;
        
        spos = epos;
        while((spos >= 0) && count) {
            if ((spos != epos) && (s[spos] == ')'))
                count++;
            else if (s[spos] == '(')
                count--;
            spos--;
        }
    } else {
        spos = epos;
        while((spos >= 0) && !isspace(s[spos]))
            spos--;
    }
    spos++;

    /* Set the cursor. */
    if ( ! time_cursor )
        time_cursor = XCreateFontCursor(XtDisplay(elk_shell), XC_watch);
    XDefineCursor(XtDisplay(elk_shell), XtWindow(elk_shell), time_cursor);
    /* Flush the display. */
    XSync(XtDisplay(elk_shell), False);

    len = epos - spos + 1;
    c = s[spos + len];
    s[spos + len] = '\0';
    p = Elk_Eval(&s[spos]);
    s[spos + len] = c;

    Elk_Echo_Output(p);

    /* Put the cursor back. */
    XDefineCursor(XtDisplay(elk_shell), XtWindow(elk_shell), None);
    XSync(XtDisplay(elk_shell), False);


    return;
}


