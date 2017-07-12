/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/edit_undo.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: edit_undo.c,v $
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
**  edit_undo.c: Functions for edit undo and redo.
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <update.h>
#include <ConstraintBox.h>

typedef struct _EditStateType {
    EditOpType      edit_op;
    FeatureType     op_type;
    Transformation  saved_current;
    Vector          *saved_features;
    Matrix          saved_axes;
    double          saved_radius;
    Vector          saved_controls[16];
    int             con_index;
    ConstraintPtr   con_feature;
    struct _EditStateType   *next;
    } EditStateType, *EditStatePtr;

static void Edit_Undo_Transformation_Op(EditStatePtr);
static void Edit_Undo_Select_Op(EditStatePtr, Boolean);
static void Edit_Undo_Add_Op(EditStatePtr);
static void Edit_Undo_Remove_Op(EditStatePtr);
static void Edit_Undo_Modify_Op(EditStatePtr);
static void Free_Stack_Elmt(EditStatePtr);


void
Edit_Undo(EditInfoPtr info)
{
    EditStatePtr    temp;

    if ( ! info->undo_stack )
    {
        XBell(XtDisplay(main_window.shell), 0);
        return;
    }

    switch ( info->undo_stack->edit_op )
    {
        case edit_drag_op:
        case edit_scaling_op:
        case edit_origin_op:
        case edit_axis_op:
        case edit_radius_op:
            Edit_Undo_Transformation_Op(info->undo_stack);
            break;
        case edit_select_op:
            Edit_Undo_Select_Op(info->undo_stack, FALSE);
            break;
        case edit_deselect_op:
            Edit_Undo_Select_Op(info->undo_stack, TRUE);
            break;
        case edit_add_op:
            Edit_Undo_Add_Op(info->undo_stack);
            break;
        case edit_remove_op:
            Edit_Undo_Remove_Op(info->undo_stack);
            break;
        case edit_modify_op:
            Edit_Undo_Modify_Op(info->undo_stack);
            break;
    }

    /* Put the first thing on the undo stack onto the redo stack. */
    temp = info->undo_stack;
    info->undo_stack = info->undo_stack->next;
    temp->next = info->redo_stack;
    info->redo_stack = temp;
}


void
Edit_Redo(Widget w, XtPointer cl, XtPointer ca)
{
    EditInfoPtr     info = Edit_Get_Info();
    EditStatePtr    temp;

    if ( ! info->redo_stack )
    {
        XBell(XtDisplay(main_window.shell), 0);
        return;
    }

    switch ( info->redo_stack->edit_op )
    {
        case edit_drag_op:
        case edit_scaling_op:
        case edit_origin_op:
        case edit_axis_op:
        case edit_radius_op:
            Edit_Undo_Transformation_Op(info->redo_stack);
            break;
        case edit_select_op:
            Edit_Undo_Select_Op(info->redo_stack, FALSE);
            break;
        case edit_deselect_op:
            Edit_Undo_Select_Op(info->redo_stack, TRUE);
            break;
        case edit_add_op:
            Edit_Undo_Add_Op(info->redo_stack);
            break;
        case edit_remove_op:
            Edit_Undo_Remove_Op(info->redo_stack);
            break;
        case edit_modify_op:
            Edit_Undo_Modify_Op(info->redo_stack);
            break;
    }

    /* Put the first thing on the redo stack onto the undo stack. */
    temp = info->redo_stack;
    info->redo_stack = info->redo_stack->next;
    temp->next = info->undo_stack;
    info->undo_stack = temp;
}


ConstraintPtr
Constraint_Copy(ConstraintPtr dest, ConstraintPtr src)
{
    int i;

    if ( ! dest )
        dest = New(ConstraintData, 1);

    *dest = *src;
    dest->c_label = Strdup(src->c_label);
    dest->c_specs = New(ConstraintSpec, src->c_num_specs);
    for ( i = 0 ; i < src->c_num_specs ; i++ )
        dest->c_specs[i] = src->c_specs[i];

    return dest;
}

void
Constraint_Free(ConstraintPtr victim)
{
    free(victim->c_label);
    if ( victim->c_num_specs )
        free(victim->c_specs);
    free(victim);
}

void
Edit_Undo_Register_State(EditOpType type, FeatureType sub_type, int index)
{
    EditStatePtr    new_elmt = New(EditStateType, 1);
    EditInfoPtr     info = Edit_Get_Info();
    EditStatePtr    temp;
    int             i;

    /* If there's anything on the redo list, trash it. Can't redo
    ** once another thing has been done.
    */
    while ( info->redo_stack )
    {
        temp = info->redo_stack;
        info->redo_stack = info->redo_stack->next;
        Free_Stack_Elmt(temp);
    }
    info->redo_stack = NULL;


    new_elmt->edit_op = type;
    new_elmt->op_type = sub_type;

    new_elmt->saved_current = info->obj->o_transform;
    if ( Obj_Is_Torus(info->obj) )
        new_elmt->saved_radius = torus_part(info->obj)->major_radius;
    else
        new_elmt->saved_radius = 0.0;
    i = 0;
    if ( Obj_Is_Control(info->obj) )
        for ( ; i < control_part(info->obj)->num_control_verts ; i++ )
            new_elmt->saved_controls[i] =
                control_part(info->obj)->control_verts[i];
    for ( ; i < 16 ; i++ )
        VNew(0, 0, 0, new_elmt->saved_controls[i]);

    new_elmt->saved_features = New(Vector, info->obj->o_num_features);
    for ( i = 0 ; i < info->obj->o_num_features ; i++ )
        new_elmt->saved_features[i] = info->features[i].world;
    new_elmt->saved_axes = info->axes;

    new_elmt->con_index = index;

    if ( type == edit_remove_op || type == edit_modify_op )
        new_elmt->con_feature =
            Constraint_Copy(NULL, info->obj->o_features[sub_type].constraints +
                                    index);

    new_elmt->next = info->undo_stack;
    info->undo_stack = new_elmt;
}


static void
Edit_Undo_Transformation_Op(EditStatePtr state_elmt)
{
    EditInfoPtr info = Edit_Get_Info();
    EditStateType   redo_state;
    int             i;

    /* Remember where we are now for redo. */
    redo_state.saved_current = info->obj->o_transform;
    redo_state.saved_features = New(Vector, info->obj->o_num_features);
    for ( i = 0 ; i < info->obj->o_num_features ; i++ )
        redo_state.saved_features[i] = info->features[i].world;
    redo_state.saved_axes = info->axes;
    if ( Obj_Is_Torus(info->obj) )
        redo_state.saved_radius = torus_part(info->obj)->major_radius;
    else
        redo_state.saved_radius = 0.0;
    i = 0;
    if ( Obj_Is_Control(info->obj) )
        for ( ; i < control_part(info->obj)->num_control_verts ; i++ )
            redo_state.saved_controls[i] =
                control_part(info->obj)->control_verts[i];
    for ( ; i < 16 ; i++ )
        VNew(0, 0, 0, redo_state.saved_controls[i]);

    /* Put everything back the way it was. */
    info->obj->o_transform = state_elmt->saved_current;
    for ( i = 0 ; i < info->obj->o_num_features ; i++ )
        info->features[i].world = state_elmt->saved_features[i];
    info->axes = state_elmt->saved_axes;
    info->axes_inverse = MInvert(&(info->axes));
    if ( Obj_Is_Torus(info->obj) )
        torus_part(info->obj)->major_radius = state_elmt->saved_radius;
    if ( Obj_Is_Control(info->obj) )
        for ( i = 0 ; i < control_part(info->obj)->num_control_verts ; i++ )
            control_part(info->obj)->control_verts[i] =
                state_elmt->saved_controls[i];

    /* Recalc all the vertices and normals. */
    Edit_Transform_Vertices(&(info->obj->o_transform), info);
    Edit_Transform_Normals(&(info->obj->o_transform), info);

    /* Save the body stuff. */
    for ( i = 0 ; i < info->obj->o_num_features ; i++ )
        VSub(info->features[i].world,
             info->obj->o_world_verts[info->obj->o_num_vertices - 1],
             info->obj->o_features[i].location);
    if ( info->obj->o_num_features > minor_feature )
    {
        info->obj->o_features[major_feature].location = info->axes.x;
        info->obj->o_features[minor_feature].location = info->axes.y;
    }

    /* Rework all the constraints. */
    Edit_Update_Constraints(info);
    Constraint_Solve_System(info->obj,
                        info->obj->o_features[origin_feature].constraints,
                        info->obj->o_features[origin_feature].num_constraints,
                        &(info->features[origin_feature].resultant));
    if ( info->obj->o_num_features > scale_feature )
    {
        Draw_Point_Constraints(scale_feature,
                        info->obj->o_features + scale_feature, info->window,
                        ViewNone, info, FALSE);
        Edit_Scale_Force_Constraints(info, FALSE);
        Constraint_Solve_System(info->obj,
                        info->obj->o_features[scale_feature].constraints,
                        info->obj->o_features[scale_feature].num_constraints,
                        &(info->features[scale_feature].resultant));
        Draw_Point_Constraints(scale_feature,
                        info->obj->o_features + scale_feature, info->window,
                        CalcView, info, FALSE);
    }
    if ( info->obj->o_num_features > minor_feature )
    {
        Edit_Solve_Axis_System(info->obj,
                        info->obj->o_features[major_feature].constraints,
                        info->obj->o_features[major_feature].num_constraints,
                        &(info->rotate_resultant));
    }
    for ( i = radius_feature ; i < info->obj->o_num_features ; i++ )
        Constraint_Solve_System(info->obj,
                        info->obj->o_features[i].constraints,
                        info->obj->o_features[i].num_constraints,
                        &(info->features[i].resultant));

    if ( ! do_maintenance ||
         ! Edit_Maintain_All_Constraints(info->obj, info->window, FALSE) )
    {
        Edit_Draw(info->window, ViewNone, info, FALSE);
        Draw_Edit_Extras(info->window, ViewNone, info, FALSE);
        Draw_All_Constraints(info->window, ViewNone, info, FALSE);

        Draw_Edit_Extras(info->window, CalcView, info, FALSE);
        Draw_All_Constraints(info->window, CalcView, info, FALSE);
        Edit_Draw(info->window, CalcView, info, TRUE);
    }


    /* Change the undo_stack head ready to be put on the redo stack. */
    state_elmt->saved_current = redo_state.saved_current;
    for ( i = 0 ; i < info->obj->o_num_features ; i++ )
        state_elmt->saved_features[i] = redo_state.saved_features[i];
    state_elmt->saved_radius = redo_state.saved_radius;
    state_elmt->saved_axes = redo_state.saved_axes;
    free(redo_state.saved_features);
}


static void
Edit_Undo_Select_Op(EditStatePtr state_elmt, Boolean state)
{
    EditInfoPtr     info = Edit_Get_Info();
    int             index = state_elmt->con_index;;

    if ( state_elmt->edit_op == edit_select_op )
        state_elmt->edit_op = edit_deselect_op;
    else
        state_elmt->edit_op = edit_select_op;

    /* Set the box highlight. */
    if ( info->obj->o_features[state_elmt->op_type].base->check_forced )
        ConstraintBoxSetConstraintState(feature_box[state_elmt->op_type], index,
            state || info->obj->o_features[state_elmt->op_type].
                        constraints[index].c_forced, FALSE);
    else
        ConstraintBoxSetConstraintState(feature_box[state_elmt->op_type],
                                        index, state, FALSE);

    /* Set the constraint status. */
    info->obj->o_features[state_elmt->op_type].constraints[index].c_status =
        state;

    /* Draw the constraints off. */
    info->obj->o_features[state_elmt->op_type].base->draw_con_func(
        state_elmt->op_type, info->obj->o_features + state_elmt->op_type,
        info->window, ViewNone, info, FALSE);

    /* Solve the system. */
    info->obj->o_features[state_elmt->op_type].base->solve_function(info->obj,
        info->obj->o_features[state_elmt->op_type].constraints,
        info->obj->o_features[state_elmt->op_type].num_constraints,
        &(info->features[state_elmt->op_type].resultant));

    /* Draw the constraints on. */
    info->obj->o_features[state_elmt->op_type].base->draw_con_func(
        state_elmt->op_type, info->obj->o_features + state_elmt->op_type,
        info->window, CalcView, info, TRUE);

    Edit_Undo_Transformation_Op(state_elmt);
}


static void
Edit_Undo_Add_Op(EditStatePtr state_elmt)
{
    EditInfoPtr info = Edit_Get_Info();

    /* Remember the inverse. */
    state_elmt->edit_op = edit_remove_op;

    /* Delete the constraint that was added. */
    state_elmt->con_feature = Constraint_Copy(NULL,
        info->obj->o_features[state_elmt->op_type].constraints +
        state_elmt->con_index);

    Edit_Remove_Constraint(Edit_Get_Info(), state_elmt->op_type,
                           state_elmt->con_index, FALSE);
}




static void
Add_Dependencies(ConstraintSpecPtr spec, ObjectInstancePtr obj, void *ptr,
                 void *ptr2, int i)
{
    if ( Spec_Is_Dependent(spec->spec_type) )
        Add_Dependency(spec_object(spec), obj);
}


static void
Edit_Undo_Remove_Op(EditStatePtr state_elmt)
{
    EditInfoPtr info = Edit_Get_Info();

    /* The inverse is an add op. */
    state_elmt->edit_op = edit_add_op;

    /* Need to re-add the dependencies, since they were removed with the
    ** constraint.
    */
    Constraint_Manipulate_Specs(state_elmt->con_feature, info->obj, NULL, NULL,
                                0, Add_Dependencies);

    Add_Object_Constraint(state_elmt->con_feature, state_elmt->op_type,
                          state_elmt->con_index, FALSE);

    free(state_elmt->con_feature);
}


static void
Edit_Undo_Modify_Op(EditStatePtr state_elmt)
{
    EditInfoPtr info = Edit_Get_Info();
    int         index = state_elmt->con_index;
    ConstraintPtr   feature;
    ConstraintPtr   orig_cons;

    /* Do the transformation bit. */
    Edit_Undo_Transformation_Op(state_elmt);

    feature = info->obj->o_features[state_elmt->op_type].constraints + index;

    /* Save for redo. */
    orig_cons = Constraint_Copy(NULL, feature);

    /* Remove dependencies. */
    Constraint_Manipulate_Specs(feature, info->obj, NULL, NULL, 0,
                                Dependencies_Remove_Object);

    /* Free specs. */
    free(feature->c_specs);

    /* Copy the new one over. */
    Constraint_Copy(feature, state_elmt->con_feature);

    /* Add dependencies. */
    Constraint_Manipulate_Specs(feature, info->obj, NULL, NULL, 0,
                                Add_Dependencies);

    /* Draw the constraints off. */
    info->obj->o_features[state_elmt->op_type].base->draw_con_func(
        state_elmt->op_type, info->obj->o_features + state_elmt->op_type,
        info->window, ViewNone, info, FALSE);

    /* Solve the system. */
    info->obj->o_features[state_elmt->op_type].base->solve_function(info->obj,
        info->obj->o_features[state_elmt->op_type].constraints,
        info->obj->o_features[state_elmt->op_type].num_constraints,
        &(info->features[state_elmt->op_type].resultant));

    /* Draw the constraints on. */
    info->obj->o_features[state_elmt->op_type].base->draw_con_func(
        state_elmt->op_type, info->obj->o_features + state_elmt->op_type,
        info->window, CalcView, info, TRUE);

    /* Set the label. */
    ConstraintBoxSetConstraintLabel(feature_box[state_elmt->op_type], index,
                                    feature->c_label);

    /* Set for redo. */
    state_elmt->con_feature = orig_cons;
}


static void
Free_Stack_Elmt(EditStatePtr elmt)
{
    free(elmt->saved_features);
    if ( elmt->edit_op == edit_remove_op )
        Constraint_Free(elmt->con_feature);
    free(elmt);
}


/*  void
**  Edit_Undo_Clear()
**  Frees the undo and redo stacks.
*/
void
Edit_Undo_Clear(EditInfoPtr info)
{
    EditStatePtr temp;

    while ( info->undo_stack )
    {
        temp = info->undo_stack;
        info->undo_stack = info->undo_stack->next;
        Free_Stack_Elmt(temp);
    }
    info->undo_stack = NULL;

    while ( info->redo_stack )
    {
        temp = info->redo_stack;
        info->redo_stack = info->redo_stack->next;
        Free_Stack_Elmt(temp);
    }
    info->redo_stack = NULL;
}

