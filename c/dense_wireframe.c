/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/dense_wireframe.c,v 1.2 1997/06/11 21:12:52 schenney Exp $
 *
 * $Log: dense_wireframe.c,v $
 * Revision 1.2  1997/06/11 21:12:52  schenney
 * Added vertex normal calculation function for control pt objects.
 *
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
**  dense_wireframe.c : Functions for manipulating and generating dense
**                      wireframes.
*/

#include <math.h>
#include <sced.h>
#include <aggregate.h>
#include <bezier.h>
#include <csg.h>
#include <csg_wire.h>
#include <edge_table.h>
#include <gen_wireframe.h>
#include <torus.h>


#define Dense_Possible(base) ( (base)->b_class == sphere_obj || \
                               (base)->b_class == cone_obj || \
                               (base)->b_class == cylinder_obj || \
                               (base)->b_class == torus_obj || \
                               (base)->b_class == spotlight_obj || \
                               (base)->b_class == bezier_obj || \
                               (base)->b_class == csg_obj || \
			       (base)->b_class == aggregate_obj )

int
Wireframe_Density_Level(ObjectInstancePtr inst)
{
    int level = 0;

    /* Work out which level it's currently at. */
    if ( inst->o_parent )
    {
        for ( level = 0 ;
              level <= inst->o_parent->b_max_density &&
              inst->o_parent->b_wireframes[level] != inst->o_wireframe ;
              level++ );

        if ( level > inst->o_parent->b_max_density )
        {
            fprintf(stderr,
                    "Sced: Existing wireframe not found in parent: Object %s\n",
                    inst->o_label);
            return 0;
        }
    }

    return level;
}


void
Wireframe_Denser_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    WindowInfoPtr   window = (WindowInfoPtr)cl;
    InstanceList    elmt;

    for ( elmt = window->selected_instances ; elmt ; elmt = elmt->next )
        Object_Change_Wire_Level(elmt->the_instance,
                            Wireframe_Density_Level(elmt->the_instance) + 1);

    View_Update(window, window->selected_instances, CalcView);
    Update_Projection_Extents(window->selected_instances);
}


void
Wireframe_Thinner_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    WindowInfoPtr   window = (WindowInfoPtr)cl;
    InstanceList    elmt;

    for ( elmt = window->selected_instances ; elmt ; elmt = elmt->next )
        Object_Change_Wire_Level(elmt->the_instance,
                            Wireframe_Density_Level(elmt->the_instance) - 1);

    View_Update(window, window->selected_instances, CalcView);
    Update_Projection_Extents(window->selected_instances);
}


/* Returns the dense wireframe for the given base and the given level. */
WireframePtr
Dense_Wireframe(BaseObjectPtr base, int level, Boolean major)
{
    if ( ! base )
        return NULL;

    if ( ! Dense_Possible(base) )
        return ( major ? base->b_major_wires[0] : base->b_wireframes[0] );

    if ( level > base->b_max_density + 1 )
        Dense_Wireframe(base, level - 1, major);

    if ( level == base->b_max_density + 1 )
    {
        base->b_wireframes = More(base->b_wireframes, WireframePtr, level + 1);
        base->b_major_wires = More(base->b_major_wires, WireframePtr, level+1);

        switch ( base->b_class )
        {
            case sphere_obj:
                base->b_wireframes[level] =
                    Dense_Sphere_Wireframe(level,
                        ( level == 1 ? NULL : base->b_wireframes[level - 1]));
                base->b_major_wires[level] =
			    Wireframe_Triangulate(base->b_wireframes[level]);
                break;
            case cone_obj:
                base->b_wireframes[level] = Dense_Cone_Wireframe(level);
                base->b_major_wires[level] =
			    Wireframe_Triangulate(base->b_wireframes[level]);
                break;
            case cylinder_obj:
                base->b_wireframes[level] = Dense_Cylinder_Wireframe(level);
                base->b_major_wires[level] =
			    Wireframe_Triangulate(base->b_wireframes[level]);
                break;
            case torus_obj:
                base->b_wireframes[level] = Dense_Torus_Wireframe(level);
                base->b_major_wires[level] =
			    Wireframe_Triangulate(base->b_wireframes[level]);
                break;
            case bezier_obj:
                base->b_wireframes[level] = Dense_Bezier_Wireframe(level);
                base->b_major_wires[level] = Dense_Bezier_Wireframe(level);
                break;
            case spotlight_obj:
                base->b_wireframes[level] = Dense_Spot_Wireframe(level);
                base->b_major_wires[level] =
			    Wireframe_Triangulate(base->b_wireframes[level]);
                break;
            case aggregate_obj:
                base->b_wireframes[level] = Agg_Generate_Wireframe(
		    ((AggregatePtr)base->b_struct)->children, level, AGG_ALL);
                base->b_major_wires[level] = Agg_Generate_Wireframe(
		    ((AggregatePtr)base->b_struct)->children, level,
		    AGG_ALL | AGG_MAJOR);
                break;
            case csg_obj:
		base->b_major_wires[level] = CSG_Generate_Wireframe(
				    (CSGNodePtr)base->b_struct, level, FALSE);
                base->b_wireframes[level] =
                    Dense_CSG_Wireframe(level, base, base->b_use_full);
                break;
            default:;
        }
        Edge_Table_Build(base->b_wireframes[level]);
        base->b_max_density++;
    }

    return ( major ? base->b_major_wires[level] : base->b_wireframes[level] );
}


void
Object_Change_Wire_Level(ObjectInstancePtr inst, int level)
{
    if ( ( ! inst->o_parent ) || ! Dense_Possible(inst->o_parent) )
        return;

    if ( level < 0 )
        level = 0;

    if ( level == 0 )
        inst->o_wireframe = inst->o_parent->b_wireframes[0];
    else
        inst->o_wireframe = Dense_Wireframe(inst->o_parent, level, FALSE);

    Object_Change_Wire(inst);
}


void
Object_Change_Wire(ObjectInstancePtr inst)
{
    Matrix  transp;
    double  temp_d;
    int     i;

    inst->o_num_vertices = inst->o_wireframe->num_vertices;
    inst->o_num_real = inst->o_wireframe->num_real_verts;
    free(inst->o_world_verts);
    inst->o_world_verts = New(Vector, inst->o_num_vertices);
    free(inst->o_main_verts);
    inst->o_main_verts = New(Vertex, inst->o_num_vertices);
    if ( Obj_Is_Torus(inst) )
    {
        Torus_Calculate_Vertices(inst->o_world_verts, inst->o_num_vertices,
                                 ((TorusPtr)inst->o_hook)->major_radius);
        Transform_Vertices(inst->o_transform, inst->o_world_verts,
                           inst->o_num_vertices)
    }
    else if ( Obj_Is_Bezier(inst) )
    {
	control_part(inst)->calc_verts(inst, inst->o_world_verts,
				       inst->o_num_real, inst->o_num_vertices);
        Transform_Vertices(inst->o_transform, inst->o_world_verts,
                           inst->o_num_vertices)
    }
    else
        for ( i = 0 ; i < inst->o_num_vertices ; i++ )
            Transform_Vector(inst->o_transform, inst->o_wireframe->vertices[i],
                             inst->o_world_verts[i])
    inst->o_num_faces = inst->o_wireframe->num_faces;
    free(inst->o_normals);
    inst->o_normals = New(Vector, inst->o_num_faces);
    if ( Obj_Is_Bezier(inst) )
        control_part(inst)->calc_face_norms(inst, inst->o_normals);
    else
    {
        MTrans(inst->o_inverse, transp);
        for ( i = 0 ; i < inst->o_num_faces ; i++ )
        {
            MVMul(transp, inst->o_wireframe->faces[i].normal,
                  inst->o_normals[i]);
            VUnit(inst->o_normals[i], temp_d, inst->o_normals[i]);
        }
    }
}



/*  Wireframe*
**  Dense_Cylinder_Wireframe(int level);
**  Returns a pointer to a NEW dense wireframe structure, with
**  2 ^ ( level + 3 ) vertices on each endcap.
*/
WireframePtr
Dense_Cylinder_Wireframe(int level)
{
    WireframePtr    result = New(Wireframe, 1);
    double          angle;
    double          total_angle;
    int             half_num;
    int             i;
    Vector          temp_v, vert_vect;
    double          temp_d;

    result->num_faces = ( 1 << ( level + 3 ) ) + 2;
    result->num_real_verts = ( 1 << ( level + 4 ) );
    result->num_normals = ( 1 << ( level + 4 ) ) + 2;
    result->num_vertices = ( 1 << ( level + 4 ) ) + 3;

    result->vertices = New(Vector, result->num_vertices);
    result->normals = New(Vector, result->num_normals);

    /* Work out what the angle subtended at the center by the arc joining
    ** neighbouring vertices is.
    */
    angle = M_PI_4 / ( 1 << level );
    total_angle = 0.0;

    half_num = ( 1 << ( level + 3 ) );
    for ( i = 0, total_angle = 0.0 ; i < half_num ; i++, total_angle += angle )
    {
        result->vertices[i].x =
        result->vertices[i + half_num].x =
        result->normals[i].x =
        result->normals[i + half_num].x = cos(total_angle);
        result->vertices[i].y =
        result->vertices[i + half_num].y =
        result->normals[i].y =
        result->normals[i + half_num].y = sin(total_angle);
        result->vertices[i].z = 1;
        result->vertices[i + half_num].z = -1;
        result->normals[i].z =
        result->normals[i + half_num].z = 0;
    }
    VNew(0, 0, 1, result->vertices[result->num_vertices - 3]);
    VNew(0, 0, -1, result->vertices[result->num_vertices - 2]);
    VNew(0, 0, 0, result->vertices[result->num_vertices - 1]);
    VNew(0, 0, 1, result->normals[result->num_normals - 2]);
    VNew(0, 0, -1, result->normals[result->num_normals - 1]);

    result->faces = New(Face, result->num_faces);
    for ( i = 0 ; i < result->num_faces - 2 ; i++ )
    {
        result->faces[i].num_vertices = 4;
        result->faces[i].vertices = New(int, 4);
        result->faces[i].normals = New(int, 4);

        result->faces[i].vertices[0] =
        result->faces[i].normals[0] = i;
        result->faces[i].vertices[1] =
        result->faces[i].normals[1] = i + 1;
        result->faces[i].vertices[2] =
        result->faces[i].normals[2] = half_num + 1 + i;
        result->faces[i].vertices[3] =
        result->faces[i].normals[3] = half_num + i;

        result->faces[i].face_attribs = NULL;
    }
    result->faces[result->num_faces - 3].vertices[1] = 0;
    result->faces[result->num_faces - 3].vertices[2] = half_num;
    result->faces[result->num_faces - 3].normals[1] = 0;
    result->faces[result->num_faces - 3].normals[2] = half_num;
    VNew(0, 0, 1, vert_vect);
    for ( i = 0 ; i < result->num_faces - 2 ; i++ )
    {
        VSub(result->vertices[result->faces[i].vertices[1]],
             result->vertices[result->faces[i].vertices[0]], temp_v);
        VCross(temp_v, vert_vect, result->faces[i].normal);
        VUnit(result->faces[i].normal, temp_d, result->faces[i].normal);
    }

    for ( i = result->num_faces - 2 ; i < result->num_faces ; i++ )
    {
        result->faces[i].num_vertices = half_num;
        result->faces[i].vertices = New(int, half_num);
        result->faces[i].normals = New(int, half_num);
        result->faces[i].face_attribs = NULL;
    }
    for ( i = 0 ; i < half_num ; i++ )
    {
        result->faces[result->num_faces-2].vertices[i] = half_num - 1 - i;
        result->faces[result->num_faces-2].normals[i] = result->num_normals - 2;
    }
    VNew(0, 0, 1, result->faces[result->num_faces - 2].normal);
    for ( i = 0 ; i < half_num ; i++ )
    {
        result->faces[result->num_faces-1].vertices[i] = half_num + i;
        result->faces[result->num_faces-1].normals[i] = result->num_normals - 1;
    }
    VNew(0, 0, -1, result->faces[result->num_faces - 1].normal);

    result->edges = NULL;
    result->attribs = NULL;
    result->num_attribs = 0;

    return result;
}



/*  WireframePtr
**  Dense_Cone_Wireframe(int level);
**  Returns a pointer to a NEW dense wireframe cone.
**  NULL on failure.
*/
WireframePtr
Dense_Cone_Wireframe(int level)
{
    WireframePtr    result = New(Wireframe, 1);
    double          angle;
    double          total_angle;
    int             i;
    double          sqrt1_5 = sqrt(0.2);
    Vector          temp_v1, temp_v2;
    double          temp_d;

    result->num_faces = ( 1 << ( level + 3 ) ) + 1;
    result->num_vertices = ( 1 << ( level + 3 ) ) + 3;
    result->num_real_verts = ( 1 << ( level + 3 ) ) + 1;
    result->num_normals = ( 1 << ( level + 3 ) ) + 1;

    result->vertices = New(Vector, result->num_vertices);
    result->normals = New(Vector, result->num_normals);

    /* Work out what the angle subtended at the center by the arc joining
    ** neighbouring vertices is.
    */
    angle = M_PI_4 / ( 1 << level );
    total_angle = 0.0;

    VNew(0, 0, 1, result->vertices[0]);
    for ( i = 1, total_angle = 0.0 ;
          i < result->num_vertices - 2 ;
          i++, total_angle += angle )
    {
        result->vertices[i].x = cos(total_angle);
        result->vertices[i].y = sin(total_angle);
        result->vertices[i].z = -1;
        result->normals[i-1].x = result->vertices[i].x * 2.0 * sqrt1_5;
        result->normals[i-1].y = result->vertices[i].y * 2.0 * sqrt1_5;
        result->normals[i-1].z = sqrt1_5;
    }
    VNew(0, 0, -1, result->vertices[result->num_vertices - 2]);
    VNew(0, 0, 0, result->vertices[result->num_vertices - 1]);
    VNew(0, 0, -1, result->normals[result->num_normals - 1]);

    result->faces = New(Face, result->num_faces);
    for ( i = 0 ; i < result->num_faces - 1 ; i++ )
    {
        result->faces[i].num_vertices = 3;
        result->faces[i].vertices = New(int, 3);
        result->faces[i].normals = New(int, 3);

        result->faces[i].vertices[0] = 0;
        result->faces[i].vertices[1] = i + 2;
        result->faces[i].vertices[2] = i + 1;
        result->faces[i].normals[0] = i;
        result->faces[i].normals[1] = i + 1;
        result->faces[i].normals[2] = i;

        result->faces[i].face_attribs = NULL;
    }
    result->faces[result->num_faces - 2].vertices[1] = 1;
    result->faces[result->num_faces - 2].normals[1] = 0;
    for ( i = 0 ; i < result->num_faces - 1 ; i++ )
    {
        VSub(result->vertices[result->faces[i].vertices[0]],
             result->vertices[result->faces[i].vertices[1]], temp_v1);
        VSub(result->vertices[result->faces[i].vertices[2]],
             result->vertices[result->faces[i].vertices[1]], temp_v2);
        VCross(temp_v1, temp_v2, result->faces[i].normal);
        VUnit(result->faces[i].normal, temp_d, result->faces[i].normal);
    }

    result->faces[result->num_faces - 1].num_vertices = result->num_vertices-3;
    result->faces[result->num_faces - 1].vertices =
        New(int, result->num_vertices - 3);
    result->faces[result->num_faces - 1].normals =
        New(int, result->num_vertices - 3);
    result->faces[result->num_faces - 1].face_attribs = NULL;
    for ( i = 0 ; i < result->num_vertices - 3 ; i++ )
    {
        result->faces[result->num_faces-1].vertices[i] = 1 + i;
        result->faces[result->num_faces-1].normals[i] = result->num_normals - 1;
    }
    VNew(0, 0, -1, result->faces[result->num_faces - 1].normal);

    result->edges = NULL;
    result->attribs = NULL;
    result->num_attribs = 0;

    return result;
}


static WireframePtr
Sphere_Build_Initial_Wireframe()
{
    WireframePtr    initial = New(Wireframe, 1);
    WireframePtr    result;
    int             i;

    initial->num_faces = 8;
    initial->num_vertices = 7;
    initial->num_normals = 6;
    initial->num_real_verts = 6;

    initial->vertices = New(Vector, 7);
    VNew(1, 0, 0, initial->vertices[0]);
    VNew(-1, 0, 0, initial->vertices[1]);
    VNew(0, 1, 0, initial->vertices[2]);
    VNew(0, -1, 0, initial->vertices[3]);
    VNew(0, 0, 1, initial->vertices[4]);
    VNew(0, 0, -1, initial->vertices[5]);
    VNew(0, 0, 0, initial->vertices[6]);

    initial->normals = New(Vector, 6);
    for ( i = 0 ; i < initial->num_normals ; i++ )
        initial->normals[i] = initial->vertices[i];

    initial->faces = New(Face, 8);
    for ( i = 0 ; i < 8 ; i++ )
    {
        initial->faces[i].num_vertices = 3;
        initial->faces[i].vertices = New(int, 3);
        initial->faces[i].normals = New(int, 3);
        VNew(1, 1, 1, initial->faces[i].normal);
        initial->faces[i].face_attribs = NULL;
    }
    initial->faces[0].normals[0] = initial->faces[0].vertices[0] = 0;
    initial->faces[0].normals[1] = initial->faces[0].vertices[1] = 4;
    initial->faces[0].normals[2] = initial->faces[0].vertices[2] = 2;
    initial->faces[1].normals[0] = initial->faces[1].vertices[0] = 2;
    initial->faces[1].normals[1] = initial->faces[1].vertices[1] = 4;
    initial->faces[1].normals[2] = initial->faces[1].vertices[2] = 1;
    initial->faces[2].normals[0] = initial->faces[2].vertices[0] = 1;
    initial->faces[2].normals[1] = initial->faces[2].vertices[1] = 4;
    initial->faces[2].normals[2] = initial->faces[2].vertices[2] = 3;
    initial->faces[3].normals[0] = initial->faces[3].vertices[0] = 3;
    initial->faces[3].normals[1] = initial->faces[3].vertices[1] = 4;
    initial->faces[3].normals[2] = initial->faces[3].vertices[2] = 0;
    initial->faces[4].normals[0] = initial->faces[4].vertices[0] = 0;
    initial->faces[4].normals[1] = initial->faces[4].vertices[1] = 2;
    initial->faces[4].normals[2] = initial->faces[4].vertices[2] = 5;
    initial->faces[5].normals[0] = initial->faces[5].vertices[0] = 2;
    initial->faces[5].normals[1] = initial->faces[5].vertices[1] = 1;
    initial->faces[5].normals[2] = initial->faces[5].vertices[2] = 5;
    initial->faces[6].normals[0] = initial->faces[6].vertices[0] = 1;
    initial->faces[6].normals[1] = initial->faces[6].vertices[1] = 3;
    initial->faces[6].normals[2] = initial->faces[6].vertices[2] = 5;
    initial->faces[7].normals[0] = initial->faces[7].vertices[0] = 3;
    initial->faces[7].normals[1] = initial->faces[7].vertices[1] = 0;
    initial->faces[7].normals[2] = initial->faces[7].vertices[2] = 5;

    Edge_Table_Build(initial);

    initial->attribs = NULL;
    initial->num_attribs = 0;

    result = Dense_Sphere_Wireframe(0, initial);

    Edge_Table_Build(result);

    Wireframe_Destroy(initial);

    return result;
}


/* Looks for the edge in the edge table, and if found, returns it's flag.
** Otherwise, sets the flag to new and returns that.
*/
static int
Edge_Add_Edge(EdgeTable table, int v1, int v2, int new)
{
    EdgePtr the_edge = Edge_Table_Get(table, v1, v2);

    if ( the_edge->val == (void*)-1 )
        the_edge->val = (void*)new;
    else
        new = (int)the_edge->val;

    return new;
}


/*  WireframePtr
**  Dense_Sphere_Wireframe(int level, WireframePtr previous);
**  Returns a pointer to a NEW dense wireframe structure for a sphere.
**  The algorithm builds on the previous approximation.
*/
WireframePtr
Dense_Sphere_Wireframe(int level, WireframePtr previous)
{
    WireframePtr    result = New(Wireframe, 1);
    Vector          temp_v1, temp_v2;
    double          temp_d;
    int             vert_count;
    int             face_count;
    int             i, j;
    int             new1, new2, new3;
    Boolean         built_prev = FALSE;

#define Midpoint(v1, v2, r) \
    (r).x = ( (v1).x + (v2).x ) * 0.5; \
    (r).y = ( (v1).y + (v2).y ) * 0.5; \
    (r).z = ( (v1).z + (v2).z ) * 0.5;

    if ( ! previous )
    {
        previous = Sphere_Build_Initial_Wireframe();
        built_prev = TRUE;
    }

    result->num_faces = previous->num_faces * 4;
    result->num_normals =
    result->num_real_verts = previous->num_real_verts +
                             previous->num_faces * 3 * 0.5;
    result->num_vertices = previous->num_vertices +
                           previous->num_faces * 3 * 0.5;

    /* Clear edge flags. */
    for ( i = 0 ; i < previous->num_real_verts ; i++ )
        for ( j = 0 ; j < previous->edges[i].num_edges ; j++ )
            previous->edges[i].edges[j].val = (void*)-1;

    /* Copy over the common vertices. */
    result->vertices = New(Vector, result->num_vertices);
    result->normals = New(Vector, result->num_normals);
    for ( i = 0 ; i < previous->num_vertices - 1 ; i++ )
        result->normals[i] = result->vertices[i] = previous->vertices[i];
    VNew(0, 0, 0, result->vertices[result->num_vertices - 1]);

    result->faces = New(Face, result->num_faces);
    /* Split faces, adding new vertices as we go. */
    face_count = 0;
    vert_count = previous->num_vertices - 1;
    for ( i = 0, face_count = 0 ; i < previous->num_faces ; i++ )
    {
        new1 = Edge_Add_Edge(previous->edges, previous->faces[i].vertices[0],
                               previous->faces[i].vertices[2], vert_count);
        if ( vert_count == new1 )
        {
            Midpoint(previous->vertices[previous->faces[i].vertices[0]],
                     previous->vertices[previous->faces[i].vertices[2]],
                     result->vertices[vert_count]);
            VUnit(result->vertices[vert_count], temp_d,
                  result->vertices[vert_count]);
            result->normals[vert_count] = result->vertices[vert_count];
            vert_count++;
        }
        new2 = Edge_Add_Edge(previous->edges, previous->faces[i].vertices[0],
                               previous->faces[i].vertices[1], vert_count);
        if ( vert_count == new2 )
        {
            Midpoint(previous->vertices[previous->faces[i].vertices[0]],
                     previous->vertices[previous->faces[i].vertices[1]],
                     result->vertices[vert_count]);
            VUnit(result->vertices[vert_count], temp_d,
                  result->vertices[vert_count]);
            result->normals[vert_count] = result->vertices[vert_count];
            vert_count++;
        }
        new3 = Edge_Add_Edge(previous->edges, previous->faces[i].vertices[1],
                               previous->faces[i].vertices[2], vert_count);
        if ( vert_count == new3 )
        {
            Midpoint(previous->vertices[previous->faces[i].vertices[1]],
                     previous->vertices[previous->faces[i].vertices[2]],
                     result->vertices[vert_count]);
            VUnit(result->vertices[vert_count], temp_d,
                  result->vertices[vert_count]);
            result->normals[vert_count] = result->vertices[vert_count];
            vert_count++;
        }

        /* Now have three new vertex indicies. */
        /* Create new faces. */
        result->faces[face_count].num_vertices = 3;
        result->faces[face_count].vertices = New(int, 3);
        result->faces[face_count].normals = New(int, 3);
        result->faces[face_count].normals[0] =
        result->faces[face_count].vertices[0] = previous->faces[i].vertices[0];
        result->faces[face_count].normals[1] =
        result->faces[face_count].vertices[1] = new2;
        result->faces[face_count].normals[2] =
        result->faces[face_count].vertices[2] = new1;
        result->faces[face_count].face_attribs = NULL;
        face_count++;

        result->faces[face_count].num_vertices = 3;
        result->faces[face_count].vertices = New(int, 3);
        result->faces[face_count].normals = New(int, 3);
        result->faces[face_count].normals[0] =
        result->faces[face_count].vertices[0] = new2;
        result->faces[face_count].normals[1] =
        result->faces[face_count].vertices[1] = previous->faces[i].vertices[1];
        result->faces[face_count].normals[2] =
        result->faces[face_count].vertices[2] = new3;
        result->faces[face_count].face_attribs = NULL;
        face_count++;

        result->faces[face_count].num_vertices = 3;
        result->faces[face_count].vertices = New(int, 3);
        result->faces[face_count].normals = New(int, 3);
        result->faces[face_count].normals[0] =
        result->faces[face_count].vertices[0] = new1;
        result->faces[face_count].normals[1] =
        result->faces[face_count].vertices[1] = new2;
        result->faces[face_count].normals[2] =
        result->faces[face_count].vertices[2] = new3;
        result->faces[face_count].face_attribs = NULL;
        face_count++;

        result->faces[face_count].num_vertices = 3;
        result->faces[face_count].vertices = New(int, 3);
        result->faces[face_count].normals = New(int, 3);
        result->faces[face_count].normals[0] =
        result->faces[face_count].vertices[0] = new1;
        result->faces[face_count].normals[1] =
        result->faces[face_count].vertices[1] = new3;
        result->faces[face_count].normals[2] =
        result->faces[face_count].vertices[2] = previous->faces[i].vertices[2];
        result->faces[face_count].face_attribs = NULL;
        face_count++;
    }

    /* Set face normals. */
    for ( i = 0 ; i < result->num_faces ; i++ )
    {
        VSub(result->vertices[result->faces[i].vertices[0]],
             result->vertices[result->faces[i].vertices[1]], temp_v1);
        VSub(result->vertices[result->faces[i].vertices[2]],
             result->vertices[result->faces[i].vertices[1]], temp_v2);
        VCross(temp_v1, temp_v2, result->faces[i].normal);
        VUnit(result->faces[i].normal, temp_d, result->faces[i].normal);
    }

    result->attribs = NULL;
    result->num_attribs = 0;
    result->edges = NULL;

    if ( built_prev )
        Wireframe_Destroy(previous);

    return result;
}


/*  WireframePtr
**  Dense_Spot_Wireframe(int level);
**  Returns a pointer to a NEW dense spotlight wireframe.
**  NULL on failure.
*/
WireframePtr
Dense_Spot_Wireframe(int level)
{
    WireframePtr    result = New(Wireframe, 1);
    double          angle;
    double          total_angle;
    int             i, j;
    Vector          temp_v1, temp_v2;
    double          temp_d;

    result->num_faces = ( 1 << ( level + 3 ) );
    result->num_vertices = ( 1 << ( level + 3 ) ) + 2;
    result->num_real_verts = ( 1 << ( level + 3 ) ) + 1;
    result->num_normals = 1;

    result->vertices = New(Vector, result->num_vertices);
    result->normals = New(Vector, result->num_normals);
    VNew(0, 0, 1, result->normals[0]);

    /* Work out what the angle subtended at the center by the arc joining
    ** neighbouring vertices is.
    */
    angle = M_PI_4 / ( 1 << level );
    total_angle = 0.0;

    VNew(0, 0, 1, result->vertices[0]);
    for ( i = 1, total_angle = 0.0 ;
          i < result->num_vertices - 1 ;
          i++, total_angle += angle )
    {
        result->vertices[i].x = cos(total_angle);
        result->vertices[i].y = sin(total_angle);
        result->vertices[i].z = 0;
    }
    VNew(0, 0, 0, result->vertices[result->num_vertices - 1]);

    result->faces = New(Face, result->num_faces);
    for ( i = 0 ; i < result->num_faces ; i++ )
    {
        result->faces[i].num_vertices = 3;
        result->faces[i].vertices = New(int, 3);
        result->faces[i].normals = New(int, 3);

        result->faces[i].vertices[0] = 0;
        result->faces[i].vertices[1] = i + 2;
        result->faces[i].vertices[2] = i + 1;

        for ( j = 0 ; j < 3 ; j++ )
            result->faces[i].normals[j] = 0;

        result->faces[i].face_attribs = NULL;
    }
    result->faces[result->num_faces - 1].vertices[1] = 1;
    for ( i = 0 ; i < result->num_faces ; i++ )
    {
        VSub(result->vertices[result->faces[i].vertices[0]],
             result->vertices[result->faces[i].vertices[1]], temp_v1);
        VSub(result->vertices[result->faces[i].vertices[2]],
             result->vertices[result->faces[i].vertices[1]], temp_v2);
        VCross(temp_v1, temp_v2, result->faces[i].normal);
        VUnit(result->faces[i].normal, temp_d, result->faces[i].normal);
    }

    result->edges = NULL;
    result->attribs = NULL;
    result->num_attribs = 0;

    return result;
}



WireframePtr
Dense_Torus_Wireframe(int level)
{
    WireframePtr    result = New(Wireframe, 1);
    int             round_num = ( 1 << ( level + 3 ) ); /* level * 8 */
    Vector          v1, v2;
    int             i, j, index;
    double          u, v, sin_u, cos_u, sin_v, cos_v;
    double          delta_uv;
    double          temp_d;

    /*  A torus wireframe has ( 2 ^ ( level + 3 ) ) ^ 2 + 1 vertices.
    **  It has ( 2 ^ (level + 3) ) ^ 2 faces.
    */
    result->num_faces = round_num * round_num;
    result->num_vertices = result->num_faces + 1;
    result->num_real_verts = result->num_faces;
    result->num_normals = result->num_faces;

    result->vertices = New(Vector, result->num_vertices);
    result->normals = New(Vector, result->num_normals);
    index = 0;
    delta_uv = M_PI / ( round_num >> 1 );
    for ( i = 0, u = 0 ; i < round_num ; i++, u += delta_uv )
    {
        sin_u = sin(u);
        cos_u = cos(u);
        for ( j = 0, v = 0 ; j < round_num ; j++, v += delta_uv )
        {
            sin_v = sin(v);
            cos_v = cos(v);
            temp_d = 2.0 + cos_v;
            VNew(temp_d * cos_u, temp_d * sin_u, sin_v,result->vertices[index]);
            VNew(cos_v * cos_u, cos_v * sin_u, sin_v, result->normals[index]);
            index++;
        }
    }
    VNew(0, 0, 0, result->vertices[index]);

    /* Allocate the faces. */
    /* Faces are numbered similarly to vertices. */
    result->faces = New(Face, result->num_faces);
    index = 0;
    for ( i = 0 ; i < round_num ; i++ )
    {
        for ( j = 0 ; j < round_num ; j++ )
        {
            result->faces[index].vertices = New(int, 4);
            result->faces[index].normals = New(int, 4);
            result->faces[index].num_vertices = 4;
            result->faces[index].normals[0] =
            result->faces[index].vertices[0] = ( i * round_num ) + j;
            result->faces[index].normals[1] =
            result->faces[index].vertices[1] = ( i * round_num ) + j + 1;
            result->faces[index].normals[2] =
            result->faces[index].vertices[2] = (( i + 1 ) * round_num ) + j + 1;
            result->faces[index].normals[3] =
            result->faces[index].vertices[3] = (( i + 1 ) * round_num ) + j;
            result->faces[index].face_attribs = NULL;
            index++;
        }
        result->faces[index - 1].normals[1] =
        result->faces[index - 1].vertices[1] = ( i * round_num );
        result->faces[index - 1].normals[2] =
        result->faces[index - 1].vertices[2] = ( ( i + 1 ) * round_num );
    }
    for ( index = result->num_faces - round_num, i = 0 ;
          index < result->num_faces ;
          index++, i++ )
    {
        result->faces[index].normals[2] =
        result->faces[index].vertices[2] = i + 1;
        result->faces[index].normals[3] =
        result->faces[index].vertices[3] = i;
    }
    result->faces[index-1].normals[2] =
    result->faces[index-1].vertices[2] = 0;

    /* Calculate normals. */
    for ( index = 0 ; index < result->num_faces ; index++ )
    {
        VSub(result->vertices[result->faces[index].vertices[3]],
             result->vertices[result->faces[index].vertices[0]], v1);
        VSub(result->vertices[result->faces[index].vertices[1]],
             result->vertices[result->faces[index].vertices[0]], v2);
        VCross(v1, v2, result->faces[index].normal);
        VUnit(result->faces[index].normal, temp_d, result->faces[index].normal);
    }

    result->edges = NULL;
    result->num_attribs = 0;
    result->attribs = NULL;

    return result;
}


WireframePtr
Dense_Bezier_Wireframe(int level)
{
    WireframePtr    result = New(Wireframe, 1);
    int             num_lines;  /* The number of lines in each direction. */
    double          step_size;
    double          u_val, v_val;
    int             i, j;
    int             count;

    num_lines = ( 1 << ( level + 2 ) ) + 1;

    result->num_vertices = num_lines * num_lines + 17;
    result->num_real_verts = num_lines * num_lines;
    result->vertices = New(Vector, result->num_vertices);
    result->num_normals = num_lines * num_lines;
    result->normals = New(Vector, result->num_normals);

    step_size = 2.0 / (double)(num_lines - 1);

    count = 0;
    for ( u_val = 0.0, i = 0 ; i < num_lines ; i++, u_val += step_size )
        for ( v_val = 0.0, j = 0 ;
              j < num_lines ;
              j++ , v_val += step_size, count++ )
        {
            VNew(u_val, v_val, 0.0, result->vertices[count]);
            VNew(0, 0, 1.0, result->normals[count]);
        }
    for ( ; count < result->num_vertices ; count++ )
        VNew(0, 0, 0, result->vertices[count]);

    result->num_faces = ( num_lines - 1 ) * ( num_lines - 1 ) * 2;
    result->faces = New(Face, result->num_faces);

    count = 0;
    for ( i = 0 ; i < num_lines - 1 ; i++ )
        for ( j = 0 ; j < num_lines - 1 ; j++ )
        {
            result->faces[count].vertices = New(int, 3);
            result->faces[count].normals = New(int, 3);
            result->faces[count].num_vertices = 3;
            result->faces[count].normals[0] =
            result->faces[count].vertices[0] = i * num_lines + j;
            result->faces[count].normals[1] =
            result->faces[count].vertices[1] = i * num_lines + j + 1;
            result->faces[count].normals[2] =
            result->faces[count].vertices[2] = ( i + 1 ) * num_lines + j;
            result->faces[count].face_attribs = NULL;
            VNew(0, 0, 1, result->faces[count].normal);
            count++;

            result->faces[count].vertices = New(int, 3);
            result->faces[count].normals = New(int, 3);
            result->faces[count].num_vertices = 3;
            result->faces[count].normals[0] =
            result->faces[count].vertices[0] = i * num_lines + j + 1;
            result->faces[count].normals[1] =
            result->faces[count].vertices[1] = ( i + 1 ) * num_lines + j + 1;
            result->faces[count].normals[2] =
            result->faces[count].vertices[2] = ( i + 1 ) * num_lines + j;
            result->faces[count].face_attribs = NULL;
            VNew(0, 0, 1, result->faces[count].normal);
            count++;
        }

    result->edges = NULL;
    result->num_attribs = 0;
    result->attribs = NULL;

    return result;
}


WireframePtr
Dense_CSG_Wireframe(int level, BaseObjectPtr base, Boolean simple)
{
    if ( simple )
        return CSG_Generate_Full_Wireframe((CSGNodePtr)base->b_struct, level);
    else
        return Wireframe_Simplify(base->b_major_wires[level], FALSE);

    return NULL; /* Not reqd. */
}




