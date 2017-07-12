/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/radius.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: radius.c,v $
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
**  radius.c: Functions for manipulating the radius constraint point of
**              a torus.
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <update.h>

static double   Radius_Get_Point_Distance(Vector, Transformation*);

static ConstraintData   view_feature;

static XPoint           drag_offset;
static Transformation   init_obj_transform;
static Vector           init_scale_pt;
static Vector           init_radius_pt;
static double           init_radius_dist;

static XPoint   constr_pt_1, constr_pt_2;

void
Edit_Start_Radius_Drag(XEvent *e, int feature_index, EditInfoPtr info)
{
    Vector  vect;

    if ( ! Interactive_Resultant(info->features[radius_feature].resultant) )
        return;

    Edit_Extract_Resultant_Data(info, radius_feature, &view_feature,
                                &constr_pt_1, &constr_pt_2);

    drag_offset.x = info->features[radius_feature].view->screen.x -e->xbutton.x;
    drag_offset.y = info->features[radius_feature].view->screen.y -e->xbutton.y;

    init_obj_transform = info->obj->o_transform;
    init_scale_pt = info->obj->o_features[scale_feature].location;
    init_radius_pt = info->features[radius_feature].world;
    init_radius_dist = torus_part(info->obj)->major_radius;

    Edit_Undo_Register_State(edit_drag_op, radius_feature, 0);

    info->drag_type = RADIUS_DRAG;

    Edit_Set_Drag_Label(RADIUS_DRAG, vect, torus_part(info->obj)->major_radius);
}


void
Edit_Continue_Radius_Drag(XEvent *e, int feature_index, EditInfoPtr info)
{
    XPoint  screen_pt;
    Vector  dummy;

    screen_pt.x = e->xmotion.x + drag_offset.x;
    screen_pt.y = e->xmotion.y + drag_offset.y;

    info->features[radius_feature].world =
        Edit_Obtain_New_Point(info, radius_feature, screen_pt,
                              &view_feature, &constr_pt_1, &constr_pt_2);

    VNew(0, 0, 0, info->drag_transform.displacement);
    Radius_Modify_Radius(init_radius_pt, init_radius_dist,
                    init_scale_pt, info->features[radius_feature].world,
                    &init_obj_transform, &(info->axes),
                    &(torus_part(info->obj)->major_radius),
                    &(info->obj->o_features[scale_feature].location),
                    &(info->drag_transform.matrix),
                    info->features[scale_feature].resultant.feature_1.c_type
                        != null_feature);

    Edit_Set_Drag_Label(RADIUS_DRAG,dummy,torus_part(info->obj)->major_radius);

    info->obj->o_dynamic_func(info, radius_feature, NULL, &init_obj_transform);
}


void
Edit_Finish_Radius_Drag(XEvent *e, int feature_index, EditInfoPtr info)
{
    Vector  dummy;

    Edit_Update_Constraints(info);
    Edit_Set_Drag_Label(NO_DRAG, dummy, 0);
}


static double
Radius_Get_Point_Distance(Vector pt, Transformation *trans)
{
    Vector  generic_pt;

    VSub(pt, trans->displacement, pt);
    MVMul(trans->matrix, pt, generic_pt);

    return sqrt(generic_pt.x * generic_pt.x + generic_pt.y * generic_pt.y);
}


double
Radius_Calculate_Parameters_No_Scale(Vector new_pt,
                                     Transformation *inverse_trans)
{
    return Radius_Get_Point_Distance(new_pt, inverse_trans);
}

double
Radius_Calculate_Parameters_Scale(Vector new_pt, double start_dist,
                                  double start_val, double start_scale_dist,
                                  Transformation *inverse_trans,
                                  double *return_scale)
{
    double  new_dist;
    double  new_val, new_scale;


    /* Get the distance of the new point. */
    new_dist = Radius_Get_Point_Distance(new_pt, inverse_trans);

    if ( IsZero(start_scale_dist) || DEqual(start_scale_dist, new_dist) ||
         DEqual(start_scale_dist, start_dist) )
    {
        *return_scale = 1.0;
        return start_val;
    }

    new_scale = start_scale_dist * ( ( start_scale_dist - start_dist ) /
                ( start_scale_dist - new_dist ) );

    new_dist = new_dist * ( new_scale / start_scale_dist );
    
    new_val = fabs(start_val + new_dist - start_dist);

    /* Calculate how much to scale. */
    *return_scale = start_scale_dist / new_scale;

    return new_val;
}


/* start_world: The initial position of the radius point in the world.
** start_radius: The initial value for the major radius.
** start_scaling: The initial position of the scaling point offset from center.
** new_world: the new location of the radius pt in world.
** init_transform: the initial transformation.
** axes: the object axes.
** new_dist: return for new major radius.
** new_scaling: return for new scaling pt offset.
** scale_transform: return for scaling matrix, if required.
** scale: whether or not dynamic scaling needs to be considered.
*/ 
void
Radius_Modify_Radius(Vector start_world, double start_radius,
                     Vector start_scaling, Vector new_world,
                     Transformation *init_transform, Matrix *axes,
                     double *new_dist, VectorPtr new_scaling,
                     Matrix *scale_transform, Boolean scale)
{
    Transformation  inverse_trans;
    Matrix          inverse_axes;
    double          start_rad_dist, start_scale_dist;
    double          new_rad_dist, delta_rad;
    double          scale_ratio;
    Vector          scale_pt, to_center;
    Vector          scale_world;
    double          temp_d;
    Matrix          temp_m;

    inverse_trans.matrix = MInvert(&(init_transform->matrix));
    inverse_trans.displacement = init_transform->displacement;
    MTrans(*axes, inverse_axes);

    start_rad_dist = Radius_Get_Point_Distance(start_world, &inverse_trans);

    VAdd(start_scaling, init_transform->displacement, scale_world);
    start_scale_dist = Radius_Get_Point_Distance(scale_world, &inverse_trans);
    MVMul(inverse_trans.matrix, start_scaling, scale_pt);

    if ( scale )
    {
        *new_dist = Radius_Calculate_Parameters_Scale(
                                        new_world, start_rad_dist,
                                        start_radius, start_scale_dist,
                                        &inverse_trans, &scale_ratio);

        *new_scaling = start_scaling;
        NewIdentityMatrix(*scale_transform);
        scale_transform->y.y = scale_transform->z.z = scale_ratio;
        temp_m = MMMul(scale_transform, axes);
        *scale_transform = MMMul(&inverse_axes, &temp_m);
    }
    else
    {
        new_rad_dist =
            Radius_Calculate_Parameters_No_Scale(new_world, &inverse_trans);
        delta_rad = new_rad_dist - start_rad_dist;
        *new_dist = start_radius + delta_rad;

        VNew(scale_pt.x, scale_pt.y, 0, to_center);
        VUnit(to_center, temp_d, to_center);
        VScalarMul(to_center, delta_rad, to_center);
        VAdd(to_center, scale_pt, scale_pt);
        MVMul(init_transform->matrix, scale_pt, *new_scaling);

        NewIdentityMatrix(*scale_transform);
    }
}


Boolean
Edit_Dynamic_Set_Radius(EditInfoPtr info, Vector new_center)
{
    /* Update the radius constraints. */
    if ( Edit_Update_Active_Object_Cons(
            info->obj->o_features[radius_feature].constraints,
            info->obj->o_features[radius_feature].num_constraints,
            info, &new_center) )
        Constraint_Solve_System(info->obj,
            info->obj->o_features[radius_feature].constraints,
            info->obj->o_features[radius_feature].num_constraints,
            &(info->features[radius_feature].resultant));

    if ( ! Point_Satisfies_Constraint(&(info->features[radius_feature].world),
                          &(info->features[radius_feature].resultant), NULL) )
    {
        int     preference =
            info->obj->o_features[radius_feature].flags & FeaturePref1 ? 1 : 2;
        Vector  start_pt;
        Matrix  scale_transform;
        Vector  disp;

        disp =
            Find_Required_Motion(&(info->features[radius_feature].world),
                             &(info->features[radius_feature].resultant), TRUE,
                             &preference);

        start_pt = info->features[radius_feature].world;
        VAdd(disp, info->features[radius_feature].world,
             info->features[radius_feature].world);

        Radius_Modify_Radius(start_pt, torus_part(info->obj)->major_radius,
                    info->obj->o_features[scale_feature].location,
                    info->features[radius_feature].world,
                    &(info->obj->o_transform), &(info->axes),
                    &(torus_part(info->obj)->major_radius),
                    &(info->obj->o_features[scale_feature].location),
                    &scale_transform,
                    info->features[scale_feature].resultant.feature_1.c_type
                        != null_feature);

        if ( info->features[scale_feature].resultant.feature_1.c_type !=
                null_feature )
            info->drag_transform.matrix = MMMul(&(info->drag_transform.matrix),
                                                &scale_transform);

        return TRUE;
    }

    return FALSE;
}


Boolean
Edit_Force_Radius_Satisfaction(EditInfoPtr info, int n, Boolean prefer)
{
    int new_pref;

    new_pref = Point_Satisfies_Constraint(
                        &(info->features[radius_feature].world),
                        &(info->features[radius_feature].resultant), NULL);
    if ( ! new_pref )
    {
        int     preferred =
            info->obj->o_features[radius_feature].flags & FeaturePref1 ? 1 : 2;
        Vector  start_pt;
        Vector  disp;

        disp =
            Find_Required_Motion(&(info->features[radius_feature].world),
                         &(info->features[radius_feature].resultant), prefer,
                         &preferred);

        if ( ! prefer )
        {
            if ( preferred == 1 )
            {
                info->obj->o_features[radius_feature].flags &=
                    ( FeatureAll ^ FeaturePref2 );
                info->obj->o_features[radius_feature].flags |= FeaturePref1;
            }
            else
            {
                info->obj->o_features[radius_feature].flags &=
                    ( FeatureAll ^ FeaturePref1 );
                info->obj->o_features[radius_feature].flags |= FeaturePref2;
            }
        }

        init_obj_transform = info->obj->o_transform;
        start_pt = info->features[radius_feature].world;
        VAdd(disp, info->features[radius_feature].world,
             info->features[radius_feature].world);

        VNew(0, 0, 0, info->drag_transform.displacement);
        Radius_Modify_Radius(start_pt, torus_part(info->obj)->major_radius,
                    info->obj->o_features[scale_feature].location,
                    info->features[radius_feature].world,
                    &(info->obj->o_transform), &(info->axes),
                    &(torus_part(info->obj)->major_radius),
                    &(info->obj->o_features[scale_feature].location),
                    &(info->drag_transform.matrix),
                    info->features[scale_feature].resultant.feature_1.c_type !=
                        null_feature);

        info->obj->o_dynamic_func(info, radius_feature, NULL,
                                  &init_obj_transform);

        Edit_Finish_Radius_Drag(NULL, 0, info);

        return TRUE;
    }
    else if ( ! prefer )
    {
        if ( new_pref == 1 )
        {
            info->obj->o_features[radius_feature].flags &=
                ( FeatureAll ^ FeaturePref2 );
            info->obj->o_features[radius_feature].flags |= FeaturePref1;
        }
        else
        {
            info->obj->o_features[radius_feature].flags &=
                ( FeatureAll ^ FeaturePref1 );
            info->obj->o_features[radius_feature].flags |= FeaturePref2;
        }
    }

    return FALSE;
}
