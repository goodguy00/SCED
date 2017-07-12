/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/csg_combine.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: csg_combine.c,v $
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
**  csg_combine.c : CSG wireframe combination functions.
*/

#include <math.h>
#include <config.h>
#include <sced.h>
#include <csg.h>
#include <csg_wire.h>

#if CSG_TIME
#include <sys/times.h>
#endif

#define INVERT 0
#define UNKNOWN 0
#define INSIDE 1
#define OUTSIDE 2
#define SAME 3
#define OPPOSITE 4

typedef enum _FaceClass {
    outside_face = 0,
    inside_face,
    edge_face } FaceClass;

static PolygonSplitFunction split_functions[3][3][3] = {
        { { CSG_Vert_Vert_Vert_Split, NULL, NULL },
          { CSG_Vert_Edge_Vert_Split, CSG_Vert_Edge_Edge_Split, NULL },
          { NULL, CSG_Vert_Face_Edge_Split, CSG_Vert_Face_Face_Split } },
        { { NULL, NULL, NULL },
          { CSG_Vert_Edge_Edge_Split, CSG_Edge_Edge_Edge_Split, NULL },
          { CSG_Edge_Face_Vert_Split, CSG_Edge_Face_Edge_Split,
            CSG_Edge_Face_Face_Split } },
        { { NULL, NULL, NULL } , { NULL, NULL, NULL },
          { CSG_Face_Face_Vert_Split, CSG_Edge_Face_Face_Split,
            CSG_Face_Face_Face_Split } } };

static void CSG_Split_Objects(CSGWireframePtr, CSGWireframePtr);
static void CSG_Calculate_Adjacency_Lists(CSGWireframePtr);
static void CSG_Add_Adjacent_Vertex(CSGVertexPtr, int);
static int  CSG_Polygon_Class(CSGFacePtr, CSGVertexPtr);
static int  CSG_Classify_Face(CSGFacePtr, CSGVertexPtr, CSGWireframePtr);
static FaceClass    Point_In_Face(CSGFacePtr, CSGVertexPtr, Vector);
static void CSG_Mark_Vertex(CSGVertexPtr, int, VertexStatus);
static void CSG_Build_Object(CSGWireframePtr, CSGWireframePtr, CSGWireframePtr,
                             Boolean*);
static int  CSG_Build_Add_Vertex(CSGWireframePtr, CSGVertexPtr);
static int  CSG_Build_Add_Normal(CSGWireframePtr, VectorPtr);
static Boolean  Point_In_Bound(Cuboid*, Vector);

#if CSG_TIME
long    time_used;
long    split_time;
long    combine_time;
struct tms  time_str;
long    start_time, end_time;
#endif

CSGWireframePtr
CSG_Combine_Wireframes(CSGWireframePtr left, CSGWireframePtr right,
                        CSGOperation op)
{
    CSGWireframePtr res;
    Boolean         flags[5];
    int             i;

#if CSG_TIME
    time_used = 0;
    split_time = 0;
    combine_time = 0;

    times(&time_str);
    start_time = time_str.tms_utime + time_str.tms_stime;
#endif

#if ( CSG_WIRE_DEBUG )
    fprintf(stderr, "FIRST SPLIT PASS\n");
#endif
    CSG_Split_Objects(left, right);
#if ( CSG_WIRE_DEBUG )
    fprintf(stderr, "SECOND SPLIT PASS\n");
#endif
    CSG_Split_Objects(right, left);
#if ( CSG_WIRE_DEBUG )
    fprintf(stderr, "THIRD SPLIT PASS\n");
#endif
    CSG_Split_Objects(left, right);

#if CSG_TIME
    times(&time_str);
    end_time = time_str.tms_utime + time_str.tms_stime;

    split_time = end_time - start_time;
    start_time = end_time;
#endif

    res = New(CSGWireframe, 1);
    res->num_vertices = 0;
    res->max_vertices = left->num_vertices + right->num_vertices;
    res->vertices = New(CSGVertex, res->max_vertices);
    res->num_normals = 0;
    res->max_normals = left->num_normals + right->num_normals;
    res->normals = New(Vector, res->max_normals);
    res->num_faces = 0;
    res->max_faces = 0;
    res->vertex_tree = NULL;
    res->normal_tree = NULL;

    switch ( op )
    {
        case csg_union_op:
            flags[INVERT] = FALSE;  flags[INSIDE] = FALSE;
            flags[OUTSIDE] = TRUE;  flags[SAME] = TRUE;
            flags[OPPOSITE] = FALSE;
            CSG_Build_Object(res, left, right, flags);
            flags[INVERT] = FALSE;  flags[INSIDE] = FALSE;
            flags[OUTSIDE] = TRUE;  flags[SAME] = FALSE;
            flags[OPPOSITE] = FALSE;
            CSG_Build_Object(res, right, left, flags);
            break;

        case csg_intersection_op:
            flags[INVERT] = FALSE;  flags[INSIDE] = TRUE;
            flags[OUTSIDE] = FALSE; flags[SAME] = TRUE;
            flags[OPPOSITE] = FALSE;
            CSG_Build_Object(res, left, right, flags);
            flags[INVERT] = FALSE;  flags[INSIDE] = TRUE;
            flags[OUTSIDE] = FALSE; flags[SAME] = FALSE;
            flags[OPPOSITE] = FALSE;
            CSG_Build_Object(res, right, left, flags);
            break;

        case csg_difference_op:
            flags[INVERT] = FALSE;  flags[INSIDE] = FALSE;
            flags[OUTSIDE] = TRUE;  flags[SAME] = FALSE;
            flags[OPPOSITE] = TRUE;
            CSG_Build_Object(res, left, right, flags);
            flags[INVERT] = TRUE;   flags[INSIDE] = TRUE;
            flags[OUTSIDE] = FALSE; flags[SAME] = FALSE;
            flags[OPPOSITE] = FALSE;
            CSG_Build_Object(res, right, left, flags);
            break;

        default:;
    }

    /* Rebuild the KD tree for the new object, to balance it more. */
    KD_Free(res->vertex_tree);
    res->vertex_tree = NULL;
    KD_Free(res->normal_tree);
    res->normal_tree = NULL;
    CSG_Build_KD_Trees(res);

#if CSG_TIME
    times(&time_str);
    end_time = time_str.tms_utime + time_str.tms_stime;

    combine_time = end_time - start_time;
    
    printf("search_time = %ld, split_time = %ld, combine_time = %ld\n\n",
            time_used, split_time, combine_time);
#endif

    /* Generate the bounding boxes. */
    for ( i = 0 ; i < res->num_faces ; i++ )
        CSG_Face_Bounding_Box(res->vertices, res->faces[i].face_vertices, 3,
                              &(res->faces[i].face_extent));
    CSG_Bounding_Box(res->vertices, res->num_vertices, &(res->obj_extent));

    return res;
}


static void
CSG_Split_Objects(CSGWireframePtr a, CSGWireframePtr b)
{
    CSGSegment  a_segment;
    int		split_result;
    int         i, j;

    if ( ! ( Extents_Intersect(&(a->obj_extent), &(b->obj_extent)) ) )
        return;

    for ( i = 0 ; i < a->num_faces ; i++ )
    {
        if ( a->faces[i].ignore ||
             ! ( Extents_Intersect(&(a->faces[i].face_extent),
                                   &(b->obj_extent)) ) )
            continue;

        for ( j = 0 ; j < b->num_faces ; j++ )
        {
            if ( b->faces[j].ignore )
                continue;

            if ( CSG_Intersect_Polygons(a->faces + i, a->vertices, a->normals,
                                        b->faces + j, b->vertices, b->normals,
                                        &a_segment))
            {
#if ( CSG_WIRE_DEBUG )
                fprintf(stderr, "faces %d and %d intersect.\n", i, j);
#endif
		split_result = split_functions[a_segment.start_status]
					      [a_segment.middle_status]
					      [a_segment.end_status]
					      (a, i, &a_segment);
		if ( split_result == CSG_REMOVED )
                    break;
		if ( split_result == CSG_OVERFLOW )
		{
		    fprintf(stderr, "Overflow in CSG calculations.\n");
		    Sced_Abort(FALSE);
		}
		if ( split_result == CSG_DEGENERATE )
		{
		    fprintf(stderr, "Degeneracy in CSG calculations.\n");
		    /* Sced_Abort(FALSE); */
		}
            }
        }
    }
}



static void
CSG_Calculate_Adjacency_Lists(CSGWireframePtr wire)
{
    int i;

#define vertex(a, b)    wire->vertices + wire->faces[a].face_vertices[b]

    for ( i = 0 ; i < wire->num_faces ; i++ )
    {
        CSG_Add_Adjacent_Vertex(vertex(i, 0), wire->faces[i].face_vertices[1]);
        CSG_Add_Adjacent_Vertex(vertex(i, 0), wire->faces[i].face_vertices[2]);
        CSG_Add_Adjacent_Vertex(vertex(i, 1), wire->faces[i].face_vertices[0]);
        CSG_Add_Adjacent_Vertex(vertex(i, 1), wire->faces[i].face_vertices[2]);
        CSG_Add_Adjacent_Vertex(vertex(i, 2), wire->faces[i].face_vertices[0]);
        CSG_Add_Adjacent_Vertex(vertex(i, 2), wire->faces[i].face_vertices[1]);
    }
#undef vertex
}


static void
CSG_Add_Adjacent_Vertex(CSGVertexPtr vert, int adj)
{
    int i;

    /* Check for it already there. */
    for ( i = 0 ; i < vert->num_adjacent ; i++ )
        if ( vert->adjacent[i] == adj )
            return;

    /* Allocate more space if needed. */
    if ( vert->num_adjacent == vert->max_num_adjacent )
    {
        if ( vert->max_num_adjacent )
            vert->adjacent = More(vert->adjacent, int,
                                  vert->max_num_adjacent + 5);
        else
            vert->adjacent = New(int, 5);
        vert->max_num_adjacent += 5;
    }

    vert->adjacent[vert->num_adjacent++] = adj;
}


static int
CSG_Polygon_Class(CSGFacePtr p, CSGVertexPtr verts)
{
    int i;

    for ( i = 0 ; i < 3 ; i++ )
    {
        if ( verts[p->face_vertices[i]].status == vertex_unknown )
            return UNKNOWN;
        if ( verts[p->face_vertices[i]].status == vertex_inside )
            return INSIDE;
        if ( verts[p->face_vertices[i]].status == vertex_outside )
            return OUTSIDE;
    }

    return UNKNOWN;
}


static int
CSG_Classify_Face(CSGFacePtr f, CSGVertexPtr verts, CSGWireframePtr other)
{
    Boolean successful = FALSE;
    int     iterations = 0;

    Boolean intersect = FALSE;
    double  nearest_dist;
    int     nearest_index = 0;

    Vector  barycenter;
    Vector  ray_dir;
    double  ray_dot;
    double  ray_dist;
    Vector  ray_point;

    double  closest_dot;
    double  closest_dist;

    FaceClass   point_class;

    Vector  temp_v;
    double  temp_d;
    int     i;

    /* Work out the barycenter. */
    barycenter = verts[f->face_vertices[0]].location;
    for ( i  = 1 ; i < 3 ; i++ )
        VAdd(barycenter, verts[f->face_vertices[i]].location, barycenter);
    temp_d = 1 / 3.0;
    VScalarMul(barycenter, temp_d, barycenter);
    ray_dir = f->face_plane.p_vector;

    while ( ! successful && iterations < 1000 )
    {
        iterations++;
        nearest_dist = HUGE_VAL;
        successful = TRUE;
        for ( i = 0 ; i < other->num_faces ; i++ )
        {
            ray_dot = VDot(ray_dir, other->faces[i].face_plane.p_vector);

            VSub(barycenter, other->faces[i].face_plane.p_point, temp_v);
            ray_dist = VDot(temp_v, other->faces[i].face_plane.p_vector);
            if ( ray_dot > 0 )
                ray_dist = -ray_dist;

            if ( ray_dist < -EPSILON )
                continue;

            if ( IsZero(ray_dot) )
                if ( IsZero(ray_dist) )
                {
                    successful = FALSE;
                    break;
                }
                else
                    continue;

            if ( IsZero(ray_dist) )
                if ( Point_In_Face(other->faces + i, other->vertices,
                                   barycenter) )
                {
                    intersect = TRUE;
                    nearest_dist = 0;
                    nearest_index = i;
                    break;
                }
                else
                    continue;

            /* Work out where the ray intersects the ith face plane. */
            temp_d = ( VDot(other->faces[i].face_plane.p_vector,
                            other->faces[i].face_plane.p_point) -
                       VDot(other->faces[i].face_plane.p_vector, barycenter) ) /
                       ray_dot;
            if ( temp_d > 0 && temp_d < nearest_dist )
            {
                VScalarMul(ray_dir, temp_d, ray_point);
                VAdd(ray_point, barycenter, ray_point);

                point_class = Point_In_Face(other->faces + i,
                                            other->vertices, ray_point);
                if ( point_class == inside_face )
                {
                    intersect = TRUE;
                    nearest_dist = temp_d;
                    nearest_index = i;
                    continue;
                }
                else if ( point_class == edge_face )
                {
                    successful = FALSE;
                    break;
                }
                else continue;
            }
        }
        if ( ! successful )
        {
            ray_dir.x += 0.05;
            ray_dir.y += 0.07;
            ray_dir.z += 0.09;
        }
    }

    if ( ! intersect )
        return OUTSIDE;

    closest_dot = VDot(ray_dir,other->faces[nearest_index].face_plane.p_vector);
    VSub(barycenter, other->faces[nearest_index].face_plane.p_point, temp_v);
    closest_dist = VDot(temp_v,other->faces[nearest_index].face_plane.p_vector);

    if ( IsZero(closest_dist) )
        if ( closest_dot < 0 )
            return OPPOSITE;
        else
            return SAME;

    if ( closest_dot < 0 )
        return OUTSIDE;

    return INSIDE;

}


#define vert(i) verts[indices[i]]

static FaceClass
Point_In_Polygon_X_Y(CSGVertexPtr verts, int *indices, int num, Vector pt)
{
    Boolean found_1 = FALSE;
    double  min_x, max_x;
    int     min_i, max_i;
    double  temp_d;
    int     i;

    /* Seek to find out how many edges a ray in the +ve x direction intersects.
    */
    if ( DEqual(vert(num-1).location.y, vert(0).location.y) )
    {
        /* Horizontal edge. */
        if ( DEqual(pt.y, vert(num-1).location.y) ||
             DEqual(pt.y, vert(0).location.y) )
        {
            min_x = min(vert(num-1).location.x, vert(0).location.x);
            max_x = max(vert(num-1).location.x, vert(0).location.x);
            if ( pt.x > min_x - EPSILON && pt.x < max_x + EPSILON )
                return edge_face;
            else
                return outside_face;
        }
        else goto start_loop;
    }

    /* Edge not horizontal. */
    if ( vert(0).location.y > vert(num-1).location.y )
        min_i = num - 1, max_i = 0;
    else
        min_i = 0 , max_i = num - 1;

    if ( pt.y >= vert(max_i).location.y ||
         pt.y < vert(min_i).location.y )
        goto start_loop;

    if ( pt.x > vert(min_i).location.x &&
         pt.x > vert(max_i).location.x )
        /* Pt to the right, no intersection. */
        goto start_loop;

    if ( pt.x < vert(min_i).location.x &&
         pt.x < vert(max_i).location.x )
    {
        /* Pt to the left of both x vals. Must cross. */
        found_1 = TRUE;
        goto start_loop;
    }

    /* So it's within the y range and to the left of some x value. */
    if ( pt.y == vert(max_i).location.y )
    {
        /* Equal max y values. Check x simply. */
        if ( DEqual(pt.x, vert(max_i).location.x) )
            return edge_face;
        else if ( pt.x < vert(max_i).location.x)
            found_1 = TRUE;
        goto start_loop;
    }

    /* Now it's strictly within the y values. */
    temp_d = ( ( vert(max_i).location.x - pt.x ) *
               ( vert(max_i).location.y - vert(min_i).location.y ) ) -
             ( ( vert(max_i).location.x - vert(min_i).location.x ) *
               ( vert(max_i).location.y - pt.y ) );
    if ( IsZero(temp_d) )
        return edge_face;
    else if ( temp_d > 0 )
        found_1 = TRUE;

start_loop:;

    for ( i = 1 ; i < num ; i++ )
    {
        if ( DEqual(vert(i-1).location.y, vert(i).location.y) )
        {
            /* Horizontal edge. */
            if ( DEqual(pt.y, vert(i-1).location.y) ||
                 DEqual(pt.y, vert(i).location.y) )
            {
                min_x = min(vert(i-1).location.x, vert(i).location.x);
                max_x = max(vert(i-1).location.x, vert(i).location.x);
                if ( pt.x > min_x - EPSILON && pt.x < max_x + EPSILON )
                    return edge_face;
                else
                    return outside_face;
            }
            else continue;
        }

        /* Edge not horizontal. */
        if ( vert(i).location.y > vert(i-1).location.y )
            min_i = i - 1, max_i = i;
        else
            min_i = i , max_i = i - 1;

        if ( pt.y >= vert(max_i).location.y ||
             pt.y < vert(min_i).location.y )
            continue;

        if ( pt.x > vert(min_i).location.x &&
             pt.x > vert(max_i).location.x )
            /* Pt to the right, no intersection. */
            continue;

        if ( pt.x < vert(min_i).location.x &&
             pt.x < vert(max_i).location.x )
        {
            /* Pt to the left of both x vals. Must cross. */
            if ( found_1 )
                return outside_face;
            found_1 = TRUE;
            continue;
        }

        /* So it's within the y range and to the left of some x value. */
        if ( pt.y == vert(max_i).location.y )
        {
            /* Equal max y values. Check x simply. */
            if ( DEqual(pt.x, vert(max_i).location.x) )
                return edge_face;
            else if ( pt.x < vert(max_i).location.x)
            {
                if ( found_1 )
                    return outside_face;
                found_1 = TRUE;
            }
            continue;
        }

        /* Now it's strictly within the y values. */
        temp_d = ( ( vert(max_i).location.x - pt.x ) *
                   ( vert(max_i).location.y - vert(min_i).location.y ) ) -
                 ( ( vert(max_i).location.x - vert(min_i).location.x ) *
                   ( vert(max_i).location.y - pt.y ) );
        if ( IsZero(temp_d) )
            return edge_face;
        else if ( temp_d > 0 )
        {
            if ( found_1 )
                return outside_face;
            found_1 = TRUE;
        }
    }

    return ( found_1 ? inside_face : outside_face );
}


static FaceClass
Point_In_Polygon_X_Z(CSGVertexPtr verts, int *indices, int num, Vector pt)
{
    Boolean found_1 = FALSE;
    double  min_x, max_x;
    int     min_i, max_i;
    double  temp_d;
    int     i;

    /* Seek to find out how many edges a ray in the +ve x direction intersects.
    */
    if ( DEqual(vert(num-1).location.z, vert(0).location.z) )
    {
        /* Horizontal edge. */
        if ( DEqual(pt.z, vert(num-1).location.z) ||
             DEqual(pt.z, vert(0).location.z) )
        {
            min_x = min(vert(num-1).location.x, vert(0).location.x);
            max_x = max(vert(num-1).location.x, vert(0).location.x);
            if ( pt.x > min_x - EPSILON && pt.x < max_x + EPSILON )
                return edge_face;
            else
                return outside_face;
        }
        else goto start_loop;
    }

    /* Edge not horizontal. */
    if ( vert(0).location.z > vert(num-1).location.z )
        min_i = num - 1, max_i = 0;
    else
        min_i = 0 , max_i = num - 1;

    if ( pt.z >= vert(max_i).location.z ||
         pt.z < vert(min_i).location.z )
        goto start_loop;

    if ( pt.x > vert(min_i).location.x &&
         pt.x > vert(max_i).location.x )
        /* Pt to the right, no intersection. */
        goto start_loop;

    if ( pt.x < vert(min_i).location.x &&
         pt.x < vert(max_i).location.x )
    {
        /* Pt to the left of both x vals. Must cross. */
        found_1 = TRUE;
        goto start_loop;
    }

    /* So it's within the z range and to the left of some x value. */
    if ( pt.z == vert(max_i).location.z )
    {
        /* Equal max z values. Check x simply. */
        if ( DEqual(pt.x, vert(max_i).location.x) )
            return edge_face;
        else if ( pt.x < vert(max_i).location.x)
            found_1 = TRUE;
        goto start_loop;
    }

    /* Now it's strictly within the z values. */
    temp_d = ( ( vert(max_i).location.x - pt.x ) *
               ( vert(max_i).location.z - vert(min_i).location.z ) ) -
             ( ( vert(max_i).location.x - vert(min_i).location.x ) *
               ( vert(max_i).location.z - pt.z ) );
    if ( IsZero(temp_d) )
        return edge_face;
    else if ( temp_d > 0 )
        found_1 = TRUE;

start_loop:;

    for ( i = 1 ; i < num ; i++ )
    {
        if ( DEqual(vert(i-1).location.z, vert(i).location.z) )
        {
            /* Horizontal edge. */
            if ( DEqual(pt.z, vert(i-1).location.z) ||
                 DEqual(pt.z, vert(i).location.z) )
            {
                min_x = min(vert(i-1).location.x, vert(i).location.x);
                max_x = max(vert(i-1).location.x, vert(i).location.x);
                if ( pt.x > min_x - EPSILON && pt.x < max_x + EPSILON )
                    return edge_face;
                else
                    return outside_face;
            }
            else continue;
        }

        /* Edge not horizontal. */
        if ( vert(i).location.z > vert(i-1).location.z )
            min_i = i - 1, max_i = i;
        else
            min_i = i , max_i = i - 1;

        if ( pt.z >= vert(max_i).location.z ||
             pt.z < vert(min_i).location.z )
            continue;

        if ( pt.x > vert(min_i).location.x &&
             pt.x > vert(max_i).location.x )
            /* Pt to the right, no intersection. */
            continue;

        if ( pt.x < vert(min_i).location.x &&
             pt.x < vert(max_i).location.x )
        {
            /* Pt to the left of both x vals. Must cross. */
            if ( found_1 )
                return outside_face;
            found_1 = TRUE;
            continue;
        }

        /* So it's within the z range and to the left of some x value. */
        if ( pt.z == vert(max_i).location.z )
        {
            /* Equal max z values. Check x simply. */
            if ( DEqual(pt.x, vert(max_i).location.x) )
                return edge_face;
            else if ( pt.x < vert(max_i).location.x)
            {
                if ( found_1 )
                    return outside_face;
                found_1 = TRUE;
            }
            continue;
        }

        /* Now it's strictly within the z values. */
        temp_d = ( ( vert(max_i).location.x - pt.x ) *
                   ( vert(max_i).location.z - vert(min_i).location.z ) ) -
                 ( ( vert(max_i).location.x - vert(min_i).location.x ) *
                   ( vert(max_i).location.z - pt.z ) );
        if ( IsZero(temp_d) )
            return edge_face;
        else if ( temp_d > 0 )
        {
            if ( found_1 )
                return outside_face;
            found_1 = TRUE;
        }
    }

    return ( found_1 ? inside_face : outside_face );
}


static FaceClass
Point_In_Polygon_Y_Z(CSGVertexPtr verts, int *indices, int num, Vector pt)
{
    Boolean found_1 = FALSE;
    double  min_y, max_y;
    int     min_i, max_i;
    double  temp_d;
    int     i;

    /* Seek to find out how many edges a ray in the +ve y direction intersects.
    */
    if ( DEqual(vert(num-1).location.z, vert(0).location.z) )
    {
        /* Horizontal edge. */
        if ( DEqual(pt.z, vert(num-1).location.z) ||
             DEqual(pt.z, vert(0).location.z) )
        {
            min_y = min(vert(num-1).location.y, vert(0).location.y);
            max_y = max(vert(num-1).location.y, vert(0).location.y);
            if ( pt.y > min_y - EPSILON && pt.y < max_y + EPSILON )
                return edge_face;
            else
                return outside_face;
        }
        else goto start_loop;
    }

    /* Edge not horizontal. */
    if ( vert(0).location.z > vert(num-1).location.z )
        min_i = num - 1, max_i = 0;
    else
        min_i = 0 , max_i = num - 1;

    if ( pt.z >= vert(max_i).location.z ||
         pt.z < vert(min_i).location.z )
        goto start_loop;

    if ( pt.y > vert(min_i).location.y &&
         pt.y > vert(max_i).location.y )
        /* Pt to the right, no intersection. */
        goto start_loop;

    if ( pt.y < vert(min_i).location.y &&
         pt.y < vert(max_i).location.y )
    {
        /* Pt to the left of both y vals. Must cross. */
        found_1 = TRUE;
        goto start_loop;
    }

    /* So it's within the z range and to the left of some y value. */
    if ( pt.z == vert(max_i).location.z )
    {
        /* Equal max z values. Check y simply. */
        if ( DEqual(pt.y, vert(max_i).location.y) )
            return edge_face;
        else if ( pt.y < vert(max_i).location.y)
            found_1 = TRUE;
        goto start_loop;
    }

    /* Now it's strictly within the z values. */
    temp_d = ( ( vert(max_i).location.y - pt.y ) *
               ( vert(max_i).location.z - vert(min_i).location.z ) ) -
             ( ( vert(max_i).location.y - vert(min_i).location.y ) *
               ( vert(max_i).location.z - pt.z ) );
    if ( IsZero(temp_d) )
        return edge_face;
    else if ( temp_d > 0 )
        found_1 = TRUE;

start_loop:;

    for ( i = 1 ; i < num ; i++ )
    {
        if ( DEqual(vert(i-1).location.z, vert(i).location.z) )
        {
            /* Horizontal edge. */
            if ( DEqual(pt.z, vert(i-1).location.z) ||
                 DEqual(pt.z, vert(i).location.z) )
            {
                min_y = min(vert(i-1).location.y, vert(i).location.y);
                max_y = max(vert(i-1).location.y, vert(i).location.y);
                if ( pt.y > min_y - EPSILON && pt.y < max_y + EPSILON )
                    return edge_face;
                else
                    return outside_face;
            }
            else continue;
        }

        /* Edge not horizontal. */
        if ( vert(i).location.z > vert(i-1).location.z )
            min_i = i - 1, max_i = i;
        else
            min_i = i , max_i = i - 1;

        if ( pt.z >= vert(max_i).location.z ||
             pt.z < vert(min_i).location.z )
            continue;

        if ( pt.y > vert(min_i).location.y &&
             pt.y > vert(max_i).location.y )
            /* Pt to the right, no intersection. */
            continue;

        if ( pt.y < vert(min_i).location.y &&
             pt.y < vert(max_i).location.y )
        {
            /* Pt to the left of both y vals. Must cross. */
            if ( found_1 )
                return outside_face;
            found_1 = TRUE;
            continue;
        }

        /* So it's within the z range and to the left of some y value. */
        if ( pt.z == vert(max_i).location.z )
        {
            /* Equal max z values. Check y simply. */
            if ( DEqual(pt.y, vert(max_i).location.y) )
                return edge_face;
            else if ( pt.y < vert(max_i).location.y)
            {
                if ( found_1 )
                    return outside_face;
                found_1 = TRUE;
            }
            continue;
        }

        /* Now it's strictly within the z values. */
        temp_d = ( ( vert(max_i).location.y - pt.y ) *
                   ( vert(max_i).location.z - vert(min_i).location.z ) ) -
                 ( ( vert(max_i).location.y - vert(min_i).location.y ) *
                   ( vert(max_i).location.z - pt.z ) );
        if ( IsZero(temp_d) )
            return edge_face;
        else if ( temp_d > 0 )
        {
            if ( found_1 )
                return outside_face;
            found_1 = TRUE;
        }
    }

    return ( found_1 ? inside_face : outside_face );
}

#undef vert

static FaceClass
Point_In_Face(CSGFacePtr face, CSGVertexPtr verts, Vector pt)
{
    if ( ! Point_In_Bound(&(face->face_extent), pt) )
        return outside_face;

    if ( fabs(face->face_plane.p_vector.x) > fabs(face->face_plane.p_vector.y) )
    {
        if ( fabs(face->face_plane.p_vector.x) >
             fabs(face->face_plane.p_vector.z ) )
            return Point_In_Polygon_Y_Z(verts, face->face_vertices, 3, pt);
        else
            return Point_In_Polygon_X_Y(verts, face->face_vertices, 3, pt);
    }
    else
    {
        if ( fabs(face->face_plane.p_vector.y) >
             fabs(face->face_plane.p_vector.z ) )
            return Point_In_Polygon_X_Z(verts, face->face_vertices, 3, pt);
        else
            return Point_In_Polygon_X_Y(verts, face->face_vertices, 3, pt);
    }
}


static void
CSG_Mark_Vertex(CSGVertexPtr verts, int index, VertexStatus new_status)
{
    int i;

    verts[index].status = new_status;
    for ( i = 0 ; i < verts[index].num_adjacent ; i++ )
        if ( verts[verts[index].adjacent[i]].status == vertex_unknown )
            CSG_Mark_Vertex(verts, verts[index].adjacent[i], new_status);
}


/*  void
**  CSG_Build_Object(CSGWireframePtr dest, CSGWireframePtr src, 
**                   CSGWireframePtr other, Boolean *flags)
**  Copies those faces of src that satisfy flags into dest. Vertices go too.
*/
static void
CSG_Build_Object(CSGWireframePtr dest, CSGWireframePtr src,
                 CSGWireframePtr other, Boolean *flags)
{
    Vector  *normals;
    int     *v_map;
    int     *n_map;
    int     face_class;
    int     i, j;

    if ( src->num_faces == 0 ) return;

    v_map = New(int, src->num_vertices);
    n_map = New(int, src->num_normals);

    for ( i = 0 ; i < src->num_vertices ; i++ ) v_map[i] = -1;
    for ( i = 0 ; i < src->num_normals ; i++ )  n_map[i] = -1;

    /* If the object is inverted, we need to invert the vertex normals. */
    if ( flags[INVERT] )
    {
	normals = New(Vector, src->num_normals);
	for ( i = 0 ; i < src->num_normals ; i++ )
	{
	    normals[i].x = -src->normals[i].x;
	    normals[i].y = -src->normals[i].y;
	    normals[i].z = -src->normals[i].z;
	}
    }
    else
	normals = src->normals;

    if ( dest->num_faces )
        dest->faces =
            More(dest->faces, CSGFace, dest->num_faces + src->num_faces);
    else
        dest->faces = New(CSGFace, src->num_faces);

    CSG_Calculate_Adjacency_Lists(src);

    for ( i = 0 ; i < src->num_faces ; i++ )
    {
        if ( src->faces[i].ignore )
            continue;

        if ( ! ( face_class = CSG_Polygon_Class(src->faces + i, src->vertices)))
        {
            face_class = CSG_Classify_Face(src->faces + i,
                                           src->vertices, other);
            for ( j = 0 ; j < 3 ; j++ )
                if ( src->vertices[src->faces[i].face_vertices[j]].status ==
                     vertex_unknown )
                    CSG_Mark_Vertex(src->vertices,
                                    src->faces[i].face_vertices[j], face_class);
        }

        if ( flags[face_class] )
        {
#if CSG_WIRE_DEBUG
    fprintf(stderr, "Dest face %d = Src face %d\n", dest->num_faces, i);
#endif
            for ( j = 0 ; j < 3 ; j++ )
            {
                if ( v_map[src->faces[i].face_vertices[j]] != -1 )
                    dest->faces[dest->num_faces].face_vertices[j] =
                        v_map[src->faces[i].face_vertices[j]];
                else
                    dest->faces[dest->num_faces].face_vertices[j] =
                    v_map[src->faces[i].face_vertices[j]] =
                        CSG_Build_Add_Vertex(dest,
                                src->vertices + src->faces[i].face_vertices[j]);
                if ( n_map[src->faces[i].face_normals[j]] != -1 )
                    dest->faces[dest->num_faces].face_normals[j] =
                        n_map[src->faces[i].face_normals[j]];
                else
                    dest->faces[dest->num_faces].face_normals[j] =
                    n_map[src->faces[i].face_normals[j]] =
                        CSG_Build_Add_Normal(dest,
                                normals + src->faces[i].face_normals[j]);
            }
            dest->faces[dest->num_faces].face_plane = src->faces[i].face_plane;
            if ( flags[INVERT] )
            {
                int temp;

                VScalarMul(dest->faces[dest->num_faces].face_plane.p_vector, -1,
                           dest->faces[dest->num_faces].face_plane.p_vector);
		temp = dest->faces[dest->num_faces].face_vertices[2];
		dest->faces[dest->num_faces].face_vertices[2] =
		    dest->faces[dest->num_faces].face_vertices[0];
		dest->faces[dest->num_faces].face_vertices[0] = temp;
		temp = dest->faces[dest->num_faces].face_normals[2];
		dest->faces[dest->num_faces].face_normals[2] =
		    dest->faces[dest->num_faces].face_normals[0];
		dest->faces[dest->num_faces].face_normals[0] = temp;
            }
            dest->faces[dest->num_faces].face_attribs =
                src->faces[i].face_attribs;
            dest->faces[dest->num_faces].ignore = FALSE;
                
            dest->num_faces++;
        }
    }

    /* Realloc for the exact size. */
    if ( dest->num_faces )
        dest->faces = More(dest->faces, CSGFace, dest->num_faces);
    else
    {
        free(dest->faces);
        dest->faces = NULL;
    }
    dest->max_faces = dest->num_faces;

    free(n_map);
    free(v_map);
    if ( flags[INVERT] )
	free(normals);
}


int
CSG_Add_KD_Compare_Vert_Func(void *existing, void *insert, void *data,int depth)
{
    CSGVertexPtr    verts = (CSGVertexPtr)data;
    int             ex_index = (int)existing;
    int             new_index = (int)insert;
    int             comp_field = depth % 3;
    Vector          diff;

    if ( CSGVEqual(verts[ex_index].location, verts[new_index].location, diff) )
        return 0;   /* A match. */

    switch ( comp_field )
    {
        case 0:
            if ( verts[new_index].location.x <
                 verts[ex_index].location.x - CSG_EPSILON )
                return -1;
            else
                return 1;
        case 1:
            if ( verts[new_index].location.y <
                 verts[ex_index].location.y - CSG_EPSILON )
                return -1;
            else
                return 1;
        case 2:
            if ( verts[new_index].location.z <
                 verts[ex_index].location.z - CSG_EPSILON )
                return -1;
            else
                return 1;
    }

    return 1;   /* To keep the compiler happy. */
}

int
CSG_Add_KD_Compare_Norm_Func(void *existing, void *insert, void *data,int depth)
{
    VectorPtr   verts = (VectorPtr)data;
    int         ex_index = (int)existing;
    int         new_index = (int)insert;
    int         comp_field = depth % 3;
    Vector      diff;

    if ( CSGVEqual(verts[ex_index], verts[new_index], diff) )
        return 0;   /* A match. */

    switch ( comp_field )
    {
        case 0:
            if ( verts[new_index].x <
                 verts[ex_index].x - CSG_EPSILON )
                return -1;
            else
                return 1;
        case 1:
            if ( verts[new_index].y <
                 verts[ex_index].y - CSG_EPSILON )
                return -1;
            else
                return 1;
        case 2:
            if ( verts[new_index].z <
                 verts[ex_index].z - CSG_EPSILON )
                return -1;
            else
                return 1;
    }

    return 1;   /* To keep the compiler happy. */
}


static int
CSG_Build_Add_Vertex(CSGWireframePtr dest, CSGVertexPtr src)
{
    int search_res;

    dest->vertices[dest->num_vertices] = *src;
    dest->vertices[dest->num_vertices].status = vertex_unknown;
    dest->vertices[dest->num_vertices].num_adjacent =
    dest->vertices[dest->num_vertices].max_num_adjacent = 0;

    /* Search for the vertex already there. */
    search_res = (int)KD_Add_Elmt(&(dest->vertex_tree),
                                  (void*)dest->num_vertices,
                                  (void*)dest->vertices,
                                  CSG_Add_KD_Compare_Vert_Func);
    if ( search_res == dest->num_vertices )
        dest->num_vertices++;

    return search_res;
}


static int
CSG_Build_Add_Normal(CSGWireframePtr dest, VectorPtr src)
{
    int search_res;

    dest->normals[dest->num_normals] = *src;

    /* Search for the vertex already there. */
    search_res = (int)KD_Add_Elmt(&(dest->normal_tree),
                                  (void*)dest->num_normals,
                                  (void*)dest->normals,
                                  CSG_Add_KD_Compare_Norm_Func);
    if ( search_res == dest->num_normals )
        dest->num_normals++;

    return search_res;
}


static Boolean
Point_In_Bound(Cuboid *bound, Vector pt)
{
    return (
        (pt.x > bound->min.x - CSG_EPSILON) &&
        (pt.x < bound->max.x + CSG_EPSILON) &&
        (pt.y > bound->min.y - CSG_EPSILON) &&
        (pt.y < bound->max.y + CSG_EPSILON) &&
        (pt.z > bound->min.z - CSG_EPSILON) &&
        (pt.z < bound->max.z + CSG_EPSILON));

}

