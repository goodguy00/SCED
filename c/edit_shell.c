/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/edit_shell.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: edit_shell.c,v $
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
**  edit_shell.c : Functions controlling the edit shell, the one with all
**                  the constraints and stuff in it.
*/

#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <instance_list.h>
#include <select_point.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/Viewport.h>
#include <ConstraintBox.h>


static void Edit_Shell_Set_Constraints();
static void Edit_Create_Edit_Shell();

extern Pixmap   menu_bitmap;
extern Pixmap   icon_bitmap;
extern char     *top_level_translations;

#define NO_LABEL "No Drag in Progress"

Widget  edit_shell = NULL;
Widget  edit_form;
static Widget   suspend_button;
static Widget   buttons[5];

static EditInfoPtr  info;

Widget  feature_box[pt_last_feature];

static Cursor   modify_cursor = 0;


void
Edit_Initialize_Shell(EditInfoPtr new_info)
{
    static int  last_num = pt_last_feature;
    Vector  dummy_vect;
    int     i;

    info = new_info;

    if ( ! edit_shell )
    {
        Edit_Create_Edit_Shell();
        XtAugmentTranslations(edit_shell,
                              XtParseTranslationTable(top_level_translations));
    }

    Edit_Shell_Set_Constraints();
    if ( info->obj->o_num_features > scale_feature )
        Edit_Scale_Force_Constraints(info, FALSE);

    Edit_Set_Drag_Label(NO_DRAG, dummy_vect, 0.0);

    for ( i = last_num ; i < info->obj->o_num_features ; i++ )
        XtRealizeWidget(feature_box[i]);
    if ( last_num < info->obj->o_num_features )
        XtManageChildren(feature_box + ( last_num - 1 ),
                         info->obj->o_num_features - last_num + 1);
    for ( i = info->obj->o_num_features ; i < last_num ; i++ )
        XtUnrealizeWidget(feature_box[i]);

    last_num = info->obj->o_num_features;

    Edit_Match_Widths();
    Edit_Sensitize_Buttons(TRUE, SenEditAll);
    XMapRaised(XtDisplay(edit_shell), XtWindow(edit_shell));
}



static void
Edit_Shell_Set_Constraints()
{
    String  *labels;
    Boolean *states;
    int     max_num;
    int     i, num;

    /* Add any constraints specific to this object, and calculate the
    ** resultants.
    */

    /* Need arrays for labels and states. */
    max_num = info->obj->o_features[origin_feature].num_constraints;
    for ( i = major_feature ; i < info->obj->o_num_features ; i++ )
        if ( max_num < info->obj->o_features[i].num_constraints )
            max_num = info->obj->o_features[i].num_constraints;
    labels = New(String, max_num);
    states = New(Boolean, max_num);

    /* Set up feature boxes. */
    for ( num = 0 ; num < info->obj->o_num_features ; num++ )
    {
        if ( ! info->obj->o_features[num].base )
        {
            XtSetSensitive(feature_box[num], FALSE);
            continue;
        }

        for ( i = 0 ;
              i < info->obj->o_features[num].num_constraints ; i++ )
        {
            labels[i] =info->obj->o_features[num].constraints[i].c_label;
            states[i] =
                info->obj->o_features[num].constraints[i].c_status ||
                info->obj->o_features[num].constraints[i].c_forced;
        }
        ConstraintBoxSetConstraints(feature_box[num],
                            info->obj->o_features[num].num_constraints,
                            labels, states);
        info->obj->o_features[num].base->solve_function(
                            info->obj, info->obj->o_features[num].constraints,
                            info->obj->o_features[num].num_constraints,
                            &(info->features[num].resultant));
    }

    free(labels);
    free(states);
}


void
Edit_Complete()
{
    if ( info->modifying )
        Edit_Cancel_Modify(info);

    info->obj->o_flags |= ObjVisible;

    /* Do the constraint maintenance bit. Update all objects depending on
    ** this one.
    */
    if ( ! do_maintenance )
        Edit_Maintain_All_Constraints(info->obj, info->window, FALSE);
    Edit_Maintain_Free_List();

    if ( info->obj == &camera_object )
        Camera_Set_Camera_From_Object(&camera);

    /* Free the selection lists. */
    /* They need to be freed because subsequent edits may make them
    ** invalid.
    */
    free(info->reference_available);
    Free_Selection_List(info->all_available);

    Edit_Undo_Clear(info);
}

static void
Edit_Shell_Suspend(Widget w, XtPointer cl, XtPointer ca)
{
    WindowInfoPtr   window = info->window;

    if ( edit_command_menu )
        XtPopdown(edit_command_menu);

    Edit_Complete();

    Edit_Sensitize_Buttons(FALSE, SenEditAll);

    /* Pop the current off the top. */
    Edit_Clear_Info(info);

    /* If there's another, return. */
    if ( Edit_Begin() )
        return;

    Cancel_Object_Edit(window);

    Edit_Clear_Cursor(window);

    View_Update(window, window->all_instances, ViewNone );
    Update_Projection_Extents(window->all_instances);

    Set_Prompt(window, "");

    changed_scene = TRUE;
}


static void
Edit_Shell_Finished(Widget w, XtPointer cl, XtPointer ca)
{
    if ( edit_command_menu )
        XtPopdown(edit_command_menu);
    Delete_Edit_Instance(info->window, info->inst);

    Edit_Shell_Suspend(w, cl, ca);
}


static void
Edit_Shell_Undo(Widget w, XtPointer cl, XtPointer ca)
{
    if ( edit_command_menu )
        XtPopdown(edit_command_menu);

    if ( info->window->current_state & ( ViewpointState | PanState ) )
    {
        Apply_Viewfrom_Text(info->window);
        return;
    }

    if ( info->selecting )
    {
        Draw_Selection_Points(XtWindow(info->window->view_widget));
        Edit_Cleanup_Selection(info, TRUE);
        return;
    }

    if ( info->window->current_state & LookState )
    {
        Draw_Selection_Points(XtWindow(info->window->view_widget));
        Cancel_Change_Look_Event(info->window);
        return;
    }

    if ( info->modifying )
    {
        Edit_Cancel_Modify(info);
        return;
    }

    Edit_Undo(info);
}

static void
Edit_Shell_Redo(Widget w, XtPointer cl, XtPointer ca)
{
    if ( edit_command_menu )
        XtPopdown(edit_command_menu);
    Edit_Redo(w, cl, ca);
}


static void
Edit_Constraint_Modify_Select(Widget w, XtPointer cl, XtPointer ca)
{
    EditInfoPtr info = Edit_Get_Info();
    int index;

    /* Store a bit of state. */
    info->modifying = TRUE;
    for ( index = 0 ; feature_box[index] != w ; index++ );
    info->current_feature = index;

    Edit_Sensitize_Buttons(FALSE,
        SenEditAll ^ ( SenEditUndo | ( 32 << info->current_feature ) ) );

    /* Change the cursor. */
    if ( ! modify_cursor )
        modify_cursor =
            XCreateFontCursor(XtDisplay(feature_box[index]), XC_pirate);
    XDefineCursor(XtDisplay(feature_box[index]), XtWindow(w), modify_cursor);
}

void
Edit_Cancel_Modify(EditInfoPtr c_info)
{
    if ( ! c_info->modifying )
        return;

    Edit_Sensitize_Buttons(TRUE, SenEditAll);

    c_info->modifying = FALSE;

    /* Change the cursor back. */
    XDefineCursor(XtDisplay(feature_box[c_info->current_feature]),
                  XtWindow(feature_box[c_info->current_feature]), None);
}

static void
Edit_Create_Axis_Menu(Widget parent, char *name, XtPointer axis)
{
    Widget  menu;
    Widget  children[2];

    menu = XtCreatePopupShell(name, simpleMenuWidgetClass, parent, NULL, 0);

    children[0] = XtCreateManagedWidget("Point At", smeBSBObjectClass, menu,
                                        NULL, 0);
    XtAddCallback(children[0], XtNcallback, Edit_Change_Axis_1_Callback, axis);

    children[1] = XtCreateManagedWidget("Direction", smeBSBObjectClass, menu,
                                        NULL, 0);
    XtAddCallback(children[1], XtNcallback, Edit_Change_Axis_2_Callback, axis);
}


static void
Edit_Create_Change_Menu(Widget parent, char *name, XtCallbackProc callback)
{
    Widget  menu;
    Widget  children[2];

    menu = XtCreatePopupShell(name, simpleMenuWidgetClass, parent, NULL, 0);

    children[0] = XtCreateManagedWidget("Point", smeBSBObjectClass, menu,
                                        NULL, 0);
    XtAddCallback(children[0], XtNcallback, callback, (XtPointer)FALSE);

    children[1] = XtCreateManagedWidget("Midpoint", smeBSBObjectClass, menu,
                                        NULL, 0);
    XtAddCallback(children[1], XtNcallback, callback, (XtPointer)TRUE);
}


static void
Edit_Create_Axis_Constraint_Menu(Widget parent)
{
    Widget  menu;
    Widget  child;
    Arg     arg;

    menu = XtCreatePopupShell("AxisAddMenu", simpleMenuWidgetClass,
                                parent, NULL, 0);

    XtSetArg(arg, XtNjustify, XtJustifyCenter);

    child = XtCreateManagedWidget("Axis", smeBSBObjectClass, menu, &arg, 1);
    XtAddCallback(child, XtNcallback, Edit_Add_Constraint_Callback,
                  (XtPointer)axis_feature);
}


static void
Edit_Create_New_Constraint_Menu(Widget parent)
{
    Widget  menu;
    Widget  children[MAX_ADD_ITEMS];
    Arg     arg;
    int     count;

    menu = XtCreatePopupShell("AddMenu", simpleMenuWidgetClass,
                                parent, NULL, 0);

    XtSetArg(arg, XtNjustify, XtJustifyCenter);

    count = 0;
    children[count] = XtCreateManagedWidget( "Plane",
                smeBSBObjectClass, menu, &arg, 1);
    XtAddCallback(children[count], XtNcallback,
            Edit_Add_Constraint_Callback, (XtPointer)plane_feature);
    count++;

    children[count] = XtCreateManagedWidget( "Line",
                smeBSBObjectClass, menu, &arg, 1);
    XtAddCallback(children[count], XtNcallback,
            Edit_Add_Constraint_Callback, (XtPointer)line_feature);
    count++;

    children[count] = XtCreateManagedWidget( "Point",
                smeBSBObjectClass, menu, &arg, 1);
    XtAddCallback(children[count], XtNcallback,
        Edit_Add_Constraint_Callback, (XtPointer)point_feature);
    count++;

    children[count] = XtCreateManagedWidget("Sphere",
                smeBSBObjectClass, menu, &arg, 1);
    XtAddCallback(children[count], XtNcallback,
        Edit_Add_Constraint_Callback, (XtPointer)sphere_feature);

    children[count] = XtCreateManagedWidget("Circle",
                smeBSBObjectClass, menu, &arg, 1);
    XtAddCallback(children[count], XtNcallback,
        Edit_Add_Constraint_Callback, (XtPointer)circle_feature);

    children[count] = XtCreateManagedWidget("Ratio Plane",
                smeBSBObjectClass, menu, &arg, 1);
    XtAddCallback(children[count], XtNcallback,
        Edit_Add_Constraint_Callback, (XtPointer)ratio_plane_feature);
    count++;

    children[count] = XtCreateManagedWidget("Ratio Point",
                smeBSBObjectClass, menu, &arg, 1);
    XtAddCallback(children[count], XtNcallback,
        Edit_Add_Constraint_Callback, (XtPointer)ratio_point_feature);
    count++;

}


static void
Edit_Notify_New_Menu(Widget w, XtPointer cl, XtPointer ca)
{
    int index;

    for ( index = 0 ; feature_box[index] != w ; index++ );
    Edit_Get_Info()->current_feature = index;
}


/* Function called in response to a delete window message from the
** window manager.
*/
void
Edit_Window_Delete_Message()
{
    if ( Edit_Get_Info() )
    {
        /* Currently editing something. Can't delete. */
        Popup_Error("Cannot delete an active window.", edit_shell, "Sorry");
        return;
    }

    /* Destroy the top level and hence all its contents. */
    XtDestroyWidget(edit_shell);
    edit_shell = NULL;
}

static void
Edit_Shell_Cancel(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(edit_command_menu);
}

void
Edit_Create_Command_Menu()
{
    Widget  box;
    Widget  children[5];
    Arg     args[5];
    int     n;
    String  translations = "<EnterWindow>:set()\n"
                           "<LeaveWindow>:unset()\n"
                           "<BtnUp>:notify() unset()";

    edit_command_menu = XtCreatePopupShell("editCommandMenu",
                        overrideShellWidgetClass, main_window.shell, NULL, 0);

    n = 0;
    XtSetArg(args[n], XtNhSpace, 0);        n++;
    XtSetArg(args[n], XtNvSpace, 0);        n++;
    box = XtCreateManagedWidget("editCommandBox", boxWidgetClass,
                                edit_command_menu, args, n);

    children[0] = XtCreateManagedWidget("Finish", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[0], XtNcallback, Edit_Shell_Finished, NULL);
    XtOverrideTranslations(children[0], XtParseTranslationTable(translations));

    children[1] = XtCreateManagedWidget("Suspend", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[1], XtNcallback, Edit_Shell_Suspend, NULL);
    XtOverrideTranslations(children[1], XtParseTranslationTable(translations));

    children[2] = XtCreateManagedWidget("Undo", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[2], XtNcallback, Edit_Shell_Undo, NULL);
    XtOverrideTranslations(children[2], XtParseTranslationTable(translations));

    children[3] = XtCreateManagedWidget("Redo", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[3], XtNcallback, Edit_Shell_Redo, NULL);
    XtOverrideTranslations(children[3], XtParseTranslationTable(translations));

    children[4] = XtCreateManagedWidget("Cancel", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[4], XtNcallback, Edit_Shell_Cancel, NULL);
    XtOverrideTranslations(children[4], XtParseTranslationTable(translations));

    XtRealizeWidget(edit_command_menu);
    XDefineCursor(XtDisplay(edit_command_menu), XtWindow(edit_command_menu),
                  XCreateFontCursor(XtDisplay(edit_command_menu),
                                    XC_top_left_arrow));
    Match_Widths(children, 5);
}

static void
Edit_Create_Edit_Shell()
{
    Arg     args[15];
    char    title_str[16];
    char    name_str[16];
    int     n, m, count;
    Widget  viewport;
    Widget  finish_button;
    Widget  undo_button;
    Widget  redo_button;
    String  horizontal;
    String  vertical;
    int     limit;
    Widget  all_boxes[pt_last_feature + 1];
#if ( ! MISSING_ATOMS )
    Atom    delete_atom = XInternAtom(XtDisplay(main_window.shell),
                                      "WM_DELETE_WINDOW", FALSE);
#endif

    /* We'll control the orientation through this trick. I'll assume that
    ** we are working horizontally. If not, the argument names will be
    ** switched and it will fall through anyway.
    */
    if ( sced_resources.edit_orientation == XtorientHorizontal )
    {
        horizontal = XtNfromHoriz;
        vertical = XtNfromVert;
        limit = sced_resources.edit_columns;
    }
    else
    {
        horizontal = XtNfromVert;
        vertical = XtNfromHoriz;
        limit = sced_resources.edit_rows;
    }

    count = 0;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Edit");            n++;
    XtSetArg(args[n], XtNiconPixmap, icon_bitmap);  n++;
    edit_shell = XtCreatePopupShell("editShell", topLevelShellWidgetClass,
                                    main_window.shell, args, n);

    n = 0;
    XtSetArg(args[n], XtNallowHoriz, TRUE);         n++;
    XtSetArg(args[n], XtNallowVert, TRUE);          n++;
    XtSetArg(args[n], XtNuseRight, TRUE);           n++;
    XtSetArg(args[n], XtNuseBottom, TRUE);          n++;
    viewport = XtCreateManagedWidget("editViewport", viewportWidgetClass,
                                     edit_shell, args, n);

    n = 0;
    edit_form = XtCreateManagedWidget("editForm", formWidgetClass, viewport,
                                      args, n);

    m = 0;
    XtSetArg(args[m], XtNtop, XtChainTop);      m++;
    XtSetArg(args[m], XtNbottom, XtChainTop);   m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);    m++;
    XtSetArg(args[m], XtNright, XtChainLeft);   m++;
    XtSetArg(args[m], XtNresizable, TRUE);      m++;

    /* Finish. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Finish");  n++;
    buttons[count++] =
    finish_button = XtCreateManagedWidget("editShellFinished",
                        commandWidgetClass, edit_form, args, n);
    XtAddCallback(finish_button, XtNcallback, Edit_Shell_Finished, NULL);

    /* Suspend. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Suspend");     n++;
    XtSetArg(args[n], XtNfromVert, finish_button);  n++;
    buttons[count++] =
    suspend_button = XtCreateManagedWidget("editShellSuspend",
                        commandWidgetClass, edit_form, args, n);
    XtAddCallback(suspend_button, XtNcallback, Edit_Shell_Suspend, NULL);

    /* Undo. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Undo");            n++;
    XtSetArg(args[n], XtNfromVert, suspend_button); n++;
    buttons[count++] =
    undo_button = XtCreateManagedWidget("editShellUndo",
                        commandWidgetClass, edit_form, args, n);
    XtAddCallback(undo_button, XtNcallback, Edit_Shell_Undo, NULL);

    /* Redo. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Redo");        n++;
    XtSetArg(args[n], XtNfromVert, undo_button);    n++;
    buttons[count++] =
    redo_button = XtCreateManagedWidget("editShellRedo",
                        commandWidgetClass, edit_form, args, n);
    XtAddCallback(redo_button, XtNcallback, Edit_Redo, NULL);

    /* Maintain. */
    n = m;
    XtSetArg(args[n], XtNlabel, "Maintain");    n++;
    XtSetArg(args[n], XtNstate, TRUE);          n++;
    XtSetArg(args[n], XtNfromVert, redo_button);    n++;
    buttons[count] = XtCreateManagedWidget("editShellMaintain",
                        toggleWidgetClass, edit_form, args, n);
    XtAddCallback(buttons[count], XtNcallback, Maintain_Toggle_Callback, NULL);

    all_boxes[0] = buttons[count];

    Edit_Create_New_Constraint_Menu(edit_form);
    Edit_Create_Axis_Constraint_Menu(edit_form);

    count = 1;

    /* Origin box. */
    n = m;
    XtSetArg(args[n], XtNtitleLabel, "Origin");         n++;
    XtSetArg(args[n], XtNtitleIsMenu, TRUE);            n++;
    XtSetArg(args[n], XtNtitleMenuName, "originMenu");  n++;
    XtSetArg(args[n], XtNnewLabel, "Add");              n++;
    XtSetArg(args[n], XtNnewMenuName, "AddMenu");       n++;
#if ( XtSpecificationRelease > 4 )
    XtSetArg(args[n], XtNleftBitmap, menu_bitmap);      n++;
#endif
    if ( count >= limit )
    {
        XtSetArg(args[n], vertical, all_boxes[count - limit]);  n++;
    }
    if ( count % limit )
    {
        XtSetArg(args[n], horizontal, all_boxes[count - 1]);    n++;
    }
    all_boxes[count++] =
    feature_box[origin_feature] =
        XtCreateManagedWidget("originBox", constraintBoxWidgetClass, edit_form,
                              args, n);
    Edit_Create_Change_Menu(feature_box[origin_feature], "originMenu",
                            Edit_Origin_Callback);
    XtAddCallback(feature_box[origin_feature], XtNstateCallback,
                  Edit_Select_Constraint_Callback, NULL);
    XtAddCallback(feature_box[origin_feature], XtNnewCallback,
                  Edit_Notify_New_Menu, NULL);
    XtAddCallback(feature_box[origin_feature], XtNmodifyCallback,
                  Edit_Constraint_Modify_Select, NULL);

    /* Major box. */
    n = m;
    XtSetArg(args[n], XtNtitleLabel, "Major Axis");     n++;
    XtSetArg(args[n], XtNtitleIsMenu, TRUE);            n++;
    XtSetArg(args[n], XtNtitleMenuName, "newMajor");    n++;
    XtSetArg(args[n], XtNnewLabel, "Add");              n++;
    XtSetArg(args[n], XtNnewMenuName, "AxisAddMenu");   n++;
#if ( XtSpecificationRelease > 4 )
    XtSetArg(args[n], XtNleftBitmap, menu_bitmap);      n++;
#endif
    if ( count >= limit )
    {
        XtSetArg(args[n], vertical, all_boxes[count - limit]);  n++;
    }
    if ( count % limit )
    {
        XtSetArg(args[n], horizontal, all_boxes[count - 1]);    n++;
    }
    all_boxes[count++] =
    feature_box[major_feature] =
                XtCreateManagedWidget("majorBox", constraintBoxWidgetClass,
                                       edit_form, args, n);
    Edit_Create_Axis_Menu(feature_box[major_feature], "newMajor",
                          (XtPointer)MAJOR_AXIS);
    XtAddCallback(feature_box[major_feature], XtNstateCallback,
                  Edit_Select_Constraint_Callback, NULL);
    XtAddCallback(feature_box[major_feature], XtNnewCallback,
                  Edit_Notify_New_Menu, NULL);
    XtAddCallback(feature_box[major_feature], XtNmodifyCallback,
                  Edit_Constraint_Modify_Select, NULL);

    /* Minor box. */
    n = m;
    XtSetArg(args[n], XtNtitleLabel, "Minor Axis");     n++;
    XtSetArg(args[n], XtNtitleIsMenu, TRUE);            n++;
    XtSetArg(args[n], XtNtitleMenuName, "newMinor");    n++;
    XtSetArg(args[n], XtNnewLabel, "Add");              n++;
    XtSetArg(args[n], XtNnewMenuName, "AxisAddMenu");   n++;
#if ( XtSpecificationRelease > 4 )
    XtSetArg(args[n], XtNleftBitmap, menu_bitmap);      n++;
#endif
    if ( count >= limit )
    {
        XtSetArg(args[n], vertical, all_boxes[count - limit]);  n++;
    }
    if ( count % limit )
    {
        XtSetArg(args[n], horizontal, all_boxes[count - 1]);    n++;
    }
    all_boxes[count++] =
    feature_box[minor_feature] =
                XtCreateManagedWidget("minorBox", constraintBoxWidgetClass,
                                       edit_form, args, n);
    Edit_Create_Axis_Menu(feature_box[minor_feature], "newMinor",
                          (XtPointer)MINOR_AXIS);
    XtAddCallback(feature_box[minor_feature], XtNstateCallback,
                  Edit_Select_Constraint_Callback, NULL);
    XtAddCallback(feature_box[minor_feature], XtNnewCallback,
                  Edit_Notify_New_Menu, NULL);
    XtAddCallback(feature_box[minor_feature], XtNmodifyCallback,
                  Edit_Constraint_Modify_Select, NULL);

    /* Scale box. */
    n = m;
    XtSetArg(args[n], XtNtitleLabel, "Scaling");        n++;
    XtSetArg(args[n], XtNtitleIsMenu, TRUE);            n++;
    XtSetArg(args[n], XtNtitleMenuName, "scaleMenu");   n++;
    XtSetArg(args[n], XtNnewLabel, "Add");              n++;
    XtSetArg(args[n], XtNnewMenuName, "AddMenu");       n++;
#if ( XtSpecificationRelease > 4 )
    XtSetArg(args[n], XtNleftBitmap, menu_bitmap);      n++;
#endif
    if ( count >= limit )
    {
        XtSetArg(args[n], vertical, all_boxes[count - limit]);  n++;
    }
    if ( count % limit )
    {
        XtSetArg(args[n], horizontal, all_boxes[count - 1]);    n++;
    }
    all_boxes[count++] =
    feature_box[scale_feature] =
        XtCreateManagedWidget("scaleBox", constraintBoxWidgetClass,
                              edit_form, args, n);
    Edit_Create_Change_Menu(feature_box[scale_feature], "scaleMenu",
                            Edit_Reference_Callback);
    XtAddCallback(feature_box[scale_feature], XtNstateCallback,
                  Edit_Select_Constraint_Callback, NULL);
    XtAddCallback(feature_box[scale_feature], XtNmodifyCallback,
                  Edit_Constraint_Modify_Select, NULL);
    XtAddCallback(feature_box[scale_feature], XtNnewCallback,
                  Edit_Notify_New_Menu, NULL);

    /* Radius box. */
    n = m;
    XtSetArg(args[n], XtNtitleLabel, "Radius");         n++;
    XtSetArg(args[n], XtNtitleIsMenu, TRUE);            n++;
    XtSetArg(args[n], XtNtitleMenuName, "radiusMenu");  n++;
    XtSetArg(args[n], XtNnewLabel, "Add");              n++;
    XtSetArg(args[n], XtNnewMenuName, "AddMenu");       n++;
#if ( XtSpecificationRelease > 4 )
    XtSetArg(args[n], XtNleftBitmap, menu_bitmap);      n++;
#endif
    if ( count >= limit )
    {
        XtSetArg(args[n], vertical, all_boxes[count - limit]);  n++;
    }
    if ( count % limit )
    {
        XtSetArg(args[n], horizontal, all_boxes[count - 1]);    n++;
    }
    all_boxes[count++] =
    feature_box[radius_feature] =
        XtCreateManagedWidget("radiusBox", constraintBoxWidgetClass,
                                       edit_form, args, n);
    Edit_Create_Change_Menu(feature_box[radius_feature], "radiusMenu",
                            Edit_Radius_Callback);
    XtAddCallback(feature_box[radius_feature], XtNstateCallback,
                  Edit_Select_Constraint_Callback, NULL);
    XtAddCallback(feature_box[radius_feature], XtNnewCallback,
                  Edit_Notify_New_Menu, NULL);
    XtAddCallback(feature_box[radius_feature], XtNmodifyCallback,
                  Edit_Constraint_Modify_Select, NULL);

    /* All the control point constraint boxes. */
    for ( ; count <= pt_last_feature ; count++ )
    {
        n = m;
        sprintf(title_str, "Control %d", count - pt0_feature - 1);
        XtSetArg(args[n], XtNtitleLabel, title_str);    n++;
        XtSetArg(args[n], XtNnewLabel, "Add");          n++;
        XtSetArg(args[n], XtNnewMenuName, "AddMenu");   n++;
#if ( XtSpecificationRelease > 4 )
        XtSetArg(args[n], XtNleftBitmap, menu_bitmap);  n++;
#endif
        if ( count >= limit )
        {
            XtSetArg(args[n], vertical, all_boxes[count - limit]);  n++;
        }
        if ( count % limit )
        {
            XtSetArg(args[n], horizontal, all_boxes[count - 1]);    n++;
        }
        sprintf(name_str, "control%dBox", count - pt0_feature - 1);
        all_boxes[count] = feature_box[count-1] =
            XtCreateManagedWidget(name_str, constraintBoxWidgetClass,
                                    edit_form, args, n);
        XtAddCallback(all_boxes[count], XtNstateCallback,
                      Edit_Select_Constraint_Callback, NULL);
        XtAddCallback(all_boxes[count], XtNnewCallback,
                      Edit_Notify_New_Menu, NULL);
        XtAddCallback(all_boxes[count], XtNmodifyCallback,
                      Edit_Constraint_Modify_Select, NULL);
    }

    XtRealizeWidget(edit_shell);

#if ( ! MISSING_ATOMS )
    XSetWMProtocols(XtDisplay(edit_shell), XtWindow(edit_shell), 
                    &delete_atom, 1);
#endif
}


/*
**  This is an amazingly complex piece of code to make the edit box look
**  good. More importantly, it keeps the boxes aligned nicely which makes
**  the dialog box much easier to understand.
*   The complexity is caused by the possible variation of orientation.
*/
void
Edit_Match_Widths()
{
    static Dimension    *max_widths = NULL;
    static int          num_widths;
    Dimension   width;
    Dimension   max_height, height;
    Dimension   borderwidth;
    int         vert_gap;
    Arg         args[3];
    Arg         set_arg;
    int         count, i, j, k;
    Dimension   temp;
    String      horiz_size, vert_size;
    Dimension   box_height, buttons_height;

    if ( ! max_widths )
    {
        if ( sced_resources.edit_orientation == XtorientHorizontal )
            num_widths = sced_resources.edit_columns;
        else
            num_widths = sced_resources.edit_rows;
        max_widths = New(Dimension, num_widths);
    }

    XtVaGetValues(edit_form, XtNdefaultDistance, &vert_gap, NULL);

    XawFormDoLayout(edit_form, FALSE);

    /* Get the values from the 5 command buttons. */
    for ( i = 0 ; i < num_widths ; i++ )
        max_widths[i] = 0;
    max_height = -vert_gap;
    XtSetArg(args[0], XtNwidth, &width);
    XtSetArg(args[1], XtNheight, &height);
    XtSetArg(args[2], XtNborderWidth, &borderwidth);
    for ( i = 0 ; i < 5 ; i++ )
    {
        XtGetValues(buttons[i], args, 3);
        if ( width > max_widths[0] )
            max_widths[0] = width;
        max_height += height + vert_gap;
        if ( i != 0 )
            max_height += ( borderwidth << 1 );
    }
    buttons_height = max_height;

    /* Sort out the orientation issue. */
    if ( sced_resources.edit_orientation == XtorientHorizontal )
    {
        horiz_size = XtNwidth;
        vert_size = XtNheight;
    }
    else
    {
        horiz_size = XtNheight;
        vert_size = XtNwidth;
        temp = max_widths[0];
        max_widths[0] = max_height;
        max_height = temp;
    }

    XtSetArg(args[0], horiz_size, &width);
    XtSetArg(args[1], vert_size, &height);
    for ( count = 1 ; count <= pt_last_feature ; count++ )
    {
        if ( ! ( count % num_widths ) )
        {
            /* Need to set all the "vert" dimensions the same. */
            if ( sced_resources.edit_orientation == XtorientHorizontal )
            {
                for ( i = count - num_widths ; i < count ; i++ )
                {
                    if ( i + num_widths <= pt_last_feature &&
                         XtIsRealized(feature_box[i + num_widths - 1]) )
                    {
                        if ( i == 0 )
                            box_height = buttons_height;
                        else
                            XtVaGetValues(feature_box[i-1],
                                          XtNheight, &box_height, NULL);
                        XtVaSetValues(feature_box[i + num_widths - 1],
                            XtNvertDistance,
                            ( vert_gap + (int)max_height - (int)box_height ),
                            NULL);
                    }
                }
            }
            else
            {
                XtSetArg(set_arg, vert_size, max_height);
                for ( i = count - num_widths ; i >= 0 && i < count ; i++ )
                {
                    if ( i )
                    {
                        if ( XtIsRealized(feature_box[i - 1]) )
                            XtSetValues(feature_box[i - 1], &set_arg, 1);
                    }
                    else
                        for ( j = 0 ; j < 5 ; j++ )
                            XtSetValues(buttons[j], &set_arg, 1);
                }
                max_height = 0;
            }
        }

        if ( XtIsRealized(feature_box[count - 1]) )
        {
            XtGetValues(feature_box[count - 1], args, 2);
            if ( height > max_height )
                max_height = height;
            if ( width > max_widths[count % num_widths] )
                max_widths[count % num_widths] = width;
        }
    }

    /* Need to set all the "horiz" dimensions the same. */
    for ( i = 0 ; i < num_widths ; i++ )
    {
        if ( sced_resources.edit_orientation == XtorientHorizontal )
        {
            XtSetArg(set_arg, horiz_size, max_widths[i]);
            for ( j = i ; j <= pt_last_feature ; j += num_widths )
            {
                if ( j )
                {
                    if ( XtIsRealized(feature_box[j - 1]) )
                        XtSetValues(feature_box[j - 1], &set_arg, 1);
                }
                else
                    for ( k = 0 ; k < 5 ; k++ )
                        XtSetValues(buttons[k], &set_arg, 1);
            }
        }
        else
            for ( j = i, k = 0 ; k < num_widths - 1 && j < pt_last_feature ;
                  j += num_widths, k++ )
                if ( XtIsRealized(feature_box[j + 1]) )
                {
                    if ( j == 0 )
                        box_height = buttons_height;
                    else
                        XtVaGetValues(feature_box[j - 1],
                                      XtNheight, &box_height, NULL);
                    XtVaSetValues(feature_box[j], XtNvertDistance,
                            ( vert_gap + (int)max_widths[i] - (int)box_height ),
                            NULL);
                }
    }

    XawFormDoLayout(edit_form, TRUE);
}


void
Edit_Sensitize_Buttons(Boolean state, int mask)
{
    EditInfoPtr info = Edit_Get_Info();
    int i;

    for ( i = 0 ; i < 5 ; i++ )
        if ( mask & ( 1 << i ) )
            XtSetSensitive(buttons[i], state);

    for ( i = origin_feature ; i < pt_last_feature ; i++ )
    {
        if ( i >= info->obj->o_num_features || ! info->obj->o_features[i].base )
            XtSetSensitive(feature_box[i], FALSE);
        else if ( mask & ( 32 << i ) )
            XtSetSensitive(feature_box[i], state);
    }
}

void
Edit_Set_Drag_Label(int op_type, Vector vect, double val)
{
    char    label_str[64];

    switch ( op_type )
    {
        case NO_DRAG:
            Set_Prompt(info->window, NO_LABEL);
            break;

        case ORIGIN_DRAG:
            sprintf(label_str, "Trans %1.5g %1.5g %1.5g",
                    vect.x, vect.y, vect.z);
            Set_Prompt(info->window, label_str);
            break;

        case SCALE_DRAG:
            sprintf(label_str, "Scale %1.5g %1.5g %1.5g",
                    vect.x, vect.y, vect.z);
            Set_Prompt(info->window, label_str);
            break;

        case ROTATE_DRAG:
            sprintf(label_str, "Rot %1.5g @ %1.5g %1.5g %1.5g", val,
                    vect.x, vect.y, vect.z);
            Set_Prompt(info->window, label_str);
            break;

        case RADIUS_DRAG:
            sprintf(label_str, "Radius %1.5g", val);
            Set_Prompt(info->window, label_str);
            break;

        case CONTROL_DRAG:
            sprintf(label_str, "Displacement %1.5g %1.5g %1.5g",
                    vect.x, vect.y, vect.z);
            Set_Prompt(info->window, label_str);
            break;
    }
}

