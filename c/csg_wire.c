/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/csg_wire.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: csg_wire.c,v $
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

#include <config.h>
#include <sced.h>
#include <csg.h>
#include <csg_wire.h>
#include <edge_table.h>
#include <hash.h>
#include <torus.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Label.h>
#include <X11/Shell.h>

typedef struct _CSGStackEntry {
    CSGNodePtr      node;
    int             add_level;
    CSGWireframePtr left_wire;
    CSGWireframePtr right_wire;
    struct _CSGStackEntry   *next;
    } CSGStackEntry, *CSGStackPtr;

static CSGWireframe *CSG_Generate_Incremental_Function();
static CSGWireframe *Create_CSG_Wireframe(CSGNodePtr, int);

static Widget       csg_generate_shell = NULL;
static Widget       label_widget;
static CSGStackPtr  node_stack = NULL;
static Boolean      interupted;
static int          num_processed;

WireframePtr
CSG_Generate_Wireframe(CSGNodePtr src, int add_level, Boolean stagger)
{
    CSGNodePtr      contracted = CSG_Contract_Tree(src);
    WireframePtr    result;
    CSGWireframePtr csg_wireframe;

    if ( stagger )
    {
        node_stack = New(CSGStackEntry, 1);
        node_stack->node = contracted;
        node_stack->left_wire = NULL;
        node_stack->right_wire = NULL;
        node_stack->add_level = add_level;
        node_stack->next = NULL;
        csg_wireframe = CSG_Generate_Incremental_Function();
    }
    else
        csg_wireframe = Create_CSG_Wireframe(contracted, add_level);

    CSG_Delete_Tree(contracted, FALSE);

    if ( ! csg_wireframe )
        return NULL;

    result = CSG_To_Wireframe(csg_wireframe);

    CSG_Destroy_Wireframe(csg_wireframe);

    return result;
}


static void
CSG_Generate_Add_Element(CSGNodePtr node, CSGWireframePtr left,
                         CSGWireframePtr right, int level)
{
    CSGStackPtr new_entry = New(CSGStackEntry, 1);

    new_entry->node = node;
    new_entry->left_wire = left;
    new_entry->right_wire = right;
    new_entry->add_level = level;
    new_entry->next = node_stack;

    node_stack = new_entry;
}

static void
CSG_Generate_Free_Entry()
{
    CSGStackPtr temp;

    if ( node_stack->left_wire )
        CSG_Destroy_Wireframe(node_stack->left_wire);
    if ( node_stack->right_wire )
        CSG_Destroy_Wireframe(node_stack->right_wire);
    temp = node_stack->next;
    free(node_stack);
    node_stack = temp;
}


static void
CSG_Generate_Free_Stack()
{
    while ( node_stack )
        CSG_Generate_Free_Entry();
}


/* This procedure is almost too painful to think about.
** It maintains a stack so that we always know what to do next, even as
** we return to and from the event loop.
** The stack invariant is that the top element is only a leaf node if
** it is the only node in the whole tree.
** Also, the left wire will always be done before the right, and there
** is no element on the stack with both left and right wires done.
**
** The procedure makes one step in the computation, then updates the
** stack to reflect what's been done and returns.
*/
static CSGWireframe*
CSG_Generate_Do_One_Step()
{
    CSGWireframe    *result;
    if ( node_stack->node->csg_op == csg_leaf_op )
    {
        /* Free the node and return the wireframe. */
        result = CSG_Generic_Wireframe(node_stack->node->csg_instance,
                                       node_stack->add_level);
        free(node_stack);
        node_stack = NULL;
        return result;
    }

    if ( ! node_stack->left_wire )
    {
        if ( node_stack->node->csg_left_child->csg_op == csg_leaf_op )
        {
            node_stack->left_wire =
                CSG_Generic_Wireframe(node_stack->node->csg_left_child->
                                      csg_instance, node_stack->add_level);
            num_processed++;
            return NULL;
        }
        CSG_Generate_Add_Element(node_stack->node->csg_left_child,
                                 NULL, NULL, node_stack->add_level);
        /* Recurse. */
        /* We'll stop at the bottom of the tree. */
        return CSG_Generate_Do_One_Step();
    }
    else if ( ! node_stack->right_wire )
    {
        /* We have the left half, now do the right half. */
        if ( node_stack->node->csg_right_child->csg_op == csg_leaf_op )
        {
            node_stack->right_wire =
                CSG_Generic_Wireframe(node_stack->node->csg_right_child->
                                      csg_instance, node_stack->add_level);
            num_processed++;
            return NULL;
        }
        CSG_Generate_Add_Element(node_stack->node->csg_right_child,
                                 NULL, NULL, node_stack->add_level);
        return CSG_Generate_Do_One_Step();
    }
    else
    {
        /* We have both left and right. */
        /* Combine and pop. */
        result = CSG_Combine_Wireframes(node_stack->left_wire,
                                        node_stack->right_wire,
                                        node_stack->node->csg_op);
        num_processed++;
        CSG_Generate_Free_Entry();
        if ( node_stack )
        {
            if ( ! node_stack->left_wire )
                node_stack->left_wire = result;
            else if ( ! node_stack->right_wire )
                node_stack->right_wire = result;
        }
        else
            return result;
    }

    return NULL; /* To keep gcc happy. */
}


static void
CSG_Generate_Interupt_Func(Widget w, XtPointer cl, XtPointer ca)
{
    interupted = TRUE;
}

static void
CSG_Generate_Create_Shell()
{
    Widget  dialog;
    Arg     args[5];
    int     n;

    csg_generate_shell = XtCreatePopupShell("CSG",
                        transientShellWidgetClass, csg_window.shell, NULL, 0);

    /* Create the dialog widget to go inside the shell. */
    n = 0;
    XtSetArg(args[n], XtNlabel, "Generating wireframe..."); n++;
    dialog = XtCreateManagedWidget("csgGenDialog", dialogWidgetClass,
                        csg_generate_shell, args, n);

    /* Add the button at the bottom of the dialog. */
    XawDialogAddButton(dialog, "Cancel", CSG_Generate_Interupt_Func, NULL);

    label_widget = XtNameToWidget(dialog, "label");
    XtVaSetValues(label_widget, XtNborderWidth, 0, NULL);

    XtRealizeWidget(csg_generate_shell);
}

static CSGWireframe*
CSG_Generate_Incremental_Function()
{
    XtAppContext    context;
    XEvent          event;
    CSGWireframe    *this_result;
    char	    str[64];

    context = XtWidgetToApplicationContext(main_window.shell);

    if ( ! csg_generate_shell )
        CSG_Generate_Create_Shell();

    num_processed = 0;
    sprintf(str, "Completed Node: %0d", num_processed);
    XtVaSetValues(label_widget, XtNlabel, str, NULL);

    Position_Shell(csg_generate_shell, FALSE);
    XtPopup(csg_generate_shell, XtGrabExclusive);

    /* Make sure the window is displayed. */
    while ( XPending(XtDisplay(csg_generate_shell)) )
    {
        XtAppNextEvent(context, &event);
        XtDispatchEvent(&event);
    }

    interupted = FALSE;
    while ( ! interupted )
    {
        /* Do some work. */
        this_result = CSG_Generate_Do_One_Step();

	sprintf(str, "Completed Node: %0d", num_processed);
	XtVaSetValues(label_widget, XtNlabel, str, NULL);

        while ( XPending(XtDisplay(csg_generate_shell)) )
        {
            XtAppNextEvent(context, &event);
            XtDispatchEvent(&event);
        }

        if ( ! node_stack )
        {
            /* Pop down the window. */
            XtPopdown(csg_generate_shell);

            return this_result;
        }
    }

    /* We've been interupted. */
    CSG_Generate_Free_Stack();
    XtPopdown(csg_generate_shell);

    return NULL;
}


/*  CSGWireframe*
**  Create_CSG_Wireframe(CSGNodePtr src, int add_level)
**  Initialises all the wireframes at each node of the tree.
**  For internal nodes, sets it to the concatenation of the children.
**  add_level is the additional density level of nodes in the wireframes.
**  It is added to the basic level of each child.
*/
static CSGWireframe*
Create_CSG_Wireframe(CSGNodePtr src, int add_level)
{
    CSGWireframePtr left_wire;
    CSGWireframePtr right_wire;
    CSGWireframePtr result;

    if ( ! src ) return NULL;

    if ( src->csg_op == csg_leaf_op )
        return CSG_Generic_Wireframe(src->csg_instance, add_level);

    left_wire = Create_CSG_Wireframe(src->csg_left_child, add_level);
    right_wire = Create_CSG_Wireframe(src->csg_right_child, add_level);

    result = CSG_Combine_Wireframes(left_wire, right_wire, src->csg_op);

    CSG_Destroy_Wireframe(left_wire);
    CSG_Destroy_Wireframe(right_wire);

    return result;
}


void
CSG_Destroy_Wireframe(CSGWireframePtr victim)
{
    int i;

    /* Free adjacency lists. */
    for ( i = 0 ; i < victim->num_vertices ; i++ )
        if ( victim->vertices[i].num_adjacent )
            free(victim->vertices[i].adjacent);

    /* Free the vertex tree. */
    if ( victim->vertex_tree )
        KD_Free(victim->vertex_tree);
    if ( victim->normal_tree )
        KD_Free(victim->normal_tree);

    /* Free the vertices. */
    free(victim->vertices);
    free(victim->normals);
    /* Free the faces. */
    free(victim->faces);
    /* Free the wireframe. */
    free(victim);
}


/*  void
**  CSG_Bounding_Box(CSGVertexPtr verts, short num, Cuboid *ret)
**  Determines the extent for the given vertices.
*/
void
CSG_Bounding_Box(CSGVertexPtr verts, short num, Cuboid *ret)
{
    int i;

    if ( num == 0 )
    {
        VNew(0, 0, 0, ret->min);
        ret->max = ret->min;
        return;
    }

    VNew(verts[0].location.x,verts[0].location.y,verts[0].location.z, ret->min);
    ret->max = ret->min;
    for ( i = 1 ; i < num ; i++ )
    {
        if ( verts[i].location.x > ret->max.x )
            ret->max.x = verts[i].location.x;
        else if ( verts[i].location.x < ret->min.x )
            ret->min.x = verts[i].location.x;
        if ( verts[i].location.y > ret->max.y )
            ret->max.y = verts[i].location.y;
        else if ( verts[i].location.y < ret->min.y )
            ret->min.y = verts[i].location.y;
        if ( verts[i].location.z > ret->max.z )
            ret->max.z = verts[i].location.z;
        else if ( verts[i].location.z < ret->min.z )
            ret->min.z = verts[i].location.z;
    }
}

/*  void
**  CSG_Face_Bounding_Box(CSGVertexPtr *verts, short num, Cuboid *ret)
**  Determines the extent for the given vertex pointer list.
*/
void
CSG_Face_Bounding_Box(CSGVertexPtr verts, int *indices, short num, Cuboid *ret)
{
    int i;

    VNew(verts[indices[0]].location.x, verts[indices[0]].location.y,
         verts[indices[0]].location.z, ret->min);
    ret->max = ret->min;
    for ( i = 1 ; i < num ; i++ )
    {
        if ( verts[indices[i]].location.x > ret->max.x )
            ret->max.x = verts[indices[i]].location.x;
        else if ( verts[indices[i]].location.x < ret->min.x )
            ret->min.x = verts[indices[i]].location.x;
        if ( verts[indices[i]].location.y > ret->max.y )
            ret->max.y = verts[indices[i]].location.y;
        else if ( verts[indices[i]].location.y < ret->min.y )
            ret->min.y = verts[indices[i]].location.y;
        if ( verts[indices[i]].location.z > ret->max.z )
            ret->max.z = verts[indices[i]].location.z;
        else if ( verts[indices[i]].location.z < ret->min.z )
            ret->min.z = verts[indices[i]].location.z;
    }
}


static void
CSG_Build_Full_Wireframe(CSGNodePtr tree, WireframePtr wire, int add_level,
                         HashTable attrib_hash)
{
    if ( ! tree ) return;

    if ( tree->csg_op == csg_leaf_op )
        Wireframe_Append(wire,
            Dense_Wireframe(tree->csg_instance->o_parent,
		Wireframe_Density_Level(tree->csg_instance) + add_level, FALSE),
		tree->csg_instance, attrib_hash);
    else
    {
        CSG_Build_Full_Wireframe(tree->csg_left_child, wire, add_level,
                                 attrib_hash);
        CSG_Build_Full_Wireframe(tree->csg_right_child, wire, add_level,
                                 attrib_hash);
    }
}


/*  void
**  CSG_Generate_Full_Wireframe(BaseObjectPtr obj)
**  Replaces the object existing wireframe with one that is simply the
**  concatenation of all its component wireframes.
**  add_level is the additional density level for this wireframe.
*/
WireframePtr
CSG_Generate_Full_Wireframe(CSGNodePtr tree, int add_level)
{
    WireframePtr    result;
    HashTable       attrib_hash = Hash_New_Table();

    /* Start with a new, empty wireframe. */
    result = New(Wireframe, 1);

    /* It has no faces, vertices or edges yet. */
    result->num_vertices =
    result->num_real_verts =
    result->num_normals =
    result->num_faces = 0;

    /* Allocate arrays to save realloc problems. */
    result->faces = NULL;
    result->vertices = NULL;
    result->normals = NULL;

    result->num_attribs = 0;
    result->attribs = NULL;

    result->edges = NULL;

    /* Traverse the tree, adding all leaves to the wireframe. */
    CSG_Build_Full_Wireframe(tree, result, add_level, attrib_hash);

    /* Add the center vertex. */
    result->num_vertices = result->num_real_verts + 1;
    result->vertices = More(result->vertices, Vector, result->num_vertices);
    VNew(0, 0, 0, result->vertices[result->num_vertices - 1]);

    Edge_Table_Build(result);

    Hash_Free(attrib_hash);

    return result;
}


