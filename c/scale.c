/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/scale.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: scale.c,v $
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
**  scale.c : functions to scale the object.
**
*/

#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <update.h>


static void Scale_Object(EditInfoPtr);

static ConstraintData   view_feature;

static Vector           orig_pt;
static double           orig_major_radius;
static Vector           orig_features[pt_last_feature];
static Vector           start_origin;
static XPoint           drag_offset;
static Transformation   init_obj_transform;

static XPoint   constr_pt_1, constr_pt_2;


void
Edit_Start_Scale_Drag(XEvent *e, int feature_index, EditInfoPtr info)
{
    Vector  vect;
    int     i;

    if ( ! Interactive_Resultant(info->features[scale_feature].resultant) )
        return;

    Edit_Extract_Resultant_Data(info, scale_feature, &view_feature,
                                &constr_pt_1, &constr_pt_2);

    drag_offset.x = info->features[scale_feature].view->screen.x - e->xbutton.x;
    drag_offset.y = info->features[scale_feature].view->screen.y - e->xbutton.y;

    VSub(info->features[scale_feature].world,
         info->features[origin_feature].world, vect);
    MVMul(info->axes, vect, info->drag_start);
    orig_pt = info->drag_start;
    init_obj_transform = info->obj->o_transform;
    start_origin = info->obj->o_features[origin_feature].location;

    for ( i = radius_feature ; i < info->obj->o_num_features ; i++ )
        orig_features[i] = info->obj->o_features[i].location;
    if ( Obj_Is_Torus(info->obj) )
        orig_major_radius = torus_part(info->obj)->major_radius;

    Edit_Undo_Register_State(edit_drag_op, scale_feature, 0);

    info->drag_type = SCALE_DRAG;

    VNew(1, 1, 1, vect);
    Edit_Set_Drag_Label(SCALE_DRAG, vect, 0);
}



void
Edit_Continue_Scale_Drag(XEvent *e, int feature_index, EditInfoPtr info)
{
    XPoint  screen_pt;
    int     i;

    screen_pt.x = e->xmotion.x + drag_offset.x;
    screen_pt.y = e->xmotion.y + drag_offset.y;

    info->features[scale_feature].world =
        Edit_Obtain_New_Point(info, scale_feature, screen_pt, &view_feature,
                              &constr_pt_1, &constr_pt_2);

    /* Work out what the complete scaling is. */
    Scale_Calculate_Transform(&(info->drag_transform),
                info->features[scale_feature].world,
                orig_pt, start_origin, info->features[origin_feature].world,
                &(info->axes), &(info->axes_inverse), TRUE);

    /* Transform the torus radius point. */
    for ( i = radius_feature ; i < info->obj->o_num_features ; i++ )
        MVMul(info->drag_transform.matrix, orig_features[i],
              info->obj->o_features[i].location);
    if ( Obj_Is_Torus(info->obj) )
        torus_part(info->obj)->major_radius = orig_major_radius;

    Scale_Object(info);
}


void
Edit_Finish_Scale_Drag(XEvent *e, int feature_index, EditInfoPtr info)
{
    /* Rework the available constraints. */
    Edit_Update_Constraints(info);
    Edit_Scale_Force_Constraints(info, FALSE);

    Edit_Set_Drag_Label(NO_DRAG, info->obj->o_features[scale_feature].location, 0);
}


/*  void
**  Scale_Calculate_Transform()
**  Works out the transformation implied by the reference motion with
**  respect to the fixed point.
*/
void
Scale_Calculate_Transform(Transformation *result, Vector reference,
        Vector orig, Vector center, Vector origin, Matrix *axes,
        Matrix *axes_inverse, Boolean prompt)
{
    Vector  orig_to_ref;
    Vector  new_pt, to_center;
    Vector  sc_vect;

    NewIdentityMatrix(result->matrix);

    /* Take the vector from world to object space. */
    VSub(reference, origin, orig_to_ref);
    MVMul(*axes, orig_to_ref, new_pt);

    /* Do each dimension seperately. */
    if ( IsZero(orig.x) )
        result->matrix.x.x = 1;
    else
        result->matrix.x.x = new_pt.x / orig.x;

    if ( IsZero(orig.y) )
        result->matrix.y.y = 1;
    else
        result->matrix.y.y = new_pt.y / orig.y;

    if ( IsZero(orig.z) )
        result->matrix.z.z = 1;
    else
        result->matrix.z.z = new_pt.z / orig.z;

    if ( prompt )
    {
        VNew(result->matrix.x.x,result->matrix.y.y,result->matrix.z.z, sc_vect);
        Edit_Set_Drag_Label(SCALE_DRAG, sc_vect, 0);
    }

    /* Take the transformation back into world space. */
    result->matrix = MMMul(&(result->matrix), axes);
    result->matrix = MMMul(axes_inverse, &(result->matrix));

    /* Correct for 0's, which give singular matrices. */
    if ( IsZero(result->matrix.x.x) )
        result->matrix.x.x = SMALL_NUM;
    if ( IsZero(result->matrix.y.y) )
        result->matrix.y.y = SMALL_NUM;
    if ( IsZero(result->matrix.z.z) )
        result->matrix.z.z = SMALL_NUM;

    /* Scale the center by the appropriate amount. */
    VScalarMul(center, -1, to_center);
    MVMul(result->matrix, to_center, new_pt);

    /* The difference in center points is the displacement. */
    VSub(new_pt, to_center, result->displacement);
}

/*  void
**  Scale_Object()
**  Scales the object from its old position as specified by new_verts to
**  a new one based on the current_scaling.  new_verts is updated
**  in the process.
*/
static void
Scale_Object(EditInfoPtr info)
{
    info->obj->o_dynamic_func(info, scale_feature, NULL, &init_obj_transform);
}


/*
**  Edit_Force_Scale_Satisfaction(EditInfoPtr info)
**  Checks to see whether the scale constraint is satisfied, and if not
**  satisfies it by scaling the object.
*/
Boolean
Edit_Force_Scale_Satisfaction(EditInfoPtr info, int n, Boolean prefer)
{
    int new_pref;
    int i;

    new_pref = Point_Satisfies_Constraint(
                            &(info->features[scale_feature].world),
                            &(info->features[scale_feature].resultant), NULL);

    if ( ! new_pref )
    {
        int     preferred =
            info->obj->o_features[scale_feature].flags & FeaturePref1 ? 1 : 2;
        Vector  to_center;
        Vector  vect;
        Vector  disp;

        disp =
            Find_Required_Motion(&(info->features[scale_feature].world),
                         &(info->features[scale_feature].resultant), prefer,
                         &preferred);

        if ( ! prefer )
        {
            if ( preferred == 1 )
            {
                info->obj->o_features[scale_feature].flags &=
                    ( FeatureAll ^ FeaturePref2 );
                info->obj->o_features[scale_feature].flags |= FeaturePref1;
            }
            else
            {
                info->obj->o_features[scale_feature].flags &=
                    ( FeatureAll ^ FeaturePref1 );
                info->obj->o_features[scale_feature].flags |= FeaturePref2;
            }
        }

        VSub(info->features[scale_feature].world,
             info->features[origin_feature].world, vect);
        MVMul(info->axes, vect, orig_pt);
        VSub(info->features[origin_feature].world,
             info->obj->o_world_verts[info->obj->o_num_vertices - 1],
             to_center);
        init_obj_transform = info->obj->o_transform;

        VAdd(info->features[scale_feature].world, disp,
             info->features[scale_feature].world);

        /* Work out what the complete scaling is. */
        Scale_Calculate_Transform(&(info->drag_transform),
                    info->features[scale_feature].world,
                    orig_pt, to_center, info->features[origin_feature].world,
                    &(info->axes), &(info->axes_inverse), FALSE);

        /* Transform the other features. */
        for ( i = radius_feature ; i < info->obj->o_num_features ; i++ )
        {
            if ( ! info->obj->o_features[i].base )  continue;
            Transform_Vector(info->drag_transform,
                             info->obj->o_features[i].location,
                             info->obj->o_features[i].location);
        }

        Scale_Object(info);

        Edit_Finish_Scale_Drag(NULL, 0, info);

        return TRUE;
    }
    else if ( ! prefer )
    {
        if ( new_pref == 1 )
        {
            info->obj->o_features[scale_feature].flags &=
                ( FeatureAll ^ FeaturePref2 );
            info->obj->o_features[scale_feature].flags |= FeaturePref1;
        }
        else
        {
            info->obj->o_features[scale_feature].flags &=
                ( FeatureAll ^ FeaturePref1 );
            info->obj->o_features[scale_feature].flags |= FeaturePref2;
        }
    }

    return FALSE;
}


/*  Boolean
**  Edit_Dynamic_Scale(EditInfoPtr info)
**  Checks to see if the scale constraints are satisfied, and if not,
**  updates the drag_transform to give a cumulative scaling that satisfies it.
**  Modifies info->features[scale_feature] and drag_transform only.
*/
Boolean
Edit_Dynamic_Scale(EditInfoPtr info, Vector center)
{
    int i;

    /* Update the scaling constraints. */
    if ( Edit_Update_Active_Object_Cons(
            info->obj->o_features[scale_feature].constraints,
            info->obj->o_features[scale_feature].num_constraints,
            info, &center) )
        Constraint_Solve_System(info->obj,
            info->obj->o_features[scale_feature].constraints,
            info->obj->o_features[scale_feature].num_constraints,
            &(info->features[scale_feature].resultant));

    if ( ! Point_Satisfies_Constraint(&(info->features[scale_feature].world),
                          &(info->features[scale_feature].resultant), NULL) )
    {
        Transformation  temp_m = info->drag_transform;
        int     preference =
            info->obj->o_features[scale_feature].flags & FeaturePref1 ? 1 : 2;
        Vector  to_center;
        Vector  vect;
        Vector  disp;

        disp =
            Find_Required_Motion(&(info->features[scale_feature].world),
                 &(info->features[scale_feature].resultant), TRUE, &preference);

        VSub(info->features[scale_feature].world,
             info->features[origin_feature].world, vect);
        MVMul(info->axes, vect, orig_pt);
        VSub(info->features[origin_feature].world, center, to_center);

        VAdd(info->features[scale_feature].world, disp,
             info->features[scale_feature].world);

        /* Work out what the complete scaling is. */
        Scale_Calculate_Transform(&(info->drag_transform),
                    info->features[scale_feature].world,
                    orig_pt, to_center, info->features[origin_feature].world,
                    &(info->axes), &(info->axes_inverse), FALSE);

        /* Transform the other features. */
        for ( i = radius_feature ; i < info->obj->o_num_features ; i++ )
        {
            if ( ! info->obj->o_features[i].base )  continue;
            Transform_Vector(info->drag_transform,
                             info->obj->o_features[i].location,
                             info->obj->o_features[i].location);
        }

        info->drag_transform.matrix = MMMul(&(info->drag_transform.matrix),
                                            &(temp_m.matrix));
        VAdd(info->drag_transform.displacement, temp_m.displacement,
             info->drag_transform.displacement);

        return TRUE;
    }

    return FALSE;
}



Boolean
Edit_Scale_Force_Constraints(EditInfoPtr info, Boolean ignore_old)
{
    Vector  body_space_ref;
    Vector  temp_v;
    Boolean old[3];

    /* Convert a vector from the origin to the ref point to body space. */
    VSub(info->features[scale_feature].world,
         info->features[origin_feature].world, temp_v);
    MVMul(info->axes, temp_v, body_space_ref);

    old[0] = info->obj->o_features[scale_feature].constraints[0].c_forced;
    old[1] = info->obj->o_features[scale_feature].constraints[1].c_forced;
    old[2] = info->obj->o_features[scale_feature].constraints[2].c_forced;

    /* Find out which ones need forcing. */
    info->obj->o_features[scale_feature].constraints[0].c_forced =
        IsZero(body_space_ref.x);
    info->obj->o_features[scale_feature].constraints[1].c_forced =
        IsZero(body_space_ref.y);
    info->obj->o_features[scale_feature].constraints[2].c_forced =
        IsZero(body_space_ref.z);

    /* Set the status of all the forced options. */
    if ( ignore_old ||
        old[0] != info->obj->o_features[scale_feature].constraints[0].c_forced )
        Edit_Select_Constraint(info, 0, scale_feature,
               info->obj->o_features[scale_feature].constraints[0].c_forced,
               TRUE, FALSE);
    if ( ignore_old ||
        old[1] != info->obj->o_features[scale_feature].constraints[1].c_forced )
        Edit_Select_Constraint(info, 1, scale_feature,
               info->obj->o_features[scale_feature].constraints[1].c_forced,
               TRUE, FALSE);
    if ( ignore_old ||
        old[2] != info->obj->o_features[scale_feature].constraints[2].c_forced )
        Edit_Select_Constraint(info, 2, scale_feature,
               info->obj->o_features[scale_feature].constraints[2].c_forced,
               TRUE, FALSE);

    return (
      old[0] != info->obj->o_features[scale_feature].constraints[0].c_forced ||
      old[1] != info->obj->o_features[scale_feature].constraints[1].c_forced ||
      old[2] != info->obj->o_features[scale_feature].constraints[2].c_forced );
}




