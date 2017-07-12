/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/bezier.c,v 1.2 1997/06/11 21:12:52 schenney Exp $
 *
 * $Log: bezier.c,v $
 * Revision 1.2  1997/06/11 21:12:52  schenney
 * Added vertex normal calculation function fro control pt objects.
 *
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
**  bezier.c: Code for manipulating bezier patches.
*/

#include <math.h>
#include <sced.h>
#include <Vector4.h>
#include <add_constraint.h>
#include <bezier.h>
#include <kd_tree.h>

static Matrix4  bezier_basis = { { -1, 3, -3, 1 }, { 3, -6, 3, 0 },
                                 { -3, 3, 0, 0 }, { 1, 0, 0, 0 } };

int bezier_map[] = { 0, 4, 5, 1, 11, 12, 13, 6, 10, 15, 14, 7, 3, 9, 8, 2 };


static void
Bezier_New_Controls(Vector *new_verts, Vector *orig_verts)
{
    Vector  temp_v;

    new_verts[0] = orig_verts[0];
    VAdd(orig_verts[0], orig_verts[1], new_verts[1]);
    VScalarMul(new_verts[1], 0.5, new_verts[1]);
    VAdd(orig_verts[1], orig_verts[2], temp_v);
    VScalarMul(temp_v, 0.5, temp_v);
    VAdd(new_verts[1], temp_v, new_verts[2]);
    VScalarMul(new_verts[2], 0.5, new_verts[2]);
    new_verts[7] = orig_verts[3];
    VAdd(orig_verts[2], orig_verts[3], new_verts[6]);
    VScalarMul(new_verts[6], 0.5, new_verts[6]);
    VAdd(temp_v, new_verts[6], new_verts[5]);
    VScalarMul(new_verts[5], 0.5, new_verts[5]);
    VAdd(new_verts[2], new_verts[5], new_verts[3]);
    VScalarMul(new_verts[3], 0.5, new_verts[3]);
    new_verts[4] = new_verts[3];
}

static void
Bezier_Subdivide(Vector controls[], int ll, int lr, int rl, int rr,
                 VectorPtr verts)
{
    Vector  new_controls[16];
    Vector  one_split_controls[32];
    Vector  next_split_controls[32];
    Vector  split_controls[4];
    int     i, j, count;
    int     new_lr, new_rl;

    if ( lr - ll == 1 )
    {
        verts[ll] = controls[0];
        verts[lr] = controls[3];
        verts[rl] = controls[12];
        verts[rr] = controls[15];
        return;
    }

    new_lr = ( ll + lr ) >> 1;
    new_rl = ( ll + rl ) >> 1;

    /* Start by evaluating all the new control points in one direction. */
    for ( i = 0 ; i < 16 ; i += 4 )
        Bezier_New_Controls(one_split_controls + ( i << 1 ), controls + i);
    
    /* Use those to get the other direction. */
    for ( i = 0 ; i < 4 ; i++ )
    {
        split_controls[0] = one_split_controls[i];
        split_controls[1] = one_split_controls[i + 8];
        split_controls[2] = one_split_controls[i + 16];
        split_controls[3] = one_split_controls[i + 24];
        Bezier_New_Controls(next_split_controls + ( i * 8 ), split_controls);
    }

    /* Do 2 subdivisions. */
    count = 0;
    for ( i = 0 ; i < 4 ; i++ )
        for ( j = 0 ; j < 4 ; j++ )
            new_controls[count++] = next_split_controls[i + 8 * j];
    Bezier_Subdivide(new_controls, ll, new_lr, new_rl, new_rl + ( new_lr - ll ),
                     verts);

    count = 0;
    for ( i = 4 ; i < 8 ; i++ )
        for ( j = 0 ; j < 4 ; j++ )
            new_controls[count++] = next_split_controls[i + 8 * j];
    Bezier_Subdivide(new_controls, new_rl, new_rl + ( new_lr - ll ), rl,
                     ( rr + rl ) >> 1, verts);

    /* Get some more control pts. */
    for ( i = 0 ; i < 4 ; i++ )
    {
        split_controls[0] = one_split_controls[i + 4];
        split_controls[1] = one_split_controls[i + 12];
        split_controls[2] = one_split_controls[i + 20];
        split_controls[3] = one_split_controls[i + 28];
        Bezier_New_Controls(next_split_controls + ( i * 8 ), split_controls);
    }

    /* And the other 2 subdivisions. */
    count = 0;
    for ( i = 0 ; i < 4 ; i++ )
        for ( j = 0 ; j < 4 ; j++ )
            new_controls[count++] = next_split_controls[i + 8 * j];
    Bezier_Subdivide(new_controls, new_lr, lr, new_rl + ( new_lr - ll ),
                     ( rr + lr ) >> 1, verts);

    count = 0;
    for ( i = 4 ; i < 8 ; i++ )
        for ( j = 0 ; j < 4 ; j++ )
            new_controls[count++] = next_split_controls[i + 8 * j];
    Bezier_Subdivide(new_controls, new_rl + ( new_lr - ll ),
                     ( rr + lr ) >> 1, ( rr + rl ) >> 1, rr, verts);

}


static void
Bezier_Calculate_Vertices(ObjectInstancePtr obj, VectorPtr verts,
                          int num_real, int num)
{
    Vector  controls[16];
    int     num_lines;
    int     i, j;

    num_lines = sqrt((double)num_real) - 1;

    /* Call the recursive subdivision procedure. */
    /* We start with the 16 control verts and move on in. */
    for ( i = 0 ; i < 16 ; i++ )
        controls[i] = control_part(obj)->control_verts[bezier_map[i]];

    Bezier_Subdivide(controls, 0, num_lines, num_real - 1 - num_lines,
                     num_real - 1, verts);

    /* Copy the control verts over. */
    for ( i = num_real, j = 0 ; i < num - 1 ; i++, j++ )
        verts[i] = control_part(obj)->control_verts[j];
    for ( ; i < num ; i++ )
        VNew(0, 0, 0, verts[i]);
}


static void
Bezier_Calculate_Normals(ObjectInstancePtr obj, VectorPtr normals)
{
    Vector  temp_v1, temp_v2;
    double  temp_d;
    int     i;

    for ( i = 0 ; i < obj->o_num_faces ; i++ )
    {
        VSub(obj->o_world_verts[obj->o_wireframe->faces[i].vertices[2]],
         obj->o_world_verts[obj->o_wireframe->faces[i].vertices[0]], temp_v1);
        VSub(obj->o_world_verts[obj->o_wireframe->faces[i].vertices[1]],
         obj->o_world_verts[obj->o_wireframe->faces[i].vertices[0]], temp_v2);
        VCross(temp_v1, temp_v2, normals[i]);
        if ( VZero(normals[i]) )
            VNew(1, 0, 0, normals[i]);
        else
            VUnit(normals[i], temp_d, normals[i]);
    }
}

void
Bezier_Calculate_Vertex_Normals(ObjectInstancePtr obj, Vector *normals, int num)
{
    int     num_lines;
    int     i, j, count;
    double  step_size;
    double  u_val, v_val;
    Matrix4 control_x, control_y, control_z;
    Vector4 u_params;
    Vector4 v_params;
    Vector4 working_1, working_2, working_3;
    Vector  u_deriv, v_deriv;
    double  temp_d;
    Matrix  transp;

    num_lines = sqrt((double)num);
    step_size = 1 / (double)(num_lines - 1);
    MTrans(obj->o_inverse, transp);

    control_x.x.x = control_part(obj)->control_verts[0].x;
    control_x.x.y = control_part(obj)->control_verts[11].x;
    control_x.x.z = control_part(obj)->control_verts[10].x;
    control_x.x.w = control_part(obj)->control_verts[3].x;
    control_x.y.x = control_part(obj)->control_verts[4].x;
    control_x.y.y = control_part(obj)->control_verts[12].x;
    control_x.y.z = control_part(obj)->control_verts[15].x;
    control_x.y.w = control_part(obj)->control_verts[9].x;
    control_x.z.x = control_part(obj)->control_verts[5].x;
    control_x.z.y = control_part(obj)->control_verts[13].x;
    control_x.z.z = control_part(obj)->control_verts[14].x;
    control_x.z.w = control_part(obj)->control_verts[8].x;
    control_x.w.x = control_part(obj)->control_verts[1].x;
    control_x.w.y = control_part(obj)->control_verts[6].x;
    control_x.w.z = control_part(obj)->control_verts[7].x;
    control_x.w.w = control_part(obj)->control_verts[2].x;

    control_y.x.x = control_part(obj)->control_verts[0].y;
    control_y.x.y = control_part(obj)->control_verts[11].y;
    control_y.x.z = control_part(obj)->control_verts[10].y;
    control_y.x.w = control_part(obj)->control_verts[3].y;
    control_y.y.x = control_part(obj)->control_verts[4].y;
    control_y.y.y = control_part(obj)->control_verts[12].y;
    control_y.y.z = control_part(obj)->control_verts[15].y;
    control_y.y.w = control_part(obj)->control_verts[9].y;
    control_y.z.x = control_part(obj)->control_verts[5].y;
    control_y.z.y = control_part(obj)->control_verts[13].y;
    control_y.z.z = control_part(obj)->control_verts[14].y;
    control_y.z.w = control_part(obj)->control_verts[8].y;
    control_y.w.x = control_part(obj)->control_verts[1].y;
    control_y.w.y = control_part(obj)->control_verts[6].y;
    control_y.w.z = control_part(obj)->control_verts[7].y;
    control_y.w.w = control_part(obj)->control_verts[2].y;

    control_z.x.x = control_part(obj)->control_verts[0].z;
    control_z.x.y = control_part(obj)->control_verts[11].z;
    control_z.x.z = control_part(obj)->control_verts[10].z;
    control_z.x.w = control_part(obj)->control_verts[3].z;
    control_z.y.x = control_part(obj)->control_verts[4].z;
    control_z.y.y = control_part(obj)->control_verts[12].z;
    control_z.y.z = control_part(obj)->control_verts[15].z;
    control_z.y.w = control_part(obj)->control_verts[9].z;
    control_z.z.x = control_part(obj)->control_verts[5].z;
    control_z.z.y = control_part(obj)->control_verts[13].z;
    control_z.z.z = control_part(obj)->control_verts[14].z;
    control_z.z.w = control_part(obj)->control_verts[8].z;
    control_z.w.x = control_part(obj)->control_verts[1].z;
    control_z.w.y = control_part(obj)->control_verts[6].z;
    control_z.w.z = control_part(obj)->control_verts[7].z;
    control_z.w.w = control_part(obj)->control_verts[2].z;


    count = 0;
    for ( i = 0, u_val = 0 ; i < num_lines ; i++, u_val += step_size )
    {
        for ( j = 0, v_val = 0 ; j < num_lines ; j++, v_val += step_size )
        {
            u_params.w = 0;
            u_params.z = 1;
            u_params.y = u_val * 2;
            u_params.x = u_val * u_val * 3;
            v_params.w = 1;
            v_params.z = v_val;
            v_params.y = v_val * v_val;
            v_params.x = v_params.y * v_val;
            M4V4Mul(bezier_basis, u_params, working_1);
            M4V4Mul(bezier_basis, v_params, working_2);
            M4V4Mul(control_x, working_2, working_3);
            u_deriv.x = V4Dot(working_1, working_3);
            M4V4Mul(control_y, working_2, working_3);
            u_deriv.y = V4Dot(working_1, working_3);
            M4V4Mul(control_z, working_2, working_3);
            u_deriv.z = V4Dot(working_1, working_3);

            v_params.w = 0;
            v_params.z = 1;
            v_params.y = v_val * 2;
            v_params.x = v_val * v_val * 3;
            u_params.w = 1;
            u_params.z = u_val;
            u_params.y = u_val * u_val;
            u_params.x = u_params.y * u_val;
            M4V4Mul(bezier_basis, u_params, working_1);
            M4V4Mul(bezier_basis, v_params, working_2);
            M4V4Mul(control_x, working_2, working_3);
            v_deriv.x = V4Dot(working_1, working_3);
            M4V4Mul(control_y, working_2, working_3);
            v_deriv.y = V4Dot(working_1, working_3);
            M4V4Mul(control_z, working_2, working_3);
            v_deriv.z = V4Dot(working_1, working_3);

            VCross(v_deriv, u_deriv, normals[count]);
            if ( VZero(normals[count]) )
                VNew(0, 0, 1, normals[count]);
            else
                VUnit(normals[count], temp_d, normals[count]);

            count++;
        }
    }
}


void
Bezier_Create_Function(ObjectInstancePtr obj)
{
    int i;

    Create_Generic_Object(obj);

    obj->o_hook = (void*)New(ControlHook, 1);
    control_part(obj)->num_control_verts = 16;
    control_part(obj)->control_verts = New(Vector, 16);
    VNew(0, 0, 0, control_part(obj)->control_verts[0]);
    VNew(0, 2, 0, control_part(obj)->control_verts[1]);
    VNew(2, 2, 0, control_part(obj)->control_verts[2]);
    VNew(2, 0, 0, control_part(obj)->control_verts[3]);
    VNew(0, 0.6666666, 0, control_part(obj)->control_verts[4]);
    VNew(0, 1.3333333, 0, control_part(obj)->control_verts[5]);
    VNew(0.6666666, 2, 0, control_part(obj)->control_verts[6]);
    VNew(1.3333333, 2, 0, control_part(obj)->control_verts[7]);
    VNew(2, 1.3333333, 0, control_part(obj)->control_verts[8]);
    VNew(2, 0.6666666, 0, control_part(obj)->control_verts[9]);
    VNew(1.3333333, 0, 0, control_part(obj)->control_verts[10]);
    VNew(0.6666666, 0, 0, control_part(obj)->control_verts[11]);
    VNew(0.6666666, 0.6666666, 0, control_part(obj)->control_verts[12]);
    VNew(0.6666666, 1.3333333, 0, control_part(obj)->control_verts[13]);
    VNew(1.3333333, 1.3333333, 0, control_part(obj)->control_verts[14]);
    VNew(1.3333333, 0.6666666, 0, control_part(obj)->control_verts[15]);
    control_part(obj)->calc_verts = Bezier_Calculate_Vertices;
    control_part(obj)->calc_face_norms = Bezier_Calculate_Normals;
    control_part(obj)->calc_vert_norms = Bezier_Calculate_Vertex_Normals;

    /* Create the many extra features. */
    obj->o_num_features += 17;
    obj->o_features = More(obj->o_features, Feature, obj->o_num_features);

    Feature_Create_Dummy_Constraints(obj->o_features + pt_dummy_feature);
    for ( i = pt0_feature ; i < pt_last_feature ; i++ )
        Feature_Create_Control_Constraints(obj->o_features + i, obj,
                                           i - pt0_feature);

    obj->o_static_func = Maintain_Control_Obj_Static;
    obj->o_dynamic_func = Maintain_Control_Obj_Dynamic;
}

void
Bezier_Destroy_Function(ObjectInstancePtr victim)
{
    free(control_part(victim)->control_verts);
    free(victim->o_hook);
    Destroy_Generic_Object(victim);
}

void
Bezier_Copy_Hook(ObjectInstancePtr src, ObjectInstancePtr dest)
{
    int i;

    ControlHookPtr  src_hook = (ControlHookPtr)src->o_hook;
    ControlHookPtr  dest_hook = (ControlHookPtr)dest->o_hook;

    for ( i = 0 ; i < src_hook->num_control_verts ; i++ )
        dest_hook->control_verts[i] = src_hook->control_verts[i];
}


Vector
Bezier_Calculate_Point(ObjectInstancePtr obj, double u_val, double v_val)
{
    Matrix4 control_matrix;
    Vector4 u_params;
    Vector4 v_params;
    Vector4 working_1, working_2, working_3;
    Vector  result;

    u_params.w = 1;
    u_params.z = u_val;
    u_params.y = u_val * u_val;
    u_params.x = u_params.y * u_val;
    v_params.w = 1;
    v_params.z = v_val;
    v_params.y = v_val * v_val;
    v_params.x = v_params.y * v_val;

    M4V4Mul(bezier_basis, u_params, working_1);
    M4V4Mul(bezier_basis, v_params, working_2);

    control_matrix.x.x = control_part(obj)->control_verts[0].x;
    control_matrix.x.y = control_part(obj)->control_verts[11].x;
    control_matrix.x.z = control_part(obj)->control_verts[10].x;
    control_matrix.x.w = control_part(obj)->control_verts[3].x;
    control_matrix.y.x = control_part(obj)->control_verts[4].x;
    control_matrix.y.y = control_part(obj)->control_verts[12].x;
    control_matrix.y.z = control_part(obj)->control_verts[15].x;
    control_matrix.y.w = control_part(obj)->control_verts[9].x;
    control_matrix.z.x = control_part(obj)->control_verts[5].x;
    control_matrix.z.y = control_part(obj)->control_verts[13].x;
    control_matrix.z.z = control_part(obj)->control_verts[14].x;
    control_matrix.z.w = control_part(obj)->control_verts[8].x;
    control_matrix.w.x = control_part(obj)->control_verts[1].x;
    control_matrix.w.y = control_part(obj)->control_verts[6].x;
    control_matrix.w.z = control_part(obj)->control_verts[7].x;
    control_matrix.w.w = control_part(obj)->control_verts[2].x;
    M4V4Mul(control_matrix, working_2, working_3);
    result.x = V4Dot(working_1, working_3);

    control_matrix.x.x = control_part(obj)->control_verts[0].y;
    control_matrix.x.y = control_part(obj)->control_verts[11].y;
    control_matrix.x.z = control_part(obj)->control_verts[10].y;
    control_matrix.x.w = control_part(obj)->control_verts[3].y;
    control_matrix.y.x = control_part(obj)->control_verts[4].y;
    control_matrix.y.y = control_part(obj)->control_verts[12].y;
    control_matrix.y.z = control_part(obj)->control_verts[15].y;
    control_matrix.y.w = control_part(obj)->control_verts[9].y;
    control_matrix.z.x = control_part(obj)->control_verts[5].y;
    control_matrix.z.y = control_part(obj)->control_verts[13].y;
    control_matrix.z.z = control_part(obj)->control_verts[14].y;
    control_matrix.z.w = control_part(obj)->control_verts[8].y;
    control_matrix.w.x = control_part(obj)->control_verts[1].y;
    control_matrix.w.y = control_part(obj)->control_verts[6].y;
    control_matrix.w.z = control_part(obj)->control_verts[7].y;
    control_matrix.w.w = control_part(obj)->control_verts[2].y;
    M4V4Mul(control_matrix, working_2, working_3);
    result.y = V4Dot(working_1, working_3);

    control_matrix.x.x = control_part(obj)->control_verts[0].z;
    control_matrix.x.y = control_part(obj)->control_verts[11].z;
    control_matrix.x.z = control_part(obj)->control_verts[10].z;
    control_matrix.x.w = control_part(obj)->control_verts[3].z;
    control_matrix.y.x = control_part(obj)->control_verts[4].z;
    control_matrix.y.y = control_part(obj)->control_verts[12].z;
    control_matrix.y.z = control_part(obj)->control_verts[15].z;
    control_matrix.y.w = control_part(obj)->control_verts[9].z;
    control_matrix.z.x = control_part(obj)->control_verts[5].z;
    control_matrix.z.y = control_part(obj)->control_verts[13].z;
    control_matrix.z.z = control_part(obj)->control_verts[14].z;
    control_matrix.z.w = control_part(obj)->control_verts[8].z;
    control_matrix.w.x = control_part(obj)->control_verts[1].z;
    control_matrix.w.y = control_part(obj)->control_verts[6].z;
    control_matrix.w.z = control_part(obj)->control_verts[7].z;
    control_matrix.w.w = control_part(obj)->control_verts[2].z;
    M4V4Mul(control_matrix, working_2, working_3);
    result.z = V4Dot(working_1, working_3);

    Transform_Vector(obj->o_transform, result, result);

    return result;
}

