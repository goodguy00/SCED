/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/torus.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: torus.c,v $
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
**  torus.c: Functions for the manipulation of tori.
*/

#include <math.h>
#include <sced.h>
#include <torus.h>
#include <add_constraint.h>


/*  Create the hook used to attach extra information to an instance.
*/
void
Torus_Create_Function(ObjectInstancePtr obj)
{
    Create_Generic_Object(obj);

    /* Create and set hook. */
    obj->o_hook = (void*)New(TorusHook, 1);
    ((TorusPtr)obj->o_hook)->major_radius = 2.0;

    /* Create the extra constrained feature. */
    obj->o_num_features++;
    obj->o_features = More(obj->o_features, Feature, obj->o_num_features);

    Feature_Create_Radius_Constraints(obj->o_features + radius_feature, obj);

    obj->o_static_func = Maintain_Torus_Static;
    obj->o_dynamic_func = Maintain_Torus_Dynamic;
}

void
Torus_Destroy_Function(ObjectInstancePtr obj)
{
    free(obj->o_hook);
    Destroy_Generic_Object(obj);
}


/*  Copy a torus hook structure.
*/
void
Torus_Copy_Hook(ObjectInstancePtr src, ObjectInstancePtr dest)
{
    TorusPtr    dest_torus = (TorusPtr)dest->o_hook;
    TorusPtr    src_torus = (TorusPtr)src->o_hook;

    dest_torus->major_radius = src_torus->major_radius;
}


/*  Transform a single wireframe vertex into object space. */
Vector
Torus_Transform_Vertex(ObjectInstancePtr obj, VectorPtr vect)
{
    Vector  to_center;
    Vector  result;
    double  dist;
    double  mult_fact;

    /* The vect is set up for a radius of 2. We need to find it's
    ** distance from the center, modify it for a bigger radius,
    ** and then transform the pt.
    */
    VNew(vect->x, vect->y, 0.0, to_center);
    dist = to_center.x * to_center.x + to_center.y * to_center.y;

    if ( IsZero(dist) )
        VNew(0, 0, vect->z, result);
    else
    {
        dist = sqrt(dist);

        /* dist is the current distance. dist - 2 is the distance from the
        ** center of the tube. The new dist will be major_rad + dist - 2.
        */
        mult_fact = ( torus_part(obj)->major_radius - 2.0 + dist ) / dist;
        VNew(to_center.x * mult_fact, to_center.y * mult_fact, vect->z, result);
    }

    Transform_Vector(obj->o_transform, result, result);

    return result;
}


/* Calculate the vertex positions for a torus with the given radius.
*/
void
Torus_Calculate_Vertices(VectorPtr verts, int num_verts, double rad)
{
    int             round_num;
    int             i, j, index;
    double          u, v, sin_u, cos_u;
    double          delta_uv;
    double          temp_d;

    round_num = (int)sqrt((double)(num_verts - 1));

    index = 0;
    delta_uv = M_PI / ( round_num >> 1 );
    for ( i = 0, u = 0 ; i < round_num ; i++, u += delta_uv )
    {
        sin_u = sin(u);
        cos_u = cos(u);
        for ( j = 0, v = 0 ; j < round_num ; j++, v += delta_uv )
        {
            temp_d = rad + cos(v);
            VNew(temp_d * cos_u, temp_d * sin_u, sin(v), verts[index]);
            index++;
        }
    }

    VNew(0, 0, 0, verts[index]);
}


void
Torus_Calculate_Vertex_Normals(ObjectInstancePtr obj, Vector *normals, int num,
                               Boolean do_transform)
{
    int     round_num;
    int     i, j, index;
    double  u, v, sin_u, cos_u;
    double  delta_uv;
    double  temp_d;
    Vector  pt, center;
    Vector  temp_v;
    Matrix  transp;

    round_num = (int)sqrt((double)num);

    index = 0;
    MTrans(obj->o_inverse, transp);
    delta_uv = M_PI / ( round_num >> 1 );
    for ( i = 0, u = 0 ; i < round_num ; i++, u += delta_uv )
    {
        sin_u = sin(u);
        cos_u = cos(u);
        VNew(torus_part(obj)->major_radius * cos_u,
             torus_part(obj)->major_radius * sin_u,
             0, center);
        for ( j = 0, v = 0 ; j < round_num ; j++, v += delta_uv )
        {
            temp_d = torus_part(obj)->major_radius + cos(v);
            VNew(temp_d * cos_u, temp_d * sin_u, sin(v), pt);
            VSub(pt, center, normals[index]);
            if ( VZero(normals[index]) )
                VNew(0, 0, 1, normals[index]);
            else
            {
                if ( do_transform )
                {
                    MVMul(transp, normals[index], temp_v);
                    VUnit(temp_v, temp_d, normals[index]);
                }
                else
                    VUnit(normals[index], temp_d, normals[index]);
            }
                    
            index++;
        }
    }
}
