/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/kd_tree.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: kd_tree.c,v $
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
**  kd_tree.c: Functions for manipulating kd_trees (or binary trees for that
**              matter).
*/

#include <sced.h>
#include <kd_tree.h>

void*
KD_Add_Elmt(KDTree *tree, void *value, void *data, KDCompareFunc func)
{
    KDTree  parent;
    KDTree  current;
    int     result;
    int     depth = 0;

    if ( ! ( *tree) )
    {
        *tree = New(KDTreeElmt, 1);
        (*tree)->value = value;
        (*tree)->left = (*tree)->right = NULL;
        return value;
    }

    current = *tree;
    while ( current )
    {
        result = func(current->value, value, data, depth);

        if ( ! result )
            return current->value;

        parent = current;

        if ( result < 0 )
            current = current->left;
        else
            current = current->right;

        depth++;
    }

    /* Must not be there. */
    if ( result < 0 )
    {
        parent->left = New(KDTreeElmt, 1);
        parent->left->value = value;
        parent->left->left = parent->left->right = NULL;
    }
    else
    {
        parent->right = New(KDTreeElmt, 1);
        parent->right->value = value;
        parent->right->left = parent->right->right = NULL;
    }

    return value;
}


void
KD_Free(KDTree victim)
{
    if ( ! victim )
        return;

    if ( victim->left )
        KD_Free(victim->left);
    if ( victim->right )
        KD_Free(victim->right);
    free(victim);
}


void
KD_Print_Tree(KDTree tree, int depth)
{
    if ( ! tree )
        return;

    printf("%d : %ld\n", depth, (long)tree->value);
    KD_Print_Tree(tree->left, depth+1);
    KD_Print_Tree(tree->right, depth+1);
}

