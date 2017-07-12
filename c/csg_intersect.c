/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/csg_intersect.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: csg_intersect.c,v $
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
**  csg_intersect.c : Functions to intersect to polygons for csg wireframing.
*/

#include <math.h>
#include <config.h>
#if __SCEDCON__
#include <convert.h>
#else
#include <sced.h>
#include <csg.h>
#include <csg_wire.h>
#endif

static Boolean  Calculate_Sign(double*, short*);

static void     CSG_Calculate_Vertex_Distances(CSGVertexPtr, int*, double*,
                                               CSGPlanePtr);
static void     CSG_Intersection_Find_Edge_Point(CSGVertexPtr, VectorPtr,
                                                 CSGVertexPtr, VectorPtr,
                                                 CSGPlanePtr,
                                                 VectorPtr, VectorPtr);
static double   CSG_Intersection_Determine_Distance(Vector*, Vector*, Vector*);

static void     CSG_Intersection_Segments(CSGFacePtr, CSGVertexPtr, VectorPtr,
                                          CSGFacePtr, CSGVertexPtr, VectorPtr,
                                          short*, short*,
                                          CSGSegmentPtr, CSGSegmentPtr);
static Boolean  CSG_Determine_Segment_Intersection(CSGSegmentPtr,CSGSegmentPtr);
static void     CSG_Validate_Segment(CSGSegmentPtr);

static Vector   diff;

/*  Boolean
**  CSG_Intersect_Polygons(CSGFacePtr a, CSGFacePtr b, CSGSegmentPtr res_seg)
**  Intersects 2 polygons for CSG combination purposes.
**  Returns True if they are non-coplanar and intersect, FALSE otherwise.
**  In the case of intersection, res_seg is filled in as appropriate.
*/
Boolean
CSG_Intersect_Polygons(CSGFacePtr a, CSGVertexPtr a_verts, VectorPtr a_norms,
                       CSGFacePtr b, CSGVertexPtr b_verts, VectorPtr b_norms,
                       CSGSegmentPtr res_seg)
{
    CSGSegment  a_seg;
    CSGSegment  b_seg;
    double      a_to_b_dists[3];
    double      b_to_a_dists[3];
    short       a_signs[3];
    short       b_signs[3];

    if ( ! ( Extents_Intersect(&(a->face_extent), &(b->face_extent)) ) )
        return FALSE;

    CSG_Calculate_Vertex_Distances(a_verts, a->face_vertices, a_to_b_dists,
                                   &(b->face_plane));
    if ( Calculate_Sign(a_to_b_dists, a_signs) )
        return FALSE;

    CSG_Calculate_Vertex_Distances(b_verts, b->face_vertices, b_to_a_dists,
                                   &(a->face_plane));
    if ( Calculate_Sign(b_to_a_dists, b_signs) )
        return FALSE;

    CSG_Intersection_Segments(a, a_verts, a_norms, b, b_verts, b_norms,
                              a_signs, b_signs, &a_seg, &b_seg);

    *res_seg = a_seg;
    if ( ! CSG_Determine_Segment_Intersection(res_seg, &b_seg) )
        return FALSE;

    CSG_Validate_Segment(res_seg);

    return TRUE;
}



static Boolean
Calculate_Sign(double *dists, short *signs)
{
    short   sum = 0;
    int     i;

    /* Perform a consistency check. */
    for ( i = 0 ; i < 3 ; i++ )
    {
        signs[i] = CSGIsZero(dists[i]) ? 0 : ( dists[i] > 0 ? 1 : -1 );
	sum += signs[i];
    }
    
    return sum == 3 || sum == -3 ||
	    ( signs[0] == 0 && signs[1] == 0 && signs[2] == 0 );
}


static void
CSG_Calculate_Vertex_Distances(CSGVertexPtr verts, int *indices, double *dists,
                               CSGPlanePtr plane)
{
    Vector  temp_v;
    int     i;

    for ( i = 0 ; i < 3 ; i++ )
    {
        VSub(verts[indices[i]].location, plane->p_point, temp_v);
        dists[i] = VDot(temp_v, plane->p_vector);
    }
}


static void
Project_Vect_Onto_Line(Vector *orig, Vector *l_pt, Vector *dir, Vector *result)
{
    Vector  temp_v;
    double  project;

    VSub(*orig, *l_pt, temp_v);
    project = VDot(*dir, temp_v) / VDot(*dir, *dir);
    VScalarMul(*dir, project, temp_v);
    VAdd(temp_v, *l_pt, *result);
}



Boolean
Extents_Intersect(Cuboid *a, Cuboid *b)
{
    Vector  int_min, int_max;

    int_min.x = max(a->min.x, b->min.x);
    int_min.y = max(a->min.y, b->min.y);
    int_min.z = max(a->min.z, b->min.z);
    int_max.x = min(a->max.x, b->max.x);
    int_max.y = min(a->max.y, b->max.y);
    int_max.z = min(a->max.z, b->max.z);

    return ( int_min.x < int_max.x + EPSILON &&
             int_min.y < int_max.y + EPSILON &&
             int_min.z < int_max.z + EPSILON );
}


void
Face_Shift_Right(CSGFacePtr face)
{
    int	temp;

    temp = face->face_vertices[2];
    face->face_vertices[2] = face->face_vertices[1];
    face->face_vertices[1] = face->face_vertices[0];
    face->face_vertices[0] = temp;

    temp = face->face_normals[2];
    face->face_normals[2] = face->face_normals[1];
    face->face_normals[1] = face->face_normals[0];
    face->face_normals[0] = temp;
}

void
Face_Shift_Left(CSGFacePtr face)
{
    int	temp;

    temp = face->face_vertices[0];
    face->face_vertices[0] = face->face_vertices[1];
    face->face_vertices[1] = face->face_vertices[2];
    face->face_vertices[2] = temp;

    temp = face->face_normals[0];
    face->face_normals[0] = face->face_normals[1];
    face->face_normals[1] = face->face_normals[2];
    face->face_normals[2] = temp;
}


static void
Reorder_Vertices(CSGFacePtr face, short *signs)
{
    short   temp;

    if ( signs[0] == -1 )
    {
	if ( signs[2] == -1 )
	{
	    Face_Shift_Right(face);
	    temp = signs[2];
	    signs[2] = signs[1];
	    signs[1] = signs[0];
	    signs[0] = temp;  
	}
	return; 
    }
    if ( signs[1] == -1 )
    {
	Face_Shift_Left(face);
	temp = signs[0];
	signs[0] = signs[1];
	signs[1] = signs[2];
	signs[2] = temp;
	return;
    }
    if ( signs[2] == -1 )
    {
	Face_Shift_Right(face);
	temp = signs[2];
	signs[2] = signs[1];
	signs[1] = signs[0];
	signs[0] = temp;
	return; 
    }
    if ( signs[0] == 0 )
    {
	if ( signs[2] == 0 )
	{
	    Face_Shift_Right(face);
	    temp = signs[2];
	    signs[2] = signs[1];
	    signs[1] = signs[0];
	    signs[0] = temp;
	}
	return;
    }
    if ( signs[1] == 0 )
    {
	Face_Shift_Left(face);
	temp = signs[0];
	signs[0] = signs[1];
	signs[1] = signs[2];
	signs[2] = temp;
	return;
    }
    if ( signs[2] == 0 )
    {
	Face_Shift_Right(face);
	temp = signs[2];
	signs[2] = signs[1];
	signs[1] = signs[0];
	signs[0] = temp;
	return;
    }
}


static void
CSG_Intersection_Segments(CSGFacePtr a, CSGVertexPtr a_verts, VectorPtr a_norms,
                          CSGFacePtr b, CSGVertexPtr b_verts, VectorPtr b_norms,
                          short *a_signs, short *b_signs,
                          CSGSegmentPtr a_seg, CSGSegmentPtr b_seg)
{
    Vector  	line_dir;
    Vector	line_pt;
    double      temp_d;

    /* Reorder the vertices. This just does a rotation so that the signs
    ** are in one of several canonical orders.
    */
    Reorder_Vertices(a, a_signs);
    Reorder_Vertices(b, b_signs);

    /* Work through the possible intersection types. */
    switch ( a_signs[0] )
    {
      case -1:
	switch ( a_signs[1] )
	{
	  case -1:
	    if ( a_signs[2] == 0 )
	    {
		VCross(a->face_plane.p_vector, b->face_plane.p_vector,line_dir);
		VUnit(line_dir, temp_d, line_dir);
		line_pt = a_verts[a->face_vertices[2]].location;

		a_seg->start_distance = 0.0;
		a_seg->start_vertex = 2;
		a_seg->start_point = line_pt;
		a_seg->start_normal = a_norms[a->face_normals[2]];
		a_seg->start_status = segment_vertex;

		a_seg->middle_status = segment_vertex;

		a_seg->end_distance = 0.0;
		a_seg->end_vertex = 2;
		a_seg->end_point = line_pt;
		a_seg->end_normal = a_norms[a->face_normals[2]];
		a_seg->end_status = segment_vertex;
	    }
	    else
	    {
		CSG_Intersection_Find_Edge_Point(a_verts + a->face_vertices[1],
						 a_norms + a->face_normals[1],
						 a_verts + a->face_vertices[2],
						 a_norms + a->face_normals[2],
						 &(b->face_plane),
						 &(a_seg->start_point),
						 &(a_seg->start_normal));
		CSG_Intersection_Find_Edge_Point(a_verts + a->face_vertices[2],
						 a_norms + a->face_normals[2],
						 a_verts + a->face_vertices[0],
						 a_norms + a->face_normals[0],
						 &(b->face_plane),
						 &(a_seg->end_point),
						 &(a_seg->end_normal));
		line_pt = a_seg->start_point;
		VSub(a_seg->end_point, a_seg->start_point, line_dir);

		a_seg->start_distance = 0.0;
		a_seg->start_vertex = 1;
		a_seg->start_status = segment_edge;
		a_seg->middle_status = segment_face;
		a_seg->end_distance = 1.0;
		a_seg->end_vertex = 2;
		a_seg->end_status = segment_edge;
	    }
	    break;
	  case 0:
	    if ( a_signs[2] == 0 )
	    {
		/* (-1, 0, 0) */
		line_pt = a_verts[a->face_vertices[1]].location;
		VSub(a_verts[a->face_vertices[2]].location, line_pt, line_dir);
		a_seg->start_distance = 0.0;
		a_seg->start_vertex = 1;
		a_seg->start_point = line_pt;
		a_seg->start_normal = a_norms[a->face_normals[1]];
		a_seg->start_status = segment_vertex;
		a_seg->middle_status = segment_edge;
		a_seg->end_distance = 1.0;
		a_seg->end_vertex = 2;
		a_seg->end_point = a_verts[a->face_vertices[2]].location;
		a_seg->end_normal = a_norms[a->face_normals[2]];
		a_seg->end_status = segment_vertex;
	    }
	    else
	    {
		/* (-1, 0, 1) */
		line_pt = a_verts[a->face_vertices[1]].location;
		CSG_Intersection_Find_Edge_Point(a_verts + a->face_vertices[2],
						 a_norms + a->face_normals[2],
						 a_verts + a->face_vertices[0],
						 a_norms + a->face_normals[0],
						 &(b->face_plane),
						 &(a_seg->end_point),
						 &(a_seg->end_normal));
		VSub(a_seg->end_point, line_pt, line_dir);
		a_seg->start_distance = 0.0;
		a_seg->start_vertex = 1;
		a_seg->start_point = line_pt;
		a_seg->start_normal = a_norms[a->face_normals[1]];
		a_seg->start_status = segment_vertex;
		a_seg->middle_status = segment_face;
		a_seg->end_distance = 1.0;
		a_seg->end_vertex = 2;
		a_seg->end_status = segment_edge;
	    }
	    break;
	  case 1:
	    if ( a_signs[2] == 0 )
	    {
		/* (-1, 1, 0) */
		CSG_Intersection_Find_Edge_Point(a_verts + a->face_vertices[0],
						 a_norms + a->face_normals[0],
						 a_verts + a->face_vertices[1],
						 a_norms + a->face_normals[1],
						 &(b->face_plane),
						 &(a_seg->start_point),
						 &(a_seg->start_normal));
		line_pt = a_seg->start_point;
		VSub(a_verts[a->face_vertices[2]].location, line_pt, line_dir);
		a_seg->start_distance = 0.0;
		a_seg->start_vertex = 0;
		a_seg->start_status = segment_edge;
		a_seg->middle_status = segment_face;
		a_seg->end_distance = 1.0;
		a_seg->end_vertex = 2;
		a_seg->end_point = a_verts[a->face_vertices[2]].location;
		a_seg->end_normal = a_norms[a->face_normals[2]];
		a_seg->end_status = segment_vertex;
	    }
	    else
	    {
		CSG_Intersection_Find_Edge_Point(a_verts + a->face_vertices[0],
						 a_norms + a->face_normals[0],
						 a_verts + a->face_vertices[1],
						 a_norms + a->face_normals[1],
						 &(b->face_plane),
						 &(a_seg->start_point),
						 &(a_seg->start_normal));
		CSG_Intersection_Find_Edge_Point(a_verts + a->face_vertices[2],
						 a_norms + a->face_normals[2],
						 a_verts + a->face_vertices[0],
						 a_norms + a->face_normals[0],
						 &(b->face_plane),
						 &(a_seg->end_point),
						 &(a_seg->end_normal));
		line_pt = a_seg->start_point;
		VSub(a_seg->end_point, a_seg->start_point, line_dir);
		a_seg->start_distance = 0.0;
		a_seg->start_vertex = 0;
		a_seg->start_status = segment_edge;
		a_seg->middle_status = segment_face;
		a_seg->end_distance = 1.0;
		a_seg->end_vertex = 2;
		a_seg->end_status = segment_edge;
	    }
	    break;
	}
	break;
      case 0:
	if ( a_signs[1] == 0 )
	{
	    /* Must be (0, 0, 1) */
	    line_pt = a_verts[a->face_vertices[0]].location;
	    VSub(a_verts[a->face_vertices[1]].location, line_pt, line_dir);
	    a_seg->start_distance = 0.0;
	    a_seg->start_vertex = 0;
	    a_seg->start_point = line_pt;
	    a_seg->start_normal = a_norms[a->face_normals[0]];
	    a_seg->start_status = segment_vertex;
	    a_seg->middle_status = segment_edge;
	    a_seg->end_distance = 1.0;
	    a_seg->end_vertex = 1;
	    a_seg->end_point = a_verts[a->face_vertices[1]].location;
	    a_seg->end_normal = a_norms[a->face_normals[1]];
	    a_seg->end_status = segment_vertex;
	}
	else
	{
	    /* Must be (0, 1, 1) */
	    VCross(a->face_plane.p_vector, b->face_plane.p_vector,line_dir);
	    VUnit(line_dir, temp_d, line_dir);
	    line_pt = a_verts[a->face_vertices[0]].location;
	    a_seg->start_distance = 0.0;
	    a_seg->start_vertex = 0;
	    a_seg->start_point = line_pt;
	    a_seg->start_normal = a_norms[a->face_normals[0]];
	    a_seg->start_status = segment_vertex;
	    a_seg->middle_status = segment_vertex;
	    a_seg->end_distance = 0.0;
	    a_seg->end_vertex = 0;
	    a_seg->end_point = a_seg->start_point;
	    a_seg->end_normal = a_seg->start_normal;
	    a_seg->end_status = segment_vertex;
	}
	break;
    }

    switch ( b_signs[0] )
    {
      case -1:
	switch ( b_signs[1] )
	{
	  case -1:
	    if ( b_signs[2] == 0 )
	    {
		/* (-1, -1, 0) */
		Project_Vect_Onto_Line(&(b_verts[b->face_vertices[2]].location),
				       &line_pt, &line_dir,
				       &(b_seg->start_point));
		b_seg->start_distance = CSG_Intersection_Determine_Distance(
				    &line_pt, &line_dir, &(b_seg->start_point));
		b_seg->start_vertex = 2;
		b_seg->start_normal = b_norms[b->face_normals[2]];
		b_seg->start_status = segment_vertex;
		b_seg->middle_status = segment_vertex;
		b_seg->end_distance = b_seg->start_distance;
		b_seg->end_vertex = 2;
		b_seg->end_point = b_seg->start_point;
		b_seg->end_normal = b_seg->start_normal;
		b_seg->end_status = segment_vertex;
	    }
	    else
	    {
		/* (-1, -1, 1) */
		CSG_Intersection_Find_Edge_Point(b_verts + b->face_vertices[1],
						 b_norms + b->face_normals[1],
						 b_verts + b->face_vertices[2],
						 b_norms + b->face_normals[2],
						 &(a->face_plane),
						 &(b_seg->start_point),
						 &(b_seg->start_normal));
		CSG_Intersection_Find_Edge_Point(b_verts + b->face_vertices[2],
						 b_norms + b->face_normals[2],
						 b_verts + b->face_vertices[0],
						 b_norms + b->face_normals[0],
						 &(a->face_plane),
						 &(b_seg->end_point),
						 &(b_seg->end_normal));
		Project_Vect_Onto_Line(&(b_seg->start_point),
				       &line_pt, &line_dir,
				       &(b_seg->start_point));
		Project_Vect_Onto_Line(&(b_seg->end_point),
				       &line_pt, &line_dir,
				       &(b_seg->end_point));

		b_seg->start_distance = CSG_Intersection_Determine_Distance(
				    &line_pt, &line_dir, &(b_seg->start_point));
		b_seg->start_vertex = 1;
		b_seg->start_status = segment_edge;
		b_seg->middle_status = segment_face;
		b_seg->end_distance = CSG_Intersection_Determine_Distance(
				    &line_pt, &line_dir, &(b_seg->end_point));
		b_seg->end_vertex = 2;
		b_seg->end_status = segment_edge;
	    }
	    break;
	  case 0:
	    if ( b_signs[2] == 0 )
	    {
		/* (-1, 0, 0) */
		Project_Vect_Onto_Line(&(b_verts[b->face_vertices[1]].location),
				       &line_pt, &line_dir,
				       &(b_seg->start_point));
		b_seg->start_distance = CSG_Intersection_Determine_Distance(
				    &line_pt, &line_dir, &(b_seg->start_point));
		b_seg->start_vertex = 1;
		b_seg->start_normal = b_norms[b->face_normals[1]];
		b_seg->start_status = segment_vertex;
		b_seg->middle_status = segment_edge;
		Project_Vect_Onto_Line(&(b_verts[b->face_vertices[2]].location),
				       &line_pt, &line_dir,
				       &(b_seg->end_point));
		b_seg->end_distance = CSG_Intersection_Determine_Distance(
				    &line_pt, &line_dir, &(b_seg->end_point));
		b_seg->end_vertex = 2;
		b_seg->end_normal = b_norms[b->face_normals[2]];
		b_seg->end_status = segment_vertex;
	    }
	    else
	    {
		/* (-1, 0, 1) */
		Project_Vect_Onto_Line(&(b_verts[b->face_vertices[1]].location),
				       &line_pt, &line_dir,
				       &(b_seg->start_point));
		b_seg->start_distance = CSG_Intersection_Determine_Distance(
				    &line_pt, &line_dir, &(b_seg->start_point));
		b_seg->start_vertex = 1;
		b_seg->start_normal = b_norms[b->face_normals[1]];
		b_seg->start_status = segment_vertex;
		b_seg->middle_status = segment_face;
		CSG_Intersection_Find_Edge_Point(b_verts + b->face_vertices[2],
						 b_norms + b->face_normals[2],
						 b_verts + b->face_vertices[0],
						 b_norms + b->face_normals[0],
						 &(a->face_plane),
						 &(b_seg->end_point),
						 &(b_seg->end_normal));
		Project_Vect_Onto_Line(&(b_seg->end_point),
				       &line_pt, &line_dir,
				       &(b_seg->end_point));
		b_seg->end_distance = CSG_Intersection_Determine_Distance(
				    &line_pt, &line_dir, &(b_seg->end_point));
		b_seg->end_vertex = 2;
		b_seg->end_status = segment_edge;
	    }
	    break;
	  case 1:
	    if ( b_signs[2] == 0 )
	    {
		/* (-1, 1, 0) */
		CSG_Intersection_Find_Edge_Point(b_verts + b->face_vertices[0],
						 b_norms + b->face_normals[0],
						 b_verts + b->face_vertices[1],
						 b_norms + b->face_normals[1],
						 &(a->face_plane),
						 &(b_seg->start_point),
						 &(b_seg->start_normal));
		Project_Vect_Onto_Line(&(b_seg->start_point),
				       &line_pt, &line_dir,
				       &(b_seg->start_point));
		b_seg->start_distance = CSG_Intersection_Determine_Distance(
				    &line_pt, &line_dir, &(b_seg->start_point));
		b_seg->start_vertex = 0;
		b_seg->start_status = segment_edge;
		b_seg->middle_status = segment_face;
		Project_Vect_Onto_Line(&(b_verts[b->face_vertices[2]].location),
				       &line_pt, &line_dir,
				       &(b_seg->end_point));
		b_seg->end_distance = CSG_Intersection_Determine_Distance(
				    &line_pt, &line_dir, &(b_seg->end_point));
		b_seg->end_vertex = 2;
		b_seg->end_normal = b_norms[b->face_normals[2]];
		b_seg->end_status = segment_vertex;
	    }
	    else
	    {
		/* (-1, 1, 1) */
		CSG_Intersection_Find_Edge_Point(b_verts + b->face_vertices[0],
						 b_norms + b->face_normals[0],
						 b_verts + b->face_vertices[1],
						 b_norms + b->face_normals[1],
						 &(a->face_plane),
						 &(b_seg->start_point),
						 &(b_seg->start_normal));
		CSG_Intersection_Find_Edge_Point(b_verts + b->face_vertices[2],
						 b_norms + b->face_normals[2],
						 b_verts + b->face_vertices[0],
						 b_norms + b->face_normals[0],
						 &(a->face_plane),
						 &(b_seg->end_point),
						 &(b_seg->end_normal));
		Project_Vect_Onto_Line(&(b_seg->start_point),
				       &line_pt, &line_dir,
				       &(b_seg->start_point));
		Project_Vect_Onto_Line(&(b_seg->end_point),
				       &line_pt, &line_dir,
				       &(b_seg->end_point));
		b_seg->start_distance = CSG_Intersection_Determine_Distance(
				    &line_pt, &line_dir, &(b_seg->start_point));
		b_seg->start_vertex = 0;
		b_seg->start_status = segment_edge;
		b_seg->middle_status = segment_face;
		b_seg->end_distance = CSG_Intersection_Determine_Distance(
				    &line_pt, &line_dir, &(b_seg->end_point));
		b_seg->end_vertex = 2;
		b_seg->end_status = segment_edge;
	    }
	    break;
	}
	break;
      case 0:
	if ( b_signs[1] == 0 )
	{
	    /* Must be (0, 0, 1) */
	    Project_Vect_Onto_Line(&(b_verts[b->face_vertices[0]].location),
				   &line_pt, &line_dir,
				   &(b_seg->start_point));
	    b_seg->start_distance = CSG_Intersection_Determine_Distance(
				&line_pt, &line_dir, &(b_seg->start_point));
	    b_seg->start_vertex = 0;
	    b_seg->start_normal = b_norms[b->face_normals[0]];
	    b_seg->start_status = segment_vertex;
	    b_seg->middle_status = segment_edge;

	    Project_Vect_Onto_Line(&(b_verts[b->face_vertices[1]].location),
				   &line_pt, &line_dir,
				   &(b_seg->end_point));
	    b_seg->end_distance = CSG_Intersection_Determine_Distance(
				&line_pt, &line_dir, &(b_seg->end_point));
	    b_seg->end_vertex = 1;
	    b_seg->end_normal = b_norms[b->face_normals[1]];
	    b_seg->end_status = segment_vertex;
	}
	else
	{
	    /* Must be (0, 1, 1) */
	    Project_Vect_Onto_Line(&(b_verts[b->face_vertices[2]].location),
				   &line_pt, &line_dir,
				   &(b_seg->start_point));
	    b_seg->start_distance = CSG_Intersection_Determine_Distance(
				&line_pt, &line_dir, &(b_seg->start_point));
	    b_seg->start_vertex = 2;
	    b_seg->start_normal = b_norms[b->face_normals[2]];
	    b_seg->start_status = segment_vertex;
	    b_seg->middle_status = segment_vertex;
	    b_seg->end_distance = b_seg->start_distance;
	    b_seg->end_vertex = 2;
	    b_seg->end_point = b_seg->start_point;
	    b_seg->end_normal = b_seg->start_normal;
	    b_seg->end_status = segment_vertex;
	}
	break;
    }
}



static void
CSG_Intersection_Find_Edge_Point(CSGVertexPtr v1, VectorPtr n1,
                                 CSGVertexPtr v2, VectorPtr n2,
                                 CSGPlanePtr pl,
                                 VectorPtr v_res, VectorPtr n_res)
{
    double  v1_dist, v2_dist;
    Vector  temp_v;
    double  temp_d;

    /* Get the signed distances from the plane (again). */
    VSub(v1->location, pl->p_point, temp_v);
    v1_dist = VDot(temp_v, pl->p_vector);
    VSub(v2->location, pl->p_point, temp_v);
    v2_dist = VDot(temp_v, pl->p_vector);

    temp_d = v1_dist - v2_dist;
    v_res->x = ( v1_dist * v2->location.x - v2_dist * v1->location.x ) / temp_d;
    v_res->y = ( v1_dist * v2->location.y - v2_dist * v1->location.y ) / temp_d;
    v_res->z = ( v1_dist * v2->location.z - v2_dist * v1->location.z ) / temp_d;

    n_res->x = ( v1_dist * n2->x - v2_dist * n1->x ) / temp_d;
    n_res->y = ( v1_dist * n2->y - v2_dist * n1->y ) / temp_d;
    n_res->z = ( v1_dist * n2->z - v2_dist * n1->z ) / temp_d;
    if ( CSGVZero(*n_res) )
        VNew(1, 0, 0, *n_res);
    else
        VUnit(*n_res, temp_d, *n_res);
}


static double
CSG_Intersection_Determine_Distance(Vector *l_pt, Vector *dir, Vector *point)
{
    Vector  temp_v;

    VSub(*point, *l_pt, temp_v);
    return VDot(*dir, temp_v) / VDot(*dir, *dir);
}


static Boolean
CSG_Determine_Segment_Intersection(CSGSegmentPtr a_seg, CSGSegmentPtr b_seg)
{
    double  dist;
    double  d1, d2;

    if ( b_seg->start_distance > b_seg->end_distance )
    {
	/* Swap the endpoints. */
        SegmentStatus   stat;
        int             vert;
        Vector          pt;

        vert = b_seg->start_vertex;
        b_seg->start_vertex = b_seg->end_vertex;
        b_seg->end_vertex = vert;
        dist = b_seg->start_distance;
        b_seg->start_distance = b_seg->end_distance;
        b_seg->end_distance = dist;
        pt = b_seg->start_point;
        b_seg->start_point = b_seg->end_point;
        b_seg->end_point = pt;
        pt = b_seg->start_normal;
        b_seg->start_normal = b_seg->end_normal;
        b_seg->end_normal = pt;
        stat = b_seg->start_status;
        b_seg->start_status = b_seg->end_status;
        b_seg->end_status = stat;
    }

    if ( a_seg->start_distance < b_seg->start_distance + CSG_EPSILON &&
	 ! CSGVEqual(a_seg->start_point, b_seg->start_point, diff) )
    {
	/* Calculate new normal. */
	if ( a_seg->end_distance == 1.0 )
	{
	    d1 = b_seg->start_distance - a_seg->start_distance;
	    d2 = 1.0 - b_seg->start_distance;
	    a_seg->start_normal.x =
		a_seg->end_normal.x * d1 + a_seg->start_normal.x * d2;
	    a_seg->start_normal.y =
		a_seg->end_normal.y * d1 + a_seg->start_normal.y * d2;
	    a_seg->start_normal.z =
		a_seg->end_normal.z * d1 + a_seg->start_normal.z * d2;
	    if ( VNearZero(a_seg->start_normal) )
		a_seg->start_normal = a_seg->end_normal;
	    else
		VUnit(a_seg->start_normal, dist, a_seg->start_normal);
	}

	a_seg->start_distance = b_seg->start_distance;
	a_seg->start_point = b_seg->start_point;
	a_seg->start_status = a_seg->middle_status;
    }

    if ( a_seg->end_distance > b_seg->end_distance - CSG_EPSILON &&
	 ! CSGVEqual(a_seg->end_point, b_seg->end_point, diff) )
    {
	/* Calculate new normal. */
	if ( a_seg->end_distance == 1.0 )
	{
	    d1 = b_seg->end_distance - a_seg->start_distance;
	    d2 = 1.0 - b_seg->end_distance;
	    a_seg->end_normal.x =
		a_seg->end_normal.x * d1 + a_seg->start_normal.x * d2;
	    a_seg->end_normal.y =
		a_seg->end_normal.y * d1 + a_seg->start_normal.y * d2;
	    a_seg->end_normal.z =
		a_seg->end_normal.z * d1 + a_seg->start_normal.z * d2;
	    if ( VNearZero(a_seg->end_normal) )
		a_seg->end_normal = a_seg->start_normal;
	    else
		VUnit(a_seg->end_normal, dist, a_seg->end_normal);
	}

	a_seg->end_distance = b_seg->end_distance;
	a_seg->end_point = b_seg->end_point;
	a_seg->end_status = a_seg->middle_status;
    }

    return ( a_seg->end_distance >= a_seg->start_distance );
}


static void
CSG_Validate_Segment(CSGSegmentPtr new)
{
    /* Check for features where the start and end vertex are identical
    ** but shouldn't be.
    */
    if ( ! CSGVEqual(new->start_point, new->end_point, diff))
	return;

    if ( new->start_status == segment_vertex &&
	 new->middle_status == segment_edge &&
	 new->end_status == segment_edge )
    {
	new->middle_status = segment_vertex;
	new->end_status = segment_vertex;
	new->end_vertex = new->start_vertex;
    }
    else if ( new->start_status == segment_edge &&
	      new->middle_status == segment_edge &&
	      new->end_status == segment_vertex )
    {
	new->middle_status = segment_vertex;
	new->start_status = segment_vertex;
	new->start_vertex = new->end_vertex;
    }
    else if ( new->start_status == segment_vertex &&
	      new->middle_status == segment_face &&
	      new->end_status == segment_face )
    {
	new->middle_status = segment_vertex;
	new->end_status = segment_vertex;
	new->end_vertex = new->start_vertex;
    }
    else if ( new->start_status == segment_face &&
	      new->middle_status == segment_face &&
	      new->end_status == segment_vertex )  
    {
	new->middle_status = segment_vertex;
	new->start_status = segment_vertex;
	new->start_vertex = new->end_vertex;
    }
    else if ( new->start_status == segment_edge &&
	      new->middle_status == segment_face &&
	      new->end_status == segment_face )
    {
	new->middle_status = segment_edge;
	new->end_status = segment_edge;
	new->end_vertex = new->start_vertex + 1;
    }
    else if ( new->start_status == segment_face &&
	      new->middle_status == segment_face &&
	      new->end_status == segment_edge )
    {
	new->middle_status = segment_edge;
	new->start_status = segment_edge;
	new->start_vertex = 0;
    }

    else if ( new->start_status == segment_edge && 
	      new->middle_status == segment_face &&
	      new->end_status == segment_edge )
    {
	/* There must be a vertex nearby. */
	/* But which one? */
	if ( new->start_vertex == 0 )
	{
	    new->start_status = segment_vertex;
	    new->middle_status = segment_vertex;
	    new->end_status = segment_vertex;
	    new->end_vertex = new->start_vertex;
	}
	else
	{
	    new->start_status = segment_vertex;
	    new->middle_status = segment_vertex;
	    new->end_status = segment_vertex;
	    new->start_vertex = new->end_vertex; /* 2 */
	}

    }
}

