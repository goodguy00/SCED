/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/edit_extras.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: edit_extras.c,v $
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
**  edit_extras.c: Functions for manipulating the extra edit features,
**                  such as reference and origin points.
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <select_point.h>
#include <update.h>

static int  change_axis;


/*  void
**  Reference_Select_Callback(ConstraintSpecPtr *spec)
**  The select_point callback that actually changes the reference point.
*/
static int
Edit_Set_New_Reference(EditInfoPtr info, Vector new_scale)
{
    Edit_Undo_Register_State(edit_scaling_op, 0, 0);

    info->features[scale_feature].world = new_scale;

    Edit_Cleanup_Selection(info, TRUE);

    VSub(info->features[scale_feature].world,
         info->obj->o_world_verts[info->obj->o_num_vertices-1],
         info->obj->o_features[scale_feature].location);

    Edit_Update_Object_Constraints(
        info->obj->o_features[scale_feature].constraints,
        info->obj->o_features[scale_feature].num_constraints, info);

    /* Need to draw because Force changes the resultant, and affects drawing. */
    Draw_Point_Constraints(scale_feature, info->obj->o_features + scale_feature,
                            info->window, ViewNone, info, FALSE);
    Edit_Scale_Force_Constraints(info, FALSE);
    Constraint_Solve_System(info->obj,
                        info->obj->o_features[scale_feature].constraints,
                        info->obj->o_features[scale_feature].num_constraints,
                        &(info->features[scale_feature].resultant));
    Draw_Point_Constraints(scale_feature, info->obj->o_features + scale_feature,
                            info->window, CalcView, info, FALSE);

    if ( ! Edit_Force_Scale_Satisfaction(info, scale_feature, TRUE) )
    {
        Draw_Edit_Extras(info->window, ViewNone, info, FALSE);
        Draw_Point_Constraints(scale_feature,
                            info->obj->o_features + scale_feature,
                            info->window, ViewNone, info, FALSE);

        Draw_Edit_Extras(info->window, CalcView, info, FALSE);
        Draw_Point_Constraints(scale_feature,
                            info->obj->o_features + scale_feature,
                            info->window, CalcView, info, TRUE);
    }

    return IGNORE;
}


static int
Reference_Select_Callback(ConstraintSpecPtr spec)
{
    Edit_Set_New_Reference(Edit_Get_Info(), Spec_Absolute(spec, select_center));

    return IGNORE;
}


static int
Reference_Select_MidPt_Callback(ConstraintSpecPtr spec)
{
    static ConstraintSpec   spec_1;
    static Vector           vect_1;
    static int              num_have = 0;
    Vector                  vect_2;
    Vector                  diff;

    Vector  midpt;

    if ( num_have )
    {
        vect_2 = Spec_Absolute(spec, select_center);
        if ( VEqual(vect_1, vect_2, diff) )
        {
            Select_Deselect_Point(&spec_1);
            num_have = 0;
            return IGNORE;
        }

        VAdd(vect_1, vect_2, midpt);
        VScalarMul(midpt, 0.5, midpt);

        Edit_Set_New_Reference(Edit_Get_Info(), midpt);

        num_have = 0;
    }
    else
    {
        num_have = 1;
        spec_1 = *spec;
        vect_1 = Spec_Absolute(spec, select_center);

        return SELECT;
    }

    return IGNORE;
}


/*  void
**  Edit_Reference_Callback(Widget w, XtPointer cl, XtPointer ca)
**  Instigates a change reference callback by invoking the point selection
**  routines.
*/
void
Edit_Reference_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    EditInfoPtr info = Edit_Get_Info();
    Boolean midpt = ( cl ? TRUE : FALSE );

    XtPopdown(info->obj->o_features[scale_feature].base->change_shell);

    if ( info->modifying )
        Edit_Cancel_Modify(info);

    /* Prepare for selection. */
    Register_Select_Operation(info->window);
    info->selecting = TRUE;
    Edit_Sensitize_Buttons(FALSE, SenEditAll ^ SenEditUndo);
    select_window = info->window;
    select_highlight = FALSE;
    select_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];
    if ( midpt )
        select_callback = Reference_Select_MidPt_Callback;
    else
        select_callback = Reference_Select_Callback;
    select_instances = info->reference_available;
    allow_text_entry = FALSE;

    /* Draw all the reference points off. */
    Edit_Draw_Selection_Points(info);
}


static void
Edit_Set_New_Origin(EditInfoPtr info, Vector new_origin)
{
    Edit_Undo_Register_State(edit_origin_op, 0, 0);

    Edit_Cleanup_Selection(info, TRUE);

    info->features[origin_feature].world = new_origin;
    VSub(info->features[origin_feature].world,
         info->obj->o_world_verts[info->obj->o_num_vertices - 1],
         info->obj->o_features[origin_feature].location);

    Edit_Update_Object_Constraints(
        info->obj->o_features[origin_feature].constraints,
        info->obj->o_features[origin_feature].num_constraints, info);
    Constraint_Solve_System(info->obj,
            info->obj->o_features[origin_feature].constraints,
            info->obj->o_features[origin_feature].num_constraints,
            &(info->features[origin_feature].resultant));

    if ( info->obj->o_num_features > scale_feature )
    {
        /* Force scale constraints. Needs to be done now before
        ** trying to satisfy origin constraints.
        */
        Draw_Point_Constraints(scale_feature,
                            info->obj->o_features + scale_feature,
                            info->window, ViewNone, info, FALSE);
        if ( Edit_Scale_Force_Constraints(info, FALSE) )
            Draw_Point_Constraints(scale_feature,
                                info->obj->o_features + scale_feature,
                                info->window, CalcView, info, FALSE);
        else
            Draw_Point_Constraints(scale_feature,
                                info->obj->o_features + scale_feature,
                                info->window, ViewNone, info, FALSE);
    }

    if ( ! Edit_Force_Origin_Satisfaction(info, origin_feature, TRUE) )
    {
        Draw_All_Constraints(info->window, ViewNone, info, FALSE);
        Draw_Edit_Extras(info->window, ViewNone, info, FALSE);

        if ( info->obj->o_num_features > minor_feature )
        {
            Edit_Update_Object_Constraints(
                info->obj->o_features[major_feature].constraints,
                info->obj->o_features[major_feature].num_constraints, info);
            Edit_Update_Object_Constraints(
                info->obj->o_features[minor_feature].constraints,
                info->obj->o_features[minor_feature].num_constraints, info);
            Edit_Solve_Axis_System(info->obj,
                info->obj->o_features[major_feature].constraints,
                info->obj->o_features[major_feature].num_constraints,
                &(info->rotate_resultant));
        }

        if ( info->obj->o_num_features > scale_feature )
        {
            Edit_Update_Object_Constraints(
                info->obj->o_features[scale_feature].constraints,
                info->obj->o_features[scale_feature].num_constraints, info);
            Constraint_Solve_System(info->obj,
                info->obj->o_features[scale_feature].constraints,
                info->obj->o_features[scale_feature].num_constraints,
                &(info->features[scale_feature].resultant));
        }

        Draw_Edit_Extras(info->window, CalcView, info, FALSE);
        Draw_All_Constraints(info->window, CalcView, info, TRUE);
    }
}


/*  void
**  Origin_Select_Callback(ConstraintSpecPtr spec)
**  The select point callback which actually changes the origin point.
*/
static int
Origin_Select_Callback(ConstraintSpecPtr spec)
{
    Edit_Set_New_Origin(Edit_Get_Info(), Spec_Absolute(spec, select_center));

    return IGNORE;
}


static int
Origin_Select_MidPt_Callback(ConstraintSpecPtr spec)
{
    static ConstraintSpec   spec_1;
    static Vector           vect_1;
    static int              num_have = 0;
    Vector                  vect_2;
    Vector                  diff;

    Vector  midpt;

    if ( num_have )
    {
        vect_2 = Spec_Absolute(spec, select_center);
        if ( VEqual(vect_1, vect_2, diff) )
        {
            Select_Deselect_Point(&spec_1);
            num_have = 0;
            return IGNORE;
        }

        VAdd(vect_1, vect_2, midpt);
        VScalarMul(midpt, 0.5, midpt);

        Edit_Set_New_Origin(Edit_Get_Info(), midpt);

        num_have = 0;
    }
    else
    {
        num_have = 1;
        spec_1 = *spec;
        vect_1 = Spec_Absolute(spec, select_center);

        return SELECT;
    }

    return IGNORE;
}


/*  void
**  Edit_Origin_Callback(Widget w, XtPointer cl, XtPointer ca)
**  The callback invoked to change the origin point. All it does is prepare
**  the view for point selection.
*/
void
Edit_Origin_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    EditInfoPtr info = Edit_Get_Info();
    Boolean midpt = ( cl ? TRUE : FALSE );

    XtPopdown(info->obj->o_features[origin_feature].base->change_shell);

    if ( info->modifying )
        Edit_Cancel_Modify(info);

    /* Prepare for selection. */
    Register_Select_Operation(info->window);
    info->selecting = TRUE;
    Edit_Sensitize_Buttons(FALSE, SenEditAll ^ SenEditUndo);
    select_window = info->window;
    select_highlight = FALSE;
    select_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];

    if ( midpt )
        select_callback = Origin_Select_MidPt_Callback;
    else
        select_callback = Origin_Select_Callback;
    select_instances = info->all_available;
    allow_text_entry = TRUE;
    prompt_offset = TRUE;

    /* Draw all the selection points on. */
    Edit_Draw_Selection_Points(info);
}


static void
Edit_Set_New_Radius(EditInfoPtr info, Vector new_radius)
{
    Edit_Undo_Register_State(edit_radius_op, 0, 0);

    Edit_Cleanup_Selection(info, TRUE);

    info->features[radius_feature].world = new_radius;
    VSub(info->features[radius_feature].world,
         info->obj->o_world_verts[info->obj->o_num_vertices - 1],
         info->obj->o_features[radius_feature].location);

    Edit_Update_Object_Constraints(
        info->obj->o_features[radius_feature].constraints,
        info->obj->o_features[radius_feature].num_constraints, info);
    Constraint_Solve_System(info->obj,
        info->obj->o_features[radius_feature].constraints,
        info->obj->o_features[radius_feature].num_constraints,
        &(info->features[radius_feature].resultant));
    if ( ! Edit_Force_Radius_Satisfaction(info, radius_feature, TRUE) )
    {
        Draw_Point_Constraints(radius_feature,
                            info->obj->o_features + radius_feature,
                            info->window, ViewNone, info, FALSE);
        Draw_Edit_Extras(info->window, ViewNone, info, FALSE);
        Draw_Edit_Extras(info->window, CalcView, info, FALSE);
        Draw_Point_Constraints(radius_feature,
                            info->obj->o_features + radius_feature,
                            info->window, CalcView, info, TRUE);
    }
}


static int
Radius_Select_Callback(ConstraintSpecPtr spec)
{
    Edit_Set_New_Radius(Edit_Get_Info(), Spec_Absolute(spec, select_center));
    return IGNORE;
}


static int
Radius_Select_MidPt_Callback(ConstraintSpecPtr spec)
{
    static ConstraintSpec   spec_1;
    static Vector           vect_1;
    static int              num_have = 0;
    Vector                  vect_2;
    Vector                  diff;

    Vector  midpt;

    if ( num_have )
    {
        vect_2 = Spec_Absolute(spec, select_center);
        if ( VEqual(vect_1, vect_2, diff) )
        {
            Select_Deselect_Point(&spec_1);
            num_have = 0;
            return IGNORE;
        }

        VAdd(vect_1, vect_2, midpt);
        VScalarMul(midpt, 0.5, midpt);

        Edit_Set_New_Radius(Edit_Get_Info(), midpt);

        num_have = 0;
    }
    else
    {
        num_have = 1;
        spec_1 = *spec;
        vect_1 = Spec_Absolute(spec, select_center);

        return SELECT;
    }

    return IGNORE;
}


/*  void
**  Edit_Radius_Callback(Widget w, XtPointer cl, XtPointer ca)
**  The callback invoked to change the Radius point. All it does is prepare
**  the view for point selection.
*/
void
Edit_Radius_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    EditInfoPtr info = Edit_Get_Info();
    Boolean midpt = ( cl ? TRUE : FALSE );

    XtPopdown(info->obj->o_features[radius_feature].base->change_shell);

    if ( info->modifying )
        Edit_Cancel_Modify(info);

    /* Prepare for selection. */
    Register_Select_Operation(info->window);
    info->selecting = TRUE;
    Edit_Sensitize_Buttons(FALSE, SenEditAll ^ SenEditUndo);
    select_window = info->window;
    select_highlight = FALSE;
    select_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];

    if ( midpt )
        select_callback = Radius_Select_MidPt_Callback;
    else
        select_callback = Radius_Select_Callback;
    select_instances = info->reference_available;
    allow_text_entry = TRUE;
    prompt_offset = TRUE;

    /* Draw all the selection points on. */
    Edit_Draw_Selection_Points(info);
}


static void
Edit_Change_Axis(EditInfoPtr info, Vector new_dir)
{
    double  temp_d;
    Vector  cross1, cross2;

    Edit_Undo_Register_State(edit_axis_op, 0, 0);

    Edit_Cleanup_Selection(info, TRUE);

    switch ( change_axis )
    {
        case MAJOR_AXIS:
            if ( VZero(new_dir) )
                return;
            VUnit(new_dir, temp_d, new_dir);
            VCross(new_dir, info->axes.y, cross1);
            if ( VZero(cross1) )
            {
                /* New axis and minor axis are parallel. */
                /* Set minor axis to be old major axis. */
                info->axes.y = info->axes.x;
                info->axes.x = new_dir;
                VCross(new_dir, info->axes.y, cross1);
                VUnit(cross1, temp_d, info->axes.z);
            }
            VCross(new_dir, cross1, cross2);
            info->axes.x = new_dir;
            VUnit(cross1, temp_d, info->axes.z);
            VUnit(cross2, temp_d, info->axes.y);
            break;
        case MINOR_AXIS:
            VCross(info->axes.x, new_dir, cross1);
            if ( VZero(cross1) )
                cross1 = info->axes.z;
            VCross(cross1, info->axes.x, cross2);
            VUnit(cross1, temp_d, info->axes.z);
            VUnit(cross2, temp_d, info->axes.y);
            break;
    }

    /* Change the body axes. */
    info->obj->o_features[major_feature].location = info->axes.x;
    info->obj->o_features[minor_feature].location = info->axes.y;

    /* Recalc the inverse. */
    info->axes_inverse = MInvert(&(info->axes));

    /* Redo alignment. */
    Edit_Update_Object_Constraints(
        info->obj->o_features[major_feature].constraints,
        info->obj->o_features[major_feature].num_constraints, info);
    Edit_Update_Object_Constraints(
        info->obj->o_features[minor_feature].constraints,
        info->obj->o_features[minor_feature].num_constraints, info);
    Edit_Solve_Axis_System(info->obj,
        info->obj->o_features[major_feature].constraints,
        info->obj->o_features[major_feature].num_constraints,
        &(info->rotate_resultant));
    if ( ! Edit_Force_Alignment_Satisfaction(info, major_feature, TRUE) )
    {
        Draw_All_Constraints(info->window, ViewNone, info, FALSE);
        Draw_Edit_Extras(info->window, ViewNone, info, FALSE);

        Edit_Update_Object_Constraints(
            info->obj->o_features[origin_feature].constraints,
            info->obj->o_features[origin_feature].num_constraints, info);
        Edit_Scale_Force_Constraints(info, FALSE);
        Constraint_Solve_System(info->obj,
            info->obj->o_features[origin_feature].constraints,
            info->obj->o_features[origin_feature].num_constraints,
            &(info->features[origin_feature].resultant));
        if ( info->obj->o_num_features > scale_feature )
        {
            Edit_Update_Object_Constraints(
                info->obj->o_features[scale_feature].constraints,
                info->obj->o_features[scale_feature].num_constraints, info);
            Edit_Scale_Force_Constraints(info, FALSE);
            Constraint_Solve_System(info->obj,
                info->obj->o_features[scale_feature].constraints,
                info->obj->o_features[scale_feature].num_constraints,
                &(info->features[scale_feature].resultant));
        }

        Draw_Edit_Extras(info->window, CalcView, info, FALSE);
        Draw_All_Constraints(info->window, CalcView, info, TRUE);
    }
}


static int
Edit_Select_1_Axis_Callback(ConstraintSpecPtr spec)
{
    EditInfoPtr info = Edit_Get_Info();
    Vector  temp_v;
    Vector  new_dir;

    /* Work out the new direction. */
    temp_v = Spec_Absolute(spec, select_center);
    VSub(temp_v, info->features[origin_feature].world, new_dir);

    if ( VZero(new_dir) )
    {
        XBell(XtDisplay(main_window.shell), 0);
        return IGNORE;
    }

    Edit_Change_Axis(Edit_Get_Info(), new_dir);

    return IGNORE;
}

static int
Edit_Select_2_Axis_Callback(ConstraintSpecPtr spec)
{
    static ConstraintSpec   spec_1;
    static Vector           vect_1;
    static int              num_have = 0;
    Vector                  vect_2;
    Vector                  new_dir;

    if ( num_have )
    {
        vect_2 = Spec_Absolute(spec, select_center);
        VSub(vect_2, vect_1, new_dir);
        if ( VZero(new_dir) )
        {
            Select_Deselect_Point(&spec_1);
            num_have = 0;
            return IGNORE;
        }

        Edit_Change_Axis(Edit_Get_Info(), new_dir);

        num_have = 0;
    }
    else
    {
        num_have = 1;
        spec_1 = *spec;
        vect_1 = Spec_Absolute(spec, select_center);

        return SELECT;
    }

    return IGNORE;
}


void
Edit_Change_Axis_Initiate(EditInfoPtr info)
{
    if ( info->modifying )
        Edit_Cancel_Modify(info);

    /* Prepare for selection. */
    Register_Select_Operation(info->window);
    info->selecting = TRUE;
    Edit_Sensitize_Buttons(FALSE, SenEditAll ^ SenEditUndo);
    select_window = info->window;
    select_highlight = FALSE;
    select_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];
    select_instances = info->all_available;
    allow_text_entry = TRUE;
    prompt_offset = TRUE;

    /* Draw all the selection points on. */
    Edit_Draw_Selection_Points(info);
}

void
Edit_Change_Axis_1_Callback(Widget widg, XtPointer cl, XtPointer ca)
{
    EditInfoPtr info = Edit_Get_Info();

    change_axis = (int)cl;

    if ( change_axis == MAJOR_AXIS )
	XtPopdown(info->obj->o_features[major_feature].base->change_shell);
    if ( change_axis == MINOR_AXIS )
	XtPopdown(info->obj->o_features[minor_feature].base->change_shell);

    Edit_Change_Axis_Initiate(info);

    select_callback = Edit_Select_1_Axis_Callback;
}



void
Edit_Change_Axis_2_Callback(Widget widg, XtPointer cl, XtPointer ca)
{
    EditInfoPtr info = Edit_Get_Info();

    change_axis = (int)cl;

    if ( change_axis == MAJOR_AXIS )
	XtPopdown(info->obj->o_features[major_feature].base->change_shell);
    if ( change_axis == MINOR_AXIS )
	XtPopdown(info->obj->o_features[minor_feature].base->change_shell);

    Edit_Change_Axis_Initiate(info);

    select_callback = Edit_Select_2_Axis_Callback;
}


void
Edit_Calculate_Extras(EditInfoPtr info)
{
}
