/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/edge_table.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: edge_table.c,v $
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
**  edge_table.c: Functions for manipulating edge tables (adjacency lists).
*/

#include <sced.h>
#include <edge_table.h>

/* Create a new table of the desired size. */
EdgeTable
Edge_Table_Create(int size)
{
    EdgeTable   result = New(EdgeTableElmt, size);
    int         i;

    for ( i = 0 ; i < size ; i++ )
    {
        result[i].edges = NULL;
        result[i].num_edges = result[i].max_num_edges = 0;
    }

    return result;
}


/* Add a new edge. */
EdgePtr
Edge_Table_Add(EdgeTable table, int p, int q)
{
    EdgePtr edge;
    int     tmp;

    if ( ( edge = Edge_Table_Get(table, p, q) ) )
        return edge;

    if ( p > q )
        tmp = p, p = q, q = tmp;

    if ( table[p].num_edges == table[p].max_num_edges )
    {
        table[p].max_num_edges += 3;
        table[p].edges = More(table[p].edges, Edge, table[p].max_num_edges);
    }

    table[p].edges[table[p].num_edges].endpt = q;
    table[p].edges[table[p].num_edges].val = NULL;
    table[p].num_edges++;

    return table[p].edges + (table[p].num_edges - 1);
}


/* Return an edge, if there. */
EdgePtr
Edge_Table_Get(EdgeTable table, int p, int q)
{
    int tmp;

    if ( p > q )
        tmp = p, p = q, q = tmp;
    
    for ( tmp = 0 ;
          tmp < table[p].num_edges && q != table[p].edges[tmp].endpt ;
          tmp++ );

    if ( tmp == table[p].num_edges )
        return NULL;
    else
        return table[p].edges + tmp;
}


/* Free an edge table. */
void
Edge_Table_Free(EdgeTable table, int size)
{
    int i;

    for ( i = 0 ; i < size ; i++ )
        if ( table[i].edges ) free(table[i].edges);
    free(table);
}


/* Build the edge table for a wireframe. */
void
Edge_Table_Build(WireframePtr wire)
{
    int i, j;

    wire->edges = Edge_Table_Create(wire->num_real_verts);

    for ( i = 0 ; i < wire->num_faces ; i++ )
    {
        if ( wire->faces[i].num_vertices < 2 )
            continue;
        Edge_Table_Add(wire->edges, wire->faces[i].vertices[0],
                    wire->faces[i].vertices[wire->faces[i].num_vertices - 1]);
        for ( j = 1 ; j < wire->faces[i].num_vertices ; j++ )
            Edge_Table_Add(wire->edges, wire->faces[i].vertices[j-1],
                           wire->faces[i].vertices[j]);
    }
}


/* Copy an edge table */
EdgeTable
Edge_Table_Copy(EdgeTable table, int size)
{
    EdgeTable   result = New(EdgeTableElmt, size);
    int         i, j;

    for ( i = 0 ; i < size ; i++ )
    {
        if ( table[i].num_edges )
        {
            result[i].num_edges = result[i].max_num_edges = table[i].num_edges;
            result[i].edges = New(Edge, result[i].num_edges);
            for ( j = 0 ; j < result[i].num_edges ; j++ )
            {
                result[i].edges[j].endpt = table[i].edges[j].endpt;
                result[i].edges[j].val = NULL;
            }
        }
        else
        {
            result[i].num_edges = result[i].max_num_edges = 0;
            result[i].edges = NULL;
        }
    }

    return result;
}


/* Count the number of edges in an edges table. */
int
Edge_Table_Count(EdgeTable table, int size)
{
    int count = 0;
    int i;

    for ( i = 0 ; i < size ; i++ )
        count += table[i].num_edges;
    return count;
}
