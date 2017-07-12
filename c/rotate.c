/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/rotate.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: rotate.c,v $
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
**  rotate.c : Functions to perform object rotation.
*/

#include <math.h>
#include <sced.h>
#include <constraint.h>
#include <edit.h>
#include <quaternion.h>
#include <update.h>


static void Rotate_Object(EditInfoPtr);
static Vector   Sphere_Point(EditInfoPtr, int, int);

static Matrix   previous_rot;
static Vector   start_center;
static Vector   start_major, start_minor;
static Transformation   init_obj_transform;

static Vector   constraint_axis;

void
Edit_Start_Rotate_Drag(XEvent *e, int feature_index, EditInfoPtr info)
{
    if ( info->rotate_resultant.feature_1.c_type != line_feature &&
         info->rotate_resultant.feature_1.c_type != null_feature )  return;

    if ( ! do_maintenance )
    {
        Edit_Draw(info->window, ViewNone, info, FALSE);
        Edit_Draw(info->window, RemoveHidden, info, TRUE);
    }

    NewIdentityMatrix(info->drag_transform.matrix);
    VNew(0, 0, 0, info->drag_transform.displacement);
    NewIdentityMatrix(previous_rot);
    start_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];
    start_major = info->obj->o_features[major_feature].location;
    start_minor = info->obj->o_features[minor_feature].location;

    if ( info->rotate_resultant.feature_1.c_type == line_feature )
    {
        Matrix  inverse_trans;

        /* Get the axis in view.  It's the same as transforming a normal. */
        MTrans(info->window->viewport.view_to_world.matrix, inverse_trans);
        MVMul(inverse_trans, info->rotate_resultant.feature_1.c_vector,
              constraint_axis);
    }

    info->drag_start = Sphere_Point(info, e->xbutton.x, e->xbutton.y);
    init_obj_transform = info->obj->o_transform;

    Edit_Undo_Register_State(edit_drag_op, major_feature, 0);

    info->drag_type = ROTATE_DRAG;
}


void
Edit_Continue_Rotate_Drag(XEvent *e, int feature_index, EditInfoPtr info)
{
    Vector  current_pt;
    Quaternion  rot_quat;
    Matrix  prev_inverse;
    Matrix  new_rot;
    Vector  temp_v;
    double  rot_angle;
    Vector  rot_axis;
    double  sin_angle;
    int     i;

    current_pt = Sphere_Point(info, e->xbutton.x, e->xbutton.y);

    /* Find the current rotation quaternion. */
    VCross(info->drag_start, current_pt, rot_quat.vect_part);
    rot_quat.real_part = VDot(info->drag_start, current_pt);

    rot_angle = acos(rot_quat.real_part);
    if ( ! IsZero(rot_angle) )
    {
        sin_angle = 1 / sin(rot_angle);
        VScalarMul(rot_quat.vect_part, sin_angle, rot_axis);
        rot_angle *= 360 / M_PI;
    }

    Edit_Set_Drag_Label(ROTATE_DRAG, rot_axis, rot_angle);

    prev_inverse = MInvert(&previous_rot);
    previous_rot = Quaternion_To_Matrix(rot_quat);
    new_rot = MMMul(&previous_rot, &prev_inverse);
    info->drag_transform.matrix =
        MMMul(&new_rot, &(info->drag_transform.matrix));

    /* Rotate the scaling and other constraint points. */
    for ( i = scale_feature ; i < info->obj->o_num_features ; i++ )
    {
        if ( ! info->obj->o_features[i].base )  continue;

        MVMul(new_rot, info->obj->o_features[i].location, temp_v);
        info->obj->o_features[i].location = temp_v;
    }

    Rotate_Object(info);
}



void
Edit_Finish_Rotate_Drag(XEvent *e, int feature_index, EditInfoPtr info)
{
    Vector  dummy;
    /* Rework the available constraints. */
    Edit_Update_Constraints(info);

    Apply_Transform(init_obj_transform, info->drag_transform,
                    info->obj->o_transform);

    info->obj->o_features[major_feature].location = info->axes.x;
    info->obj->o_features[minor_feature].location = info->axes.y;

    Edit_Set_Drag_Label(NO_DRAG, dummy, 0);
}


static void
Rotate_Object(EditInfoPtr info)
{
    Vector  to_center;
    Vector  new_center;

    /* Need to know how far it's moved.  So rotate the center about the
    ** fixed point and see where it ends up.
    */
    VSub(start_center, info->features[origin_feature].world, to_center);
    MVMul(info->drag_transform.matrix, to_center, new_center);
    VAdd(new_center, info->features[origin_feature].world, new_center);
    VSub(new_center, start_center, info->drag_transform.displacement);

    /* Rotate the body axes. */
    MVMul(previous_rot, start_major,
          info->obj->o_features[major_feature].location);
    info->axes.x = info->obj->o_features[major_feature].location;
    MVMul(previous_rot, start_minor,
          info->obj->o_features[minor_feature].location);
    info->axes.y = info->obj->o_features[minor_feature].location;
    VCross(info->axes.x, info->axes.y, info->axes.z);
    info->axes_inverse = MInvert(&(info->axes));

    info->obj->o_dynamic_func(info, major_feature, &start_center,
                              &init_obj_transform);
}



/*  Matrix
**  Quaternion_To_Matrix(Quaternion q)
**  Converts a quaternion to a rotation matrix.
*/
Matrix
Quaternion_To_Matrix(Quaternion q)
{
    Matrix  result;
    double  xx, yy, zz;
    double  xy, xz, yz;
    double  wx, wy, wz;
    Vector  v1, v2, v3;

    xx = q.vect_part.x * q.vect_part.x;
    yy = q.vect_part.y * q.vect_part.y;
    zz = q.vect_part.z * q.vect_part.z;

    xy = q.vect_part.x * q.vect_part.y;
    xz = q.vect_part.x * q.vect_part.z;
    yz = q.vect_part.y * q.vect_part.z;

    wx = q.real_part * q.vect_part.x;
    wy = q.real_part * q.vect_part.y;
    wz = q.real_part * q.vect_part.z;

    VNew(1 - 2 * yy - 2 * zz, 2 * xy - 2 * wz, 2 * xz + 2 * wy, v1);
    VNew(2 * xy + 2 * wz, 1 - 2 * xx - 2 * zz, 2 * yz - 2 * wx, v2);
    VNew(2 * xz - 2 * wy, 2 * yz + 2 * wx, 1 - 2 * xx - 2 * yy, v3);
    MNew(v1, v2, v3, result);

    return result;
}


extern Quaternion
QQMul(Quaternion *q1, Quaternion *q2)
{
    Quaternion  res;
    Vector      temp_v;

    res.real_part = q1->real_part * q2->real_part -
                    VDot(q1->vect_part, q2->vect_part);
    VCross(q1->vect_part, q2->vect_part, res.vect_part);
    VScalarMul(q1->vect_part, q2->real_part, temp_v);
    VAdd(temp_v, res.vect_part, res.vect_part);
    VScalarMul(q2->vect_part, q1->real_part, temp_v);
    VAdd(temp_v, res.vect_part, res.vect_part);

    return res;
}

extern void
Quaternion_To_Axis_Angle(Quaternion *q, Vector *axis, double *angle)
{
    double  half_angle;
    double  sin_half_angle;

    half_angle = acos(q->real_part);
    sin_half_angle = sin(half_angle);
    *angle = half_angle * 2;
    if ( IsZero(sin_half_angle) )
        VNew(0, 0, 1, *axis);
    else
    {
        sin_half_angle = 1 / sin_half_angle;
        VScalarMul(q->vect_part, sin_half_angle, *axis);
    }
}


static Vector
Sphere_Point(EditInfoPtr info, int x, int y)
{
    Vector  sphere_pt;
    Vector  res;
    double  pt_rad;
    double  s;
    Vector  temp_v;
    Vector  projection;
    double  norm;

    sphere_pt.x = ( x - info->features[origin_feature].view->screen.x ) * 2.0 /
                    (double)info->features[major_feature].circle.width;
    sphere_pt.y = ( info->features[origin_feature].view->screen.y - y ) * 2.0 /
                    (double)info->features[major_feature].circle.width;
    pt_rad = sphere_pt.x * sphere_pt.x + sphere_pt.y * sphere_pt.y;
    if ( pt_rad > 1.0 )
    {
        /* Bring it back to the edge. */
        s = 1.0 / sqrt(pt_rad);
        sphere_pt.x *= s;
        sphere_pt.y *= s;
        sphere_pt.z = 0.0;
    }
    else
        sphere_pt.z = -sqrt(1.0 - pt_rad);

    if ( info->rotate_resultant.feature_1.c_type == null_feature )
    {
        MVMul(info->window->viewport.view_to_world.matrix, sphere_pt, res);
        return res;
    }

    s = VDot(sphere_pt, constraint_axis);
    VScalarMul(constraint_axis, s, temp_v);
    VSub(sphere_pt, temp_v, projection);

    norm = VMod(projection);

    if ( norm > 0 )
    {
        s = 1.0 / norm;
        if ( projection.z > 0 ) s = -s;
        VScalarMul(projection, s, sphere_pt);
    }
    else if ( constraint_axis.z == 1.0 )
        VNew(1, 0, 0, sphere_pt);
    else
    {
        VNew(-constraint_axis.y, constraint_axis.x, 0, temp_v);
        VUnit(temp_v, norm, sphere_pt);
    }

    MVMul(info->window->viewport.view_to_world.matrix, sphere_pt, res);

    return res;
}



void
Edit_Align(EditInfoPtr info, Matrix *matrix)
{
    Vector  temp_v;
    int     i;

    previous_rot = info->drag_transform.matrix = *matrix;

    /* Rotate the scaling and radius points. */
    for ( i = scale_feature ; i < info->obj->o_num_features ; i++ )
    {
        if ( ! info->obj->o_features[i].base )  continue;

        MVMul(previous_rot, info->obj->o_features[i].location, temp_v);
        info->obj->o_features[i].location = temp_v;
    }

    start_center = info->obj->o_world_verts[info->obj->o_num_vertices - 1];
    start_major = info->obj->o_features[major_feature].location;
    start_minor = info->obj->o_features[minor_feature].location;
    init_obj_transform = info->obj->o_transform;

    Rotate_Object(info);

    Edit_Finish_Rotate_Drag(NULL, 0, info);
}


