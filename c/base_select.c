/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/base_select.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: base_select.c,v $
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
**  sced: A Constraint Based Object Scene Editor
**
**  base_select_box.c : Functions for displaying, modifying and using the
**                      various select base object dialogs.
*/

#include <sced.h>
#include <csg.h>
#include <aggregate.h>
#include <base_objects.h>
#include <SimpleWire.h>
#include <X11/Shell.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Viewport.h>

extern void Create_New_Object(Widget, XtPointer, XtPointer);

typedef struct _BaseSelect {
    int             state;
    Widget          *widgets;
    int             num_widgets;
    int             max_num_widgets;
    Widget          shell;
    char            *shell_name;
    Widget          form;
    XtCallbackProc  callback;
    } BaseSelect, *BaseSelectPtr;


static void CSG_Select_Callback(Widget, XtPointer, XtPointer);
static void Agg_Select_Callback(Widget, XtPointer, XtPointer);
static void Wire_Select_Callback(Widget, XtPointer, XtPointer);

static void Create_Base_Select_Shell(BaseSelectPtr);


static BaseSelect   csg_select =
        { 0, NULL, 0, 0, NULL, "csgSelectShell", NULL, CSG_Select_Callback };
static BaseSelect   wire_select =
        { 0, NULL, 0, 0, NULL, "wireSelectShell", NULL, Wire_Select_Callback };
static BaseSelect   agg_select =
        { 0, NULL, 0, 0, NULL, "aggSelectShell", NULL, Agg_Select_Callback };
static BaseSelectPtr    current_base = NULL;



void
Base_Select_Popup(GenericObject obj_type, int state)
{
    switch ( obj_type )
    {
        case csg_obj:
            current_base = &csg_select;
            break;
        case wireframe_obj:
            current_base = &wire_select;
            break;
        case aggregate_obj:
            current_base = &agg_select;
            break;
        default:;
    }

    current_base->state = state;

    if ( ! current_base->num_widgets ) return;

    Position_Shell(current_base->shell, FALSE);
    XtPopup(current_base->shell, XtGrabExclusive);
}

void
Base_Add_Select_Option(BaseObjectPtr base)
{
    Dimension   form_width, form_height;
    int         gap;
    Arg         args[10];
    int         n;

    switch ( base->b_class )
    {
        case csg_obj:
            current_base = &csg_select;
            break;
        case wireframe_obj:
            current_base = &wire_select;
            break;
        case aggregate_obj:
            current_base = &agg_select;
            break;
        default:;
    }

    if ( ! current_base->shell )
        Create_Base_Select_Shell(current_base);

    if ( current_base->num_widgets == current_base->max_num_widgets )
    {
        current_base->max_num_widgets += 5;
        current_base->widgets = More(current_base->widgets, Widget,
                                     current_base->max_num_widgets);
    }

    XtVaGetValues(current_base->form, XtNwidth, &form_width,
                                      XtNheight, &form_height,
                                      XtNdefaultDistance, &gap,
                                      NULL);

    n = 0;
    XtSetArg(args[n], XtNbasePtr, base);    n++;
    XtSetArg(args[n], XtNwidth, 118);       n++;
    XtSetArg(args[n], XtNheight, 118);      n++;
    current_base->widgets[current_base->num_widgets] =
        XtCreateManagedWidget("baseNewObject", simpleWireWidgetClass,
                              current_base->form, args, n);
    XtAddCallback(current_base->widgets[current_base->num_widgets], XtNcallback,
                  current_base->callback, (XtPointer)current_base);
    current_base->num_widgets++;

}


static void
CSG_Select_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(csg_select.shell);

    switch ( csg_select.state )
    {
        case csg_select_new:
            Create_New_Object(w, NULL, ca);
            break;
        case csg_select_edit:
            CSG_Destroy_Base_Object(w, NULL, FALSE);
            break;
        case csg_select_delete:
            CSG_Destroy_Base_Object(w, NULL, TRUE);
            break;
        case csg_select_copy:
            CSG_Copy_Base_Object(w, NULL);
            break;
        case csg_select_export:
            Base_Export(w, NULL);
            break;
        case csg_select_change:
            Base_Change_Select_Callback(w, NULL);
            break;
    }
}


static void
Wire_Select_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(wire_select.shell);

    switch ( wire_select.state )
    {
        case WIRE_CREATE:
            Create_New_Object(w, NULL, ca);
            break;
        case WIRE_DELETE:
            Wireframe_Destroy_Base_Object(w, NULL);
            break;
    }
}


static void
Agg_Select_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(agg_select.shell);

    switch ( agg_select.state )
    {
        case AGG_CREATE:
            Create_New_Object(w, NULL, ca);
            break;
        case AGG_DELETE:
            Agg_Destroy_Base_Object(w, NULL);
            break;
        case AGG_COPY:
            Agg_Copy_Base_Object(w, NULL);
            break;
        case AGG_EXPORT:
            Base_Export(w, NULL);
            break;
    }
}


static void
Base_Select_Cancel_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(current_base->shell);
}


void
Base_Select_Destroy_Widget(GenericObject obj_type, int index)
{
    int i;

    switch ( obj_type )
    {
        case csg_obj:
            current_base = &csg_select;
            break;
        case aggregate_obj:
            current_base = &agg_select;
            break;
        case wireframe_obj:
            current_base = &wire_select;
            break;
        default:;
    }

    /* Destroy it's widget. */
    XtDestroyWidget(current_base->widgets[index]);
    for ( i = index + 1 ; i < current_base->num_widgets ; i++ )
        current_base->widgets[i-1] = current_base->widgets[i];
    current_base->num_widgets--;
}


static void
Create_Base_Select_Shell(BaseSelectPtr selection)
{
    Widget  outer_form;
    Widget  cancel_button;
    Widget  viewport_widget;
    String  shell_geometry;
    unsigned    shell_width, shell_height;
    int     junk;
    int     gap;
    Arg     args[15];
    int     n;

    n = 0;
    XtSetArg(args[n], XtNtitle, "CSG Select");  n++;
    selection->shell = XtCreatePopupShell(selection->shell_name,
                        transientShellWidgetClass, main_window.shell, args, n);

    /* Get the parents size. */
    XtVaGetValues(selection->shell, XtNgeometry, &shell_geometry, NULL);
    XParseGeometry(shell_geometry, &junk, &junk, &shell_width, &shell_height);

    /* Create the first level form. */
    n = 0;
    XtSetArg(args[n], XtNwidth, (int)shell_width);      n++;
    XtSetArg(args[n], XtNheight, (int)shell_height);    n++;
    outer_form = XtCreateManagedWidget("baseSelectOuterForm", formWidgetClass,
                                        selection->shell, args, n);
    XtVaGetValues(outer_form, XtNdefaultDistance, &gap, NULL);

    /* Create the viewport. */
    n = 0;
    XtSetArg(args[n], XtNwidth, (int)(shell_width - 2 * gap));      n++;
    XtSetArg(args[n], XtNheight, (int)(shell_height - 35 - 3*gap)); n++;
    XtSetArg(args[n], XtNtop, XtChainTop);          n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);    n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);        n++;
    XtSetArg(args[n], XtNright, XtChainRight);      n++;
    XtSetArg(args[n], XtNallowHoriz, TRUE);         n++;
    XtSetArg(args[n], XtNallowVert, TRUE);          n++;
    XtSetArg(args[n], XtNforceBars, TRUE);          n++;
    XtSetArg(args[n], XtNuseBottom, TRUE);          n++;
    XtSetArg(args[n], XtNuseRight, TRUE);           n++;
    viewport_widget = XtCreateManagedWidget("baseSelectViewport",
                        viewportWidgetClass, outer_form, args, n);

    /* Create the box inside the viewport. */
    n = 0;
    XtSetArg(args[n], XtNwidth, (int)(shell_width - 2 * gap - 40));     n++;
    XtSetArg(args[n], XtNheight, (int)(shell_height - 35 - 3*gap - 40));n++;
    selection->form = XtCreateManagedWidget("baseSelectForm", boxWidgetClass,
                                            viewport_widget, args, n);

    /* The cancel button. */
    n = 0;
    XtSetArg(args[n], XtNlabel, "Cancel");          n++;
    XtSetArg(args[n], XtNfromVert, viewport_widget);    n++;
    XtSetArg(args[n], XtNleft, XtChainLeft);        n++;
    XtSetArg(args[n], XtNright, XtChainLeft);       n++;
    XtSetArg(args[n], XtNtop, XtChainBottom);       n++;
    XtSetArg(args[n], XtNbottom, XtChainBottom);    n++;
    XtSetArg(args[n], XtNresizable, TRUE);          n++;
    cancel_button = XtCreateManagedWidget("baseSelectCancel",
                                    commandWidgetClass, outer_form, args, n);
    XtAddCallback(cancel_button, XtNcallback, Base_Select_Cancel_Callback,NULL);

    XtRealizeWidget(selection->shell);
}
