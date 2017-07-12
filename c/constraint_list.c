/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/constraint_list.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: constraint_list.c,v $
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
**  constraint_box.c : Functions top manage a constraint selection box.
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <add_constraint.h>
#include <ConstraintBox.h>


static Boolean
Edit_Modify_Select(EditInfoPtr info, int box, int index)
{
    ConstraintPtr   constraint;

    Edit_Cancel_Modify(info);

    if ( info->obj->o_features[box].base->num_defaults > index )
    {
        XBell(XtDisplay(info->window->shell), 0);
        return FALSE;
    }

    info->current_feature = box;
    info->constr_index = index;

    /* Save the Undo info. */
    constraint = info->obj->o_features[box].constraints + index;

    /* Reset the toggle back. */
    ConstraintBoxSetConstraintState(feature_box[box], index,
                                    constraint->c_status, FALSE);

    Add_Modify_Constraint(info, constraint);

    return TRUE;
}


void
Edit_Remove_Constraint(EditInfoPtr info, int box, int index, Boolean save_state)
{
    if ( save_state )
        Edit_Undo_Register_State(edit_remove_op, box, index);
    Remove_Constraint(info->obj, &(info->obj->o_features[box].constraints),
                      &(info->obj->o_features[box].num_constraints), index);

    ConstraintBoxRemoveConstraint(feature_box[box], index);
    Edit_Match_Widths();
}



void
Edit_Select_Constraint_Callback(Widget box, XtPointer cl_data, XtPointer ca)
{
    EditInfoPtr info = Edit_Get_Info();
    Boolean     new_state = ((ConstraintBoxData*)ca)->state;
    int         index = ((ConstraintBoxData*)ca)->index;
    int         box_index;

    for ( box_index = 0 ;
          box_index < pt_last_feature && feature_box[box_index] != box ;
          box_index++ );

    if ( info->modifying && Edit_Modify_Select(info, box_index, index) )
        return;

    Edit_Select_Constraint(info, index, box_index, new_state, FALSE, TRUE);
}

void
Edit_Select_Constraint(EditInfoPtr info, int index, int num,
                       Boolean new_state, Boolean forced, Boolean draw)
{
    if ( info->obj->o_features[num].base->check_forced )
    {
        if ( ! forced )
        {
            if ( info->obj->o_features[num].constraints[index].c_status )
            {
                if ( draw )
                    Edit_Undo_Register_State(edit_deselect_op, num, index);
                info->obj->o_features[num].constraints[index].c_status = FALSE;
                if ( index < 3 )
                    new_state =
                        info->obj->o_features[num].constraints[index].c_forced;
            }
            else
            {
                if ( draw )
                    Edit_Undo_Register_State(edit_select_op, num, index);
                info->obj->o_features[num].constraints[index].c_status = TRUE;
                new_state = TRUE;
            }
        }
        else
            new_state = new_state ||
                        info->obj->o_features[num].constraints[index].c_status;
        ConstraintBoxSetConstraintState(feature_box[num], index, new_state,
                                        FALSE);
    }
    else
    {
        if ( draw )
            Edit_Undo_Register_State(
                ( new_state ? edit_select_op : edit_deselect_op), num, index);
        info->obj->o_features[num].constraints[index].c_status = new_state;
    }

    /* Draw the old constraint feedback off. */
    if ( draw )
        info->obj->o_features[num].base->draw_con_func(num,
                                    info->obj->o_features + num,
                                    info->window, ViewNone, info, FALSE);

    /* Solve the constraint system. */
    info->obj->o_features[num].base->solve_function(info->obj,
                                    info->obj->o_features[num].constraints,
                                    info->obj->o_features[num].num_constraints,
                                    &(info->features[num].resultant));

    /* Draw it back on. */
    if ( draw )
        info->obj->o_features[num].base->draw_con_func(num,
                                    info->obj->o_features + num,
                                    info->window, CalcView, info, TRUE);

    /* Force constraint satisfaction. */
    info->obj->o_features[num].base->satisfy_function(info, num, FALSE);
}


static void
Modify_Constraint(EditInfoPtr info, ConstraintPtr constraint,
                  ConstraintSpecPtr specs, VectorPtr points, int flags,
                  double r1, double r2)
{
    ConstraintSpecPtr   orig_specs;
    double              orig_r1, orig_r2;
    int			orig_flags;
    int i;

    /* Remove dependencies. */
    Constraint_Manipulate_Specs(constraint, info->obj, NULL, NULL, 0,
                                Dependencies_Remove_Object);

    /* Count the number of specs. */
    orig_specs = constraint->c_specs;
    orig_flags = constraint->c_spec_flags;
    constraint->c_num_specs = 0;
    for ( i = 0 ; i < MAX_NUM_SPECS ; i++ )
        if ( flags & ( 1 << i ) )
            constraint->c_num_specs++;

    constraint->c_specs = New(ConstraintSpec, constraint->c_num_specs);
    for ( i = 0 ; i < constraint->c_num_specs ; i++ )
    {
        constraint->c_specs[i] = specs[i];
        if ( Spec_Is_Dependent(specs[i].spec_type) )
            Add_Dependency(spec_object(specs + i), info->obj);
    }

    if ( flags & ( RatioPlane | RatioPoint ) )
    {
        orig_r1 = constraint->c_radius;
        orig_r2 = constraint->c_ratio;
        constraint->c_radius = r1;
        constraint->c_ratio = r2;
    }
    constraint->c_spec_flags = flags;

    if ( update_con_function[constraint->c_type](constraint, points,
                                      &(info->features[origin_feature].world)) )
        free(orig_specs);
    else
    {
        free(constraint->c_specs);
        constraint->c_specs = orig_specs;
        constraint->c_radius = orig_r1;
        constraint->c_ratio = orig_r2;
	constraint->c_spec_flags = orig_flags;
        update_con_function[constraint->c_type](constraint, points,
                                    &(info->features[origin_feature].world));
    }
}

void
Edit_Modify_Constraint(EditInfoPtr info, int index, ConstraintSpecPtr specs,
                       VectorPtr points, int flags, char *name, double r1,
                       double r2, Boolean save_undo)
{
    ConstraintPtr   constraint;

    /* Save undo info. */
    if ( save_undo )   
        Edit_Undo_Register_State(edit_modify_op, info->current_feature, index);

    /* Do the modification. */
    constraint =
        info->obj->o_features[info->current_feature].constraints + index;
    Modify_Constraint(info, constraint, specs, points, flags, r1, r2);

    /* Draw old constraints off. */
    info->obj->o_features[info->current_feature].base->draw_con_func(
        info->current_feature, info->obj->o_features + info->current_feature,
        info->window, ViewNone, info, TRUE);

    /* Solve the system of constraints. */
    info->obj->o_features[info->current_feature].base->solve_function(
        info->obj,
        info->obj->o_features[info->current_feature].constraints,
        info->obj->o_features[info->current_feature].num_constraints,
        &(info->features[info->current_feature].resultant));

    /* Draw new constraints on. */
    info->obj->o_features[info->current_feature].base->draw_con_func(
        info->current_feature, info->obj->o_features + info->current_feature,
        info->window, CalcView, info, TRUE);

    /* Force constraint satisfaction. */
    info->obj->o_features[info->current_feature].base->satisfy_function(info,
                                                info->current_feature, FALSE);

    /* Set the label. */
    if ( strcmp(name, constraint->c_label) )
    {
        free(constraint->c_label);
        constraint->c_label = Strdup(name);
        ConstraintBoxSetConstraintLabel(feature_box[info->current_feature],
                                        index, name);
    }

    Edit_Match_Widths();
}
