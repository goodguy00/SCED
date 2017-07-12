/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/aggregate.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: aggregate.c,v $
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
**  aggregate.c: Functions for maintaining aggregate objects.
*/

#include <sced.h>
#include <aggregate.h>
#include <base_objects.h>
#include <edge_table.h>
#include <hash.h>
#include <instance_list.h>
#include <layers.h>
#include <SimpleWire.h>


static void Agg_Update_Selected_References(ObjectInstancePtr, InstanceList);

extern Boolean  CSG_Set_Clear_Attribs();

extern char     *top_level_translations;

void
Agg_Window_Popup(Widget w, XtPointer cl, XtPointer ca)
{
    InstanceList        instances = *(InstanceList*)cl;
    InstanceList        elmt;
    InstanceList        victim;
    ObjectInstancePtr   obj;
    Boolean             first_found = ( w ? FALSE : TRUE );
    Boolean             clear_attribs = FALSE;


    if ( ! agg_window.shell )
    {
        Create_Agg_Display();
        XtAugmentTranslations(agg_window.shell,
                              XtParseTranslationTable(top_level_translations));
    }

    XMapRaised(XtDisplay(agg_window.shell), XtWindow(agg_window.shell));

    /* Check for instances. */
    for ( elmt = instances ; elmt ; elmt = elmt->next )
    {
        if ( ! ( victim = Find_Object_In_Instances(elmt->the_instance,
                                                   main_window.all_instances)))
            continue;

        if ( ! first_found )
        {
            first_found = TRUE;
            clear_attribs = CSG_Set_Clear_Attribs();
        }

        if ( main_window.all_instances == victim )
            main_window.all_instances = victim->next;
        Delete_Element(victim);

        obj = victim->the_instance;
        obj->o_flags &= ( ObjAll ^ ObjSelected );
        free(victim);

        if ( clear_attribs )
            obj->o_attribs->defined = FALSE;

        /* Check for the victim in the edit lists. */
        if ( ( victim = Find_Object_In_Instances(obj,
                                            main_window.edit_instances) ) )
            Delete_Edit_Instance(&main_window, victim);

        /* Update dependencies. It's messy in this case. */
        Agg_Update_Selected_References(obj, instances);

        Append_Element(&(agg_window.all_instances), obj);
        if ( Layer_Is_Visible(obj->o_layer) )
            obj->o_flags |= ObjVisible;
    }

    View_Update(&main_window, main_window.all_instances, ViewNone);
    View_Update(&agg_window, agg_window.all_instances, CalcView);
    Update_Projection_Extents(instances);

    Free_Selection_List(instances);
    *(InstanceList*)cl = NULL;

    changed_scene = TRUE;
}


static Boolean
Agg_Update_Selected_References_Test(ObjectInstancePtr obj, void *ptr)
{
    return ( ! Find_Object_In_Instances(obj, (InstanceList)ptr) );
}


/*  void
**  Agg_Update_Selected_References(ObjectInstancePtr obj, InstanceList list)
**  Updates all the reference specs for constraints in obj so that
**  the only ones left appear in list.
*/
static void
Agg_Update_Selected_References(ObjectInstancePtr obj, InstanceList list)
{
    int i;

    Constraint_Change_References(obj, Agg_Update_Selected_References_Test,
                                 (void*)list);

    /* Also need to check that all dependents appear in the list. */
    for ( i = obj->o_num_depend - 1 ; i >= 0 ; i-- )
    {
        if ( Agg_Update_Selected_References_Test(obj->o_dependents[i].obj,
                                                 (void*)list) )
            Constraint_Remove_References(obj->o_dependents[i].obj, obj);
    }
}


void
Agg_Update_List_References(InstanceList list, InstanceList friends)
{
    for ( ; list ; list = list->next )
        Agg_Update_Selected_References(list->the_instance, friends);
}


InstanceList
Aggregate_Contains_Basetype(BaseObjectPtr obj, BaseObjectPtr base, Boolean list)
{
    InstanceList    temp_l;
    InstanceList    found = NULL;

    if ( obj->b_class != aggregate_obj )
        return NULL;

    for ( temp_l = ((AggregatePtr)obj->b_struct)->children ;
          temp_l ;
          temp_l = temp_l->next )
        if ( temp_l->the_instance->o_parent == base )
            if ( list )
                Insert_Element(&found, temp_l->the_instance);
            else
                found = (InstanceList)1;

    return found;
}


InstanceList
Aggregate_Contains_Light(BaseObjectPtr obj, Boolean list)
{
    InstanceList    temp_l;
    InstanceList    found = NULL;

    if ( obj->b_class != aggregate_obj )
        return NULL;

    for ( temp_l = ((AggregatePtr)obj->b_struct)->children ;
          temp_l ;
          temp_l = temp_l->next )
        if ( Obj_Is_Light(temp_l->the_instance) )
            if ( list )
                Insert_Element(&found, temp_l->the_instance);
            else
                found = (InstanceList)1;

    return found;
}


InstanceList
Aggregate_Contains_Spotlight(BaseObjectPtr obj, Boolean list)
{
    InstanceList    temp_l;
    InstanceList    found = NULL;

    if ( obj->b_class != aggregate_obj )
        return NULL;

    for ( temp_l = ((AggregatePtr)obj->b_struct)->children ;
          temp_l ;
          temp_l = temp_l->next )
        if ( Obj_Is_Spot(temp_l->the_instance) )
            if ( list )
                Insert_Element(&found, temp_l->the_instance);
            else
                found = (InstanceList)1;

    return found;
}


InstanceList
Aggregate_Contains_Dirlight(BaseObjectPtr obj, Boolean list)
{
    InstanceList    temp_l;
    InstanceList    found = NULL;

    if ( obj->b_class != aggregate_obj )
        return NULL;

    for ( temp_l = ((AggregatePtr)obj->b_struct)->children ;
          temp_l ;
          temp_l = temp_l->next )
        if ( Obj_Is_Dir(temp_l->the_instance) )
            if ( list )
                Insert_Element(&found, temp_l->the_instance);
            else
                found = (InstanceList)1;

    return found;
}


InstanceList
Aggregate_Contains_Arealight(BaseObjectPtr obj, Boolean list)
{
    InstanceList    temp_l;
    InstanceList    found = NULL;

    if ( obj->b_class != aggregate_obj )
        return NULL;

    for ( temp_l = ((AggregatePtr)obj->b_struct)->children ;
          temp_l ;
          temp_l = temp_l->next )
        if ( Obj_Is_Area(temp_l->the_instance) )
            if ( list )
                Insert_Element(&found, temp_l->the_instance);
            else
                found = (InstanceList)1;

    return found;
}


InstanceList
Aggregate_Contains_Pointlight(BaseObjectPtr obj, Boolean list)
{
    InstanceList    temp_l;
    InstanceList    found = NULL;

    if ( obj->b_class != aggregate_obj )
        return NULL;

    for ( temp_l = ((AggregatePtr)obj->b_struct)->children ;
          temp_l ;
          temp_l = temp_l->next )
        if ( Obj_Is_Point(temp_l->the_instance) )
            if ( list )
                Insert_Element(&found, temp_l->the_instance);
            else
                found = (InstanceList)1;

    return found;
}


WireframePtr
Agg_Generate_Wireframe(InstanceList insts, int add_level, int flags)
{
    InstanceList    inst;
    WireframePtr    result;
    HashTable       attrib_hash = Hash_New_Table();

    if ( flags & AGG_MAJOR )
	flags |= AGG_OBJS;

    /* Start with a new, empty wireframe. */
    result = New(Wireframe, 1);

    /* It has no faces, vertices or edges yet. */
    result->num_vertices =
    result->num_real_verts =
    result->num_normals =
    result->num_faces = 0;

    result->faces = NULL;
    result->vertices = NULL;
    result->normals = NULL;

    result->num_attribs = 0;
    result->attribs = NULL;

    result->edges = NULL;

    /* Traverse the list, adding all objects to the wireframe. */
    for ( inst = insts ; inst ; inst = inst->next )
    {
	if ( Obj_Is_Construction(inst->the_instance) )
	    continue;

	if ( Obj_Is_Light(inst->the_instance) )
	{
	    if ( flags & AGG_OBJS )
		continue;
	}
	else
	{
	    if ( flags & AGG_LIGHTS )
		continue;
	}
        Wireframe_Append(result,
            Dense_Wireframe(inst->the_instance->o_parent,
                    Wireframe_Density_Level(inst->the_instance) + add_level,
		    flags & AGG_MAJOR),
            inst->the_instance, attrib_hash);
    }

    /* Add the center vertex. */
    result->num_vertices = result->num_real_verts + 1;
    result->vertices = More(result->vertices, Vector, result->num_vertices);
    VNew(0, 0, 0, result->vertices[result->num_vertices - 1]);

    Hash_Free(attrib_hash);

    return result;
}


void
Agg_Complete_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    WireframePtr    main_wireframe;
    InstanceList    elmt;
    InstanceList    victim;
    ObjectInstancePtr   obj;
    int             ref_num;
    char            default_name[24];
    char            *label;

    /* Prompt for the name (and give an opportunity to cancel). */
    sprintf(default_name, "Aggregate_%d", object_count[aggregate_obj]);
    label = Complete_Get_Name(default_name);

    if ( ! label )
        return;

    main_wireframe =
	Agg_Generate_Wireframe(agg_window.selected_instances, 0, AGG_ALL);
    Edge_Table_Build(main_wireframe);
    if (( ref_num = Select_Base_Reference(&main_wireframe, NULL, NULL) ) == -1 )
    {
        Wireframe_Destroy(main_wireframe);
        return;
    }

    elmt = Merge_Selection_Lists(agg_window.selected_instances, NULL);
    Base_Add_Select_Option(
	Add_Agg_Base_Object(elmt, label, main_wireframe, NULL, ref_num));

    object_count[aggregate_obj]++;

    /* Clear up any constraint references. */
    Agg_Update_List_References(agg_window.selected_instances,
                               agg_window.selected_instances);

    /* Remove selected instances from the window. */
    for ( elmt = agg_window.selected_instances ; elmt ; elmt = elmt->next )
    {
        victim = Find_Object_In_Instances(elmt->the_instance,
                                          agg_window.all_instances);
        if ( agg_window.all_instances == victim )
            agg_window.all_instances = victim->next;
        Delete_Element(victim);

        obj = victim->the_instance;
        free(victim);

        /* Check for the victim in the edit lists. */
        if ((victim = Find_Object_In_Instances(obj, agg_window.edit_instances)))
            Delete_Edit_Instance(&agg_window, victim);
    }
    Free_Selection_List(agg_window.selected_instances);
    agg_window.selected_instances = NULL;

    View_Update(&agg_window, agg_window.all_instances, ViewNone);
}


void
Agg_Copy_Base_Object(Widget base_widget, BaseObjectPtr base_obj)
{
    /* Copy all the instances from the base and add them to the window. */
    if ( base_widget )
        XtVaGetValues(base_widget, XtNbasePtr, &base_obj, NULL);

    Copy_Object_System(&agg_window,
		       ((AggregatePtr)base_obj->b_struct)->children, FALSE);

    View_Update(&agg_window, agg_window.all_instances, CalcView);
    Update_Projection_Extents(agg_window.all_instances);
}


void
Agg_Reset()
{
    InstanceList    inst, temp_inst;

    Free_Selection_List(agg_window.selected_instances);
    agg_window.selected_instances = NULL;

    for ( inst = agg_window.edit_instances ; inst ; inst = temp_inst )
    {
        temp_inst = inst->next;
        Delete_Edit_Instance(&agg_window, inst);
    }
    agg_window.edit_instances = NULL;

    /* Destroy all the instances first. */
    for ( inst = agg_window.all_instances ; inst != NULL ; inst = temp_inst )
    {
        temp_inst = inst->next;
        Destroy_Instance(Delete_Instance(inst));
    }
    agg_window.all_instances = NULL;

    View_Update(&agg_window, agg_window.all_instances, ViewNone);
}


void
Aggregate_Destroy(AggregatePtr agg)
{
    InstanceList    inst, temp_inst;

    for ( inst = agg->children ; inst != NULL ; inst = temp_inst )
    {
        temp_inst = inst->next;
        Destroy_Instance(Delete_Instance(inst));
    }

    free(agg);
}

