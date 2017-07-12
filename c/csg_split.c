/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/csg_split.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: csg_split.c,v $
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
**  csg_split.c : Functions for splitting CSG polygons.
*/

#include <sys/times.h>

#include <math.h>
#include <config.h>
#include <sced.h>
#include <csg.h>
#include <csg_wire.h>

static int  	CSG_Split_Add_Vertex(CSGWireframePtr, Vector);
static int  	CSG_Split_Add_Normal(CSGWireframePtr, Vector);
static Boolean  CSG_Split_Add_Face(CSGWireframePtr, int, int, int,
				   int, int, int, AttributePtr);

#define vertex(f, i)    ( w->vertices[w->faces[f].face_vertices[i]] )

#if CSG_TIME
extern long time_used;
#endif

int
CSG_Vert_Vert_Vert_Split(CSGWireframePtr w, short index, CSGSegmentPtr seg)
{
#if ( CSG_WIRE_DEBUG_1 )
    fprintf(stderr,"V-V-V face %d vertex %d\n", index, seg->start_vertex);
#endif

    vertex(index, seg->start_vertex).status = vertex_boundary;

    return CSG_OK;
}


int
CSG_Vert_Edge_Vert_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
#if ( CSG_WIRE_DEBUG_1 )
    fprintf(stderr,"V-E-V face %d vertices %d %d\n", index, seg->start_vertex,
        seg->end_vertex);
#endif

    vertex(index, seg->start_vertex).status = vertex_boundary;
    vertex(index, seg->end_vertex).status = vertex_boundary;

    return CSG_OK;
}


int
CSG_Vert_Edge_Edge_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
    int		new_v, new_n;
    int		res;

    if ( seg->start_status == segment_vertex )
    {
	if ( seg->start_vertex == 1 )
	    Face_Shift_Left(w->faces + index);
	vertex(index, 0).status = vertex_boundary;
	new_v = CSG_Split_Add_Vertex(w, seg->end_point);
	new_n = CSG_Split_Add_Normal(w, seg->end_normal);
    }
    else
    {
	if ( seg->start_vertex == 1 )
	    Face_Shift_Left(w->faces + index);
	vertex(index, 1).status = vertex_boundary;
	new_v = CSG_Split_Add_Vertex(w, seg->start_point);
	new_n = CSG_Split_Add_Normal(w, seg->start_normal);
    }

    if ( ( res = CSG_Split_Add_Face(w,
				    w->faces[index].face_vertices[2],
				    new_v,
				    w->faces[index].face_vertices[1],
				    w->faces[index].face_normals[2],
				    new_n,
				    w->faces[index].face_normals[1],
				    w->faces[index].face_attribs)) != CSG_OK )
	return res;

    w->faces[index].face_vertices[1] = new_v;
    w->faces[index].face_normals[1] = new_n;

    return CSG_OK;
}


int
CSG_Vert_Face_Edge_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
    int		new_v, new_n;
    int		res;

#if ( CSG_WIRE_DEBUG )
    fprintf(stderr,"V-F-E face %d vert %d edge %d\n", index,
        seg->end_status == segment_vertex ? seg->end_vertex : seg->start_vertex,
        seg->end_status == segment_vertex ? seg->start_vertex :seg->end_vertex);
#endif

    vertex(index, 1).status = vertex_boundary;
    new_v = CSG_Split_Add_Vertex(w, seg->end_point);
    new_n = CSG_Split_Add_Normal(w, seg->end_normal);

    if ( ( res = CSG_Split_Add_Face(w,
				    w->faces[index].face_vertices[2],
				    new_v,
				    w->faces[index].face_vertices[1],
				    w->faces[index].face_normals[2],
				    new_n,
				    w->faces[index].face_normals[1],
				    w->faces[index].face_attribs)) != CSG_OK )
	return res;

    w->faces[index].face_vertices[2] = new_v;
    w->faces[index].face_normals[2] = new_n;

    return CSG_OK;
}


int
CSG_Edge_Face_Vert_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
    int		new_v, new_n;
    int		res;

#if ( CSG_WIRE_DEBUG )
    fprintf(stderr,"E-F-V face %d vert %d edge %d\n", index,
        seg->end_status == segment_vertex ? seg->end_vertex : seg->start_vertex,
        seg->end_status == segment_vertex ? seg->start_vertex :seg->end_vertex);
#endif

    vertex(index, 2).status = vertex_boundary;
    new_v = CSG_Split_Add_Vertex(w, seg->start_point);
    new_n = CSG_Split_Add_Normal(w, seg->start_normal);

    if ( ( res = CSG_Split_Add_Face(w,
				    w->faces[index].face_vertices[2],
				    new_v,
				    w->faces[index].face_vertices[1],
				    w->faces[index].face_normals[2],
				    new_n,
				    w->faces[index].face_normals[1],
				    w->faces[index].face_attribs)) != CSG_OK )
	return res;

    w->faces[index].face_vertices[1] = new_v;
    w->faces[index].face_normals[1] = new_n;

    return CSG_OK;
}


int
CSG_Vert_Face_Face_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
    int		new_v, new_n;
    int		res;

#if ( CSG_WIRE_DEBUG )
    fprintf(stderr,"V-F-F face %d vert %d edge %d\n", index,
        seg->start_vertex, seg->end_vertex);
#endif

    vertex(index, 1).status = vertex_boundary;
    new_v = CSG_Split_Add_Vertex(w, seg->end_point);
    new_n = CSG_Split_Add_Normal(w, seg->end_normal);

    if ( ( res = CSG_Split_Add_Face(w,
				    w->faces[index].face_vertices[2],
				    new_v,
				    w->faces[index].face_vertices[1],
				    w->faces[index].face_normals[2],
				    new_n,
				    w->faces[index].face_normals[1],
				    w->faces[index].face_attribs)) != CSG_OK )
	return res;
    if ( ( res = CSG_Split_Add_Face(w,
				    w->faces[index].face_vertices[0],
				    new_v,
				    w->faces[index].face_vertices[2],
				    w->faces[index].face_normals[0],
				    new_n,
				    w->faces[index].face_normals[2],
				    w->faces[index].face_attribs)) != CSG_OK )
	return res;

    w->faces[index].face_vertices[2] = new_v;
    w->faces[index].face_normals[2] = new_n;

    return CSG_OK;
}


int
CSG_Face_Face_Vert_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
    int		new_v, new_n;
    int		res;

#if ( CSG_WIRE_DEBUG )
    fprintf(stderr,"F-F-V face %d vert %d edge %d\n", index,
        seg->end_vertex, seg->start_vertex);
#endif

    vertex(index, 2).status = vertex_boundary;
    new_v = CSG_Split_Add_Vertex(w, seg->start_point);
    new_n = CSG_Split_Add_Normal(w, seg->start_normal);

    if ( ( res = CSG_Split_Add_Face(w,
				    w->faces[index].face_vertices[1],
				    new_v,
				    w->faces[index].face_vertices[0],
				    w->faces[index].face_normals[1],
				    new_n,
				    w->faces[index].face_normals[0],
				    w->faces[index].face_attribs)) != CSG_OK )
	return res;
    if ( ( res = CSG_Split_Add_Face(w,
				    w->faces[index].face_vertices[2],
				    new_v,
				    w->faces[index].face_vertices[1],
				    w->faces[index].face_normals[2],
				    new_n,
				    w->faces[index].face_normals[1],
				    w->faces[index].face_attribs)) != CSG_OK )
	return res;

    w->faces[index].face_vertices[1] = new_v;
    w->faces[index].face_normals[1] = new_n;

    return CSG_OK;
}


int
CSG_Edge_Edge_Edge_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
    int		new_v1, new_v2, new_n1, new_n2;
    int		res;
    Vector	diff;

#if ( CSG_WIRE_DEBUG )
    fprintf(stderr,"E-E-E face %d edge %d %d\n", index,
        seg->end_vertex, seg->start_vertex);
#endif

    if ( seg->start_vertex == 1 )
	Face_Shift_Left(w->faces + index);

    if ( CSGVEqual(seg->start_point, seg->end_point, diff) )
    {
	new_v1 = CSG_Split_Add_Vertex(w, seg->start_point);
	new_n1 = CSG_Split_Add_Normal(w, seg->start_normal);

	if ( ( res = CSG_Split_Add_Face(w,
					w->faces[index].face_vertices[2],
					new_v1,
					w->faces[index].face_vertices[1],
					w->faces[index].face_normals[2],
					new_n1,
					w->faces[index].face_normals[1],
					w->faces[index].face_attribs)) !=
					CSG_OK )
	    return res;

	if ( seg->start_vertex == 0 && seg->end_vertex == 2 )
	{
	    w->faces[index].face_vertices[2] = new_v1;
	    w->faces[index].face_normals[2] = new_n1;
	}
	else
	{
	    w->faces[index].face_vertices[1] = new_v1;
	    w->faces[index].face_normals[1] = new_n1;
	}
    }
    else
    {
	new_v1 = CSG_Split_Add_Vertex(w, seg->start_point);
	new_n1 = CSG_Split_Add_Normal(w, seg->start_normal);
	new_v2 = CSG_Split_Add_Vertex(w, seg->end_point);
	new_n2 = CSG_Split_Add_Normal(w, seg->end_normal);

	if ( ( res = CSG_Split_Add_Face(w, new_v1, new_v2,
					w->faces[index].face_vertices[2],
					new_n1, new_n2,
					w->faces[index].face_normals[2],
					w->faces[index].face_attribs)) !=
					CSG_OK )
	    return res;
	if ( ( res = CSG_Split_Add_Face(w,
					w->faces[index].face_vertices[2],
					new_v2,
					w->faces[index].face_vertices[1],
					w->faces[index].face_normals[2],
					new_n2,
					w->faces[index].face_normals[1],
					w->faces[index].face_attribs)) !=
					CSG_OK )
	    return res;

	w->faces[index].face_vertices[1] = new_v1;
	w->faces[index].face_normals[1] = new_n1;
    }

    return CSG_OK;
}




int
CSG_Edge_Face_Edge_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
    int		new_v1, new_v2, new_n1, new_n2;
    int		res;

#if ( CSG_WIRE_DEBUG )
    fprintf(stderr,"E-F-E face %d edges %d %d\n", index, seg->start_vertex,
        seg->end_vertex);
#endif

    new_v1 = CSG_Split_Add_Vertex(w, seg->start_point);
    new_n1 = CSG_Split_Add_Normal(w, seg->start_normal);
    new_v2 = CSG_Split_Add_Vertex(w, seg->end_point);
    new_n2 = CSG_Split_Add_Normal(w, seg->end_normal);

    if ( ( res = CSG_Split_Add_Face(w, new_v2, new_v1,
				    w->faces[index].face_vertices[2],
				    new_n2, new_n1,
				    w->faces[index].face_normals[2],
				    w->faces[index].face_attribs)) != CSG_OK )
	return res;
    if ( ( res = CSG_Split_Add_Face(w, new_v1, new_v2,
				    w->faces[index].face_vertices[0],
				    new_n1, new_n2,
				    w->faces[index].face_normals[0],
				    w->faces[index].face_attribs)) != CSG_OK )
	return res;

    if ( seg->start_vertex == 0 )
    {
	w->faces[index].face_vertices[0] = new_v1;
	w->faces[index].face_normals[0] = new_n1;
    }
    else
    {
	w->faces[index].face_vertices[2] = new_v1;
	w->faces[index].face_normals[2] = new_n1;
    }

    return CSG_OK;
}


int
CSG_Edge_Face_Face_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
    int		new_v1, new_v2, new_n1, new_n2;
    int		res;

#if ( CSG_WIRE_DEBUG )
    fprintf(stderr,"E-F-F face %d edges %d %d\n", index, seg->start_vertex,
        seg->end_vertex);
#endif

    new_v1 = CSG_Split_Add_Vertex(w, seg->start_point);
    new_n1 = CSG_Split_Add_Normal(w, seg->start_normal);
    new_v2 = CSG_Split_Add_Vertex(w, seg->end_point);
    new_n2 = CSG_Split_Add_Normal(w, seg->end_normal);

    if ( seg->start_status == segment_face )
    {
	if ( ( res = CSG_Split_Add_Face(w, new_v2, new_v1,
					w->faces[index].face_vertices[2],
					new_n2, new_n1,
					w->faces[index].face_normals[2],
					w->faces[index].face_attribs)) !=
					CSG_OK )
	    return res;
	if ( ( res = CSG_Split_Add_Face(w, new_v1, new_v2,
					w->faces[index].face_vertices[0],
					new_n1, new_n2,
					w->faces[index].face_normals[0],
					w->faces[index].face_attribs)) !=
					CSG_OK )
	    return res;
	if ( ( res = CSG_Split_Add_Face(w,
					w->faces[index].face_vertices[2],
					new_v1,
					w->faces[index].face_vertices[1],
					w->faces[index].face_normals[2],
					new_n1,
					w->faces[index].face_normals[1],
					w->faces[index].face_attribs)) !=
					CSG_OK )
	    return res;

	w->faces[index].face_vertices[2] = new_v1;
	w->faces[index].face_normals[2] = new_n1;
    }
    else if ( seg->start_vertex == 1 )
    {
	if ( ( res = CSG_Split_Add_Face(w, new_v1, new_v2,
					w->faces[index].face_vertices[1],
					new_n1, new_n2,
					w->faces[index].face_normals[1],
					w->faces[index].face_attribs)) !=
					CSG_OK )
	    return res;
	if ( ( res = CSG_Split_Add_Face(w, new_v2, new_v1,
					w->faces[index].face_vertices[2],
					new_n2, new_n1,
					w->faces[index].face_normals[2],
					w->faces[index].face_attribs)) !=
					CSG_OK )
	    return res;
	if ( ( res = CSG_Split_Add_Face(w,
					w->faces[index].face_vertices[0],
					new_v2,
					w->faces[index].face_vertices[2],
					w->faces[index].face_normals[0],
					new_n2,
					w->faces[index].face_normals[2],
					w->faces[index].face_attribs)) !=
					CSG_OK )
	    return res;
	w->faces[index].face_vertices[2] = new_v2;
	w->faces[index].face_normals[2] = new_n2;
    }
    else
    {
	if ( ( res = CSG_Split_Add_Face(w, new_v1, new_v2,
					w->faces[index].face_vertices[0],
					new_n1, new_n2,
					w->faces[index].face_normals[0],
					w->faces[index].face_attribs)) !=
					CSG_OK )
	    return res;
	if ( ( res = CSG_Split_Add_Face(w, new_v2, new_v1,
					w->faces[index].face_vertices[1],
					new_n2, new_n1,
					w->faces[index].face_normals[1],
					w->faces[index].face_attribs)) !=
					CSG_OK )
	    return res;
	if ( ( res = CSG_Split_Add_Face(w,
					w->faces[index].face_vertices[2],
					new_v2,
					w->faces[index].face_vertices[1],
					w->faces[index].face_normals[2],
					new_n2,
					w->faces[index].face_normals[1],
					w->faces[index].face_attribs)) !=
					CSG_OK )
	    return res;
	w->faces[index].face_vertices[1] = new_v2;
	w->faces[index].face_normals[1] = new_n2;
    }

    return CSG_OK;
}


int
CSG_Face_Face_Face_Split(CSGWireframePtr w, short index, CSGSegment *seg)
{
    int		new_v1, new_v2, new_n1, new_n2;
    int		res;
    Vector	diff;

#if ( CSG_WIRE_DEBUG )
    fprintf(stderr,"F-F-F face %d start %d end %d\n", index, seg->start_vertex,
            seg->end_vertex);
#endif

    if ( CSGVEqual(seg->start_point, seg->end_point, diff) )
    {
	new_v1 = CSG_Split_Add_Vertex(w, seg->start_point);
	new_n1 = CSG_Split_Add_Normal(w, seg->start_normal);

	if ( ( res = CSG_Split_Add_Face(w,
					w->faces[index].face_vertices[2],
					new_v1,
					w->faces[index].face_vertices[1],
					w->faces[index].face_normals[2],
					new_n1,
					w->faces[index].face_normals[1],
					w->faces[index].face_attribs)) !=
					CSG_OK )
	    return res;
	if ( ( res = CSG_Split_Add_Face(w,
					w->faces[index].face_vertices[0],
					new_v1,
					w->faces[index].face_vertices[2],
					w->faces[index].face_normals[0],
					new_n1,
					w->faces[index].face_normals[2],
					w->faces[index].face_attribs)) !=
					CSG_OK )
	    return res;
	w->faces[index].face_vertices[2] = new_v1;
	w->faces[index].face_normals[2] = new_n1;
    }
    else
    {
	new_v1 = CSG_Split_Add_Vertex(w, seg->start_point);
	new_n1 = CSG_Split_Add_Normal(w, seg->start_normal);
	new_v2 = CSG_Split_Add_Vertex(w, seg->end_point);
	new_n2 = CSG_Split_Add_Normal(w, seg->end_normal);

	if ( seg->start_vertex == 1 )
	{
	    if ( ( res = CSG_Split_Add_Face(w, new_v2, new_v1,
					    w->faces[index].face_vertices[2],
					    new_n2, new_n1,
					    w->faces[index].face_normals[2],
					    w->faces[index].face_attribs)) !=
					    CSG_OK )
		return res;
	    if ( ( res = CSG_Split_Add_Face(w,
					    w->faces[index].face_vertices[2],
					    new_v1,
					    w->faces[index].face_vertices[1],
					    w->faces[index].face_normals[2],
					    new_n1,
					    w->faces[index].face_normals[1],
					    w->faces[index].face_attribs)) !=
					    CSG_OK )
		return res;
	    if ( ( res = CSG_Split_Add_Face(w,
					    w->faces[index].face_vertices[0],
					    new_v2,
					    w->faces[index].face_vertices[2],
					    w->faces[index].face_normals[0],
					    new_n2,
					    w->faces[index].face_normals[2],
					    w->faces[index].face_attribs)) !=
					    CSG_OK )
		return res;
	    if ( ( res = CSG_Split_Add_Face(w, new_v1, new_v2,
					    w->faces[index].face_vertices[0],
					    new_n1, new_n2,
					    w->faces[index].face_normals[0],
					    w->faces[index].face_attribs)) !=
					    CSG_OK )
		return res;
	    w->faces[index].face_vertices[2] = new_v1;
	    w->faces[index].face_normals[2] = new_n1;
	}
	else
	{
	    if ( ( res = CSG_Split_Add_Face(w, new_v2, new_v1,
					    w->faces[index].face_vertices[1],
					    new_n2, new_n1,
					    w->faces[index].face_normals[1],
					    w->faces[index].face_attribs)) !=
					    CSG_OK )
		return res;
	    if ( ( res = CSG_Split_Add_Face(w,
					    w->faces[index].face_vertices[2],
					    new_v2,
					    w->faces[index].face_vertices[1],
					    w->faces[index].face_normals[2],
					    new_n2,
					    w->faces[index].face_normals[1],
					    w->faces[index].face_attribs)) !=
					    CSG_OK )
		return res;
	    if ( ( res = CSG_Split_Add_Face(w,
					    w->faces[index].face_vertices[0],
					    new_v2,
					    w->faces[index].face_vertices[2],
					    w->faces[index].face_normals[0],
					    new_n2,
					    w->faces[index].face_normals[2],
					    w->faces[index].face_attribs)) !=
					    CSG_OK )
		return res;
	    if ( ( res = CSG_Split_Add_Face(w, new_v1, new_v2,
					    w->faces[index].face_vertices[0],
					    new_n1, new_n2,
					    w->faces[index].face_normals[0],
					    w->faces[index].face_attribs)) !=
					    CSG_OK )
		return res;
	    w->faces[index].face_vertices[2] = new_v1;
	    w->faces[index].face_normals[2] = new_n1;
	}
    }

    return CSG_OK;
}


/*  int
**  CSG_Split_Add_Vertex(CSGWireframePtr wire, Vector location)
**  Adds a new vertex to the wireframe, updating pointers as it goes.
**  Returns the new vertex.
*/
static int
CSG_Split_Add_Vertex(CSGWireframePtr wire, Vector location)
{
    int search_res;

#if CSG_TIME
    struct tms  time_str;
    long    start_time, end_time;

    times(&time_str);
    start_time = time_str.tms_utime + time_str.tms_stime;
#endif

    if ( wire->num_vertices == wire->max_vertices )
    {
        wire->max_vertices += 25;
        wire->vertices = More(wire->vertices, CSGVertex, wire->max_vertices);
    }

    wire->vertices[wire->num_vertices].location = location;
    wire->vertices[wire->num_vertices].status = vertex_unknown;
    wire->vertices[wire->num_vertices].num_adjacent = 0;
    wire->vertices[wire->num_vertices].max_num_adjacent = 0;

    /* Search through for an identical vertex. */
    search_res = (int)KD_Add_Elmt(&(wire->vertex_tree),
                                  (void*)wire->num_vertices,
                                  (void*)wire->vertices,
                                  CSG_Add_KD_Compare_Vert_Func);

    if ( search_res == wire->num_vertices )
        wire->num_vertices++;

#if CSG_TIME
    times(&time_str);
    end_time = time_str.tms_utime + time_str.tms_stime;

    time_used += ( end_time - start_time );
#endif

    return search_res;
}


static int
CSG_Split_Add_Normal(CSGWireframePtr wire, Vector dir)
{
    int search_res;

#if CSG_TIME
    struct tms  time_str;
    long    start_time, end_time;

    times(&time_str);
    start_time = time_str.tms_utime + time_str.tms_stime;
#endif

    if ( wire->num_normals == wire->max_normals )
    {
        wire->max_normals += 25;
        wire->normals = More(wire->normals, Vector, wire->max_normals);
    }

    wire->normals[wire->num_normals] = dir;

    /* Search through for an identical vertex. */
    search_res = (int)KD_Add_Elmt(&(wire->normal_tree),
                                  (void*)wire->num_normals,
                                  (void*)wire->normals,
                                  CSG_Add_KD_Compare_Norm_Func);

    if ( search_res == wire->num_normals )
        wire->num_normals++;

#if CSG_TIME
    times(&time_str);
    end_time = time_str.tms_utime + time_str.tms_stime;

    time_used += ( end_time - start_time );
#endif

    return search_res;
}


static Boolean
CSG_Split_Add_Face(CSGWireframePtr wire, int v0, int v1, int v2,
		   int n0, int n1, int n2, AttributePtr attribs)
{
    Vector  temp_1, temp_2;
    double  temp_d;

    if ( wire->num_faces == MAX_SIGNED_SHORT )
	return CSG_OVERFLOW;

    if ( wire->num_faces == wire->max_faces )
    {
	wire->max_faces += 10;
	wire->faces = More(wire->faces, CSGFace, wire->max_faces);
    }

    wire->faces[wire->num_faces].face_vertices[0] = v0;
    wire->faces[wire->num_faces].face_vertices[1] = v1;
    wire->faces[wire->num_faces].face_vertices[2] = v2;
    wire->faces[wire->num_faces].face_normals[0] = n0;
    wire->faces[wire->num_faces].face_normals[1] = n1;
    wire->faces[wire->num_faces].face_normals[2] = n2;

    CSG_Face_Bounding_Box(wire->vertices,
			  wire->faces[wire->num_faces].face_vertices, 3,
                          &(wire->faces[wire->num_faces].face_extent));

    VSub(wire->vertices[v0].location, wire->vertices[v1].location, temp_1);
    VSub(wire->vertices[v2].location, wire->vertices[v1].location, temp_2);
    VCross(temp_2, temp_1, wire->faces[wire->num_faces].face_plane.p_vector);
    if ( VZero(wire->faces[wire->num_faces].face_plane.p_vector) )
	return CSG_DEGENERATE;
    VUnit(wire->faces[wire->num_faces].face_plane.p_vector, temp_d,
	  wire->faces[wire->num_faces].face_plane.p_vector);
    wire->faces[wire->num_faces].face_plane.p_point =
	wire->vertices[v0].location;

    wire->faces[wire->num_faces].face_attribs = attribs;

    wire->faces[wire->num_faces].ignore = FALSE;

#if CSG_WIRE_DEBUG
    fprintf(stderr, "Creating face %d: %d %d %d\n", wire->num_faces, v0,v1,v2);
#endif

    wire->num_faces++;

    return CSG_OK;
}

