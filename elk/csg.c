/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/elk/RCS/csg.c,v 1.0 1997/05/06 20:43:59 schenney Exp $
 *
 * $Log: csg.c,v $
 * Revision 1.0  1997/05/06 20:43:59  schenney
 * Initial revision
 *
 */
/* csg.c: Elk functions for csg related things.
*/

#include "elk_private.h"
#include <edge_table.h>
#include <X11/Xaw/Tree.h>


int
Elk_CSG_Print(Object w, Object port, int raw, int depth, int len)
{
    Printf(port, "#[csgnode 0x%x]\n", (CSGNodePtr)ELKCSGNODE(w)->csg_node);
    return 0;
}

int
Elk_CSG_Equal(Object a, Object b)
{
    return (ELKCSGNODE(a)->csg_node == ELKCSGNODE(b)->csg_node);
}

int
Elk_CSG_Equiv(Object a, Object b)
{
    return (ELKCSGNODE(a)->csg_node == ELKCSGNODE(b)->csg_node);
}

/*********************************************************************
 *
 * Elk_CSG_node() finds the csg node associated with a given instance.
 *
 * Parameter: obj - the instance to search for.
 *
 * Return: An object containing the csg node.
 *         Void if the node is not present.
 *
 *********************************************************************/
Object
Elk_CSG_Node(Object obj)
{
    Object      res_obj;
    CSGNodePtr  result;

    /* Check that the window exists. */
    if ( ! csg_window.shell )
        return Void;

    Check_Type(obj, T_Object3d);

    /* Look for a solution. */
    if ( ! ( result =
      Find_Instance_In_Displayed((ObjectInstancePtr)ELKOBJECT3(obj)->object3)))
        return Void;

    res_obj = Alloc_Object(sizeof(Elkcsgnode), T_CSGNode, 0);
    ELKCSGNODE(res_obj)->csg_node = result;

    return res_obj;
}


static CSGTreeRoot*
Elk_CSG_Get_Root_Node(CSGNodePtr node)
{
    int         i;

    for ( i = 0 ;
          i < num_displayed_trees &&
          displayed_trees[i].tree->csg_widget != node->csg_widget ;
          i++ );

    if ( i == num_displayed_trees )
        return NULL;

    return displayed_trees + i;

}


Object
Elk_CSG_Display(Object tree)
{
    CSGTreeRoot *root;
    CSGNodePtr  node = ELKCSGNODE(tree)->csg_node;

    /* Check that the window exists. */
    if ( ! csg_window.shell )
        return Void;

    Check_Type(tree, T_CSGNode);

    if ( ! node )
        return Void;

    root = Elk_CSG_Get_Root_Node(node);
    if ( ! root )
        return Void;

    if ( ! root->displayed )
    {
        CSG_Add_Instances_To_Displayed(root->instances);
        root->displayed = TRUE;
    }
    
    return tree;
}


Object
Elk_CSG_Hide(Object tree)
{
    CSGTreeRoot *root;
    CSGNodePtr  node = ELKCSGNODE(tree)->csg_node;
    int         i;

    /* Check that the window exists. */
    if ( ! csg_window.shell )
        return Void;

    Check_Type(tree, T_CSGNode);

    if ( ! node )
        return Void;

    for ( i = 0 ;
          i < num_displayed_trees &&
          displayed_trees[i].tree->csg_widget != node->csg_widget ;
          i++ );
    if ( ! root )
        return Void;

    if ( root->displayed )
    {
        CSG_Remove_Instances_From_Displayed(root->instances);
        root->displayed = FALSE;
    }
    
    return tree;
}

Object
Elk_CSG_Attach(Object left_obj, Object right_obj, Object op_obj)
{
    Object          res_obj;
    CSGTreeRoot     *root;
    CSGOperation    op;
    CSGNodePtr      left_child;
    CSGNodePtr      right_child;

    /* Check that the window exists. */
    if ( ! csg_window.shell )
        return Void;

    Check_Type(left_obj, T_CSGNode);
    Check_Type(right_obj, T_CSGNode);
    Check_Type(op_obj, T_Symbol);

    if ( EQ(op_obj, Sym_Union) )
        op = csg_union_op;
    else if ( EQ(op_obj, Sym_Intersection) )
        op = csg_intersection_op;
    else if ( EQ(op_obj, Sym_Difference) )
        op = csg_difference_op;
    else
        Primitive_Error("Invalid CSG Operation: ~s", op_obj);

    left_child = ELKCSGNODE(left_obj)->csg_node;
    right_child = ELKCSGNODE(right_obj)->csg_node;

    if ( ! left_child || ! right_child )
        return Void;

    /* Just check that the left child is a root node. */
    root = Elk_CSG_Get_Root_Node(left_child);
    if ( ! root )
        return Void;

    /* Right is the root we actually want. */
    root = Elk_CSG_Get_Root_Node(right_child);
    if ( ! root )
        return Void;

    Delete_Displayed_Tree(root->tree);

    /* Do the attach. */
    right_child->csg_parent = NULL;
    CSG_Add_Node(right_child, left_child, op, NULL, 0);

    XawTreeForceLayout(csg_tree_widget);

    changed_scene = TRUE;

    res_obj = Alloc_Object(sizeof(Elkcsgnode), T_CSGNode, 0);
    ELKCSGNODE(res_obj)->csg_node = left_child->csg_parent;

    return res_obj;
}

Object
Elk_CSG_Complete(Object csgobj, Object labelobj)
{
    CSGNodePtr      tree;
    char            *label;
    BaseObjectPtr   new_base;
    WireframePtr    *main_wireframe;
    WireframePtr    *simple_wireframe;

    Check_Type(csgobj, T_CSGNode);
    Check_Type(labelobj, T_String);
    tree = ELKCSGNODE(csgobj)->csg_node;
    label = STRING(labelobj)->data;

    if ( ! ( Elk_CSG_Get_Root_Node ) )
        return Void;

    main_wireframe = New(WireframePtr, 1);
    simple_wireframe = New(WireframePtr, 1);
    main_wireframe[0] = CSG_Generate_Wireframe(tree, 0, FALSE);
    simple_wireframe[0] = Wireframe_Simplify(main_wireframe[0], FALSE);
    Edge_Table_Build(simple_wireframe[0]);

    Delete_Displayed_Tree(tree);

    CSG_Free_Widgets(tree); 
    CSG_Update_Reference_Constraints(tree, tree);
    new_base = Add_CSG_Base_Object(tree, label, 0, main_wireframe,
                                    simple_wireframe, 0, FALSE);
    object_count[csg_obj]++;
    Base_Add_Select_Option(new_base);

    return Make_String(new_base->b_label, strlen(new_base->b_label) + 1);
}


