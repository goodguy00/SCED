/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/opengl_out.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: opengl_out.c,v $
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
**  Sced: A Constraint Based Object Scene Editor
**
**  opengl_out.c: Code for outputing OpenGL commands to render a given object.
*/

#include <sced.h>
#include <SimpleWire.h>

typedef struct _VNEntry {
    int v;
    int n;
    int count;
    int max;
    int *faces;
    struct _VNEntry *left;
    struct _VNEntry *right;
    } VNEntry, *VNTree;

#define Attribs_Defined(a)  ( (a) && ( ((AttributePtr)(a))->defined ) )


static void
VN_Insert_Entry(VNTree *tree, int v, int n, int face)
{
    if ( ! *tree )
    {
        *tree = New(VNEntry, 1);
        (*tree)->v = v;
        (*tree)->n = n;
        (*tree)->count = 1;
        (*tree)->max = 3;
        (*tree)->faces = New(int, 3);
        (*tree)->faces[0] = face;
        (*tree)->left = (*tree)->right = NULL;
        return;
    }

    if ( v == (*tree)->v )
    {
        if ( n == (*tree)->n )
        {
            if ( (*tree)->count == (*tree)->max )
            {
                (*tree)->max += 3;
                (*tree)->faces = More((*tree)->faces, int, (*tree)->max);
            }
            (*tree)->faces[(*tree)->count] = face;
            (*tree)->count++;
        }
        else if ( n < (*tree)->n )
            VN_Insert_Entry(&((*tree)->left), v, n, face);
        else
            VN_Insert_Entry(&((*tree)->right), v, n, face);
    }
    else if ( v < (*tree)->v )
        VN_Insert_Entry(&((*tree)->left), v, n, face);
    else
        VN_Insert_Entry(&((*tree)->right), v, n, face);
}


static int
VN_Tree_Count(VNTree tree)
{
    if ( ! tree )
        return 0;

    return 1 + VN_Tree_Count(tree->left) + VN_Tree_Count(tree->right);
}

static void
VN_Tree_To_List(VNTree tree, VNTree *list, int *p)
{
    if ( ! tree )
        return;

    list[(*p)++] = tree;
    VN_Tree_To_List(tree->left, list, p);
    VN_Tree_To_List(tree->right, list, p);
}


static void
VN_Tree_Decr(VNTree tree, int v, int n, int f)
{
    int i, j;

    if ( v == tree->v )
    {
        if ( n == tree->n )
        {
            /* Find the face. */
            for ( i = 0 ; tree->faces[i] != f ; i++ );
            for ( j = i + 1 ; j < tree->count ; j++ )
                tree->faces[j-1] = tree->faces[j];
            tree->count--;
        }
        else if ( n < tree->n )
            VN_Tree_Decr(tree->left, v, n, f);
        else
            VN_Tree_Decr(tree->right, v, n, f);
    }
    else if ( v < tree->v )
        VN_Tree_Decr(tree->left, v, n, f);
    else
        VN_Tree_Decr(tree->right, v, n, f);
}


static void
VN_Tree_Dump(VNTree tree, int depth)
{
    int i;

    if ( ! tree )
        return;

    for ( i = 0 ; i < depth ; i++ )
        printf("  ");
    printf("%d,%d: %d\n", tree->v, tree->n, tree->count);

    VN_Tree_Dump(tree->left, depth + 1);
    VN_Tree_Dump(tree->right, depth + 1);
}


static void
VN_Tree_Free(VNTree tree)
{
    if ( ! tree )
        return;

    VN_Tree_Free(tree->left);
    VN_Tree_Free(tree->right);
    free(tree);
}


static int
VN_Tree_Compare(const void *a, const void *b)
{
    VNTree  x = *(VNTree*)a;
    VNTree  y = *(VNTree*)b;

    if ( x->count > y->count )
        return -1;
    else if ( x->count < y->count )
        return 1;
    return 0;
}


static void
OpenGL_Set_AB(FacePtr face, int v, int *a, int *b, int *c, int *d)
{
    if ( v == face->vertices[0] )
    {
        *a = face->vertices[2];
        *b = face->vertices[1];
        *c = face->normals[2];
        *d = face->normals[1];
    }
    else if ( v == face->vertices[1] )
    {
        *a = face->vertices[0];
        *b = face->vertices[2];
        *c = face->normals[0];
        *d = face->normals[2];
    }
    else
    {
        *a = face->vertices[1];
        *b = face->vertices[0];
        *c = face->normals[1];
        *d = face->normals[0];
    }
}


static void
OpenGL_Export_Fan(FILE *f, WireframePtr wire, VNTree *vert_list, int index)
{
    int max_num_verts = vert_list[index]->count + 1;
    int num_fans = 0;
    int *used = New(int, vert_list[index]->count);
    int *counts = New(int, vert_list[index]->count);
    int **verts = New(int*, vert_list[index]->count);
    int **norms = New(int*, vert_list[index]->count);
    int i, j;
    int temp;
    int num_used;
    int last_normal;

    for ( i = 0 ; i < vert_list[index]->count ; i++ )
    {
        used[i] = 0;
        counts[i] = 1;
        verts[i] = New(int, max_num_verts);
        norms[i] = New(int, max_num_verts);
    }

    /* Need to put the vertices in order for the fan. */
    for ( i = 0 ; i < vert_list[index]->count ; i++ )
        OpenGL_Set_AB(wire->faces + vert_list[index]->faces[i],
                      vert_list[index]->v, verts[i], verts[i] + 1,
                      norms[i], norms[i] + 1);

    num_fans = 0;
    used[0] = 1;
    for ( num_used = 1 ; num_used < vert_list[index]->count ; num_used++ )
    {
        /* Look for the next face in the fan around from the current one. */
        for ( i = 1 ; i < vert_list[index]->count ; i++ )
        {
            if ( used[i] )
                continue;
            if ( verts[num_fans][counts[num_fans]] == verts[i][0] )
            {
                counts[num_fans]++;
                verts[num_fans][counts[num_fans]] = verts[i][1];
                norms[num_fans][counts[num_fans]] = norms[i][1];
                used[i] = 1;
                break;
            }
        }
        if ( i == vert_list[index]->count )
            /* Try to put something in front instead of on the end. */
            for ( i = 1 ; i < vert_list[index]->count ; i++ )
            {
                if ( used[i] )
                    continue;
                if ( verts[num_fans][0] == verts[i][1] )
                {
                    for ( j = counts[num_fans] ; j >= 0 ; j-- )
                    {
                        verts[num_fans][j+1] = verts[num_fans][j];
                        norms[num_fans][j+1] = norms[num_fans][j];
                    }
                    counts[num_fans]++;
                    verts[num_fans][0] = verts[i][0];
                    norms[num_fans][0] = norms[i][0];
                    used[i] = 1;
                    break;
                }
            }
        if ( i == vert_list[index]->count )
        {
            num_fans++;
            /* Find the first unused one and start a new fan. */
            for ( j = num_fans ; used[j] ; j++ );
            /* Swap this pos to the next fan pos. */
            temp = verts[num_fans][0];
            verts[num_fans][0] = verts[j][0];
            verts[j][0] = temp;
            temp = verts[num_fans][1];
            verts[num_fans][1] = verts[j][1];
            verts[j][1] = temp;
            temp = norms[num_fans][0];
            norms[num_fans][0] = norms[j][0];
            norms[j][0] = temp;
            temp = norms[num_fans][1];
            norms[num_fans][1] = norms[j][1];
            norms[j][1] = temp;
            used[j] = used[num_fans];
            used[num_fans] = 1;
        }
    }

    /* Now we have a set of fans. */
    for ( i = 0 ; i <= num_fans ; i++ )
    {
        fprintf(f, "glBegin(GL_TRIANGLE_FAN);\n");
        fprintf(f, "\tglNormal3f(%f, %f, %f);\n",
                wire->normals[vert_list[index]->n].x,
                wire->normals[vert_list[index]->n].y,
                wire->normals[vert_list[index]->n].z);
        last_normal = vert_list[index]->n;
        fprintf(f, "\tglVertex3f(%f, %f, %f);\n",
                wire->vertices[vert_list[index]->v].x,
                wire->vertices[vert_list[index]->v].y,
                wire->vertices[vert_list[index]->v].z);

        for ( j = 0 ; j <= counts[i] ; j++ )
        {
            if ( norms[i][j] != last_normal )
            {
                fprintf(f, "\tglNormal3f(%f, %f, %f);\n",
                        wire->normals[norms[i][j]].x,
                        wire->normals[norms[i][j]].y,
                        wire->normals[norms[i][j]].z);
                last_normal = norms[i][j];
            }
            fprintf(f, "\tglVertex3f(%f, %f, %f);\n",
                    wire->vertices[verts[i][j]].x,
                    wire->vertices[verts[i][j]].y,
                    wire->vertices[verts[i][j]].z);
        }
        fprintf(f, "glEnd();\n\n");
    }
}


static void
OpenGL_Export_Faces(FILE *f, WireframePtr wire, int start, int end)
{
    VNTree  vn_tree = NULL;
    int     i, j, k;
    VNTree  *vert_list;
    int     v_count = 0;
    int     temp;

    for ( i = start ; i < end ; i++ )
        for ( j = 0 ; j < wire->faces[i].num_vertices ; j++ )
            VN_Insert_Entry(&vn_tree, wire->faces[i].vertices[j],
                            wire->faces[i].normals[j], i);
    vert_list = New(VNTree, VN_Tree_Count(vn_tree));
    VN_Tree_To_List(vn_tree, vert_list, &v_count);

    /* For each vertex, export the fan surrounding it. */
    for ( i = 0 ; i < v_count ; i++ )
    {
        qsort(vert_list + i, v_count - i, sizeof(VNTree), VN_Tree_Compare);
        if ( ! vert_list[i]->count )
            break;
        OpenGL_Export_Fan(f, wire, vert_list, i);
        /* Reduce the counts at all the exported vertices. */
        temp = vert_list[i]->count;
        for ( k = vert_list[i]->count - 1 ; k >= 0 ; k-- )
            for ( j = 0 ; j < 3 ; j++ )
                VN_Tree_Decr(vn_tree,
                    wire->faces[vert_list[i]->faces[k]].vertices[j],
                    wire->faces[vert_list[i]->faces[k]].normals[j],
                    vert_list[i]->faces[k]);
    }

    VN_Tree_Free(vn_tree);
    free(vert_list);
}


static int
Face_Compare_Func(const void *a, const void *b)
{
    unsigned long   a_val, b_val;

    if ( ((FacePtr)a)->face_attribs &&
         Attribs_Defined(((FacePtr)a)->face_attribs) )
        a_val = (unsigned long)((FacePtr)a)->face_attribs;
    else
        a_val = 0;

    if ( ((FacePtr)b)->face_attribs &&
         Attribs_Defined(((FacePtr)b)->face_attribs) )
        b_val = (unsigned long)((FacePtr)b)->face_attribs;
    else
        b_val = 0;

    if ( a_val == b_val )
        return 0;
    else if ( a_val < b_val )
        return -1;
    else
        return 1;

    return 1; /* To keep compilers happy. */
}

static void
OpenGL_Export_Wireframe_Main(FILE *f, WireframePtr wire, char *name)
{
    int             i, offset;
    unsigned long   last, this;

    fprintf(f, "/*\n**\tFile generated by Sced version "VERSION"\n");
    fprintf(f, "**\tObject %s\n*/\n\n", name);

    /* Sort the faces on attributes. */
    qsort((void*)wire->faces, wire->num_faces, sizeof(Face), Face_Compare_Func);
    offset = 0;
    last = 0;
    for ( i = 0 ; i < wire->num_faces ; i++ )
    {
        this = ( Attribs_Defined(wire->faces[i].face_attribs) ?
                 (unsigned long)wire->faces[i].face_attribs : 0 );
        if ( this != last )
        {
            /* Export the attributes. This just means a glColor3f call. */
            if ( last && ((AttributePtr)last)->defined )
                fprintf(f, "glColor3f(%f, %f, %f);\n",
                        ((AttributePtr)last)->color.red,
                        ((AttributePtr)last)->color.green,
                        ((AttributePtr)last)->color.blue);
            OpenGL_Export_Faces(f, wire, offset, i);
            offset = i;
            last = this;
        }
    }
    if ( last && ((AttributePtr)last)->defined )
        fprintf(f, "glColor3f(%f, %f, %f);\n",
                ((AttributePtr)last)->color.red,
                ((AttributePtr)last)->color.green,
                ((AttributePtr)last)->color.blue);
    OpenGL_Export_Faces(f, wire, offset, i);
}


void
OpenGL_Export_Base(BaseObjectPtr base, char *name)
{
    FILE            *outfile = NULL;
    char            *def_name;
    char            *new_name;

    if ( ! name )
    {
	def_name = New(char, strlen(base->b_label) + 8);
	strcpy(def_name, base->b_label);
	strcat(def_name, ".c");
	outfile = File_Select_Open(def_name, "OpenGL Export", "w", &new_name);
	free(new_name);
    }
    else
	outfile = fopen(name, "w");
    if ( ! outfile )
        return;

    OpenGL_Export_Wireframe_Main(outfile, base->b_major_wires[0],base->b_label);

    fclose(outfile);
}

