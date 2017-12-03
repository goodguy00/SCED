/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/save.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: save.c,v $
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
**  save.c : Functions needed to save the scene.   It's remarkably complex.
**
**  External Functions:
**
**  void Save_Dialog_Func(Widget, XtPointer, XtPointer);
**  Puts up the save dialog box.
*/

#include <sced.h>
#include <add_constraint.h>
#include <aggregate.h>
#include <attributes.h>
#include <base_objects.h>
#include <csg.h>
#include <instance_list.h>
#include <layers.h>
#include <time.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <X11/Shell.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/AsciiSrc.h>
#include <X11/Xaw/Dialog.h>
#include <View.h>

#if HAVE_STRING_H
#include <string.h>
#elif HAVE_STRINGS_H
#include <strings.h>
#endif

#if RADIANCE_SUPPORT
extern void Radiance_Save_Extras(FILE*);
#endif

static void 	Save_Close(FILE*);
static FILE*    Save_Name_Func(char**);

static int  Save_Header(FILE *);
static int  Save_Viewports(FILE *);
static int  Save_CSG_Tree(FILE*, CSGNodePtr);
static int  Save_Base_Object(FILE*, BaseObjectPtr);
static int  Save_BaseTypes(FILE *);
static int  Save_Instances(FILE *);
static int  Save_Instance_List(FILE *file, InstanceList insts);
static int  Save_Instance(FILE *, ObjectInstancePtr);
static int  Save_Wireframe_Attributes(FILE*, WireframePtr);
static int  Save_Attributes(FILE*, AttributePtr);
static int  Save_Wireframe(FILE*, WireframePtr, Boolean);
static int  Save_Instance(FILE*, ObjectInstancePtr);
static int  Save_Camera(FILE*);
static int  Save_Lights(FILE*);
static int  Save_Features(FILE*, ObjectInstancePtr);
static int  Save_Aggregate(FILE*);
static int  Save_CSG_Trees(FILE*);

extern Viewport *view_list;
extern String   *label_list;
extern int      num_views;

static intptr_t  on_completion;

static XtIntervalId autosave_id = -1;

static Boolean  write_to_pipe = FALSE;

typedef struct _BaseElmt {
    BaseObjectPtr   	object;
    struct _BaseElmt    **dependents;
    int		    	num_dependents;
    int		    	max_num_dependents;
    int		    	dfs_mark;
    } BaseElmt, *BaseElmtPtr;

static void Save_Base_Build_Dependencies(BaseObjectPtr, BaseElmtPtr,
					 BaseElmtPtr, int*);

/* Need a guard to ensure that the autosave does not try to
** save while something else is running. This requires some reasoning
** about when the autosave can be called. The answer to that is: Only
** when the program dispatches an event. Now that is done only under
** controlled curcumstances. A careful check of when Save_Func
** is called reveals that the only time that X events can happen
** mid save sequence is when the dialog box for the name is up. So
** protect that thread, but let any others compete with the guarantee
** that, while we may save twice without doing anything, we won't try
** to save mid some other save operation.
** Event driven programming is so much fun.
*/
static Boolean  save_in_progress = FALSE;


/*  void
**  Save_Dialog_Func(Widget, XtPointer, XtPointer);
**  If necessary, creates, then pops up the save file dialog box.
*/
void
Save_Dialog_Func(Widget w, XtPointer cl_data, XtPointer ca_data)
{
    FILE    *outfile;
    char    path_name[1024];
    char    *new_file_name;

    save_in_progress = TRUE;

    on_completion = (intptr_t)cl_data;

    if ( on_completion != SAVE_NONAME || ! io_file_name )
    {
        if ( ! io_file_name )
        {
            if ( sced_preferences.scene_path )
                strcpy(path_name, sced_preferences.scene_path);
            else
                getcwd(path_name, 1024);
            strcat(path_name, "/*.scn");
        }
        else
            strcpy(path_name, io_file_name);
        if ( ( new_file_name = File_Select(path_name, "Save to:") ) )
        {
            if ( io_file_name )
                free(io_file_name);
            io_file_name = new_file_name;
            outfile = Save_Name_Func(&io_file_name);
            if ( outfile )
                Save_Func(outfile);
        }
    }
    else
    {
        outfile = Save_Name_Func(&io_file_name);
        if ( outfile )
            Save_Func(outfile);
    }

    save_in_progress = FALSE;
}

void
Save_Auto_Callback(XtPointer cl, XtIntervalId *id)
{
    FILE    *outfile;

    if ( ! save_in_progress )
    {
        outfile = Save_Name_Func(&io_file_name);
        if ( outfile )
            Save_Func(outfile);
    }

    autosave_id = XtAppAddTimeOut(app_context,
                                  sced_preferences.autosave_time * 60000,
                                  Save_Auto_Callback, NULL);
}

void
Save_Auto_Cancel()
{
    if ( autosave_id != -1 )
        XtRemoveTimeOut(autosave_id);
    autosave_id = -1;
}

void
Save_Auto_Startup()
{
    if ( ! sced_preferences.autosave_time )
        return;

    autosave_id = XtAppAddTimeOut(app_context,
                                  sced_preferences.autosave_time * 60000,
                                  Save_Auto_Callback, NULL);
}


static void
Save_Base_Build_CSG_Dependencies(CSGNodePtr tree, BaseElmtPtr parent,
				 BaseElmtPtr bases, int *num)
{
    if ( ! tree )
	return;

    if ( tree->csg_op == csg_leaf_op )
    {
	if ( Obj_Is_Derived(tree->csg_instance) )
	    Save_Base_Build_Dependencies(tree->csg_instance->o_parent, parent,
					 bases, num);
    }
    else
    {
	Save_Base_Build_CSG_Dependencies(tree->csg_left_child, parent, bases,
					 num);
	Save_Base_Build_CSG_Dependencies(tree->csg_right_child, parent, bases,
					 num);
    }
}


static void
Save_Base_Build_Dependencies(BaseObjectPtr base, BaseElmtPtr parent,
			     BaseElmtPtr bases, int *num)
{
    int	this_num;

    for ( this_num = 0 ; this_num < *num ; this_num++ )
	if ( bases[this_num].object == base )
	    return;

    bases[*num].object = base;
    bases[*num].dependents = NULL;
    bases[*num].num_dependents = 0;
    bases[*num].max_num_dependents = 0;
    bases[*num].dfs_mark = 0;
    if ( parent )
    {
	if ( parent->max_num_dependents == parent->num_dependents )
	{
	    parent->max_num_dependents += 3;
	    parent->dependents = More(parent->dependents, BaseElmtPtr,
				      parent->max_num_dependents);
	}
	parent->dependents[parent->num_dependents++] = bases + this_num;
    }
    (*num)++;
    if ( base->b_class == csg_obj )
	Save_Base_Build_CSG_Dependencies((CSGNodePtr)base->b_struct,
					 bases + this_num, bases, num);
    else if ( base->b_class == aggregate_obj )
    {
	InstanceList	elmt;
	for ( elmt = ((AggregatePtr)base->b_struct)->children ;
	      elmt ;
	      elmt = elmt->next )
	    if ( Obj_Is_Derived(elmt->the_instance) )
		Save_Base_Build_Dependencies(elmt->the_instance->o_parent,
					     bases + this_num, bases, num);
    }
}


static void
Save_Base_DFS(BaseElmtPtr base, int *mark)
{
    int	i;

    for ( i = 0 ; i < base->num_dependents ; i++ )
	Save_Base_DFS(base->dependents[i], mark);

    base->dfs_mark = (*mark)++;
}


static int
Save_Base_Elmt_Compare(const void *a, const void *b)
{
    BaseElmtPtr	b1 = (BaseElmtPtr)a;
    BaseElmtPtr	b2 = (BaseElmtPtr)b;

    if ( b1->dfs_mark > b2->dfs_mark )
	return 1;
    if ( b1->dfs_mark < b2->dfs_mark )
	return -1;
    return 0;
}

static int
Save_Base_Objects(FILE *outfile, BaseObjectPtr base)
{
    BaseElmtPtr	bases = New(BaseElmt, num_base_objects);
    int		num_bases = 0;
    int		i;
    int		mark = 0;

    /* Save information about non-default base objects. */
    fprintf(outfile, "BaseObjects\n");

    for ( i = 0 ; i < NUM_GENERIC_OBJS ; i++ )
        fprintf(outfile, "%ld\n", (long)(base_objects[i]));

    Save_Base_Build_Dependencies(base, NULL, bases, &num_bases);
    Save_Base_DFS(bases, &mark);
    qsort((void*)bases, num_bases, sizeof(BaseElmt), Save_Base_Elmt_Compare);

    for ( i = 0 ; i < num_bases ; i++ )
	Save_Base_Object(outfile, bases[i].object);

    return fprintf(outfile, "\n");
}

void
Save_Base(BaseObjectPtr base, char *name)
{
    FILE    *outfile;
    char    path_name[1024];
    char    *new_file_name;

    save_in_progress = TRUE;

    if ( sced_preferences.scene_path )
	strcpy(path_name, sced_preferences.scene_path);
    else
	getcwd(path_name, 1024);
    strcat(path_name, "/");
    strcat(path_name, base->b_label);
    strcat(path_name, ".base");

    if ( ( new_file_name = name ) ||
	 ( new_file_name = File_Select(path_name, "Save to:") ) )
    {
	outfile = Save_Name_Func(&new_file_name);
	if ( outfile )
	{
	    if ( Save_Header(outfile) < 0 ||
		 Save_Declarations(outfile) < 0 ||
		 Save_Base_Objects(outfile, base) < 0 )
	    {
		Popup_Error("Write Failed!", main_window.shell, "Error");
		Save_Close(outfile);
		return;
	    }

	    Save_Close(outfile);
	}
    }

    save_in_progress = FALSE;
}


static FILE*
Save_Name_Func(char **name)
{
    char    *extension;
    char    *command_line;
    FILE    *result;

    if ( ! *name )
        return NULL;

    if ( sced_preferences.compress_output )
    {
        /* Make sure the extension is right. */
        extension = strrchr(*name, '.');
        if ( ! extension ||
               ( strcmp(extension, ".gz") &&
                 strcmp(extension, ".z") &&
                 strcmp(extension, ".Z") ) )
        {
            *name = More(*name, char, strlen(*name) + 5);
#if ( HAVE_GZIP )
            strcat(*name, ".gz");
#else
            strcat(*name, ".Z");
#endif
        }

        command_line = New(char, strlen(*name) + 20);

#if ( HAVE_GZIP )
        strcpy(command_line, "gzip -c > ");
#else
        strcpy(command_line, "compress -c > ");
#endif
        strcat(command_line, *name);

        result =  popen(command_line, "w");
        free(command_line);

        write_to_pipe = TRUE;

        return result;
    }
    else
        return ( fopen(*name, "w") );
}


static void
Save_Close(FILE *victim)
{
    if ( write_to_pipe )
        pclose(victim);
    else
        fclose(victim);

    write_to_pipe = FALSE;
}



/*  void
**  Save_Func(FILE *outfile)
**  Initiates the saving of a file from a command button.
*/
void    
Save_Func(FILE *outfile)
{
    if ( Save_Header(outfile) < 0 ||
         Save_Viewports(outfile) < 0 ||
         Save_Lights(outfile) < 0 ||
         Save_Camera(outfile) < 0 ||
         Save_Declarations(outfile) < 0 ||
         Save_BaseTypes(outfile) < 0 ||
         Save_Instances(outfile) < 0 ||
         Save_Aggregate(outfile) < 0 ||
         Save_CSG_Trees(outfile) < 0 )
    {
        Popup_Error("Write Failed!", main_window.shell, "Error");
        Save_Close(outfile);
        return;
    }

    Save_Close(outfile);

    if ( on_completion == SAVE_QUIT )
        Sced_Exit(0);

    changed_scene = FALSE;

    if ( on_completion == SAVE_LOAD )
        Load_Dialog_Func(NULL, NULL, NULL);

    if ( on_completion == SAVE_RESET )
        Reset_Dialog_Func(NULL, NULL, NULL);

    if ( on_completion == SAVE_CLEAR )
        Clear_Dialog_Func(NULL, NULL, NULL);

    on_completion = SAVE_ONLY;

}

/*  int
**  Save_Header(FILE *file)
**  Writes an appropriate header to file.
*/
static int
Save_Header(FILE *file)
{
    time_t  current_time;

    time(&current_time);

    fprintf(file, "# Sced Scene Description file\n# %s\n",ctime(&current_time));
    fprintf(file, "Internal\n");

    return ( fprintf(file, "Version "VERSION"."SUB_VERSION"\n") );
}


static int
Save_Viewport(FILE *file, ViewportPtr vp, int width, int height, int mag)
{
    fprintf(file, "LookFrom "); VPrint(file, vp->view_from);
    fprintf(file, "LookAt ");   VPrint(file, vp->view_at);
    fprintf(file, "LookUp ");   VPrint(file,vp->view_up);
    fprintf(file, "ViewDist %1.15g\n", vp->view_distance);
    fprintf(file, "EyeDist %1.15g\n", vp->eye_distance);
    fprintf(file, "Magnify %d\n", mag);
    fprintf(file, "Mode %d\n", vp->draw_mode);
    return fprintf(file, "Screen %d %d\n", width, height);
}


/*  int
**  Save_Viewports(FILE *file)
**  Saves the viewport information to file.
*/
static int
Save_Viewports(FILE *file)
{
    Dimension   width, height;
    int         i;

    /* First the main_viewport information. */
    XtVaGetValues(main_window.view_widget,
                XtNdesiredWidth, &width,
                XtNdesiredHeight, &height, NULL);
    fprintf(file, "MainViewport\n");
    Save_Viewport(file, &(main_window.viewport), (int)width, (int)height,
                  main_window.magnify);
    fprintf(file, "Axes %ld\n", (long)&(main_window.axes));
    fprintf(file, "\n");

    /* Then the CSG Viewport. */
    if ( csg_window.view_widget )
    {
        XtVaGetValues(csg_window.view_widget,
                    XtNdesiredWidth, &width,
                    XtNdesiredHeight, &height, NULL);
        fprintf(file, "CSGViewport\n");
        Save_Viewport(file, &(csg_window.viewport), (int)width, (int)height,
                      csg_window.magnify);
        fprintf(file, "Axes %ld\n", (long)&(csg_window.axes));
    }

    if ( agg_window.view_widget )
    {
        XtVaGetValues(agg_window.view_widget,
                    XtNdesiredWidth, &width,
                    XtNdesiredHeight, &height, NULL);
        fprintf(file, "AggViewport\n");
        Save_Viewport(file, &(agg_window.viewport), (int)width, (int)height,
                      agg_window.magnify);
        fprintf(file, "Axes %ld\n", (long)&(agg_window.axes));
    }

    /* Then any saved non defaults. */
    for ( i = 0 ; i < num_views ; i++ )
    {
        if ( view_list[i].is_default ) continue;
        fprintf(file, "Viewport \"%s\"\n", label_list[i + 1]);
        Save_Viewport(file, view_list + i, (int)view_list[i].scr_width,
                      (int)view_list[i].scr_height, view_list[i].magnify);
    }

    Save_Layers(file);

    return fprintf(file, "\n");
}



/*  int
**  Save_Camera(FILE *outfile)
**  Saves the camera specifications.  It's full of special cases.
*/
static int
Save_Camera(FILE *outfile)
{
    int i;

    fprintf(outfile, "Camera %d\n",
            ( camera_object.o_flags & ObjVisible ) ? 1 : 0 );

    fprintf(outfile, "%ld\n", (long)&camera_object);

    fprintf(outfile, "Transformation\n");
    MPrint(outfile, camera_object.o_transform.matrix);
    VPrint(outfile, camera_object.o_transform.displacement);

    Save_Features(outfile, &camera_object);

    fprintf(outfile, "Dependents %d", camera_object.o_num_depend);
    for ( i = 0 ; i < camera_object.o_num_depend ; i++ )
        fprintf(outfile, " %ld %d", (long)(camera_object.o_dependents[i].obj),
                                    (int)(camera_object.o_dependents[i].count));

    fprintf(outfile, "\n");

    switch ( target_renderer )
    {
        case NoTarget:
            fprintf(outfile, "None\n");
            break;

        case Rayshade:
            fprintf(outfile, "Rayshade\n");
            break;

        case POVray:
            fprintf(outfile, "POVray\n");
            break;

        case Radiance:
            fprintf(outfile, "Radiance ");
#if RADIANCE_SUPPORT
            Radiance_Save_Extras(outfile);
#else
            fprintf(outfile, "\n");
#endif
            break;

        case Renderman:
            fprintf(outfile, "Renderman\n");
            break;

        case VRML:
            fprintf(outfile, "VRML\n");
            break;

        case LastTarget:
            break;
    }

    if ( ! camera.defined )
        fprintf(outfile, "Default\n");
    fprintf(outfile, "LookFrom ");  VPrint(outfile, camera.location);
    fprintf(outfile, "LookAt ");    VPrint(outfile, camera.look_at);
    fprintf(outfile, "LookUp ");    VPrint(outfile, camera.look_up);
    fprintf(outfile, "EyeDist %1.15g\n", camera.eye_dist);
    fprintf(outfile, "HFOV %1.15g\n", camera.horiz_fov);
    fprintf(outfile, "VFOV %1.15g\n", camera.vert_fov);

    fprintf(outfile, "Screen %d %d\n", (int)camera.scr_width,
            (int)camera.scr_height);

    return fprintf(outfile, "\n");
}

static int
Save_Base_Object(FILE *file, BaseObjectPtr base)
{
    int j;

    fprintf(file, "\"%s\"\n", base->b_label);
    if ( base->b_class == csg_obj )
	fprintf(file, "CSG\n");
    else if ( base->b_class == wireframe_obj )
	fprintf(file, "Wireframe\n");
    else if ( base->b_class == aggregate_obj )
	fprintf(file, "Aggregate\n");
    fprintf(file, "%ld\n", (long)(base));
    fprintf(file, "Reference %d\n", base->b_ref_num);
    if ( base->b_class == csg_obj )
    {
	Save_CSG_Tree(file, (CSGNodePtr)base->b_struct);
	fprintf(file, "%d\n", base->b_max_density);
	for ( j = 0 ; j < base->b_max_density + 1 ; j++ )
	    Save_Wireframe(file, base->b_major_wires[j],
			   base->b_use_full);

	if ( sced_preferences.save_simple_wires &&
	     ! base->b_use_full )
	    for ( j = 0 ; j < base->b_max_density + 1 ; j++ )
		Save_Wireframe(file, base->b_wireframes[j],
			       FALSE);
    }
    else if ( base->b_class == wireframe_obj )
    {
	fprintf(file, "%d\n", base->b_max_density);
	Save_Wireframe(file, base->b_major_wires[0], FALSE);
	if ( sced_preferences.save_simple_wires )
	    Save_Wireframe(file, base->b_wireframes[0], FALSE);
    }
    else if ( base->b_class == aggregate_obj )
    {
	fprintf(file, "%d\n",
	    Instance_List_Length(
		((AggregatePtr)base->b_struct)->children));
	Save_Instance_List(file,
		((AggregatePtr)base->b_struct)->children);
    }

    return fprintf(file, "\n");
}


/*  int
**  Save_BaseTypes(FILE *file)
**  Saves information about base types to file.
*/
static int
Save_BaseTypes(FILE *file)
{
    int	i;

    /* Save information about non-default base objects. */
    fprintf(file, "BaseObjects\n");

    for ( i = 0 ; i < NUM_GENERIC_OBJS ; i++ )
        fprintf(file, "%ld\n", (long)(base_objects[i]));

    for ( ; i < num_base_objects ; i++ )
	Save_Base_Object(file, base_objects[i]);

    return fprintf(file, "\n");
}


/*  int
**  Save_CSG_Tree(FILE *file, CSGNodePtr tree)
**  Saves the tree to file.
*/
static int
Save_CSG_Tree(FILE *file, CSGNodePtr tree)
{
    if ( ! tree ) return 1;

    if ( tree->csg_op == csg_leaf_op )
        Save_Instance(file, tree->csg_instance);
    else
    {
        switch ( tree->csg_op )
        {
            case csg_union_op:
                fprintf(file, "Union\n");
                break;
            case csg_intersection_op:
                fprintf(file, "Intersection\n");
                break;
            case csg_difference_op:
                fprintf(file, "Difference\n");
                break;
            default:;
        }
        Save_CSG_Tree(file, tree->csg_left_child);
        Save_CSG_Tree(file, tree->csg_right_child);
    }

    return 1;
}


static int
Save_Instance_List(FILE *file, InstanceList insts)
{
    for ( ; insts ; insts = insts->next )
        Save_Instance(file, insts->the_instance);

    return fprintf(file, "\n");
}


/*  int
**  Save_Instances(FILE *file)
**  Saves the instance list to file.
*/
static int
Save_Instances(FILE *file)
{
    fprintf(file, "Instances\n");

    return Save_Instance_List(file, main_window.all_instances);
}

static int
Save_Instance(FILE *file, ObjectInstancePtr inst)
{
    int i;

    fprintf(file, "\"%s\"\n", inst->o_label);
    fprintf(file, "%ld\n", (long)inst);
    fprintf(file, "%ld\n", (long)(inst->o_parent));
    fprintf(file, "Dense %d\n", Wireframe_Density_Level(inst));

    if ( Obj_Is_Torus(inst) )
        fprintf(file, "%1.15g\n", ((TorusPtr)inst->o_hook)->major_radius);
    else if ( Obj_Is_Control(inst) )
    {
        fprintf(file, "%d\n", control_part(inst)->num_control_verts);
        for ( i = 0 ; i < control_part(inst)->num_control_verts ; i++ )
            VPrint(file, control_part(inst)->control_verts[i]);
    }

    fprintf(file, "Transformation\n");
    MPrint(file, inst->o_transform.matrix);
    VPrint(file, inst->o_transform.displacement);

    Save_Attributes(file, inst->o_attribs);

    fprintf(file, "Alias ");
    if ( inst->o_aliases )
    {
        fprintf(file, "1 ");
        for ( i = NoTarget ; i < LastTarget ; i++ )
            if ( inst->o_aliases[i] )
                Save_String(file, (char*)inst->o_aliases[i]);
            else
                Save_String(file, "");
        fprintf(file, "\n");
    }
    else
        fprintf(file, "0\n");

    fprintf(file, "LOD ");
    if ( inst->o_lods )
    {
        fprintf(file, "%d ", inst->o_lods->num_lods);
        for ( i = 0 ; i < inst->o_lods->num_lods ; i++ )
            fprintf(file, "%g ", inst->o_lods->lods[i]);
        fprintf(file, "\n");
    }
    else
        fprintf(file, "0\n");

    fprintf(file, "Layer %d\n", inst->o_layer);

    Save_Features(file, inst);

    fprintf(file, "Dependents %d", inst->o_num_depend);
    for ( i = 0 ; i < inst->o_num_depend ; i++ )
        fprintf(file, " %ld %d", (long)(inst->o_dependents[i].obj),
                                 (int)(inst->o_dependents[i].count));
    fprintf(file, "\n");

    return fprintf(file, "\n");
}



/*  int
**  Save_Wireframe(FILE *file, WireframePtr w, Boolean full)
**  Saves information about wireframe w to file.
*/
static int
Save_Wireframe(FILE *file, WireframePtr w, Boolean full)
{
    int     i, j;

#define WVPrint(v) fprintf(file, "%1.15g %1.15g %1.15g\n", (v).x, (v).y, (v).z);

    if ( full )
        fprintf(file, "Wireframe Full\n");
    else
        fprintf(file, "Wireframe\n");

    Save_Wireframe_Attributes(file, w);

    fprintf(file, "%d\n", w->num_real_verts);
    for ( i = 0 ; i < w->num_real_verts ; i++ )
        WVPrint(w->vertices[i]);

    fprintf(file, "%d\n", w->num_normals);
    for ( i = 0 ; i < w->num_normals ; i++ )
        WVPrint(w->normals[i]);

    fprintf(file, "%d\n", w->num_faces);
    for ( i = 0 ; i < w->num_faces ; i++ )
    {
        fprintf(file, "%d ", w->faces[i].num_vertices);
        for ( j = 0 ; j < w->faces[i].num_vertices ; j++ )
            fprintf(file, "%d %d ", w->faces[i].vertices[j],
                    w->faces[i].normals[j]);
        fprintf(file, "%ld ", (long)(w->faces[i].face_attribs));
        WVPrint(w->faces[i].normal);
    }

    return fprintf(file, "\n");
}




static int
Save_Wireframe_Attributes(FILE *file, WireframePtr wire)
{
    int i;

    fprintf(file, "%d\n", wire->num_attribs);
    for ( i = 0 ; i < wire->num_attribs ; i++ )
        Save_Attributes(file, wire->attribs[i]);

    return 1;
}



/*  int
**  Save_Attributes(FILE *file, AttributePtr a)
**  Saves the attributes from a.
*/
static int
Save_Attributes(FILE *file, AttributePtr a)
{
    Renderer    rend;

    fprintf(file, "Attributes %ld %d\n", (long)a, a->defined ? 1 : 0);

    fprintf(file, "%g %g %g ", a->color.red, a->color.green,
                a->color.blue);
    fprintf(file, "%g ", a->diff_coef);
    fprintf(file, "%g %g ", a->spec_coef, a->spec_power);
    fprintf(file, "%g ", a->reflect_coef);
    fprintf(file, "%g ", a->transparency);
    fprintf(file, "%g ", a->refract_index);
    fprintf(file, "%d ", ( a->use_extension ? 1 : 0 ));
    for ( rend = NoTarget ; rend < LastTarget ; rend++ )
        if ( a->extension[rend] )
            Save_String(file, a->extension[rend]);
        else
            Save_String(file, "");
    fprintf(file, "\n");
    fprintf(file, "%d ", ( a->use_obj_trans ? 1 : 0 ));
    fprintf(file, "%d ", ( a->open ? 1 : 0 ));
    fprintf(file, "%g %g %g ", a->intensity.red, a->intensity.green,
                a->intensity.blue);
    fprintf(file, "%g ", a->radius);
    fprintf(file, "%g ", a->tightness);
    fprintf(file, "%d ", a->samples);
    fprintf(file, "%d ", ( a->jitter ? 1 : 0 ));
    fprintf(file, "%d", ( a->invert ? 1 : 0 ));

    return fprintf(file, "\n");
}



/*  int
**  Save_Lights(FILE *file)
**  Saves information for the defined lights.
*/
static int
Save_Lights(FILE *file)
{
    fprintf(file, "Ambient %g %g %g\n", ambient_light.red,
            ambient_light.green, ambient_light.blue);

    return fprintf(file, "\n");
}



static void
Save_Constraint_Specs(FILE *outfile, ConstraintSpec *spec)
{
    fprintf(outfile, "%d ", (int)spec->spec_type);
    if ( spec->spec_type == reference_spec ||
         spec->spec_type == vertex_spec ||
         spec->spec_type == parameter_spec ||
         spec->spec_type == feature_spec )
        fprintf(outfile, "%ld ", (long)spec->spec_data);
    VPrint(outfile, spec->spec_vector);
}


static void
Save_Constraint(FILE *outfile, ConstraintPtr feat)
{
    int i;

    /* Export type, label and status. */
    fprintf(outfile, "%d %d \"%s\" %d %d ",
            (int)feat->c_type, (int)feat->c_spec_flags, feat->c_label,
            (int)feat->c_status, (int)feat->c_forced);

    /* Export the defining values. */
    switch ( feat->c_type )
    {
        case plane_feature:
            VPrint(outfile, feat->c_vector);
            VPrint(outfile, feat->c_point);
            if ( feat->c_spec_flags & RatioPlane )
                fprintf(outfile, "%1.15g %1.15g\n",
                        feat->c_radius, feat->c_ratio);
            break;
        case line_feature:
            VPrint(outfile, feat->c_vector);
            VPrint(outfile, feat->c_point);
            break;
        case sphere_feature:
            VPrint(outfile, feat->c_point);
            fprintf(outfile, "%1.15g\n", feat->c_radius);
            break;
        case circle_feature:
            VPrint(outfile, feat->c_vector);
            VPrint(outfile, feat->c_point); 
            fprintf(outfile, "%1.15g\n", feat->c_radius);
            break;
        case point_feature:
            VPrint(outfile, feat->c_point);
            if ( feat->c_spec_flags & RatioPoint )
                fprintf(outfile, "%1.15g %1.15g\n",
                        feat->c_radius, feat->c_ratio);
            break;
        case ratio_point_feature:
        case ratio_plane_feature:
        case axis_feature:
        case null_feature:
        case inconsistent_feature:
            break;
    }

    /* Export the specs. */
    fprintf(outfile, "%d\n", feat->c_num_specs);
    for ( i = 0 ; i < feat->c_num_specs ; i++ )
        Save_Constraint_Specs(outfile, feat->c_specs + i);
}

/*  int
**  Save_Features(FILE *outfile, ObjectInstancePtr inst)
**  Saves a list of features and constraints.
*/
static int
Save_Features(FILE *outfile, ObjectInstancePtr inst)
{
    int i, j;

    fprintf(outfile, "Features %d\n", inst->o_num_features);
    for ( i = 0 ; i < inst->o_num_features ; i++ )
    {
        fprintf(outfile, "%d ", Feature_Base_Offset(inst->o_features[i].base));
        VPrint(outfile, inst->o_features[i].location);
        fprintf(outfile, "%d\n", inst->o_features[i].num_constraints);
        for ( j = 0 ; j < inst->o_features[i].num_constraints ; j++ )
            Save_Constraint(outfile, inst->o_features[i].constraints + j);
        fprintf(outfile, "%d\n\n", inst->o_features[i].flags);
    }

    return fprintf(outfile, "\n");
}


static int
Save_Aggregate(FILE *outfile)
{
    fprintf(outfile, "Aggregate %d\n",
            Instance_List_Length(agg_window.all_instances));
    return Save_Instance_List(outfile, agg_window.all_instances);
}


static int
Save_CSG_Trees(FILE *outfile)
{
    int i;

    fprintf(outfile, "CSG\n");
    fprintf(outfile, "%d\n", num_displayed_trees);

    for ( i = 0 ; i < num_displayed_trees ; i++ )
    {
        Save_CSG_Tree(outfile, displayed_trees[i].tree);
        fprintf(outfile, "%d\n\n", displayed_trees[i].displayed ? 1 : 0 );
    }

    return fprintf(outfile, "\n");
}


int
Save_String(FILE *outfile, char *string)
{
    int length = strlen(string);
    int i;

    fprintf(outfile, "\"");
    for ( i = 0 ; i < length ; i++ )
        if ( string[i] == '\"' )
            fprintf(outfile, "\\\"");
        else
            fprintf(outfile, "%c", string[i]);
    return fprintf(outfile, "\"");
}


