/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/align.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: align.c,v $
 * Revision 1.0  1997/05/06 20:30:22  schenney
 * Initial revision
 *
 */
#define PATCHLEVEL 0
/*
**    ScEd: A Constraint Based Scene Editor
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
**  align.c : Functions for axis alignment operations.
*/


#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <quaternion.h>
#include <select_point.h>
#include <update.h>
#include <X11/Xaw/Toggle.h>


/* In rotate.c */
extern Matrix   previous_rot;
extern Vector   start_center;
extern Transformation   init_object_transform;


void
Edit_Solve_Axis_System(ObjectInstancePtr obj, ConstraintPtr cons, int num,
                       ResultantPtr res)
{
    EditInfoPtr     edit_info = Edit_Get_Info();

    Constraint_Solve_System(obj,
                            obj->o_features[major_feature].constraints,
                            obj->o_features[major_feature].num_constraints,
                            &(edit_info->features[major_feature].resultant));
    Constraint_Solve_System(obj,
                            obj->o_features[minor_feature].constraints,
                            obj->o_features[minor_feature].num_constraints,
                            &(edit_info->features[minor_feature].resultant));

    edit_info->rotate_resultant.feature_1.c_type =
    edit_info->rotate_resultant.feature_2.c_type = null_feature;
    switch ( edit_info->features[major_feature].resultant.feature_1.c_type )
    {
        case null_feature:
            switch (
                edit_info->features[minor_feature].resultant.feature_1.c_type )
            {
                case null_feature:
                    break;
                case line_feature:
                    edit_info->rotate_resultant.feature_1 =
                        edit_info->features[minor_feature].resultant.feature_1;
                    break;
                default:
                    edit_info->rotate_resultant.feature_1.c_type =
                        inconsistent_feature;
            }
            break;

        case line_feature:
            if (
              edit_info->features[minor_feature].resultant.feature_1.c_type ==
                    null_feature )
                edit_info->rotate_resultant.feature_1 =
                    edit_info->features[major_feature].resultant.feature_1;
            else
                edit_info->rotate_resultant.feature_1.c_type =
                    inconsistent_feature;
            break;

        default:
            edit_info->rotate_resultant.feature_1.c_type = inconsistent_feature;
    }
}


Quaternion
Build_Rotate_Quaternion(Vector axis, double cos_angle)
{
    Quaternion  quat;
    double  sin_half_angle;
    double  cos_half_angle;
    double  angle;

    /* The quaternion requires half angles. */
    if ( cos_angle > 1.0 ) cos_angle = 1.0;
    if ( cos_angle < -1.0 ) cos_angle = -1.0;
    angle = acos(cos_angle);
    sin_half_angle = sin(angle / 2);
    cos_half_angle = cos(angle / 2);

    VScalarMul(axis, sin_half_angle, quat.vect_part);
    quat.real_part = cos_half_angle;

    return quat;
}


Matrix
Major_Align_Matrix(Vector new_dir, Matrix *old_axes)
{
    Vector  axis, unit_axis;
    double  temp_d;
    Matrix  identity;
    Vector  diff;

    NewIdentityMatrix(identity);

    /* Build a quaternion for the rotation. */
    VCross(old_axes->x, new_dir, axis);

    if ( VZero(axis) )
    {
        if ( VEqual(new_dir, old_axes->x, diff) )
            return identity;    /* Already aligned. */
        else
            axis = old_axes->y;
    }

    VUnit(axis, temp_d, unit_axis);
    return Quaternion_To_Matrix(
            Build_Rotate_Quaternion(unit_axis, VDot(new_dir, old_axes->x)));

}


Matrix
Minor_Align_Matrix(Vector new_dir, Matrix *old_axes, Boolean maintain_major)
{
    Vector  new_other, unit_new, axis;
    double  cos_angle;
    double  temp_d;
    Matrix  identity;
    Vector  diff;

    NewIdentityMatrix(identity);

    if ( maintain_major )
    {
        /* Work out where the new other axis is. */
        VCross(old_axes->x, new_dir, new_other);

        if ( VZero(new_other) )
            return identity;

        VUnit(new_other, temp_d, unit_new);

        VCross(old_axes->z, unit_new, axis);
        cos_angle = VDot(unit_new, old_axes->z);
    }
    else
    {
        VCross(old_axes->y, new_dir, axis);
        cos_angle = VDot(new_dir, old_axes->y);
    }

    if ( VZero(axis) )
    {
        if ( VEqual(unit_new, old_axes->z, diff ) )
            return identity;    /* Already aligned. */
        else
            axis = old_axes->x;
    }
    VUnit(axis, temp_d, axis);

    return Quaternion_To_Matrix(Build_Rotate_Quaternion(axis, cos_angle));
}


static Boolean
Edit_Major_Align(Vector direction)
{
    EditInfoPtr info = Edit_Get_Info();
    Matrix  matrix;

    matrix = Major_Align_Matrix(direction, &(info->axes));

    if ( MIsIdentity(matrix) )
        return FALSE;

    Edit_Align(info, &matrix);

    return TRUE;
}

static Boolean
Edit_Minor_Align(Vector direction, Boolean maintain_major)
{
    EditInfoPtr info = Edit_Get_Info();
    Vector  new_other;
    Matrix  matrix;

    /* Work out where the new other axis is. */
    VCross(info->axes.x, direction, new_other);

    if ( VZero(new_other) )
    {
        XBell(XtDisplay(main_window.shell), 0);
        return FALSE;
    }

    matrix = Minor_Align_Matrix(direction, &(info->axes), maintain_major);

    if ( MIsIdentity(matrix) )
        return FALSE;

    Edit_Align(info, &matrix);

    return TRUE;
}

/*  void
**  Edit_Force_Alignment_Satisfaction(EditInfoPtr align_info, Boolean dummy)
**  Updates any origin dependent constraints, and checks and modifies
**  the axis alignment.
*/
Boolean
Edit_Force_Alignment_Satisfaction(EditInfoPtr align_info, int n, Boolean dummy)
{
    Boolean result = FALSE;

    if ( align_info->features[major_feature].resultant.feature_1.c_type ==
         line_feature )
    {
        result =
            Edit_Major_Align(
              align_info->features[major_feature].resultant.feature_1.c_vector);
        if ( align_info->features[minor_feature].resultant.feature_1.c_type ==
             line_feature )
            result =
              Edit_Minor_Align(
              align_info->features[minor_feature].resultant.feature_1.c_vector,
                                 TRUE) || result;
    }
    else if ( align_info->features[minor_feature].resultant.feature_1.c_type ==
              line_feature )
        result =
            Edit_Minor_Align(
              align_info->features[minor_feature].resultant.feature_1.c_vector,
              FALSE);

    return result;
}


/*  Boolean
**  Edit_Dynamic_Align(EditInfoPtr align_info)
**  Updates and checks alignment constraints in the middle of a drag op.
**  Assumes it is called with updated info->scaling and origin points, and
**  the current center of the body.
**  It does not change anything permanent itself, just returns the transform
**  needed to bring about alignment.
*/
Boolean
Edit_Dynamic_Align(EditInfoPtr align_info, Transformation *result,
                   Vector new_center)
{
    Feature new_org;
    Vector  start_center;
    Matrix  new_axes;
    Matrix  major_matrix;
    Matrix  minor_matrix;
    Vector  to_center;
    Vector  temp_v;
    Matrix  transp;
    Boolean changed = FALSE;
    int     i;

    new_axes = align_info->axes;
    start_center = new_center;
    VSub(align_info->features[origin_feature].world, start_center,
         new_org.location);

    changed = FALSE;
    for ( i = 0 ;
          i < align_info->obj->o_features[major_feature].num_constraints ;
          i++ )
        changed = Edit_Update_Constraint_Specs(
            align_info->obj->o_features[major_feature].constraints + i,
            &start_center, align_info->obj->o_features,
            &(align_info->axes.z)) || changed;
    for ( i = 0 ;
          i < align_info->obj->o_features[minor_feature].num_constraints ;
          i++ )
        changed = Edit_Update_Constraint_Specs(
            align_info->obj->o_features[minor_feature].constraints + i,
            &start_center, align_info->obj->o_features,
            &(align_info->axes.z)) || changed;
    if ( changed )
    {
        Edit_Solve_Axis_System(align_info->obj,
            align_info->obj->o_features[major_feature].constraints,
            align_info->obj->o_features[major_feature].num_constraints,
            &(align_info->rotate_resultant));

        if ( align_info->features[major_feature].resultant.feature_1.c_type ==
             line_feature )
        {
            major_matrix = Major_Align_Matrix(
              align_info->features[major_feature].resultant.feature_1.c_vector,
              &(align_info->axes));

            /* Transform the key points. */
            /* The center first. */
            VSub(new_center, align_info->features[origin_feature].world,
                 to_center);
            MVMul(major_matrix, to_center, temp_v);
            VAdd(temp_v, align_info->features[origin_feature].world,
                 new_center);
            VSub(align_info->features[origin_feature].world, new_center,
                 new_org.location);

            /* The axes. */
            MTrans(major_matrix, transp);
            new_axes = MMMul(&(align_info->axes), &transp);
        }
        else
            NewIdentityMatrix(major_matrix)

        /* Check minor axis. */
        if ( align_info->features[minor_feature].resultant.feature_1.c_type ==
             line_feature )
        {
            minor_matrix = Minor_Align_Matrix(
              align_info->features[minor_feature].resultant.feature_1.c_vector,
              &(new_axes),
              ( align_info->features[major_feature].resultant.feature_1.c_type
                == line_feature ) );

            /* Transform the key points. */
            /* The center first. */
            VSub(new_center, align_info->features[origin_feature].world,
                 to_center);
            MVMul(minor_matrix, to_center, temp_v);
            VAdd(temp_v, align_info->features[origin_feature].world,
                 new_center);
            VSub(align_info->features[origin_feature].world, new_center,
                 new_org.location);

            /* The axes. */
            MTrans(minor_matrix, transp);
            new_axes = MMMul(&new_axes, &transp);
        }
        else
            NewIdentityMatrix(minor_matrix)

        result->matrix = major_matrix;
        result->matrix = MMMul(&minor_matrix, &(result->matrix));
    }
    else
    {
        VNew(0, 0, 0, result->displacement);
        MNew(result->displacement, result->displacement, result->displacement,
             result->matrix);
    }

    if ( changed )
        VSub(new_center, start_center, result->displacement);

    return changed;
}

