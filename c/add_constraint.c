/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/add_constraint.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: add_constraint.c,v $
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
**  add_constraint.c: Functions for adding new constraints - the dialogs etc.
*/

#include <math.h>
#include <sced.h>
#include <edit.h>
#include <add_constraint.h>
#include <select_point.h>
#include <ConstraintBox.h>
#include <ConstraintPt.h>


static int  Add_Plane_Constraint_Callback(ConstraintSpecPtr);
static int  Add_Line_Constraint_Callback(ConstraintSpecPtr);
static int  Add_Point_Constraint_Callback(ConstraintSpecPtr);
static int  Add_Sphere_Constraint_Callback(ConstraintSpecPtr);
static int  Add_Circle_Constraint_Callback(ConstraintSpecPtr);
static int  Add_RatioPt_Constraint_Callback(ConstraintSpecPtr);
static int  Add_Axis_Constraint_Callback(ConstraintSpecPtr);

static int  Edit_Select_Point_Callback(ConstraintSpecPtr);

static int  feature_counts[NUM_CONSTRAINT_TYPES] = { 0 };

static AddPointDialogPtr    add_dialog_box;

static Vector           points[MAX_NUM_SPECS];
static ConstraintSpec   specs[MAX_NUM_SPECS];


static char*
Edit_Prepare_Set_Extra_Label(AddPointDialogPtr box, int index)
{
    EditInfoPtr info = Edit_Get_Info();

    if ( box == &new_sphere_dialog || box == &new_circle_dialog )
    {
        if ( info->current_feature == origin_feature )
            return "";
        else
            return "Origin Pt";
    }
    else if ( info->current_feature == origin_feature ||
              info->current_feature == major_feature ||
              info->current_feature == minor_feature )
        return "Origin Pt";
    else if ( info->current_feature == scale_feature )
        return "Scaling Pt";
    else if ( info->current_feature == radius_feature )
        return "Radius Pt";

    return "";
}


static void
Edit_Reset_Add_Dialog(AddPointDialogPtr box)
{
    XawTextBlock    text_block;
    int             old_length;
    Boolean         have_extra, title_menu;
    Arg             args[5];
    int             i, n;

    /* Set done and selected. */
    XtSetSensitive(box->done, FALSE);
    XtVaSetValues(box->select, XtNstate, FALSE, NULL);

    /* Clear all toggles and points. */
    for ( i = 0 ; i < box->points.num_widgets ; i++ )
    {
        XtVaGetValues(box->points.point_widgets[i],
                      XtNextraRequired, &have_extra,
                      XtNtitleIsMenu, &title_menu, NULL);
        n = 0;
        XtSetArg(args[n], XtNlabel, "?? ?? ??");    n++;
        if ( i == 0 )
        {
            XtSetArg(args[n], XtNtoggleState, TRUE);    n++;
        }
        if ( have_extra )
        {
            XtSetArg(args[n], XtNextraState, FALSE);    n++;
            XtSetArg(args[n], XtNextraLabel,
                     Edit_Prepare_Set_Extra_Label(box, i)); n++;
        }
        if ( title_menu )
        {
            XtSetArg(args[n], XtNtitleLabel, "Point");  n++;
        }
        XtSetValues(box->points.point_widgets[i], args, n);
    }

    if ( box == &new_ratiopt_dialog )
    {
        /* Set ratio. */
        old_length = strlen(box->ratio_string);
        text_block.firstPos = 0;
        text_block.format = FMT8BIT;
        text_block.length = strlen("1:1");
        text_block.ptr = "1:1";
        XawTextReplace(box->ratio, 0, old_length + 1, &text_block);
    }

    /* Resize. */
    Add_Set_Widths(box);
}

static void
Edit_Set_Add_Dialog(EditInfoPtr info, AddPointDialogPtr box,
                    ConstraintPtr feature)
{
    /* Set all the toggles and labels. */
    XawTextBlock    text_block;
    int             old_length;
    char            new_label[48];
    int             i;
    Arg             args[5];
    int             n;
    Boolean         have_extra, title_menu;

    for ( i = 0 ; i < 6 ; i++ )
    {
        if ( feature->c_spec_flags & ( 1 << i ) )
        {
            update_spec_function[feature->c_specs[i].spec_type](
                feature->c_specs + i,
                info->obj->o_world_verts + (info->obj->o_num_vertices - 1),
                info->obj->o_features, &(info->axes.z), points + i);
            specs[i] = feature->c_specs[i];

            XtVaGetValues(box->points.point_widgets[i],
                          XtNextraRequired, &have_extra,
                          XtNtitleIsMenu, &title_menu, NULL);
            n = 0;
            sprintf(new_label, "%1.3g %1.3g %1.3g",
                    points[i].x, points[i].y, points[i].z);
            XtSetArg(args[n], XtNlabel, new_label); n++;
            if ( i == 0 )
            {
                XtSetArg(args[n], XtNtoggleState, TRUE);    n++;
            }
            if ( have_extra )
            {
                if ( specs[i].spec_type == feature_spec )
                {
                    XtSetArg(args[n], XtNextraState, TRUE);     n++;
                }
                else
                {
                    XtSetArg(args[n], XtNextraState, FALSE);    n++;
                }
                XtSetArg(args[n], XtNextraLabel,
                         Edit_Prepare_Set_Extra_Label(box, i)); n++;
            }
            if ( title_menu )
            {
                if ( i == 1 )
                {
                    if ( feature->c_spec_flags & TwoisPt )
                    {
                        XtSetArg(args[n], XtNtitleLabel, "Point");  n++;
                    }
                    else
                    {
                        XtSetArg(args[n], XtNtitleLabel, "Normal"); n++;
                    }
                }
                else if ( i == 2 )
                {
                    if ( feature->c_spec_flags & ThreeisPt )
                    {
                        XtSetArg(args[n], XtNtitleLabel, "Point");  n++;
                    }
                    else
                    {
                        XtSetArg(args[n], XtNtitleLabel, "Normal"); n++;
                    }
                }
            }
            XtSetValues(box->points.point_widgets[i], args, n);
        }
        else if ( i < box->points.num_widgets )
            /* Must be non-essentials. Not many options. */
            XtVaSetValues(box->points.point_widgets[i], XtNlabel, "?? ?? ??",
                          NULL);
    }

    if ( box == &new_ratiopt_dialog )
    {
        /* Set ratio. */
        sprintf(new_label, "%g:%g", feature->c_radius, feature->c_ratio);
        old_length = strlen(box->ratio_string);
        text_block.firstPos = 0;
        text_block.format = FMT8BIT;
        text_block.length = strlen(new_label);
        text_block.ptr = new_label;
        XawTextReplace(box->ratio, 0, old_length + 1, &text_block);
    }

    /* Resize. */
    Add_Set_Widths(box);
}


void
Edit_Popup_Add_Dialog(EditInfoPtr info, AddPointDialogPtr box)
{
    Position_Shell(box->shell, TRUE);
    XtPopup(box->shell, XtGrabNone);

    /* Prepare for point selection. */
    select_callback = Edit_Select_Point_Callback;
    select_window = info->window;
    select_highlight = TRUE;
    allow_text_entry = TRUE;
    prompt_offset = TRUE;
    select_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];
    select_instances = info->all_available;

    info->selecting = TRUE;

    Edit_Sensitize_Buttons(FALSE, SenEditAll);

    /* Draw all the reference points. */
    Edit_Draw_Selection_Points(info);

    Register_Select_Operation(info->window);
}

void
Edit_Add_Constraint_Callback(Widget w, XtPointer cl_data, XtPointer ca)
{
    EditInfoPtr     info = Edit_Get_Info();
    ConstraintSpecType  type = (ConstraintSpecType)cl_data;
    XawTextBlock    text_block;
    int             old_length;
    char            default_name[20];

    XtPopdown(info->obj->o_features[info->current_feature].base->add_shell);

    if ( info->modifying )
        Edit_Cancel_Modify(info);

    info->constr_index = -1;

    switch ( type )
    {
        case plane_feature:
            if ( ! new_plane_dialog.shell )
                Add_Create_Plane_Dialog();
            add_dialog_box = &new_plane_dialog;
            add_dialog_box->flags = ( TwoisPt | ThreeisPt );
            sprintf(default_name, "Plane %d", feature_counts[plane_feature]);
            break;

        case line_feature:
            if ( ! new_line_dialog.shell )
                Add_Create_Line_Dialog();
            add_dialog_box = &new_line_dialog;
            add_dialog_box->flags = TwoisPt;
            sprintf(default_name, "Line %d", feature_counts[line_feature]);
            break;

        case point_feature:
            if ( ! new_point_dialog.shell )
                Add_Create_Point_Dialog();
            add_dialog_box = &new_point_dialog;
            add_dialog_box->flags = 0;
            sprintf(default_name, "Point %d", feature_counts[point_feature]);
            break;

        case sphere_feature:
            if ( ! new_sphere_dialog.shell )
                Add_Create_Sphere_Dialog();
            add_dialog_box = &new_sphere_dialog;
            add_dialog_box->flags = 0;
            sprintf(default_name, "Sphere %d", feature_counts[sphere_feature]);
            break;

        case circle_feature:
            if ( ! new_circle_dialog.shell )
                Add_Create_Circle_Dialog();
            add_dialog_box = &new_circle_dialog;
            add_dialog_box->flags = ( TwoisPt | ThreeisPt );
            sprintf(default_name, "Circle %d", feature_counts[circle_feature]);
            break;

        case ratio_plane_feature:
            if ( ! new_ratiopt_dialog.shell )
                Add_Create_RatioPt_Dialog();
            add_dialog_box = &new_ratiopt_dialog;
            add_dialog_box->flags = RatioPlane;
            sprintf(default_name, "Ratio Pl %d",
                    feature_counts[ratio_plane_feature]);
            break;

        case ratio_point_feature:
            if ( ! new_ratiopt_dialog.shell )
                Add_Create_RatioPt_Dialog();
            add_dialog_box = &new_ratiopt_dialog;
            add_dialog_box->flags = RatioPoint;
            sprintf(default_name, "Ratio Pt %d",
                    feature_counts[ratio_point_feature]);
            break;

        case axis_feature:
            if ( ! new_axis_dialog.shell )
                Add_Create_Axis_Dialog();
            add_dialog_box = &new_axis_dialog;
            add_dialog_box->flags = Axis;
            sprintf(default_name, "Axis %d", feature_counts[axis_feature]);
            break;

        default:;
    }

    /* Set name. */
    old_length = strlen(add_dialog_box->name_string);
    text_block.firstPos = 0;
    text_block.format = FMT8BIT;
    text_block.length = strlen(default_name);
    text_block.ptr = default_name;
    XawTextReplace(add_dialog_box->name, 0, old_length + 1, &text_block);

    XtSetSensitive(add_dialog_box->remove, FALSE);
    XtSetSensitive(add_dialog_box->select, TRUE);

    Edit_Reset_Add_Dialog(add_dialog_box);

    Edit_Popup_Add_Dialog(info, add_dialog_box);
}

void
Add_Modify_Constraint(EditInfoPtr info, ConstraintPtr feature)
{
    XawTextBlock    text_block;
    int             old_length;

    switch ( feature->c_type )
    {
        case plane_feature:
            if ( feature->c_spec_flags & RatioPlane )
            {
                if ( ! new_ratiopt_dialog.shell )
                    Add_Create_RatioPt_Dialog();
                add_dialog_box = &new_ratiopt_dialog;
            }
            else
            {
                if ( ! new_plane_dialog.shell )
                    Add_Create_Plane_Dialog();
                add_dialog_box = &new_plane_dialog;
            }
            break;

        case line_feature:
            if ( feature->c_spec_flags & Axis )
            {
                if ( ! new_axis_dialog.shell )
                    Add_Create_Axis_Dialog();
                add_dialog_box = &new_axis_dialog;
            }
            else
            {
                if ( ! new_line_dialog.shell )
                    Add_Create_Line_Dialog();
                add_dialog_box = &new_line_dialog;
            }
            break;

        case point_feature:
            if ( feature->c_spec_flags & RatioPoint )
            {
                if ( ! new_ratiopt_dialog.shell )
                    Add_Create_RatioPt_Dialog();
                add_dialog_box = &new_ratiopt_dialog;
            }
            else
            {
                if ( ! new_point_dialog.shell )
                    Add_Create_Point_Dialog();
                add_dialog_box = &new_point_dialog;
            }
            break;

        case sphere_feature:
            if ( ! new_sphere_dialog.shell )
                Add_Create_Sphere_Dialog();
            add_dialog_box = &new_sphere_dialog;
            break;

        case circle_feature:
            if ( ! new_circle_dialog.shell )
                Add_Create_Circle_Dialog();
            add_dialog_box = &new_circle_dialog;
            break;

        default:;
    }

    add_dialog_box->flags = feature->c_spec_flags;

    /* Set name. */
    old_length = strlen(add_dialog_box->name_string);
    text_block.firstPos = 0;
    text_block.format = FMT8BIT;
    text_block.length = strlen(feature->c_label);
    text_block.ptr = feature->c_label;
    XawTextReplace(add_dialog_box->name, 0, old_length + 1, &text_block);

    XtSetSensitive(add_dialog_box->remove, ! feature->c_status);
    XtSetSensitive(add_dialog_box->select, FALSE);

    Edit_Set_Add_Dialog(info, add_dialog_box, feature);

    Edit_Popup_Add_Dialog(info, add_dialog_box);
}

void
Add_Dialog_Remove_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    EditInfoPtr info = Edit_Get_Info();

    XtPopdown(add_dialog_box->shell);

    Edit_Cleanup_Selection(info, TRUE);

    Edit_Remove_Constraint(info, info->current_feature,info->constr_index,TRUE);
}

static Boolean
Add_Spec_OK(ConstraintSpecPtr spec, EditInfoPtr info, Boolean allow_self)
{
    if ( ! Spec_Is_Dependent(spec->spec_type) )
        return TRUE;

    if ( ! allow_self && spec_object(spec) == info->obj )
    {
        if ( spec->spec_type == vertex_spec )
            return ( (int)(spec->spec_vector.x) <
                     info->current_feature - pt0_feature );
        else
            return FALSE;
    }

    return ! ( spec_object(spec)->o_flags & ObjDepends );
}

static void
Add_Check_Specifiers(ConstraintSpecPtr spec, EditInfoPtr info,
                     Boolean allow_self)
{
    if ( ! Add_Spec_OK(spec, info, allow_self) )
    {
        if ( info->current_feature == origin_feature )
        {
            spec->spec_vector = Spec_Absolute(spec, select_center);
            spec->spec_type = absolute_spec;
            Popup_Error("Reference specifier changed to Absolute",
                        info->window->shell, "Error");
        }
        else
        {
            spec->spec_vector = Spec_Absolute(spec, select_center);
            VSub(spec->spec_vector, select_center, spec->spec_vector);
            spec->spec_type = offset_spec;
            Popup_Error("Reference specifier changed to Offset",
                        info->window->shell, "Error");
        }
        spec->spec_data = (void*)NULL;
    }
}


static int
Add_Get_Set_Toggle(AddPointDialogPtr box)
{
    Boolean state = FALSE;
    int i;

    for ( i = 0 ; i < box->points.num_widgets && ! state ; i++ )
        XtVaGetValues(box->points.point_widgets[i], XtNtoggleState, &state,
                      NULL);
    return i - 1;
}

static void
Add_Set_Next_Toggle(AddPointDialogPtr box)
{
    int     current = Add_Get_Set_Toggle(box);
    Boolean state;
    int     i;

    if ( box == &new_point_dialog )
        return;

    i = current + 1;
    if ( i == box->points.num_widgets )
        i = 0;
    while ( i != current )
    {
        XtVaGetValues(box->points.point_widgets[i], XtNtoggleState, &state,
                      NULL);
        if ( ! state )
            break;
        i = ( i == box->points.num_widgets - 1 ? 0 : i + 1 );
    }

    if ( i != current )
        XtVaSetValues(box->points.point_widgets[i], XtNtoggleState, TRUE, NULL);
}


static Boolean
Add_Plane_Done(int flags)
{
    return ( ( flags & HavePt1 ) && ( flags & HavePt2 ) && ( flags & HavePt3 ));
}

static Boolean
Add_Line_Done(int flags)
{
    return ( ( flags & HavePt1 ) && ( flags & HavePt2 ) );
}

static Boolean
Add_Point_Done(int flags)
{
    return ( flags & HavePt1 );
}


static Boolean
Add_Sphere_Done(int flags)
{
    return ( ( flags & HavePt1 ) && ( flags & HavePt2 ) );
}


static Boolean
Add_Circle_Done(int flags)
{
    return ( ( flags & HavePt1 ) && ( flags & HavePt2 ) && ( flags & HavePt3 ));
}

static Boolean
Add_RatioPt_Done(int flags)
{
    return ( ( flags & HavePt1 ) && ( flags & HavePt2 ) );
}

static Boolean
Add_Axis_Done(int flags)
{
    return ( ( flags & HavePt1 ) && ( flags & HavePt2 ) );
}


static Boolean
Add_Test_Done(AddPointDialogPtr box)
{
    if ( box == &new_plane_dialog )
        return Add_Plane_Done(box->flags);
    else if ( box == &new_line_dialog )
        return Add_Line_Done(box->flags);
    else if ( box == &new_point_dialog )
        return Add_Point_Done(box->flags);
    else if ( box == &new_sphere_dialog )
        return Add_Sphere_Done(box->flags);
    else if ( box == &new_circle_dialog )
        return Add_Circle_Done(box->flags);
    else if ( box == &new_ratiopt_dialog )
        return Add_RatioPt_Done(box->flags);
    else if ( box == &new_axis_dialog )
        return Add_Axis_Done(box->flags);

    return FALSE;
}

static Boolean
Add_Test_Point_Validity(ConstraintType con_type, int flags, Vector location,
                        int active)
{
    ConstraintData  data;
    Vector          orig_pt = points[active];
    Boolean         result;

    data.c_type = con_type;
    data.c_spec_flags = flags;
    data.c_radius = data.c_ratio = 1.0;
    points[active] = location;

    result = update_con_function[con_type](&data, points, &location);

    points[active] = orig_pt;
    return result;
}

static Boolean
Add_Any_Constraint(ConstraintPtr feature)
{
    EditInfoPtr info = Edit_Get_Info();
    int         i;

    feature->c_specs = New(ConstraintSpec, feature->c_num_specs);
    for ( i = 0 ; i < feature->c_num_specs ; i++ )
    {
        feature->c_specs[i] = specs[i];
        if ( Spec_Is_Dependent(specs[i].spec_type) )
            Add_Dependency(spec_object(specs + i), info->obj);
    }

    if ( ! update_con_function[feature->c_type](feature, points,
                                    &(info->features[origin_feature].world)) )
    {
        XBell(XtDisplay(main_window.shell), 0);
        return FALSE;
    }

    return TRUE;
}


static Boolean
Add_Plane_Constraint(ConstraintPtr feature, int flags)
{
    feature->c_type = plane_feature;
    feature->c_spec_flags = flags;
    if ( ( flags & TwoisNorm ) && ( flags & ThreeisNorm ) && (flags & HavePt4) )
        feature->c_num_specs = 4;
    else
        feature->c_num_specs = 3;

    return Add_Any_Constraint(feature);
}


static Boolean
Add_Line_Constraint(ConstraintPtr feature, int flags)
{
    feature->c_type = line_feature;
    feature->c_spec_flags = flags;
    if ( flags & HavePt3 )
        feature->c_num_specs = 3;
    else
        feature->c_num_specs = 2;

    return Add_Any_Constraint(feature);
}


static Boolean
Add_Point_Constraint(ConstraintPtr feature, int flags)
{
    feature->c_type = point_feature;
    feature->c_spec_flags = flags;
    feature->c_num_specs = 1;

    return Add_Any_Constraint(feature);
}


static Boolean
Add_Sphere_Constraint(ConstraintPtr feature, int flags)
{
    feature->c_type = sphere_feature;
    feature->c_spec_flags = flags;
    if ( flags & HavePt3 )
        feature->c_num_specs = 3;
    else
        feature->c_num_specs = 2;

    return Add_Any_Constraint(feature);
}

static Boolean
Add_Circle_Constraint(ConstraintPtr feature, int flags)
{
    feature->c_type = circle_feature;
    feature->c_spec_flags = flags;
    if ( ( flags & TwoisNorm ) && ( flags & ThreeisNorm ) && (flags & HavePt4) )
        feature->c_num_specs = 4;
    else
        feature->c_num_specs = 3;
    if ( flags & HavePt5 )
        feature->c_num_specs++;
    if ( flags & HavePt6 )
        feature->c_num_specs++;

    return Add_Any_Constraint(feature);
}

static Boolean
Add_Ratio_Point_Constraint(ConstraintPtr feature, int flags)
{
    feature->c_type = point_feature;
    feature->c_spec_flags = flags;
    feature->c_num_specs = 2;

    return Add_Any_Constraint(feature);
}

static Boolean
Add_Ratio_Plane_Constraint(ConstraintPtr feature, int flags)
{
    feature->c_type = plane_feature;
    feature->c_spec_flags = flags;
    feature->c_num_specs = 2;

    return Add_Any_Constraint(feature);
}


static Boolean
Add_Axis_Constraint(ConstraintPtr feature, int flags)
{
    feature->c_type = line_feature;
    feature->c_spec_flags = flags;
    if ( flags & HavePt3 )
        feature->c_num_specs = 3;
    else
        feature->c_num_specs = 2;

    return Add_Any_Constraint(feature);
}

static void
Add_Set_Point(AddPointDialogPtr box, int pt, ConstraintSpecPtr spec,
              Boolean ref_allowed)
{
    EditInfoPtr info = Edit_Get_Info();
    char        new_label[48];

    Add_Check_Specifiers(spec, info, ref_allowed);

    specs[pt] = *spec;
    points[pt] = Spec_Absolute(spec, select_center);

    sprintf(new_label, "%1.3g %1.3g %1.3g",
            points[pt].x, points[pt].y, points[pt].z);
    XtVaSetValues(box->points.point_widgets[pt], XtNlabel, new_label, NULL);

    Add_Set_Next_Toggle(box);
    XtSetSensitive(box->done, Add_Test_Done(box));
    Add_Set_Widths(box);
}


static int
Add_Plane_Constraint_Callback(ConstraintSpecPtr spec)
{
    int     active_pt = Add_Get_Set_Toggle(&new_plane_dialog);
    Boolean is_origin;
    Boolean allow_ref = FALSE;
    Vector  location = Spec_Absolute(spec, select_center);

    if ( Add_Plane_Done(new_plane_dialog.flags | ( 1 << active_pt ) ) &&
         ! Add_Test_Point_Validity(plane_feature,
                                   new_plane_dialog.flags | ( 1 << active_pt ),
                                   location, active_pt) )
    {
        XBell(XtDisplay(main_window.shell), 0);
        return IGNORE;
    }

    switch ( active_pt )
    {
        case 0:
            if ( new_plane_dialog.flags & HavePt1 )
            {
                XtVaGetValues(new_plane_dialog.points.point_widgets[0],
                              XtNextraState, &is_origin, NULL);
                if ( is_origin )
                    XtVaSetValues(new_plane_dialog.points.point_widgets[0],
                                  XtNextraState, FALSE, NULL);
                else
                    /* Deselect old one. */
                    Select_Deselect_Point(specs);
            }
            new_plane_dialog.flags |= HavePt1;
            break;
        case 1:
            if ( new_plane_dialog.flags & HavePt2 )
                Select_Deselect_Point(specs + 1);
            new_plane_dialog.flags |= HavePt2;
            if ( ( new_plane_dialog.flags & HavePt1 ) &&
                 specs[0].spec_type == feature_spec )
                allow_ref = TRUE;
            break;
        case 2:
            if ( new_plane_dialog.flags & HavePt3 )
                Select_Deselect_Point(specs + 2);
            new_plane_dialog.flags |= HavePt3;
            if ( ( new_plane_dialog.flags & HavePt1 ) &&
                 specs[0].spec_type == feature_spec )
                allow_ref = TRUE;
            break;
        case 3:
            if ( new_plane_dialog.flags & HavePt4 )
                Select_Deselect_Point(specs + 3);
            new_plane_dialog.flags |= HavePt4;
            if ( ( new_plane_dialog.flags & HavePt1 ) &&
                 specs[0].spec_type == feature_spec )
                allow_ref = TRUE;
            break;
    }

    Add_Set_Point(&new_plane_dialog, active_pt, spec, allow_ref);

    return SELECT;
}

/*  void
**  Add_Line_Constraint_Callback(int *points)
**  Adds a line as defined by the two points into select_verts.
*/
static int
Add_Line_Constraint_Callback(ConstraintSpecPtr spec)
{
    int     active_pt = Add_Get_Set_Toggle(&new_line_dialog);
    Boolean is_origin;
    Boolean allow_ref = FALSE;
    Vector  location = Spec_Absolute(spec, select_center);

    if ( Add_Line_Done(new_line_dialog.flags | ( 1 << active_pt ) ) &&
         ! Add_Test_Point_Validity(line_feature,
                                   new_line_dialog.flags | ( 1 << active_pt ),
                                   location, active_pt) )
    {
        XBell(XtDisplay(main_window.shell), 0);
        return IGNORE;
    }

    switch ( active_pt )
    {
        case 0:
            if ( new_line_dialog.flags & HavePt1 )
            {
                XtVaGetValues(new_line_dialog.points.point_widgets[0],
                              XtNextraState, &is_origin, NULL);
                if ( is_origin )
                    XtVaSetValues(new_line_dialog.points.point_widgets[0],
                                  XtNextraState, FALSE, NULL);
                else
                    /* Deselect old one. */
                    Select_Deselect_Point(specs);
            }
            new_line_dialog.flags |= HavePt1;
            break;
        case 1:
            if ( new_line_dialog.flags & HavePt2 )
                Select_Deselect_Point(specs + 1);
            new_line_dialog.flags |= HavePt2;
            if ( ( new_line_dialog.flags & HavePt1 ) &&
                 specs[0].spec_type == feature_spec )
                allow_ref = TRUE;
            break;
        case 2:
            if ( new_line_dialog.flags & HavePt3 )
                Select_Deselect_Point(specs + 2);
            new_line_dialog.flags |= HavePt3;
            if ( ( new_line_dialog.flags & HavePt1 ) &&
                 specs[0].spec_type == feature_spec )
                allow_ref = TRUE;
            break;
    }

    Add_Set_Point(&new_line_dialog, active_pt, spec, allow_ref);

    return SELECT;
}


/*  void
**  Add_Line_Constraint_Callback(int *points)
**  Adds the point indexed by *points.
*/
static int
Add_Point_Constraint_Callback(ConstraintSpecPtr spec)
{
    if ( new_point_dialog.flags & HavePt1 )
        Select_Deselect_Point(specs);
        
    new_point_dialog.flags |= HavePt1;

    Add_Set_Point(&new_point_dialog, 0, spec, FALSE);

    return SELECT;
}


/* Add a sphere constraint. First point is the center, second gives radius. */
static int
Add_Sphere_Constraint_Callback(ConstraintSpecPtr spec)
{
    EditInfoPtr info = Edit_Get_Info();
    int     active_pt = Add_Get_Set_Toggle(&new_sphere_dialog);
    Boolean is_origin;
    Vector  location = Spec_Absolute(spec, select_center);

    if ( Add_Sphere_Done(new_sphere_dialog.flags | ( 1 << active_pt ) ) &&
         ! Add_Test_Point_Validity(sphere_feature,
                                   new_sphere_dialog.flags | ( 1 << active_pt ),
                                   location, active_pt) )
    {
        XBell(XtDisplay(main_window.shell), 0);
        return IGNORE;
    }

    switch ( active_pt )
    {
        case 0:
            new_sphere_dialog.flags |= HavePt1;
            if ( new_sphere_dialog.flags & HavePt1 )
            {
                if ( info->current_feature != origin_feature )
                {
                    XtVaGetValues(new_sphere_dialog.points.point_widgets[0],
                                  XtNextraState, &is_origin, NULL);
                    if ( is_origin )
                        XtVaSetValues(new_sphere_dialog.points.point_widgets[0],
                                      XtNextraState, FALSE, NULL);
                }
                else
                    is_origin = FALSE;

                if ( is_origin )
                    XtVaSetValues(new_sphere_dialog.points.point_widgets[0],
                                  XtNextraState, FALSE, NULL);
                else
                    /* Deselect old one. */
                    Select_Deselect_Point(specs);
            }
            break;
        case 1:
            if ( new_sphere_dialog.flags & HavePt2 )
                Select_Deselect_Point(specs + 1);
            new_sphere_dialog.flags |= HavePt2;
            break;
        case 2:
            if ( new_sphere_dialog.flags & HavePt3 )
                Select_Deselect_Point(specs + 2);
            new_sphere_dialog.flags |= HavePt3;
            break;
    }

    Add_Set_Point(&new_sphere_dialog, active_pt, spec, FALSE);

    return SELECT;
}

static int
Add_Circle_Constraint_Callback(ConstraintSpecPtr spec)
{
    EditInfoPtr info = Edit_Get_Info();
    int     active_pt = Add_Get_Set_Toggle(&new_circle_dialog);
    Boolean is_origin;
    Vector  location = Spec_Absolute(spec, select_center);

    if ( Add_Circle_Done(new_circle_dialog.flags | ( 1 << active_pt ) ) &&
         ! Add_Test_Point_Validity(circle_feature,
                                   new_circle_dialog.flags | ( 1 << active_pt ),
                                   location, active_pt) )
    {
        XBell(XtDisplay(main_window.shell), 0);
        return IGNORE;
    }

    switch ( active_pt )
    {
        case 0:
            if ( new_circle_dialog.flags & HavePt1 )
            {
                if ( info->current_feature != origin_feature )
                {
                    XtVaGetValues(new_circle_dialog.points.point_widgets[0],
                                  XtNextraState, &is_origin, NULL);
                    if ( is_origin )
                        XtVaSetValues(new_circle_dialog.points.point_widgets[0],
                                      XtNextraState, FALSE, NULL);
                }
                else
                    is_origin = FALSE;

                if ( is_origin )
                    XtVaSetValues(new_circle_dialog.points.point_widgets[0],
                                  XtNextraState, FALSE, NULL);
                else
                    /* Deselect old one. */
                    Select_Deselect_Point(specs);
            }
            new_circle_dialog.flags |= HavePt1;
            break;
        case 1:
            if ( new_circle_dialog.flags & HavePt2 )
                Select_Deselect_Point(specs + 1);
            new_circle_dialog.flags |= HavePt2;
            break;
        case 2:
            if ( new_circle_dialog.flags & HavePt3 )
                Select_Deselect_Point(specs + 2);
            new_circle_dialog.flags |= HavePt3;
            break;
        case 3:
            if ( new_circle_dialog.flags & HavePt4 )
                Select_Deselect_Point(specs + 3);
            new_circle_dialog.flags |= HavePt4;
            break;
        case 4:
            if ( new_circle_dialog.flags & HavePt5 )
                Select_Deselect_Point(specs + 4);
            new_circle_dialog.flags |= HavePt5;
            break;
        case 5:
            if ( new_circle_dialog.flags & HavePt6 )
                Select_Deselect_Point(specs + 5);
            new_circle_dialog.flags |= HavePt6;
            break;
    }

    Add_Set_Point(&new_circle_dialog, active_pt, spec, FALSE);

    return SELECT;
}


static int
Add_RatioPt_Constraint_Callback(ConstraintSpecPtr spec)
{
    int     active_pt = Add_Get_Set_Toggle(&new_ratiopt_dialog);
    Vector  location = Spec_Absolute(spec, select_center);

    if ( Add_RatioPt_Done(new_ratiopt_dialog.flags | ( 1 << active_pt ) ) &&
         ! Add_Test_Point_Validity(point_feature,
                                  new_ratiopt_dialog.flags | ( 1 << active_pt ),
                                  location, active_pt) )
    {
        XBell(XtDisplay(main_window.shell), 0);
        return IGNORE;
    }

    switch ( active_pt )
    {
        case 0:
            if ( new_ratiopt_dialog.flags & HavePt1 )
                Select_Deselect_Point(specs);
            new_ratiopt_dialog.flags |= HavePt1;
            break;
        case 1:
            if ( new_ratiopt_dialog.flags & HavePt2 )
                Select_Deselect_Point(specs + 1);
            new_ratiopt_dialog.flags |= HavePt2;
            break;
    }

    Add_Set_Point(&new_ratiopt_dialog, active_pt, spec, FALSE);

    return SELECT;
}


static int
Add_Axis_Constraint_Callback(ConstraintSpecPtr spec)
{
    int     active_pt = Add_Get_Set_Toggle(&new_axis_dialog);
    Boolean is_origin;
    Vector  location = Spec_Absolute(spec, select_center);

    if ( Add_Axis_Done(new_axis_dialog.flags | ( 1 << active_pt ) ) &&
         ! Add_Test_Point_Validity(line_feature,
                                   new_axis_dialog.flags | ( 1 << active_pt ),
                                   location, active_pt) )
    {
        XBell(XtDisplay(main_window.shell), 0);
        return IGNORE;
    }

    switch ( active_pt )
    {
        case 0:
            if ( new_axis_dialog.flags & HavePt1 )
            {
                XtVaGetValues(new_axis_dialog.points.point_widgets[0],
                              XtNextraState, &is_origin, NULL);
                if ( is_origin )
                    XtVaSetValues(new_axis_dialog.points.point_widgets[0],
                                  XtNextraState, FALSE, NULL);
                else
                    /* Deselect old one. */
                    Select_Deselect_Point(specs);
            }
            new_axis_dialog.flags |= HavePt1;
            break;
        case 1:
            if ( new_axis_dialog.flags & HavePt2 )
                Select_Deselect_Point(specs + 1);
            new_axis_dialog.flags |= HavePt2;
            break;
        case 2:
            if ( new_axis_dialog.flags & HavePt3 )
                Select_Deselect_Point(specs + 2);
            new_axis_dialog.flags |= HavePt3;
            break;
    }

    Add_Set_Point(&new_axis_dialog, active_pt, spec, FALSE);

    return SELECT;
}


static int
Edit_Select_Point_Callback(ConstraintSpecPtr spec)
{
    if ( add_dialog_box == &new_plane_dialog )
        return Add_Plane_Constraint_Callback(spec);
    else if ( add_dialog_box == &new_line_dialog )
        return Add_Line_Constraint_Callback(spec);
    else if ( add_dialog_box == &new_point_dialog )
        return Add_Point_Constraint_Callback(spec);
    else if ( add_dialog_box == &new_sphere_dialog )
        return Add_Sphere_Constraint_Callback(spec);
    else if ( add_dialog_box == &new_circle_dialog )
        return Add_Circle_Constraint_Callback(spec);
    else if ( add_dialog_box == &new_ratiopt_dialog )
        return Add_RatioPt_Constraint_Callback(spec);
    else if ( add_dialog_box == &new_axis_dialog )
        return Add_Axis_Constraint_Callback(spec);

    return IGNORE;
}

void
Add_Extra_Callback(AddPointDialogPtr box, int index, Boolean state)
{
    static Boolean  last_was_err = FALSE;
    EditInfoPtr     info = Edit_Get_Info();
    char            new_label[48];
    Vector          diff;
    ConstraintSpec  last_spec;

    if ( last_was_err )
    {
        last_was_err = FALSE;
        return;
    }

    if ( ! state )
    {
        /* Unset flags etc. */
        box->flags &= ( AllFlags ^ HavePt1 );
        XtVaSetValues(box->points.point_widgets[index],
                      XtNlabel, "?? ?? ??",
                      XtNtoggleState, TRUE, NULL);
        XtSetSensitive(box->done, FALSE);
        Add_Set_Widths(box);
        return;
    }

    /* Remember old one. */
    last_spec = specs[0];

    if ( info->current_feature == origin_feature ||
         info->current_feature == major_feature ||
         info->current_feature == minor_feature )
    {
        sprintf(new_label, "%1.3g %1.3g %1.3g",
                info->features[origin_feature].world.x,
                info->features[origin_feature].world.y,
                info->features[origin_feature].world.z);
        points[0] = info->features[origin_feature].world;
        specs[0].spec_type = feature_spec;
        specs[0].spec_data = (void*)origin_feature;
        specs[0].spec_vector = info->features[origin_feature].world;
    }
    else if ( info->current_feature == scale_feature )
    {
        sprintf(new_label, "%1.3g %1.3g %1.3g",
                info->features[scale_feature].world.x,
                info->features[scale_feature].world.y,
                info->features[scale_feature].world.z);
        points[0] = info->features[scale_feature].world;
        specs[0].spec_type = feature_spec;
        specs[0].spec_data = (void*)scale_feature;;
        specs[0].spec_vector = info->features[scale_feature].world;
    }
    else if ( info->current_feature == radius_feature )
    {
        sprintf(new_label, "%1.3g %1.3g %1.3g",
                info->features[radius_feature].world.x,
                info->features[radius_feature].world.y,
                info->features[radius_feature].world.z);
        points[0] = info->features[radius_feature].world;
        specs[0].spec_type = feature_spec;
        specs[0].spec_data = (void*)radius_feature;
        specs[0].spec_vector = info->features[radius_feature].world;
    }

    if ( box == &new_line_dialog && ( new_line_dialog.flags & HavePt2 ) )
    {
        if ( VEqual(points[0], points[1], diff) )
        {
            XBell(XtDisplay(main_window.shell), 0);
            last_was_err = TRUE;
            XtVaSetValues(box->points.point_widgets[index],
                          XtNextraState, FALSE, NULL);
            return;
        }
    }
    else if ( box == &new_plane_dialog &&
              ( new_plane_dialog.flags & HavePt2 ) &&
              ( new_plane_dialog.flags & HavePt3 ) &&
              ! ( new_plane_dialog.flags & HavePt4 ) )
    {
        /* Check co-linearity. */
        if ( Points_Colinear_f(points[0], points[1], points[2]))
        {
            XBell(XtDisplay(main_window.shell), 0);
            last_was_err = TRUE;
            XtVaSetValues(box->points.point_widgets[index],
                          XtNextraState, FALSE, NULL);
            return;
        }
    }

    if ( box->flags & HavePt1 )
        Select_Deselect_Point(&last_spec);

    box->flags |= HavePt1;

    XtVaSetValues(box->points.point_widgets[index], XtNlabel, new_label, NULL);

    Add_Set_Next_Toggle(box);
    XtSetSensitive(box->done, Add_Test_Done(box));
    Add_Set_Widths(box);
}

void
Add_Plane_Plane_Pt_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    if ( new_plane_dialog.menu_hit == 1 )
    {
        new_plane_dialog.flags &= ( AllFlags ^ TwoisNorm );
        new_plane_dialog.flags |= TwoisPt;

        XtVaSetValues(new_plane_dialog.points.point_widgets[1],
                      XtNtitleLabel, "Point", NULL);
    }
    else
    {
        new_plane_dialog.flags &= ( AllFlags ^ ThreeisNorm );
        new_plane_dialog.flags |= ThreeisPt;

        XtVaSetValues(new_plane_dialog.points.point_widgets[2],
                      XtNtitleLabel, "Point", NULL);
    }

    XtSetSensitive(new_plane_dialog.done,
                   Add_Plane_Done(new_plane_dialog.flags));
    Add_Set_Widths(&new_plane_dialog);
}

void
Add_Plane_Normal_Pt_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    if ( new_plane_dialog.menu_hit == 1 )
    {
        new_plane_dialog.flags &= ( AllFlags ^ TwoisPt );
        new_plane_dialog.flags |= TwoisNorm;

        XtVaSetValues(new_plane_dialog.points.point_widgets[1],
                      XtNtitleLabel, "Normal", NULL);
    }
    else
    {
        new_plane_dialog.flags &= ( AllFlags ^ ThreeisPt );
        new_plane_dialog.flags |= ThreeisNorm;

        XtVaSetValues(new_plane_dialog.points.point_widgets[2],
                      XtNtitleLabel, "Normal", NULL);
    }

    XtSetSensitive(new_plane_dialog.done,
                   Add_Plane_Done(new_plane_dialog.flags));
    Add_Set_Widths(&new_plane_dialog);
}


void
Add_Sphere_Extra_Callback(AddPointDialogPtr box, int index, Boolean state)
{
    static Boolean  last_was_err = FALSE;
    EditInfoPtr info = Edit_Get_Info();
    char        new_label[48];
    ConstraintSpec  last_spec;

    if ( info->current_feature == origin_feature )
        return;

    if ( last_was_err )
    {
        last_was_err = FALSE;
        return;
    }

    if ( ! state )
    {
        /* Unset flags etc. */
        box->flags &= ( AllFlags ^ HavePt1 );
        XtVaSetValues(box->points.point_widgets[index],
                      XtNlabel, "?? ?? ??",
                      XtNtoggleState, TRUE, NULL);
        XtSetSensitive(box->done, FALSE);
        Add_Set_Widths(box);
        return;
    }

    /* Remember old one. */
    last_spec = specs[0];

    sprintf(new_label, "%1.3g %1.3g %1.3g",
            info->features[origin_feature].world.x,
            info->features[origin_feature].world.y,
            info->features[origin_feature].world.z);
    points[0] = info->features[origin_feature].world;
    specs[0].spec_type = feature_spec;
    specs[0].spec_data = (void*)origin_feature;
    specs[0].spec_vector = info->features[origin_feature].world;

    if ( box == &new_circle_dialog &&
         ( new_circle_dialog.flags & HavePt2 ) &&
         ( new_circle_dialog.flags & HavePt3 ) &&
         ! ( new_circle_dialog.flags & HavePt4 ) )
    {
        /* Check co-linearity. */
        if ( Points_Colinear_f(points[0], points[1], points[2]))
        {
            XBell(XtDisplay(main_window.shell), 0);
            last_was_err = TRUE;
            XtVaSetValues(box->points.point_widgets[index],
                          XtNextraState, FALSE, NULL);
            return;
        }
    }

    XtVaSetValues(box->points.point_widgets[index], XtNlabel, new_label, NULL);

    if ( box->flags & HavePt1 )
        Select_Deselect_Point(&last_spec);

    box->flags |= HavePt1;

    Add_Set_Next_Toggle(box);
    XtSetSensitive(box->done, Add_Test_Done(box));
    Add_Set_Widths(box);
}

void
Add_Circle_Plane_Pt_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    if ( new_circle_dialog.menu_hit == 1 )
    {
        new_circle_dialog.flags &= ( AllFlags ^ TwoisNorm );
        new_circle_dialog.flags |= TwoisPt;

        XtVaSetValues(new_circle_dialog.points.point_widgets[1],
                      XtNtitleLabel, "Point", NULL);
    }
    else
    {
        new_circle_dialog.flags &= ( AllFlags ^ ThreeisNorm );
        new_circle_dialog.flags |= ThreeisPt;

        XtVaSetValues(new_circle_dialog.points.point_widgets[2],
                      XtNtitleLabel, "Point", NULL);
    }

    XtSetSensitive(new_circle_dialog.done,
                   Add_Circle_Done(new_circle_dialog.flags));
    Add_Set_Widths(&new_circle_dialog);
}

void
Add_Circle_Normal_Pt_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    if ( new_circle_dialog.menu_hit == 1 )
    {
        new_circle_dialog.flags &= ( AllFlags ^ TwoisPt );
        new_circle_dialog.flags |= TwoisNorm;

        XtVaSetValues(new_circle_dialog.points.point_widgets[1],
                      XtNtitleLabel, "Normal", NULL);
    }
    else
    {
        new_circle_dialog.flags &= ( AllFlags ^ ThreeisPt );
        new_circle_dialog.flags |= ThreeisNorm;

        XtVaSetValues(new_circle_dialog.points.point_widgets[2],
                      XtNtitleLabel, "Normal", NULL);
    }

    XtSetSensitive(new_circle_dialog.done,
                   Add_Circle_Done(new_circle_dialog.flags));
    Add_Set_Widths(&new_circle_dialog);
}

void
Add_Axis_Extra_Callback(AddPointDialogPtr box, int index, Boolean state)
{
    static Boolean  last_was_err = FALSE;
    EditInfoPtr     info = Edit_Get_Info();
    char            new_label[48];
    ConstraintSpec  last_spec;
    Vector          diff;

    if ( last_was_err )
    {
        last_was_err = FALSE;
        return;
    }

    if ( ! state )
    {
        /* Unset flags etc. */
        box->flags &= ( AllFlags ^ HavePt1 );
        XtVaSetValues(box->points.point_widgets[index],
                      XtNlabel, "?? ?? ??",
                      XtNtoggleState, TRUE, NULL);
        XtSetSensitive(box->done, FALSE);
        Add_Set_Widths(box);
        return;
    }

    /* Remember old one. */
    last_spec = specs[0];

    sprintf(new_label, "%1.3g %1.3g %1.3g",
            info->features[origin_feature].world.x,
            info->features[origin_feature].world.y,
            info->features[origin_feature].world.z);
    points[0] = info->features[origin_feature].world;
    specs[0].spec_type = feature_spec;
    specs[0].spec_data = (void*)origin_feature;
    specs[0].spec_vector = info->features[origin_feature].world;

    if ( ( new_axis_dialog.flags & HavePt2 ) &&
         ( new_axis_dialog.flags & HavePt3 ) )
    {
        /* Check co-linearity. */
        if ( Points_Colinear_f(points[0], points[1], points[2]) )
        {
            XBell(XtDisplay(main_window.shell), 0);
            last_was_err = TRUE;
            XtVaSetValues(box->points.point_widgets[index],
                          XtNextraState, FALSE, NULL);
            return;
        }
    }
    else if ( new_axis_dialog.flags & HavePt2 )
    {
        if ( VEqual(points[0], points[1], diff) )
        {
            XBell(XtDisplay(main_window.shell), 0);
            return;
        }
    }

    XtVaSetValues(box->points.point_widgets[index], XtNlabel, new_label, NULL);

    if ( box->flags & HavePt1 )
        Select_Deselect_Point(&last_spec);

    box->flags |= HavePt1;

    Add_Set_Next_Toggle(box);
    XtSetSensitive(box->done, Add_Test_Done(box));
    Add_Set_Widths(box);
}

void
Add_Dialog_Cancel_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(add_dialog_box->shell);

    Edit_Cleanup_Selection(Edit_Get_Info(), TRUE);
}


void
Add_Dialog_Reset_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    AddPointDialogPtr   box = (AddPointDialogPtr)cl;
    int i;

    for ( i = 0 ; i < 6 ; i++ )
        if ( box->flags & ( 1 << i ) )
            Select_Deselect_Point(specs + i);

    if ( box == &new_plane_dialog || box == &new_circle_dialog )
        box->flags = ( TwoisPt | ThreeisPt );
    else if ( box == &new_line_dialog )
        box->flags = TwoisPt;
    else if ( box == &new_ratiopt_dialog )
    {
        if ( box->flags & RatioPoint )
            box->flags = RatioPoint;
        else
            box->flags = RatioPlane;
    }
    else
        box->flags = 0;

    Edit_Reset_Add_Dialog(box);
}

static void
Add_Dialog_Add_Constraint(EditInfoPtr info)
{
    ConstraintData  new_feature;
    Boolean     selected;
    Boolean     add = FALSE;

    /* Get the start status. */
    XtVaGetValues(add_dialog_box->select, XtNstate, &selected, NULL);

    new_feature.c_label = Strdup(add_dialog_box->name_string);
    new_feature.c_status = FALSE;
    new_feature.c_forced = FALSE;
    VNew(0, 0, 0, new_feature.c_vector);
    VNew(0, 0, 0, new_feature.c_point);
    new_feature.c_value = 0.0;
    new_feature.c_radius = 0.0;
    new_feature.c_ratio = 0.0;

    if ( add_dialog_box == &new_plane_dialog )
        add = Add_Plane_Constraint(&new_feature, add_dialog_box->flags);
    else if ( add_dialog_box == &new_line_dialog )
        add = Add_Line_Constraint(&new_feature, add_dialog_box->flags);
    else if ( add_dialog_box == &new_point_dialog )
        add = Add_Point_Constraint(&new_feature, add_dialog_box->flags);
    else if ( add_dialog_box == &new_sphere_dialog )
        add = Add_Sphere_Constraint(&new_feature, add_dialog_box->flags);
    else if ( add_dialog_box == &new_circle_dialog )
        add = Add_Circle_Constraint(&new_feature, add_dialog_box->flags);
    else if ( add_dialog_box == &new_ratiopt_dialog )
    {
        /* Defaults. */
        new_feature.c_radius = 1.0;
        new_feature.c_ratio = 1.0;
        /* Get values from the string. */
        sscanf(add_dialog_box->ratio_string, "%lf:%lf",
               &(new_feature.c_radius), &(new_feature.c_ratio));
        if ( new_ratiopt_dialog.flags & RatioPlane )
          add = Add_Ratio_Plane_Constraint(&new_feature, add_dialog_box->flags);
        else
          add = Add_Ratio_Point_Constraint(&new_feature, add_dialog_box->flags);
    }
    else if ( add_dialog_box == &new_axis_dialog )
        add = Add_Axis_Constraint(&new_feature, add_dialog_box->flags);
    else
        fprintf(stderr, "Add dialog error\n");

    /* Add the constraint. */
    if ( add )
        Add_Object_Constraint(&new_feature, info->current_feature, -1,selected);
}

void
Add_Dialog_Done_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    EditInfoPtr info = Edit_Get_Info();

    XtPopdown(add_dialog_box->shell);

    Edit_Cleanup_Selection(info, TRUE);

    if ( info->constr_index != -1 )
    {
        double  ratio_1, ratio_2;

        if ( add_dialog_box == &new_ratiopt_dialog )
        {
            /* Defaults. */
            ratio_1 = 1.0;
            ratio_2 = 1.0;
            /* Get values from the string. */
            sscanf(add_dialog_box->ratio_string, "%lf:%lf", &ratio_1, &ratio_2);
        }
        else
        {
            ratio_1 = ratio_2 = 0.0;
        }
        Edit_Modify_Constraint(info, info->constr_index, specs, points,
                               add_dialog_box->flags,
                               add_dialog_box->name_string,
                               ratio_1, ratio_2, TRUE);
    }
    else
        Add_Dialog_Add_Constraint(info);
}




void
Add_Object_Constraint(ConstraintPtr new, int feature, int index,
                      Boolean selected)
{
    EditInfoPtr info = Edit_Get_Info();
    int         i;

    if ( new->c_type == plane_feature && ( new->c_spec_flags & RatioPlane ) )
        feature_counts[ratio_plane_feature]++;
    else if ( new->c_type == point_feature &&
              ( new->c_spec_flags & RatioPoint ) )
        feature_counts[ratio_point_feature]++;
    else if ( new->c_type == line_feature && ( new->c_spec_flags & Axis ) )
        feature_counts[axis_feature]++;
    else
        feature_counts[new->c_type]++;

    if ( index == -1 )
    {
        index = info->obj->o_features[feature].num_constraints;
        Edit_Undo_Register_State(edit_add_op, feature, index);
    }

    info->obj->o_features[feature].constraints =
        More(info->obj->o_features[feature].constraints, ConstraintData,
             info->obj->o_features[feature].num_constraints + 1);

    /* Shuffle things up. */
    for ( i = info->obj->o_features[feature].num_constraints ; i > index ; i-- )
        info->obj->o_features[feature].constraints[i] =
            info->obj->o_features[feature].constraints[i-1];

    info->obj->o_features[feature].constraints[index] = *new;
    info->obj->o_features[feature].num_constraints++;


    if ( selected )
        new->c_status = TRUE;
    ConstraintBoxAddConstraint(feature_box[feature], new->c_label,
                               new->c_status || new->c_forced, index);

    Edit_Match_Widths();

    if ( selected )
    {
        /* Status will be set by Edit_Select_... */
        new->c_status = FALSE;
        Edit_Select_Constraint(info, index, feature, TRUE, FALSE, TRUE);
    }
}
