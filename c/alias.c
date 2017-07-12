/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/alias.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: alias.c,v $
 * Revision 1.0  1997/05/06 20:30:22  schenney
 * Initial revision
 *
 */
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
**  alias.c: Functions to handle object aliasing.
*/


#include <sced.h>
#include <instance_list.h>
#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>

static void Alias_Create_Shell();
static void Alias_Set_Object_Alias(char*, ObjectInstancePtr);
static void Alias_Set_List_Aliases(char*, InstanceList);

WindowInfoPtr   alias_window;
Boolean         popup_done;

Widget  alias_dialog_shell = NULL;
Widget  alias_text;

void
Alias_Object_Alias_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtAppContext    context;
    XEvent          event;

    alias_window = (WindowInfoPtr)cl;

    if ( ! alias_window->selected_instances )
        return;

    if ( target_renderer == NoTarget )
    {
        Popup_Error("No Target Renderer specified.", alias_window->shell,
                    "Error");
        return;
    }

    if ( ! alias_dialog_shell )
        Alias_Create_Shell();

    popup_done = FALSE;

    if ( alias_window->selected_instances->the_instance->o_aliases &&
     alias_window->selected_instances->the_instance->o_aliases[target_renderer])
        XtVaSetValues(alias_text, XtNstring, (char*)alias_window->
            selected_instances->the_instance->o_aliases[target_renderer], NULL);
    else
        XtVaSetValues(alias_text, XtNstring, "", NULL);

    /* Set the position of the popup. */
    Position_Shell(alias_dialog_shell, FALSE);

    XtPopup(alias_dialog_shell, XtGrabExclusive);

    context = XtWidgetToApplicationContext(main_window.shell);
    while ( ! popup_done )
    {
        XtAppNextEvent(context, &event);
        XtDispatchEvent(&event);
    }
}


static void
Alias_Dialog_Done(Widget w, XtPointer cl, XtPointer ca)
{
    char    *alias_string;

    XtPopdown(alias_dialog_shell);

    XtVaGetValues(alias_text, XtNstring, &alias_string, NULL);

    Alias_Set_List_Aliases(alias_string, alias_window->selected_instances);

    popup_done = TRUE;
}


static void
Alias_Dialog_Cancel(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(alias_dialog_shell);
    popup_done = TRUE;
}


static void
Alias_Create_Shell()
{
    Widget  alias_form;
    Widget  alias_label;
    Widget  done_button, cancel_button;
    Arg     args[15];
    int     n, m;
    Dimension   height;
    String      shell_geometry;
    unsigned    shell_width, shell_height;
    int         gap;
    int         junk;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Alias");           n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    alias_dialog_shell = XtCreatePopupShell("aliasShell",
                        transientShellWidgetClass, main_window.shell, args, n);

    XtVaGetValues(alias_dialog_shell, XtNgeometry, &shell_geometry,NULL);
    XParseGeometry(shell_geometry, &junk, &junk, &shell_width, &shell_height);

    n = 0;
    alias_form = XtCreateManagedWidget("aliasForm", formWidgetClass,
                        alias_dialog_shell, args, n);
    XtVaGetValues(alias_form, XtNdefaultDistance, &gap, NULL);

    m = 0;
    XtSetArg(args[m], XtNtop, XtChainTop);      m++;
    XtSetArg(args[m], XtNbottom, XtChainTop);   m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);    m++;
    XtSetArg(args[m], XtNright, XtChainLeft);   m++;
    XtSetArg(args[m], XtNresizable, TRUE);      m++;

    n = m;
    XtSetArg(args[n], XtNlabel, "Alias:");  n++;
    XtSetArg(args[n], XtNborderWidth, 0);   n++;
    alias_label = XtCreateManagedWidget("aliasLabel", labelWidgetClass,
                    alias_form, args, n);
    XtVaGetValues(alias_label, XtNheight, &height, NULL);

    n = m;
    XtSetArg(args[n], XtNfromVert, alias_label);    n++;
    XtSetArg(args[n], XtNwidth, (int)shell_height - 2 - 2  * (int)gap); n++;
    XtSetArg(args[n], XtNheight,
             (int)shell_height - (int)height * 2 - 6 - 4 * (int)gap);   n++;
    XtSetArg(args[n], XtNstring, "");               n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    n++;
    XtSetArg(args[n], XtNresize, TRUE);             n++;
    XtSetArg(args[n], XtNscrollVertical, XawtextScrollWhenNeeded);  n++;
    alias_text = XtCreateManagedWidget("aliasText", asciiTextWidgetClass,
                    alias_form, args, n);

    n = m;
    XtSetArg(args[n], XtNlabel, "Done");        n++;
    XtSetArg(args[n], XtNfromVert, alias_text); n++;
    done_button = XtCreateManagedWidget("aliasDone", commandWidgetClass,
                    alias_form, args, n);
    XtAddCallback(done_button, XtNcallback, Alias_Dialog_Done, NULL);

    n = m;
    XtSetArg(args[n], XtNlabel, "Cancel");          n++;
    XtSetArg(args[n], XtNfromHoriz, done_button);   n++;
    XtSetArg(args[n], XtNfromVert, alias_text);     n++;
    cancel_button = XtCreateManagedWidget("aliasCancel", commandWidgetClass,
                    alias_form, args, n);
    XtAddCallback(cancel_button, XtNcallback, Alias_Dialog_Cancel, NULL);

    XtRealizeWidget(alias_dialog_shell);
}


static void
Alias_Set_Object_Alias(char *alias, ObjectInstancePtr obj)
{
    int i;

    if ( ! obj->o_aliases )
    {
        obj->o_aliases = (void**)New(void*, LastTarget);
        for ( i = NoTarget ; i < LastTarget ; i++ )
            obj->o_aliases[i] = NULL;
    }

    if ( obj->o_aliases[target_renderer] )
        free(obj->o_aliases[target_renderer]);

    if ( alias[0] == '\0' )
        obj->o_aliases[target_renderer] = NULL;
    else
        obj->o_aliases[target_renderer] = Strdup(alias);
}


static void
Alias_Set_List_Aliases(char *alias, InstanceList list)
{
    InstanceList    inst;

    for ( inst = list ; inst ; inst = inst->next )
        Alias_Set_Object_Alias(alias, inst->the_instance);
}
