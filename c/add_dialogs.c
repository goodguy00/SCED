/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/add_dialogs.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: add_dialogs.c,v $
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
**  add_dialog.c : Functions for creating and interacting with the
**                  add constraint dialog boxes.
*/

#include <sced.h>
#include <add_constraint.h>
#include <ConstraintPt.h>
#include <X11/Xaw/SmeBSB.h>

extern Pixmap   menu_bitmap;

AddPointDialog  new_plane_dialog =
            {NULL, {0, NULL, NULL, NULL}, NULL, {'\0'}, NULL, {'\0'}, NULL, 0};
AddPointDialog  new_line_dialog =
            {NULL, {0, NULL, NULL, NULL}, NULL, {'\0'}, NULL, {'\0'}, NULL, 0};
AddPointDialog  new_point_dialog =
            {NULL, {0, NULL, NULL, NULL}, NULL, {'\0'}, NULL, {'\0'}, NULL, 0};
AddPointDialog  new_sphere_dialog =
            {NULL, {0, NULL, NULL, NULL}, NULL, {'\0'}, NULL, {'\0'}, NULL, 0};
AddPointDialog  new_circle_dialog =
            {NULL, {0, NULL, NULL, NULL}, NULL, {'\0'}, NULL, {'\0'}, NULL, 0};
AddPointDialog  new_ratiopt_dialog =
            {NULL, {0, NULL, NULL, NULL}, NULL, {'\0'}, NULL, {'\0'}, NULL, 0};
AddPointDialog  new_axis_dialog =
            {NULL, {0, NULL, NULL, NULL}, NULL, {'\0'}, NULL, {'\0'}, NULL, 0};


void
Add_Set_Widths(AddPointDialogPtr box)
{
    ConstraintPtGeometry    geom;
    int max_title, max_label, max_toggle, max_extra;
    int i;

    max_title = max_label = max_toggle = max_extra = 0;

    for ( i = 0 ; i < box->points.num_widgets ; i++ )
    {
        ConstraintPt_Query_Geometry(box->points.point_widgets[i], &geom);
        if ( geom.title_width > max_title )
            max_title = geom.title_width;
        if ( geom.label_width > max_label )
            max_label = geom.label_width;
        if ( geom.toggle_width > max_toggle )
            max_toggle = geom.toggle_width;
        if ( geom.extra_width > max_extra )
            max_extra = geom.extra_width;
    }

    geom.title_width = max_title;
    geom.label_width = max_label;
    geom.toggle_width = max_toggle;
    geom.extra_width = max_extra;

    for ( i = 0 ; i < box->points.num_widgets ; i++ )
        ConstraintPt_Set_Geometry(box->points.point_widgets[i], &geom);
}


static void
Add_Dialog_Toggle_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    AddPointDialogPtr   box = (AddPointDialogPtr)cl;
    int             i;

    if ( ! box->points.toggle_callback )
        return;

    for ( i = 0 ; box->points.point_widgets[i] != w ; i++ );

    (box->points.toggle_callback)(box, i, ( ca ? TRUE : FALSE ));
}

static void
Add_Dialog_Extra_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    AddPointDialogPtr   box = (AddPointDialogPtr)cl;
    int             i;

    if ( ! box->points.extra_callback )
        return;

    for ( i = 0 ; box->points.point_widgets[i] != w ; i++ );

    (box->points.extra_callback)(box, i, ( ca ? TRUE : FALSE ));
}


static void
Add_Dialog_Menu_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    AddPointDialogPtr   box = (AddPointDialogPtr)cl;
    int i;

    for ( i = 0 ; box->points.point_widgets[i] != w ; i++ );
    box->menu_hit = i;
}


static void
Add_Create_Common(AddPointDialogPtr box, Widget form, Widget above)
{
    Widget  cancel, reset;
    Arg     args[12];
    int     m, n;

    /* Set constant resources. */
    m = 0;
    XtSetArg(args[m], XtNresizable, TRUE);      m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);    m++;
    XtSetArg(args[m], XtNright, XtChainLeft);   m++;
    XtSetArg(args[m], XtNtop, XtChainTop);      m++;
    XtSetArg(args[m], XtNbottom, XtChainTop);   m++;

    /* The name text box. */
    n = m;
    XtSetArg(args[n], XtNfromVert, above);          n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    n++;
    XtSetArg(args[n], XtNlength, MAX_NAME_LENGTH);  n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);   n++;
    XtSetArg(args[n], XtNstring, box->name_string); n++;
    XtSetArg(args[n], XtNresize, XawtextResizeWidth);           n++;
    box->name = XtCreateManagedWidget("addName", asciiTextWidgetClass, form,
                                      args, n);
    XtOverrideTranslations(box->name,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

    /* The selected toggle. */
    n = m;
    XtSetArg(args[n], XtNfromVert, above);          n++;
    XtSetArg(args[n], XtNfromHoriz, box->name);     n++;
    XtSetArg(args[n], XtNlabel, "Selected");        n++;
    box->select = XtCreateManagedWidget("addSelect", toggleWidgetClass, form,
                                        args, n);

    /* The Done Button. */
    n = m;
    XtSetArg(args[n], XtNfromVert, box->name);  n++;
    XtSetArg(args[n], XtNlabel, "Done");        n++;
    box->done = XtCreateManagedWidget("addDone", commandWidgetClass, form,
                                      args, n);
    XtAddCallback(box->done, XtNcallback,
                  Add_Dialog_Done_Callback, (XtPointer)box);

    /* The reset button. */
    n = m;
    XtSetArg(args[n], XtNfromVert, box->name);  n++;
    XtSetArg(args[n], XtNfromHoriz, box->done); n++;
    XtSetArg(args[n], XtNlabel, "Reset");       n++;
    reset = XtCreateManagedWidget("addReset", commandWidgetClass, form, args,n);
    XtAddCallback(reset, XtNcallback, Add_Dialog_Reset_Callback,(XtPointer)box);

    /* The remove button. */
    n = m;
    XtSetArg(args[n], XtNfromVert, box->name);  n++;
    XtSetArg(args[n], XtNfromHoriz, reset);     n++;
    XtSetArg(args[n], XtNlabel, "Remove");      n++;
    box->remove = XtCreateManagedWidget("addRemove", commandWidgetClass, form,
                                        args, n);
    XtAddCallback(box->remove, XtNcallback, Add_Dialog_Remove_Callback, NULL);

    /* The cancel button. */
    n = m;
    XtSetArg(args[n], XtNfromVert, box->name);      n++;
    XtSetArg(args[n], XtNfromHoriz, box->remove);   n++;
    XtSetArg(args[n], XtNlabel, "Cancel");          n++;
    cancel = XtCreateManagedWidget("addCancel", commandWidgetClass, form,
                                   args, n);
    XtAddCallback(cancel, XtNcallback,
                  Add_Dialog_Cancel_Callback, (XtPointer)box);

}


void
Add_Create_Plane_Dialog()
{
    Widget  form;
    Widget  menu, menu_child;
    Arg     args[15];
    int     m, n;
    int     i;

    n = 0;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    XtSetArg(args[n], XtNtitle, "Plane");           n++;
    new_plane_dialog.shell = XtCreatePopupShell("addPlaneDialog",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("addPlaneForm", formWidgetClass,
                                 new_plane_dialog.shell, args, n);

    /* Set up the point structure. */
    new_plane_dialog.points.num_widgets = 4;
    new_plane_dialog.points.point_widgets = New(Widget, 4);
    new_plane_dialog.points.toggle_callback = NULL;
    new_plane_dialog.points.extra_callback = Add_Extra_Callback;

    /* Create the menu. */
    n = 0;
    menu = XtCreatePopupShell("addPlaneMenu", simpleMenuWidgetClass,
                              new_plane_dialog.shell, args, n);
    menu_child = XtCreateManagedWidget("Plane Pt", smeBSBObjectClass,
                                       menu, args, n);
    XtAddCallback(menu_child, XtNcallback, Add_Plane_Plane_Pt_Callback, NULL);
    menu_child = XtCreateManagedWidget("Normal Pt", smeBSBObjectClass,
                                       menu, args, n);
    XtAddCallback(menu_child, XtNcallback, Add_Plane_Normal_Pt_Callback, NULL);

    /* Set constant resources. */
    m = 0;
    XtSetArg(args[m], XtNresizable, TRUE);      m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);    m++;
    XtSetArg(args[m], XtNright, XtChainLeft);   m++;
    XtSetArg(args[m], XtNtop, XtChainTop);      m++;
    XtSetArg(args[m], XtNbottom, XtChainTop);   m++;

    /* Start creating widgets. */
    n = m;
    XtSetArg(args[n], XtNlabel, "?? ?? ??");        n++;
    XtSetArg(args[n], XtNextraRequired, TRUE);          n++;
    XtSetArg(args[n], XtNtitleLabel, "Point");          n++;
    XtSetArg(args[n], XtNtoggleState, TRUE);            n++;
    new_plane_dialog.points.point_widgets[0] =
        XtCreateManagedWidget("addPlanePoint", constraintPtWidgetClass,
                              form, args, n);
    XtAddCallback(new_plane_dialog.points.point_widgets[0], XtNtoggleCallback,
                  Add_Dialog_Toggle_Callback,
                  (XtPointer)&new_plane_dialog);
    XtAddCallback(new_plane_dialog.points.point_widgets[0], XtNextraCallback,
                  Add_Dialog_Extra_Callback,
                  (XtPointer)&new_plane_dialog);
    ConstraintPt_Change_Radio_Group(new_plane_dialog.points.point_widgets[0],
                                    new_plane_dialog.points.point_widgets[0]);
    XtVaSetValues(new_plane_dialog.points.point_widgets[0],
                  XtNtoggleState, TRUE, NULL);

    for ( i = 1 ; i < 3 ; i++ )
    {
        n = m;
        XtSetArg(args[n], XtNfromVert,
                 new_plane_dialog.points.point_widgets[i-1]);   n++;
        XtSetArg(args[n], XtNlabel, "?? ?? ??");                n++;
        XtSetArg(args[n], XtNleftBitmap, menu_bitmap);          n++;
        XtSetArg(args[n], XtNtitleIsMenu, TRUE);                n++;
        XtSetArg(args[n], XtNtitleLabel, "Point");              n++;
        XtSetArg(args[n], XtNtitleMenuName, "addPlaneMenu");    n++;
        new_plane_dialog.points.point_widgets[i] =
            XtCreateManagedWidget("addPlanePoint", constraintPtWidgetClass,
                                  form, args, n);
        XtAddCallback(new_plane_dialog.points.point_widgets[i],
                      XtNtoggleCallback, Add_Dialog_Toggle_Callback,
                      (XtPointer)&new_plane_dialog);
        XtAddCallback(new_plane_dialog.points.point_widgets[i],
                      XtNtitleCallback, Add_Dialog_Menu_Callback,
                      (XtPointer)&new_plane_dialog);
        ConstraintPt_Change_Radio_Group(
            new_plane_dialog.points.point_widgets[i],
            new_plane_dialog.points.point_widgets[0]);
    }

    n = m;
    XtSetArg(args[n], XtNfromVert, new_plane_dialog.points.point_widgets[2]);
    n++;
    XtSetArg(args[n], XtNlabel, "?? ?? ??");            n++;
    XtSetArg(args[n], XtNtitleLabel, "Normal Pt");      n++;
    new_plane_dialog.points.point_widgets[3] =
        XtCreateManagedWidget("addPlanePoint", constraintPtWidgetClass,
                              form, args, n);
    XtAddCallback(new_plane_dialog.points.point_widgets[3], XtNtoggleCallback,
                  Add_Dialog_Toggle_Callback,
                  (XtPointer)&new_plane_dialog);
    ConstraintPt_Change_Radio_Group(new_plane_dialog.points.point_widgets[3],
                                    new_plane_dialog.points.point_widgets[0]);

    Add_Create_Common(&new_plane_dialog, form,
                      new_plane_dialog.points.point_widgets[3]);

    XtRealizeWidget(new_plane_dialog.shell);
}


void
Add_Create_Line_Dialog()
{
    Widget  form;
    Arg     args[15];
    int     m, n;

    n = 0;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    XtSetArg(args[n], XtNtitle, "Line");            n++;
    new_line_dialog.shell = XtCreatePopupShell("addLineDialog",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("addLineForm", formWidgetClass,
                                 new_line_dialog.shell, args, n);

    /* Set up the point structure. */
    new_line_dialog.points.num_widgets = 3;
    new_line_dialog.points.point_widgets = New(Widget, 3);
    new_line_dialog.points.toggle_callback = NULL;
    new_line_dialog.points.extra_callback = Add_Extra_Callback;

    /* Set constant resources. */
    m = 0;
    XtSetArg(args[m], XtNresizable, TRUE);      m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);    m++;
    XtSetArg(args[m], XtNright, XtChainLeft);   m++;
    XtSetArg(args[m], XtNtop, XtChainTop);      m++;
    XtSetArg(args[m], XtNbottom, XtChainTop);   m++;

    /* Start creating widgets. */
    n = m;
    XtSetArg(args[n], XtNlabel, "?? ?? ??");            n++;
    XtSetArg(args[n], XtNextraRequired, TRUE);          n++;
    XtSetArg(args[n], XtNtitleLabel, "Point");          n++;
    XtSetArg(args[n], XtNtoggleState, TRUE);            n++;
    new_line_dialog.points.point_widgets[0] =
        XtCreateManagedWidget("addLinePoint", constraintPtWidgetClass,
                              form, args, n);
    XtAddCallback(new_line_dialog.points.point_widgets[0], XtNtoggleCallback,
                  Add_Dialog_Toggle_Callback, (XtPointer)&new_line_dialog);
    XtAddCallback(new_line_dialog.points.point_widgets[0], XtNextraCallback,
                  Add_Dialog_Extra_Callback, (XtPointer)&new_line_dialog);
    ConstraintPt_Change_Radio_Group(new_line_dialog.points.point_widgets[0],
                                    new_line_dialog.points.point_widgets[0]);
    XtVaSetValues(new_line_dialog.points.point_widgets[0],
                  XtNtoggleState, TRUE, NULL);

    n = m;
    XtSetArg(args[n], XtNfromVert, new_line_dialog.points.point_widgets[0]);
    n++;
    XtSetArg(args[n], XtNlabel, "?? ?? ??");                n++;
    XtSetArg(args[n], XtNtitleLabel, "Point");              n++;
    new_line_dialog.points.point_widgets[1] =
        XtCreateManagedWidget("addLinePoint", constraintPtWidgetClass,
                              form, args, n);
    XtAddCallback(new_line_dialog.points.point_widgets[1],
                  XtNtoggleCallback, Add_Dialog_Toggle_Callback,
                  (XtPointer)&new_line_dialog);
    ConstraintPt_Change_Radio_Group(new_line_dialog.points.point_widgets[1],
                                    new_line_dialog.points.point_widgets[0]);

    n = m;
    XtSetArg(args[n], XtNfromVert, new_line_dialog.points.point_widgets[1]);
    n++;
    XtSetArg(args[n], XtNlabel, "?? ?? ??");            n++;
    XtSetArg(args[n], XtNtitleLabel, "Normal Pt");      n++;
    new_line_dialog.points.point_widgets[2] =
        XtCreateManagedWidget("addLinePoint", constraintPtWidgetClass,
                              form, args, n);
    XtAddCallback(new_line_dialog.points.point_widgets[2], XtNtoggleCallback,
                  Add_Dialog_Toggle_Callback,
                  (XtPointer)&new_line_dialog);
    ConstraintPt_Change_Radio_Group(new_line_dialog.points.point_widgets[2],
                                    new_line_dialog.points.point_widgets[0]);

    Add_Create_Common(&new_line_dialog, form,
                      new_line_dialog.points.point_widgets[2]);

    XtRealizeWidget(new_line_dialog.shell);
}


void
Add_Create_Point_Dialog()
{
    Widget  form;
    Arg     args[15];
    int     m, n;

    n = 0;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    XtSetArg(args[n], XtNtitle, "Point");           n++;
    new_point_dialog.shell = XtCreatePopupShell("addPointDialog",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("addPointForm", formWidgetClass,
                                 new_point_dialog.shell, args, n);

    /* Set up the point structure. */
    new_point_dialog.points.num_widgets = 1;
    new_point_dialog.points.point_widgets = New(Widget, 1);
    new_point_dialog.points.toggle_callback = NULL;
    new_point_dialog.points.extra_callback = NULL;

    /* Set constant resources. */
    m = 0;
    XtSetArg(args[m], XtNresizable, TRUE);      m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);    m++;
    XtSetArg(args[m], XtNright, XtChainLeft);   m++;
    XtSetArg(args[m], XtNtop, XtChainTop);      m++;
    XtSetArg(args[m], XtNbottom, XtChainTop);   m++;

    /* Start creating widgets. */
    n = m;
    XtSetArg(args[n], XtNlabel, "?? ?? ??");            n++;
    XtSetArg(args[n], XtNtitleLabel, "Point");          n++;
    XtSetArg(args[n], XtNtoggleState, TRUE);            n++;
    new_point_dialog.points.point_widgets[0] =
        XtCreateManagedWidget("addPointPoint", constraintPtWidgetClass,
                              form, args, n);
    XtAddCallback(new_point_dialog.points.point_widgets[0], XtNtoggleCallback,
                  Add_Dialog_Toggle_Callback,
                  (XtPointer)&new_point_dialog);
    ConstraintPt_Change_Radio_Group(new_point_dialog.points.point_widgets[0],
                                    new_point_dialog.points.point_widgets[0]);
    XtVaSetValues(new_point_dialog.points.point_widgets[0],
                  XtNtoggleState, TRUE, NULL);

    Add_Create_Common(&new_point_dialog, form,
                      new_point_dialog.points.point_widgets[0]);

    XtRealizeWidget(new_point_dialog.shell);
}


void
Add_Create_Sphere_Dialog()
{
    Widget  form;
    Arg     args[15];
    int     m, n;

    n = 0;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    XtSetArg(args[n], XtNtitle, "Sphere");          n++;
    new_sphere_dialog.shell = XtCreatePopupShell("addSphereDialog",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("addSphereForm", formWidgetClass,
                                 new_sphere_dialog.shell, args, n);

    /* Set up the point structure. */
    new_sphere_dialog.points.num_widgets = 3;
    new_sphere_dialog.points.point_widgets = New(Widget, 3);
    new_sphere_dialog.points.toggle_callback = NULL;
    new_sphere_dialog.points.extra_callback = Add_Sphere_Extra_Callback;

    /* Set constant resources. */
    m = 0;
    XtSetArg(args[m], XtNresizable, TRUE);      m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);    m++;
    XtSetArg(args[m], XtNright, XtChainLeft);   m++;
    XtSetArg(args[m], XtNtop, XtChainTop);      m++;
    XtSetArg(args[m], XtNbottom, XtChainTop);   m++;

    /* Start creating widgets. */
    n = m;
    XtSetArg(args[n], XtNlabel, "?? ?? ??");        n++;
    XtSetArg(args[n], XtNextraRequired, TRUE);      n++;
    XtSetArg(args[n], XtNtitleLabel, "Centre");     n++;
    new_sphere_dialog.points.point_widgets[0] =
        XtCreateManagedWidget("addSpherePoint", constraintPtWidgetClass,
                              form, args, n);
    XtAddCallback(new_sphere_dialog.points.point_widgets[0], XtNtoggleCallback,
                  Add_Dialog_Toggle_Callback, (XtPointer)&new_sphere_dialog);
    XtAddCallback(new_sphere_dialog.points.point_widgets[0], XtNextraCallback,
                  Add_Dialog_Extra_Callback, (XtPointer)&new_sphere_dialog);
    ConstraintPt_Change_Radio_Group(new_sphere_dialog.points.point_widgets[0],
                                    new_sphere_dialog.points.point_widgets[0]);
    XtVaSetValues(new_sphere_dialog.points.point_widgets[0],
                  XtNtoggleState, TRUE, NULL);

    n = m;
    XtSetArg(args[n], XtNfromVert, new_sphere_dialog.points.point_widgets[0]);
    n++;
    XtSetArg(args[n], XtNlabel, "?? ?? ??");            n++;
    XtSetArg(args[n], XtNtitleLabel, "Radius Pt");      n++;
    new_sphere_dialog.points.point_widgets[1] =
        XtCreateManagedWidget("addSpherePoint", constraintPtWidgetClass,
                              form, args, n);
    XtAddCallback(new_sphere_dialog.points.point_widgets[1],
                  XtNtoggleCallback, Add_Dialog_Toggle_Callback,
                  (XtPointer)&new_sphere_dialog);
    ConstraintPt_Change_Radio_Group(new_sphere_dialog.points.point_widgets[1],
                                    new_sphere_dialog.points.point_widgets[0]);

    n = m;
    XtSetArg(args[n], XtNfromVert, new_sphere_dialog.points.point_widgets[1]);
    n++;
    XtSetArg(args[n], XtNlabel, "?? ?? ??");        n++;
    XtSetArg(args[n], XtNtitleLabel, "Radius Pt");  n++;
    new_sphere_dialog.points.point_widgets[2] =
        XtCreateManagedWidget("addSpherePoint", constraintPtWidgetClass,
                              form, args, n);
    XtAddCallback(new_sphere_dialog.points.point_widgets[2], XtNtoggleCallback,
                  Add_Dialog_Toggle_Callback, (XtPointer)&new_sphere_dialog);
    ConstraintPt_Change_Radio_Group(new_sphere_dialog.points.point_widgets[2],
                                    new_sphere_dialog.points.point_widgets[0]);

    Add_Create_Common(&new_sphere_dialog, form,
                      new_sphere_dialog.points.point_widgets[2]);

    XtRealizeWidget(new_sphere_dialog.shell);
}


void
Add_Create_Circle_Dialog()
{
    Widget  form;
    Widget  menu, menu_child;
    Arg     args[15];
    int     m, n;
    int     i;

    n = 0;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    XtSetArg(args[n], XtNtitle, "Circle");          n++;
    new_circle_dialog.shell = XtCreatePopupShell("addCircleDialog",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("addCircleForm", formWidgetClass,
                                 new_circle_dialog.shell, args, n);

    /* Set up the point structure. */
    new_circle_dialog.points.num_widgets = 6;
    new_circle_dialog.points.point_widgets = New(Widget, 6);
    new_circle_dialog.points.toggle_callback = NULL;
    new_circle_dialog.points.extra_callback = Add_Sphere_Extra_Callback;

    /* Create the menu. */
    n = 0;
    menu = XtCreatePopupShell("addCircleMenu", simpleMenuWidgetClass,
                              new_circle_dialog.shell, args, n);
    menu_child = XtCreateManagedWidget("Plane Pt", smeBSBObjectClass,
                                       menu, args, n);
    XtAddCallback(menu_child, XtNcallback, Add_Circle_Plane_Pt_Callback, NULL);
    menu_child = XtCreateManagedWidget("Normal Pt", smeBSBObjectClass,
                                       menu, args, n);
    XtAddCallback(menu_child, XtNcallback, Add_Circle_Normal_Pt_Callback, NULL);

    /* Set constant resources. */
    m = 0;
    XtSetArg(args[m], XtNresizable, TRUE);      m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);    m++;
    XtSetArg(args[m], XtNright, XtChainLeft);   m++;
    XtSetArg(args[m], XtNtop, XtChainTop);      m++;
    XtSetArg(args[m], XtNbottom, XtChainTop);   m++;

    /* Start creating widgets. */
    n = m;
    XtSetArg(args[n], XtNlabel, "?? ?? ??");        n++;
    XtSetArg(args[n], XtNextraRequired, TRUE);      n++;
    XtSetArg(args[n], XtNtitleLabel, "Center");     n++;
    XtSetArg(args[n], XtNtoggleState, TRUE);        n++;
    new_circle_dialog.points.point_widgets[0] =
        XtCreateManagedWidget("addCirclePoint", constraintPtWidgetClass,
                              form, args, n);
    XtAddCallback(new_circle_dialog.points.point_widgets[0], XtNtoggleCallback,
                  Add_Dialog_Toggle_Callback,
                  (XtPointer)&new_circle_dialog);
    XtAddCallback(new_circle_dialog.points.point_widgets[0], XtNextraCallback,
                  Add_Dialog_Extra_Callback,
                  (XtPointer)&new_circle_dialog);
    ConstraintPt_Change_Radio_Group(new_circle_dialog.points.point_widgets[0],
                                    new_circle_dialog.points.point_widgets[0]);
    XtVaSetValues(new_circle_dialog.points.point_widgets[0],
                  XtNtoggleState, TRUE, NULL);

    for ( i = 1 ; i < 3 ; i++ )
    {
        n = m;
        XtSetArg(args[n], XtNfromVert,
                 new_circle_dialog.points.point_widgets[i-1]);  n++;
        XtSetArg(args[n], XtNlabel, "?? ?? ??");            n++;
        XtSetArg(args[n], XtNleftBitmap, menu_bitmap);      n++;
        XtSetArg(args[n], XtNtitleIsMenu, TRUE);            n++;
        XtSetArg(args[n], XtNtitleLabel, "Point");          n++;
        XtSetArg(args[n], XtNtitleMenuName, "addCircleMenu");   n++;
        new_circle_dialog.points.point_widgets[i] =
            XtCreateManagedWidget("addCirclePoint", constraintPtWidgetClass,
                                  form, args, n);
        XtAddCallback(new_circle_dialog.points.point_widgets[i],
                      XtNtoggleCallback, Add_Dialog_Toggle_Callback,
                      (XtPointer)&new_circle_dialog);
        XtAddCallback(new_circle_dialog.points.point_widgets[i],
                      XtNtitleCallback, Add_Dialog_Menu_Callback,
                      (XtPointer)&new_circle_dialog);
        ConstraintPt_Change_Radio_Group(
            new_circle_dialog.points.point_widgets[i],
            new_circle_dialog.points.point_widgets[0]);
    }

    n = m;
    XtSetArg(args[n], XtNfromVert, new_circle_dialog.points.point_widgets[2]);
    n++;
    XtSetArg(args[n], XtNlabel, "?? ?? ??");        n++;
    XtSetArg(args[n], XtNtitleLabel, "Normal Pt");  n++;
    new_circle_dialog.points.point_widgets[3] =
        XtCreateManagedWidget("addCirclePoint", constraintPtWidgetClass,
                              form, args, n);
    XtAddCallback(new_circle_dialog.points.point_widgets[3], XtNtoggleCallback,
                  Add_Dialog_Toggle_Callback, (XtPointer)&new_circle_dialog);
    ConstraintPt_Change_Radio_Group(new_circle_dialog.points.point_widgets[3],
                                    new_circle_dialog.points.point_widgets[0]);

    for ( i = 4 ; i < 6 ; i++ )
    {
        n = m;
        XtSetArg(args[n], XtNfromVert,
                 new_circle_dialog.points.point_widgets[i-1]);  n++;
        XtSetArg(args[n], XtNlabel, "?? ?? ??");            n++;
        XtSetArg(args[n], XtNtitleLabel, "Radius Pt");      n++;
        new_circle_dialog.points.point_widgets[i] =
            XtCreateManagedWidget("addCirclePoint", constraintPtWidgetClass,
                                  form, args, n);
        XtAddCallback(new_circle_dialog.points.point_widgets[i],
                      XtNtoggleCallback, Add_Dialog_Toggle_Callback,
                      (XtPointer)&new_circle_dialog);
        ConstraintPt_Change_Radio_Group(
            new_circle_dialog.points.point_widgets[i],
            new_circle_dialog.points.point_widgets[0]);
    }

    Add_Create_Common(&new_circle_dialog, form,
                      new_circle_dialog.points.point_widgets[5]);

    XtRealizeWidget(new_circle_dialog.shell);
}


void
Add_Create_RatioPt_Dialog()
{
    Widget  form;
    Widget  ratio_label;
    Arg     args[15];
    int     m, n;

    n = 0;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    XtSetArg(args[n], XtNtitle, "RatioPt");         n++;
    new_ratiopt_dialog.shell = XtCreatePopupShell("addRatioPtDialog",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("addRatioPtForm", formWidgetClass,
                                 new_ratiopt_dialog.shell, args, n);

    /* Set up the point structure. */
    new_ratiopt_dialog.points.num_widgets = 2;
    new_ratiopt_dialog.points.point_widgets = New(Widget, 2);
    new_ratiopt_dialog.points.toggle_callback = NULL;
    new_ratiopt_dialog.points.extra_callback = NULL;

    /* Set constant resources. */
    m = 0;
    XtSetArg(args[m], XtNresizable, TRUE);      m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);    m++;
    XtSetArg(args[m], XtNright, XtChainLeft);   m++;
    XtSetArg(args[m], XtNtop, XtChainTop);      m++;
    XtSetArg(args[m], XtNbottom, XtChainTop);   m++;

    /* Start creating widgets. */
    n = m;
    XtSetArg(args[n], XtNlabel, "?? ?? ??");    n++;
    XtSetArg(args[n], XtNtitleLabel, "Point");  n++;
    XtSetArg(args[n], XtNtoggleState, TRUE);    n++;
    new_ratiopt_dialog.points.point_widgets[0] =
        XtCreateManagedWidget("addRatioPtPoint", constraintPtWidgetClass,
                              form, args, n);
    XtAddCallback(new_ratiopt_dialog.points.point_widgets[0], XtNtoggleCallback,
                  Add_Dialog_Toggle_Callback,
                  (XtPointer)&new_ratiopt_dialog);
    ConstraintPt_Change_Radio_Group(new_ratiopt_dialog.points.point_widgets[0],
                                    new_ratiopt_dialog.points.point_widgets[0]);
    XtVaSetValues(new_ratiopt_dialog.points.point_widgets[0],
                  XtNtoggleState, TRUE, NULL);

    n = m;
    XtSetArg(args[n], XtNfromVert, new_ratiopt_dialog.points.point_widgets[0]);
    n++;
    XtSetArg(args[n], XtNlabel, "?? ?? ??");    n++;
    XtSetArg(args[n], XtNtitleLabel, "Point");  n++;
    new_ratiopt_dialog.points.point_widgets[1] =
        XtCreateManagedWidget("addRatioPtPoint", constraintPtWidgetClass,
                              form, args, n);
    XtAddCallback(new_ratiopt_dialog.points.point_widgets[1],
                  XtNtoggleCallback, Add_Dialog_Toggle_Callback,
                  (XtPointer)&new_ratiopt_dialog);
    ConstraintPt_Change_Radio_Group(new_ratiopt_dialog.points.point_widgets[1],
                                    new_ratiopt_dialog.points.point_widgets[0]);

    /* The ratio stuff. */
    n = m;
    XtSetArg(args[n], XtNfromVert, new_ratiopt_dialog.points.point_widgets[1]);
    n++;
    XtSetArg(args[n], XtNlabel, "Ratio");   n++;
    XtSetArg(args[n], XtNborderWidth, 0);   n++;
    ratio_label = XtCreateManagedWidget("addRatioPtRatioLabel",
                                        labelWidgetClass, form, args, n);

    n = m;
    XtSetArg(args[n], XtNfromVert, new_ratiopt_dialog.points.point_widgets[1]);
    n++;
    XtSetArg(args[n], XtNfromHoriz, ratio_label);   n++;
    XtSetArg(args[n], XtNeditType, XawtextEdit);    n++;
    XtSetArg(args[n], XtNlength, MAX_RATIO_LENGTH); n++;
    XtSetArg(args[n], XtNuseStringInPlace, TRUE);   n++;
    XtSetArg(args[n], XtNstring, new_ratiopt_dialog.ratio_string);  n++;
    XtSetArg(args[n], XtNresize, XawtextResizeWidth);           n++;
    new_ratiopt_dialog.ratio = XtCreateManagedWidget("addRatioPtRatio",
                                asciiTextWidgetClass, form, args, n);
    XtOverrideTranslations(new_ratiopt_dialog.ratio,
        XtParseTranslationTable(":<Key>Return: no-op(RingBell)"));

    Add_Create_Common(&new_ratiopt_dialog, form, new_ratiopt_dialog.ratio);

    XtRealizeWidget(new_ratiopt_dialog.shell);
}


void
Add_Create_Axis_Dialog()
{
    Widget  form;
    Arg     args[15];
    int     m, n;
    int     i;

    n = 0;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    XtSetArg(args[n], XtNtitle, "Axis");            n++;
    new_axis_dialog.shell = XtCreatePopupShell("addAxisDialog",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    form = XtCreateManagedWidget("addAxisForm", formWidgetClass,
                                 new_axis_dialog.shell, args, n);

    /* Set up the point structure. */
    new_axis_dialog.points.num_widgets = 3;
    new_axis_dialog.points.point_widgets = New(Widget, 3);
    new_axis_dialog.points.toggle_callback = NULL;
    new_axis_dialog.points.extra_callback = Add_Axis_Extra_Callback;

    /* Set constant resources. */
    m = 0;
    XtSetArg(args[m], XtNresizable, TRUE);      m++;
    XtSetArg(args[m], XtNleft, XtChainLeft);    m++;
    XtSetArg(args[m], XtNright, XtChainLeft);   m++;
    XtSetArg(args[m], XtNtop, XtChainTop);      m++;
    XtSetArg(args[m], XtNbottom, XtChainTop);   m++;

    /* Start creating widgets. */
    n = m;
    XtSetArg(args[n], XtNlabel, "?? ?? ??");    n++;
    XtSetArg(args[n], XtNextraRequired, TRUE);  n++;
    XtSetArg(args[n], XtNtitleLabel, "Point");  n++;
    new_axis_dialog.points.point_widgets[0] =
        XtCreateManagedWidget("addAxisPoint", constraintPtWidgetClass,
                              form, args, n);
    XtAddCallback(new_axis_dialog.points.point_widgets[0], XtNtoggleCallback,
                  Add_Dialog_Toggle_Callback, (XtPointer)&new_axis_dialog);
    XtAddCallback(new_axis_dialog.points.point_widgets[0], XtNextraCallback,
                  Add_Dialog_Extra_Callback, (XtPointer)&new_axis_dialog);
    ConstraintPt_Change_Radio_Group(new_axis_dialog.points.point_widgets[0],
                                    new_axis_dialog.points.point_widgets[0]);
    XtVaSetValues(new_axis_dialog.points.point_widgets[0],
                  XtNtoggleState, TRUE, NULL);

    for ( i = 1 ; i < 3 ; i++ )
    {
        n = m;
        XtSetArg(args[n], XtNfromVert,
                 new_axis_dialog.points.point_widgets[i-1]);    n++;
        XtSetArg(args[n], XtNlabel, "?? ?? ??");                n++;
        XtSetArg(args[n], XtNtitleLabel, "Point");              n++;
        new_axis_dialog.points.point_widgets[i] =
            XtCreateManagedWidget("addAxisPoint", constraintPtWidgetClass,
                                  form, args, n);
        XtAddCallback(new_axis_dialog.points.point_widgets[i],
                      XtNtoggleCallback, Add_Dialog_Toggle_Callback,
                      (XtPointer)&new_axis_dialog);
        ConstraintPt_Change_Radio_Group(new_axis_dialog.points.point_widgets[i],
                                    new_axis_dialog.points.point_widgets[0]);
    }

    Add_Create_Common(&new_axis_dialog, form,
                      new_axis_dialog.points.point_widgets[2]);

    XtRealizeWidget(new_axis_dialog.shell);
}
