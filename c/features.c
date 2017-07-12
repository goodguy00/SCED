/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/features.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: features.c,v $
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
**  features.c: Functions for manipulating constrained features.
*/


#include <math.h>
#include <sced.h>
#include <add_constraint.h>
#include <constraint.h>
#include <edit.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>

static Boolean  Feature_Select_Point(int, int, int, EditInfoPtr);
static Boolean  Feature_Select_Line(int, int, int, EditInfoPtr);
static Boolean  Feature_Select_True(int, int, int, EditInfoPtr);
static Boolean  Feature_Select_False(int, int, int, EditInfoPtr);
static Boolean  Feature_Point_Active(int, EditInfoPtr);
static Boolean  Feature_Axis_Active(int, EditInfoPtr);

extern void Origin_Draw_Init(FeatureBasePtr);
extern void Scale_Draw_Init(FeatureBasePtr);
extern void Major_Draw_Init(FeatureBasePtr);
extern void Minor_Draw_Init(FeatureBasePtr);
extern void Radius_Draw_Init(FeatureBasePtr);
extern void Control_Draw_Init(FeatureBasePtr);

extern void Draw_Feature_Point(int, FeaturePtr, WindowInfoPtr, int,EditInfoPtr);
extern void Draw_Major_Feature(int, FeaturePtr, WindowInfoPtr, int,EditInfoPtr);
extern void Draw_Minor_Feature(int, FeaturePtr, WindowInfoPtr, int,EditInfoPtr);
extern void Draw_Control_Point(int, FeaturePtr, WindowInfoPtr, int,EditInfoPtr);


static FeatureBase  feature_base[] = {
    { FALSE, Origin_Draw_Init, Draw_Feature_Point, Draw_Point_Constraints,
      0, 0, Feature_Select_Point, 0, 10, Feature_Point_Active,
      Feature_Select_Point, NULL, NULL,
      Edit_Start_Origin_Drag, Edit_Continue_Origin_Drag,
      Edit_Finish_Origin_Drag, FALSE, Constraint_Solve_System,
      Edit_Force_Origin_Satisfaction, 3 },
    { FALSE, Major_Draw_Init, Draw_Major_Feature, Draw_Axis_Constraints,
      0, 0, Feature_Select_True, 0, 40, Feature_Axis_Active,
      Feature_Select_Line, NULL, NULL,
      Edit_Start_Rotate_Drag, Edit_Continue_Rotate_Drag,
      Edit_Finish_Rotate_Drag, FALSE, Edit_Solve_Axis_System,
      Edit_Force_Alignment_Satisfaction, 1 },
    { FALSE, Minor_Draw_Init, Draw_Minor_Feature, Draw_Axis_Constraints,
      0, 0, Feature_Select_False, 0, 40, Feature_Axis_Active,
      Feature_Select_Line, NULL, NULL,
      NULL, NULL, NULL, FALSE, Edit_Solve_Axis_System,
      Edit_Force_Alignment_Satisfaction, 1 },
    { FALSE, Scale_Draw_Init, Draw_Feature_Point, Draw_Point_Constraints,
      0, 0, Feature_Select_Point, 0, 20, Feature_Point_Active,
      Feature_Select_Point, NULL, NULL,
      Edit_Start_Scale_Drag, Edit_Continue_Scale_Drag,
      Edit_Finish_Scale_Drag, TRUE, Constraint_Solve_System,
      Edit_Force_Scale_Satisfaction, 3 },
    { FALSE, Radius_Draw_Init, Draw_Feature_Point, Draw_Point_Constraints,
      0, 0, Feature_Select_Point, 0, 30, Feature_Point_Active,
      Feature_Select_Point, NULL, NULL,
      Edit_Start_Radius_Drag, Edit_Continue_Radius_Drag,
      Edit_Finish_Radius_Drag, FALSE, Constraint_Solve_System,
      Edit_Force_Radius_Satisfaction, 1 },
    { FALSE, Control_Draw_Init, Draw_Control_Point, Draw_Point_Constraints,
      0, 0, Feature_Select_Point, 0, 35, Feature_Point_Active,
      Feature_Select_Point, NULL, NULL,
      Edit_Start_Control_Drag, Edit_Continue_Control_Drag,
      Edit_Finish_Control_Drag, FALSE, Constraint_Solve_System,
      Edit_Force_Control_Satisfaction, 3 } };


static void
Menu_Cancel(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown((Widget)cl);
}


static Widget
Feature_Create_Change_Popup(XtCallbackProc callback)
{
    Widget  result;
    Widget  box;
    Widget  children[3];
    Arg     args[5];
    int     n;
    String  translations = "<EnterWindow>:set()\n"
                           "<LeaveWindow>:unset()\n"
                           "<BtnUp>:notify() unset()";

    result = XtCreatePopupShell("changePopup", overrideShellWidgetClass,
                                main_window.shell, NULL, 0);

    n = 0;
    XtSetArg(args[n], XtNhSpace, 0);        n++;
    XtSetArg(args[n], XtNvSpace, 0);        n++;
    box = XtCreateManagedWidget("changePopupBox", boxWidgetClass,
                                result, args, n);

    children[0] = XtCreateManagedWidget("Point", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[0], XtNcallback, callback, (XtPointer)FALSE);
    XtOverrideTranslations(children[0], XtParseTranslationTable(translations));

    children[1] = XtCreateManagedWidget("Midpoint", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[1], XtNcallback, callback, (XtPointer)TRUE);
    XtOverrideTranslations(children[1], XtParseTranslationTable(translations));

    children[2] = XtCreateManagedWidget("Cancel", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[2], XtNcallback, Menu_Cancel, (XtPointer)result);
    XtOverrideTranslations(children[2], XtParseTranslationTable(translations));

    XtRealizeWidget(result);
    XDefineCursor(XtDisplay(result), XtWindow(result),
                  XCreateFontCursor(XtDisplay(result), XC_top_left_arrow));
    Match_Widths(children, 3);

    return result;
}


static Widget
Feature_Create_Axis_Change_Popup(XtPointer data)
{
    Widget  result;
    Widget  box;
    Widget  children[3];
    Arg     args[5];
    int     n;
    String  translations = "<EnterWindow>:set()\n"
                           "<LeaveWindow>:unset()\n"
                           "<BtnUp>:notify() unset()";

    result = XtCreatePopupShell("changePopup", overrideShellWidgetClass,
                                main_window.shell, NULL, 0);

    n = 0;
    XtSetArg(args[n], XtNhSpace, 0);        n++;
    XtSetArg(args[n], XtNvSpace, 0);        n++;
    box = XtCreateManagedWidget("changePopupBox", boxWidgetClass,
                                result, args, n);

    children[0] = XtCreateManagedWidget("Point At", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[0], XtNcallback, Edit_Change_Axis_1_Callback, data);
    XtOverrideTranslations(children[0], XtParseTranslationTable(translations));

    children[1] = XtCreateManagedWidget("Direction", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[1], XtNcallback, Edit_Change_Axis_2_Callback, data);
    XtOverrideTranslations(children[1], XtParseTranslationTable(translations));

    children[2] = XtCreateManagedWidget("Cancel", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[2], XtNcallback, Menu_Cancel, (XtPointer)result);
    XtOverrideTranslations(children[2], XtParseTranslationTable(translations));

    XtRealizeWidget(result);
    XDefineCursor(XtDisplay(result), XtWindow(result),
                  XCreateFontCursor(XtDisplay(result), XC_top_left_arrow));
    Match_Widths(children, 3);

    return result;
}


static Widget
Feature_New_Constraint_Popup()
{
    Widget  result;
    Widget  box;
    Widget  children[MAX_ADD_ITEMS + 1];
    Arg     args[5];
    int     n;
    String  translations = "<EnterWindow>:set()\n"
                           "<LeaveWindow>:unset()\n"
                           "<BtnUp>:notify() unset()";

    result = XtCreatePopupShell("newConstraintPopup", overrideShellWidgetClass,
                                main_window.shell, NULL, 0);

    n = 0;
    XtSetArg(args[n], XtNhSpace, 0);        n++;
    XtSetArg(args[n], XtNvSpace, 0);        n++;
    box = XtCreateManagedWidget("newPopupBox", boxWidgetClass,
                                result, args, n);

    children[0] = XtCreateManagedWidget("Plane", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[0], XtNcallback, Edit_Add_Constraint_Callback,
                  (XtPointer)plane_feature);
    XtOverrideTranslations(children[0], XtParseTranslationTable(translations));

    children[1] = XtCreateManagedWidget("Line", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[1], XtNcallback, Edit_Add_Constraint_Callback,
                  (XtPointer)line_feature);
    XtOverrideTranslations(children[1], XtParseTranslationTable(translations));

    children[2] = XtCreateManagedWidget("Point", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[2], XtNcallback, Edit_Add_Constraint_Callback,
                  (XtPointer)point_feature);
    XtOverrideTranslations(children[2], XtParseTranslationTable(translations));

    children[3] = XtCreateManagedWidget("Sphere", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[3], XtNcallback, Edit_Add_Constraint_Callback,
                  (XtPointer)sphere_feature);
    XtOverrideTranslations(children[3], XtParseTranslationTable(translations));

    children[4] = XtCreateManagedWidget("Circle", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[4], XtNcallback, Edit_Add_Constraint_Callback,
                  (XtPointer)circle_feature);
    XtOverrideTranslations(children[4], XtParseTranslationTable(translations));

    children[5] = XtCreateManagedWidget("Ratio Plane", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[5], XtNcallback, Edit_Add_Constraint_Callback,
                  (XtPointer)ratio_plane_feature);
    XtOverrideTranslations(children[5], XtParseTranslationTable(translations));

    children[6] = XtCreateManagedWidget("Ratio Point", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[6], XtNcallback, Edit_Add_Constraint_Callback,
                  (XtPointer)ratio_point_feature);
    XtOverrideTranslations(children[6], XtParseTranslationTable(translations));

    children[7] = XtCreateManagedWidget("Cancel", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[7], XtNcallback, Menu_Cancel, (XtPointer)result);
    XtOverrideTranslations(children[7], XtParseTranslationTable(translations));

    XtRealizeWidget(result);
    XDefineCursor(XtDisplay(result), XtWindow(result),
                  XCreateFontCursor(XtDisplay(result), XC_top_left_arrow));
    Match_Widths(children, 8);

    return result;
}


static Widget
Feature_Axis_Constraint_Popup()
{
    Widget  result;
    Widget  box;
    Widget  children[2];
    Arg     args[5];
    int     n;
    String  translations = "<EnterWindow>:set()\n"
                           "<LeaveWindow>:unset()\n"
                           "<BtnUp>:notify() unset()";

    result = XtCreatePopupShell("axisConstraintPopup", overrideShellWidgetClass,
                                main_window.shell, NULL, 0);
    n = 0;
    XtSetArg(args[n], XtNhSpace, 0);        n++;
    XtSetArg(args[n], XtNvSpace, 0);        n++;
    box = XtCreateManagedWidget("newPopupBox", boxWidgetClass,
                                result, args, n);

    children[0] = XtCreateManagedWidget("Axis", commandWidgetClass, box,
                                        NULL, 0);
    XtAddCallback(children[0], XtNcallback, Edit_Add_Constraint_Callback,
                  (XtPointer)axis_feature);
    XtOverrideTranslations(children[0], XtParseTranslationTable(translations));

    children[1] = XtCreateManagedWidget("Cancel", commandWidgetClass,
                                        box, NULL, 0);
    XtAddCallback(children[1], XtNcallback, Menu_Cancel, (XtPointer)result);
    XtOverrideTranslations(children[1], XtParseTranslationTable(translations));


    XtRealizeWidget(result);
    XDefineCursor(XtDisplay(result), XtWindow(result),
                  XCreateFontCursor(XtDisplay(result), XC_top_left_arrow));
    Match_Widths(children, 2);

    return result;
}


void
Feature_Create_Bases()
{
    Widget  new_constraint_popup = Feature_New_Constraint_Popup();
    Widget  axis_constraint_popup = Feature_Axis_Constraint_Popup();

    feature_base[origin_feature].select_cursor =
        XCreateFontCursor(XtDisplay(main_window.shell), XC_diamond_cross);
    feature_base[origin_feature].add_shell = new_constraint_popup;
    feature_base[origin_feature].change_shell =
        Feature_Create_Change_Popup(Edit_Origin_Callback);

    feature_base[major_feature].select_cursor =
        XCreateFontCursor(XtDisplay(main_window.shell), XC_exchange);
    feature_base[major_feature].add_shell = axis_constraint_popup;
    feature_base[major_feature].change_shell =
        Feature_Create_Axis_Change_Popup((XtPointer)MAJOR_AXIS);

    feature_base[minor_feature].select_cursor =
        XCreateFontCursor(XtDisplay(main_window.shell), XC_exchange);
    feature_base[minor_feature].add_shell = axis_constraint_popup;
    feature_base[minor_feature].change_shell =
        Feature_Create_Axis_Change_Popup((XtPointer)MINOR_AXIS);

    feature_base[scale_feature].select_cursor =
        XCreateFontCursor(XtDisplay(main_window.shell), XC_sizing);
    feature_base[scale_feature].add_shell = new_constraint_popup;
    feature_base[scale_feature].change_shell =
        Feature_Create_Change_Popup(Edit_Reference_Callback);

    feature_base[radius_feature].select_cursor =
        XCreateFontCursor(XtDisplay(main_window.shell), XC_double_arrow);
    feature_base[radius_feature].add_shell = new_constraint_popup;
    feature_base[radius_feature].change_shell =
        Feature_Create_Change_Popup(Edit_Radius_Callback);

    feature_base[pt0_feature].select_cursor =
        XCreateFontCursor(XtDisplay(main_window.shell), XC_double_arrow);
    feature_base[pt0_feature].add_shell = new_constraint_popup;
    feature_base[pt0_feature].change_shell = NULL;
}


FeatureBasePtr
Feature_Base(int offset)
{
    if ( offset == -1)
        return NULL;

    return feature_base + offset;
}


int
Feature_Base_Offset(FeatureBasePtr feat_base)
{
    if ( ! feat_base )
        return -1;

    return ( feat_base - feature_base );
}


static Boolean
Feature_Select_Point(int x, int y, int feature, EditInfoPtr info)
{
    return ( ( x - info->features[feature].view->screen.x ) *
             ( x - info->features[feature].view->screen.x ) +
             ( y - info->features[feature].view->screen.y ) *
             ( y - info->features[feature].view->screen.y ) <
             sced_resources.edit_pt_rad * sced_resources.edit_pt_rad );
}


static Boolean
Feature_Select_Line(int x, int y, int feature, EditInfoPtr info)
{
    XPoint  line_dir, to_pt;
    Vector  dist_vect;
    Vector  unit_dir;
    double  temp_d;
    double  pt_dist;

    line_dir.x = info->features[feature].view[1].screen.x -
                 info->features[feature].view[0].screen.x;
    line_dir.y = info->features[feature].view[1].screen.y -
                 info->features[feature].view[0].screen.y;
    temp_d = line_dir.x * line_dir.x + line_dir.y * line_dir.y;
    if ( temp_d == 0 )
        return FALSE;
    temp_d = sqrt(temp_d);
    unit_dir.x = line_dir.x / temp_d;
    unit_dir.y = line_dir.y / temp_d;
    to_pt.x = x - info->features[feature].view[0].screen.x;
    to_pt.y = y - info->features[feature].view[0].screen.y;
    pt_dist = to_pt.x * unit_dir.x + to_pt.y * unit_dir.y;
    if ( pt_dist < 0.0 || pt_dist > temp_d )
        return FALSE;
    dist_vect.x = x - pt_dist * unit_dir.x -
                  info->features[feature].view[0].screen.x;
    dist_vect.y = y - pt_dist * unit_dir.y -
                  info->features[feature].view[0].screen.y;
    return dist_vect.x * dist_vect.x + dist_vect.y * dist_vect.y < 9;
}


static Boolean
Feature_Select_True(int x, int y, int feature, EditInfoPtr info)
{
    return TRUE;
}

static Boolean
Feature_Select_False(int x, int y, int feature, EditInfoPtr info)
{
    return FALSE;
}

static Boolean
Feature_Point_Active(int feature, EditInfoPtr info)
{
    return Interactive_Resultant(info->features[feature].resultant);
}

static Boolean
Feature_Axis_Active(int feature, EditInfoPtr info)
{
    return ( info->rotate_resultant.feature_1.c_type == null_feature ||
             info->rotate_resultant.feature_1.c_type == line_feature );
}


void
Feature_Create_Origin_Constraints(FeaturePtr feature)
{
    ConstraintPtr   f;
    int             i;

    feature->base = Feature_Base(origin_feature);

    /* Everyone starts with their origin at their center. */
    VNew(0, 0, 0, feature->location);

    /* The default Origin constraints are planes parallel to the axis planes.*/
    feature->num_constraints = 3;
    feature->constraints = New(ConstraintData, 3);

    feature->constraints[0].c_label = Strdup("X-Y Plane");
    VNew(0, 0, 1, feature->constraints[0].c_vector);
    feature->constraints[1].c_label = Strdup("Y-Z Plane");
    VNew(1, 0, 0, feature->constraints[1].c_vector);
    feature->constraints[2].c_label = Strdup("Z-X Plane");
    VNew(0, 1, 0, feature->constraints[2].c_vector);

    for ( i = 0 ; i < 3 ; i++ )
    {
        f = feature->constraints + i;

        f->c_type = plane_feature;
        f->c_status = f->c_forced = FALSE;
        VNew(0, 0, 0, f->c_point);
        f->c_value = f->c_radius = 0;
        f->c_spec_flags = ConstNorm;
        f->c_num_specs = 1;
        f->c_specs = New(ConstraintSpec, 1);
        f->c_specs->spec_type = feature_spec;
        f->c_specs->spec_data = (void*)origin_feature;
        f->c_specs->spec_vector = feature->location;
    }

    feature->flags = 0;
}

void
Feature_Create_Major_Constraints(FeaturePtr feature, int x, int y, int z)
{
    feature->base = Feature_Base(major_feature);
    VNew(x, y, z, feature->location);

    /* The default Rotation constraints are the major and minor axes. */
    feature->num_constraints = 1;
    feature->constraints = New(ConstraintData, 1);
    feature->constraints->c_type = line_feature;
    feature->constraints->c_label = Strdup("Current");
    feature->constraints->c_status =
    feature->constraints->c_forced = FALSE;
    VNew(x, y, z, feature->constraints->c_vector);
    VNew(0, 0, 0, feature->constraints->c_point);
    feature->constraints->c_radius = 0;
    feature->constraints->c_spec_flags = Axis | DirAxis;
    feature->constraints->c_num_specs = 1;
    feature->constraints->c_specs = New(ConstraintSpec, 1);
    feature->constraints->c_specs->spec_type = feature_spec;
    feature->constraints->c_specs->spec_data = (void*)major_feature;
    VNew(x, y, z, feature->constraints->c_specs->spec_vector);

    feature->flags = 0;
}

void
Feature_Create_Minor_Constraints(FeaturePtr feature, int x, int y, int z)
{
    feature->base = Feature_Base(minor_feature);
    VNew(x, y, z, feature->location);

    feature->num_constraints = 1;
    feature->constraints = New(ConstraintData, 1);
    feature->constraints->c_type = line_feature;
    feature->constraints->c_label = Strdup("Current");
    feature->constraints->c_status =
    feature->constraints->c_forced = FALSE;
    VNew(x, y, z, feature->constraints->c_vector);
    VNew(0, 0, 0, feature->constraints->c_point);
    feature->constraints->c_radius = 0;
    feature->constraints->c_spec_flags = Axis | DirAxis;
    feature->constraints->c_num_specs = 1;
    feature->constraints->c_specs = New(ConstraintSpec, 1);
    feature->constraints->c_specs->spec_type = feature_spec;
    feature->constraints->c_specs->spec_data = (void*)minor_feature;
    VNew(x, y, z, feature->constraints->c_specs->spec_vector);

    feature->flags = 0;
}


static void
Feature_Create_Default_Scale_Constraints(FeaturePtr feature, Vector *loc)
{
    ConstraintPtr   f;
    int             i;

    feature->base = Feature_Base(scale_feature);
    feature->location = *loc;
    feature->flags = 0;

    feature->constraints[0].c_label = Strdup("Major Plane");
    VNew(0, 0, 1, feature->constraints[0].c_vector);
    feature->constraints[0].c_num_specs = 2;
    feature->constraints[0].c_specs = New(ConstraintSpec, 2);
    feature->constraints[0].c_specs[1].spec_type = feature_spec;
    feature->constraints[0].c_specs[1].spec_data = (void*)major_feature;
    VNew(0, 0, 1, feature->constraints[0].c_specs[1].spec_vector);

    feature->constraints[1].c_label = Strdup("Minor Plane");
    VNew(1, 0, 0, feature->constraints[1].c_vector);
    feature->constraints[1].c_num_specs = 2;
    feature->constraints[1].c_specs = New(ConstraintSpec, 2);
    feature->constraints[1].c_specs[1].spec_type = feature_spec;
    feature->constraints[1].c_specs[1].spec_data = (void*)minor_feature;
    VNew(1, 0, 0, feature->constraints[1].c_specs[1].spec_vector);

    feature->constraints[2].c_label = Strdup("Other Plane");
    VNew(0, 1, 0, feature->constraints[2].c_vector);
    feature->constraints[2].c_num_specs = 2;
    feature->constraints[2].c_specs = New(ConstraintSpec, 2);
    feature->constraints[2].c_specs[1].spec_type = other_spec;
    feature->constraints[2].c_specs[1].spec_data = NULL;
    VNew(0, 1, 0, feature->constraints[2].c_specs[1].spec_vector);

    for ( i = 0 ; i < 3 ; i++ )
    {
        f = feature->constraints + i;

        f->c_type = plane_feature;
        f->c_status = f->c_forced = FALSE;
        f->c_point = feature->location;
        f->c_value = VDot(f->c_vector, f->c_point);
        f->c_radius = 0;
        f->c_spec_flags = NormPlane;
        f->c_specs[0].spec_type = feature_spec;
        f->c_specs[0].spec_data = (void*)scale_feature;
        f->c_specs[0].spec_vector = feature->location;
    }

    /* Check for forced constraints. */
    if ( DEqual(feature->location.x, 0) )
        feature->constraints[0].c_forced = TRUE;
    if ( DEqual(feature->location.y, 0) )
        feature->constraints[1].c_forced = TRUE;
    if ( DEqual(feature->location.z, 0) )
        feature->constraints[2].c_forced = TRUE;
}

void
Feature_Create_Scale_Constraints(FeaturePtr feature, Vector *loc)
{
    feature->num_constraints = 3;
    feature->constraints = New(ConstraintData, 3);

    Feature_Create_Default_Scale_Constraints(feature, loc);
}

void
Feature_Create_Camera_Scale_Constraints(FeaturePtr feature,
                                        ObjectInstancePtr obj, Vector *loc)
{
    ConstraintPtr   f;

    feature->num_constraints = 4;
    feature->constraints = New(ConstraintData, 4);

    Feature_Create_Default_Scale_Constraints(feature, loc);

    f = feature->constraints + 3;

    f->c_type = line_feature;
    f->c_spec_flags = HavePt1 | HavePt2;
    f->c_label = Strdup("ChangeFOV");
    f->c_status = f->c_forced = FALSE;  
    VNew(1, 1, 0, f->c_vector);
    VNew(0, 0, 0, f->c_point);
    f->c_value = f->c_radius = 0;
    f->c_num_specs = 2;
    f->c_specs = New(ConstraintSpec, 2);

    f->c_specs[0].spec_type = feature_spec;
    f->c_specs[0].spec_data = (void*)scale_feature;
    f->c_specs[0].spec_vector = feature->location;

    f->c_specs[1].spec_type = reference_spec;
    f->c_specs[1].spec_data = (void*)obj;
    VNew(0, 0, 0, f->c_specs[1].spec_vector);
}

void
Feature_Create_Uniform_Scale_Constraints(FeaturePtr feature, Vector *loc)
{
    ConstraintPtr   f;
    double          temp_d;

    feature->num_constraints = 4;
    feature->constraints = New(ConstraintData, 4);

    Feature_Create_Default_Scale_Constraints(feature, loc);

    f = feature->constraints + 3;

    f->c_type = line_feature;
    f->c_spec_flags = HavePt1 | HavePt2;
    f->c_label = Strdup("Uniform");
    f->c_status = f->c_forced = FALSE;  
    if ( VZero(feature->location) )
        VNew(1, 1, 1, f->c_vector);
    else
        VUnit(feature->location, temp_d, f->c_vector);
    f->c_point = feature->location;
    f->c_value = f->c_radius = 0;
    f->c_num_specs = 2;
    f->c_specs = New(ConstraintSpec, 2);

    f->c_specs[0].spec_type = feature_spec;
    f->c_specs[0].spec_data = (void*)origin_feature;;
    VNew(0, 0, 0, f->c_specs[0].spec_vector);

    f->c_specs[1].spec_type = feature_spec;
    f->c_specs[1].spec_data = (void*)scale_feature;
    f->c_specs[1].spec_vector = feature->location;
}

void
Feature_Create_Cyl_Scale_Constraints(FeaturePtr feature,
                                     ObjectInstancePtr obj, Vector *loc)
{
    ConstraintPtr   f;

    feature->num_constraints = 4;
    feature->constraints = New(ConstraintData, 4);

    Feature_Create_Default_Scale_Constraints(feature, loc);

    f = feature->constraints + 3;

    f->c_type = line_feature;
    f->c_spec_flags = HavePt1 | HavePt2;
    f->c_label = Strdup("ChangeRad");
    f->c_status = f->c_forced = FALSE;  
    VNew(M_SQRT1_2, M_SQRT1_2, 0, f->c_vector);
    VNew(0, 0, 1, f->c_point);
    f->c_value = f->c_radius = 0;
    f->c_num_specs = 2;
    f->c_specs = New(ConstraintSpec, 2);

    f->c_specs[0].spec_type = feature_spec;
    f->c_specs[0].spec_data = (void*)scale_feature;
    f->c_specs[0].spec_vector = feature->location;

    f->c_specs[1].spec_type = reference_spec;
    f->c_specs[1].spec_data = (void*)obj;
    VNew(0, 0, 1, f->c_specs[1].spec_vector);

}

void
Feature_Create_Cone_Scale_Constraints(FeaturePtr feature,
                                     ObjectInstancePtr obj, Vector *loc)
{
    ConstraintPtr   f;

    feature->num_constraints = 4;
    feature->constraints = New(ConstraintData, 4);

    Feature_Create_Default_Scale_Constraints(feature, loc);

    f = feature->constraints + 3;

    f->c_type = line_feature;
    f->c_spec_flags = HavePt1 | HavePt2;
    f->c_label = Strdup("ChangeRad");
    f->c_status = f->c_forced = FALSE;  
    VNew(M_SQRT1_2, M_SQRT1_2, 0, f->c_vector);
    VNew(0, 0, -1, f->c_point);
    f->c_value = f->c_radius = 0;
    f->c_num_specs = 2;
    f->c_specs = New(ConstraintSpec, 2);

    f->c_specs[0].spec_type = feature_spec;
    f->c_specs[0].spec_data = (void*)scale_feature;
    f->c_specs[0].spec_vector = feature->location;

    f->c_specs[1].spec_type = reference_spec;
    f->c_specs[1].spec_data = (void*)obj;
    VNew(0, 0, -1, f->c_specs[1].spec_vector);
}

void
Feature_Create_Radius_Constraints(FeaturePtr feature, ObjectInstancePtr obj)
{
    feature->base = Feature_Base(radius_feature);

    VNew(1.0, 0, 0, feature->location);

    feature->num_constraints = 1;
    feature->constraints = New(ConstraintData, 1);
    feature->constraints->c_type = line_feature;
    feature->constraints->c_spec_flags = HavePt1 | HavePt2;
    feature->constraints->c_label = Strdup("RadiusLine");
    feature->constraints->c_status = feature->constraints->c_forced = FALSE;
    VNew(1, 0, 0, feature->constraints->c_vector);
    VNew(0, 0, 0, feature->constraints->c_point);
    feature->constraints->c_value = 0.0;
    feature->constraints->c_radius = 0.0;
    feature->constraints->c_num_specs = 2;
    feature->constraints->c_specs = New(ConstraintSpec, 2);

    feature->constraints->c_specs[0].spec_type = reference_spec;
    feature->constraints->c_specs[0].spec_data = (void*)obj;
    VNew(0, 0, 0, feature->constraints->c_specs[0].spec_vector);

    feature->constraints->c_specs[1].spec_type = feature_spec;
    feature->constraints->c_specs[1].spec_data = (void*)radius_feature;
    VNew(1, 0, 0, feature->constraints->c_specs[1].spec_vector);

    feature->flags = 0;
}


void
Feature_Create_Control_Constraints(FeaturePtr feature, ObjectInstancePtr obj,
                                  int num)
{
    ConstraintPtr   f;
    int             i;

    feature->base = Feature_Base(pt0_feature);
    feature->location = control_part(obj)->control_verts[num];

    feature->num_constraints = 3;
    feature->constraints = New(ConstraintData, 3);
    feature->flags = 0;

    feature->constraints[0].c_label = Strdup("Major Plane");
    VNew(0, 0, 1, feature->constraints[0].c_vector);
    feature->constraints[0].c_num_specs = 2;
    feature->constraints[0].c_specs = New(ConstraintSpec, 2);
    feature->constraints[0].c_specs[1].spec_type = feature_spec;
    feature->constraints[0].c_specs[1].spec_data = (void*)major_feature;
    VNew(0, 0, 1, feature->constraints[0].c_specs[1].spec_vector);

    feature->constraints[1].c_label = Strdup("Minor Plane");
    VNew(1, 0, 0, feature->constraints[1].c_vector);
    feature->constraints[1].c_num_specs = 2;
    feature->constraints[1].c_specs = New(ConstraintSpec, 2);
    feature->constraints[1].c_specs[1].spec_type = feature_spec;
    feature->constraints[1].c_specs[1].spec_data = (void*)minor_feature;
    VNew(1, 0, 0, feature->constraints[1].c_specs[1].spec_vector);

    feature->constraints[2].c_label = Strdup("Other Plane");
    VNew(0, 1, 0, feature->constraints[2].c_vector);
    feature->constraints[2].c_num_specs = 2;
    feature->constraints[2].c_specs = New(ConstraintSpec, 2);
    feature->constraints[2].c_specs[1].spec_type = other_spec;
    feature->constraints[2].c_specs[1].spec_data = NULL;
    VNew(0, 1, 0, feature->constraints[2].c_specs[1].spec_vector);

    for ( i = 0 ; i < 3 ; i++ )
    {
        f = feature->constraints + i;

        f->c_type = plane_feature;
        f->c_status = f->c_forced = FALSE;
        f->c_point = feature->location;
        f->c_value = VDot(f->c_vector, f->c_point);
        f->c_radius = 0;
        f->c_spec_flags = NormPlane;
        f->c_specs[0].spec_type = vertex_spec;
        f->c_specs[0].spec_data = (void*)obj;
        f->c_specs[0].spec_vector.x = (double)num;
        f->c_specs[0].spec_vector.y = f->c_specs[0].spec_vector.z = 0.0;
    }
}


void
Feature_Create_Dummy_Constraints(FeaturePtr feature)
{
    feature->base = NULL;

    VNew(0, 0, 0, feature->location);
    feature->num_constraints = 0;
    feature->constraints = NULL;
    feature->flags = 0;
}


/*  void
**  Calculate_Circle(Vertex pt, XArc *arc)
**  Calculates the circle that will be used to indicate a drag point.
*/
static void
Calculate_Circle(Vertex *pt, XArc *arc)
{
    arc->x = (short)(pt->screen.x - sced_resources.edit_pt_rad);
    arc->y = (short)(pt->screen.y - sced_resources.edit_pt_rad);
    arc->width = sced_resources.edit_pt_rad  << 1;
    arc->height = sced_resources.edit_pt_rad  << 1;
    arc->angle1 = 0;
    arc->angle2 = 23040;
}

void
Feature_Calculate_Point_Display(int num, FeatureBasePtr base, EditInfoPtr info)
{
    if ( info->features[num].num_view == 0 )
    {
        info->features[num].view = New(Vertex, 1);
        info->features[num].num_view = 1;
    }

    Convert_World_To_View(&(info->features[num].world),
                          info->features[num].view,
                          1, &(info->window->viewport));
    Convert_View_To_Screen(info->features[num].view, 1,
                           &(info->window->viewport),
                           (short)info->window->width,
                           (short)info->window->height,
                           info->window->magnify);
    Calculate_Circle(info->features[num].view, &(info->features[num].circle));
}


void
Feature_Calculate_Major_Display(int num, FeatureBasePtr base, EditInfoPtr info)
{
    Vector  temp_v;
    int     arcball_radius;

    if ( info->features[major_feature].num_view == 0 )
    {
        info->features[major_feature].view = New(Vertex, 2);
        info->features[major_feature].num_view = 2;
    }

    /* Calculate the arcball radius information. */
    arcball_radius =
        min( (short)info->window->width -
                info->features[origin_feature].view[0].screen.x,
             info->features[origin_feature].view[0].screen.x );
    arcball_radius = min( arcball_radius,
                          info->features[origin_feature].view[0].screen.y );
    arcball_radius =
        min( arcball_radius,
             (short)info->window->height -
                info->features[origin_feature].view[0].screen.y );
    if ( arcball_radius < 0 )
        arcball_radius = -arcball_radius;
    info->features[major_feature].circle.x =
        info->features[origin_feature].view[0].screen.x - arcball_radius;
    info->features[major_feature].circle.y =
        info->features[origin_feature].view[0].screen.y - arcball_radius;
    info->features[major_feature].circle.width = 2 * arcball_radius;
    info->features[major_feature].circle.height = 2 * arcball_radius;
    info->features[major_feature].circle.angle1 = 0;
    info->features[major_feature].circle.angle2 = 23040;

    VScalarMul(info->axes.x,
               sced_resources.obj_x_axis_length /
               (double)sced_resources.obj_axis_denom, temp_v);
    VAdd(info->features[origin_feature].world, temp_v,
         info->features[major_feature].world);

    Convert_World_To_View(&(info->features[major_feature].world),
                          info->features[major_feature].view,
                          1, &(info->window->viewport));
    Convert_World_To_View(&(info->features[origin_feature].world),
                          info->features[major_feature].view + 1,
                          1, &(info->window->viewport));
    Convert_View_To_Screen(info->features[major_feature].view, 2,
                           &(info->window->viewport),
                           (short)info->window->width,
                           (short)info->window->height,
                           info->window->magnify);
}


void
Feature_Calculate_Minor_Display(int num, FeatureBasePtr base, EditInfoPtr info)
{
    Vector  temp_v;

    if ( info->features[minor_feature].num_view == 0 )
    {
        info->features[minor_feature].view = New(Vertex, 2);
        info->features[minor_feature].num_view = 2;
    }

    VScalarMul(info->axes.y,
               sced_resources.obj_y_axis_length /
               (double)sced_resources.obj_axis_denom, temp_v);
    VAdd(info->features[origin_feature].world, temp_v,
         info->features[minor_feature].world);

    Convert_World_To_View(&(info->features[minor_feature].world),
                          info->features[minor_feature].view,
                          1, &(info->window->viewport));
    Convert_World_To_View(&(info->features[origin_feature].world),
                          info->features[minor_feature].view + 1,
                          1, &(info->window->viewport));
    Convert_View_To_Screen(info->features[minor_feature].view, 2,
                           &(info->window->viewport),
                           (short)info->window->width,
                           (short)info->window->height,
                           info->window->magnify);
}

