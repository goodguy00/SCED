/*
 * $Header$
 *
 * $Log$
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
**	Sced: A Constraint Based Object Scene Editor
**
**	elk_aggregate.c: Functions for manipulating aggregate objects.
*/

#include "elk_private.h"
#include <edge_table.h>

Object
Elk_Agg_Complete(Object listobj, Object labelobj)
{
    WireframePtr    main_wireframe;
    InstanceList    instances = Elk_List_To_Instances(listobj);
    InstanceList    elmt, victim;
    BaseObjectPtr   new_base;
    char	    *label;

    if ( ! agg_window.shell )
	return Void;

    Check_Type(labelobj, T_String);
    label = STRING(labelobj)->data;

    /* Look through the instances. If they're not in the agg window, remove
    ** them.
    */
    elmt = instances;
    while ( elmt )
        if ( ! Find_Object_In_Instances(elmt->the_instance,
                                        agg_window.all_instances) )
	{
	    victim = elmt;
	    elmt = elmt->next;
	    if ( victim == instances )
		instances = victim->next;
	    Delete_Element(victim);
	    free(victim);
	}
	else
	    elmt = elmt->next;

    if ( ! instances )
	return Void;

    main_wireframe = Agg_Generate_Wireframe(instances, 0, AGG_ALL);
    Edge_Table_Build(main_wireframe);

    elmt = Merge_Selection_Lists(instances, NULL);
    new_base = Add_Agg_Base_Object(elmt, label, main_wireframe, NULL, 0);
    Base_Add_Select_Option(new_base);
    object_count[aggregate_obj]++;

    /* Clear up any constraint references. */
    Agg_Update_List_References(instances, instances);

    /* Remove selected instances from the window. */
    for ( elmt = instances ; elmt ; elmt = elmt->next )
    {
        victim = Find_Object_In_Instances(elmt->the_instance,
                                          agg_window.all_instances);
        if ( agg_window.all_instances == victim )
            agg_window.all_instances = victim->next;
        Delete_Element(victim);

        free(victim);

        /* Check for the victim in the selected lists. */
        if ( ( victim = Find_Object_In_Instances(elmt->the_instance,
				                agg_window.selected_instances)))
	{
	    if ( agg_window.selected_instances == victim )
		agg_window.selected_instances = victim->next;
	    Delete_Element(victim);
	}

        /* Check for the victim in the edit lists. */
        if ((victim = Find_Object_In_Instances(elmt->the_instance,
					       agg_window.edit_instances)))
            Delete_Edit_Instance(&agg_window, victim);
    }

    Free_Selection_List(instances);
    View_Update(&agg_window, agg_window.all_instances, ViewNone);

    return Make_String(new_base->b_label, strlen(new_base->b_label) + 1);
}


