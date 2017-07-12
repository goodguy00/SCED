/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/load_internal.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: load_internal.c,v $
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
**  load_internal.c : Code needed to load a scene description file.
**
**  External Functions:
**
**  void Load_Dialog_Func(Widget, XtPointer, XtPointer);
**  Puts up the load dialog box.
**
**  void Load_Internal_File(FILE*, Boolean)
**  Loads the contents of a file.
**
*/

#include <sced.h>
#include <add_constraint.h>
#include <aggregate.h>
#include <attributes.h>
#include <base_objects.h>
#include <constraint.h>
#include <csg.h>
#include <csg_wire.h>
#include <edge_table.h>
#include <edit_types.h>
#include <gen_wireframe.h>
#include <hash.h>
#include <instance_list.h>
#include <layers.h>
#include <load.h>
#include <update.h>
#include <View.h>

#define Input_Error \
    { \
        fprintf(stderr, "Malformed input file line %d\n", line_num); \
        Sced_Exit(1); \
    }

#define Load_Float(f) \
    { \
        if ((token = yylex()) == INT_TOKEN) \
            f = (float)lex_int; \
        else if (token == FLOAT_TOKEN) \
            f = (float)lex_float; \
        else \
            Input_Error; \
    }
#define Load_Double(f) \
    { \
        if ((token = yylex()) == INT_TOKEN) \
            f = (double)lex_int; \
        else if (token == FLOAT_TOKEN) \
            f = lex_float; \
        else \
            Input_Error; \
    }

#define Load_Vector(v) \
    { Load_Double((v).x); Load_Double((v).y); Load_Double((v).z); }
#define Load_Vector_f(v) \
    { Load_Float((v).x); Load_Float((v).y); Load_Float((v).z); }

#if RADIANCE_SUPPORT
extern void     Radiance_Set_Extras(int, char*, char*, int, int, int, char*);
#endif

#if PREV_SUPPORT
extern void Load_Old_Format(FILE*, int);
#endif

static int  Load_Handle_Options();
static int  Load_Declarations(Boolean);
static int  Load_Includes();
static int  Load_Layers();
static int  Load_Basetypes();
static int  Load_Instance_List(InstanceList*, int);
static int  Load_Instances();
static int  Load_Attributes(AttributePtr, AttributePtr);
static int  Load_Aliases(void***);
static int  Load_LOD(LODInfoPtr*);
static int  Load_Camera(Camera *, Boolean, Boolean*);
static int  Load_Lights(Boolean);
static int  Load_Constraint(ConstraintPtr);
static int  Load_Constraints(ConstraintPtr*, int*);
static void Load_Features(ObjectInstancePtr);
static int  Load_CSG_Trees();
static int  Load_Aggregate_Insts();
static int  Load_Wireframe(WireframePtr*, Boolean*);
static int  Load_Wireframe_Attributes(AttributePtr**, int);

static ObjectInstancePtr    Load_Instance(char*);
static CSGNodePtr           Load_CSG_Tree(CSGNodePtr);

static void Refresh_Instance_Pointers(Boolean);

static HashTable    load_hash;
static InstanceList new_instances = NULL;

static int          layer_offset;

/*  void
**  Load_Internal_File(FILE *file, int merge)
**  Loads the information contained in file.
*/
void
Load_Internal_File(FILE *file, int merge)
{
    Camera  dummy_cam;
    Boolean new_camera;
    int     token;

    if ( ( token = yylex() ) == VERS_TOKEN )
    {
        Load_Double(version);
        if ( version < 1.0 /* VERSION_FLOAT - 0.0001 */ )
        {
#if PREV_SUPPORT
            Load_Old_Format(file, merge);
#else
            fprintf(stderr,
                    "Sced: Support for old format files is not compiled in.\n");
#endif
            version = VERSION_FLOAT;
            return;
        }
        token = yylex();
    }

    load_hash = Hash_New_Table();

    while ( token != EOF_TOKEN && token != END_TOKEN )
    {
        switch (token)
        {
            case OPTIONS_TOKEN:
                token = Load_Handle_Options();
                break;

            case MAINVIEW_TOKEN:
                if ( merge )
                    token = Load_View(NULL, NULL);
                else
                {
                    token = Load_View(&(main_window.viewport), NULL);
                    if ( main_window.view_widget &&
                         main_window.viewport.scr_width )
                        XtVaSetValues(main_window.view_widget,
                            XtNwidth, main_window.viewport.scr_width,
                            XtNdesiredWidth, main_window.viewport.scr_width,
                            XtNheight, main_window.viewport.scr_height,
                            XtNdesiredHeight, main_window.viewport.scr_height,
                            NULL);
                    main_window.magnify = main_window.viewport.magnify;
                }
                break;

            case CSGVIEW_TOKEN:
                if ( merge )
                    token = Load_View(NULL, NULL);
                else
                {
                    token = Load_View(&(csg_window.viewport), NULL);
                    if ( csg_window.view_widget &&
                         csg_window.viewport.scr_width )
                        XtVaSetValues(csg_window.view_widget,
                            XtNwidth, csg_window.viewport.scr_width,
                            XtNdesiredWidth, csg_window.viewport.scr_width,
                            XtNheight, csg_window.viewport.scr_height,
                            XtNdesiredHeight, csg_window.viewport.scr_height,
                            NULL);
                    csg_window.magnify = csg_window.viewport.magnify;
                }
                break;

            case AGGVIEW_TOKEN:
                if ( merge )
                    token = Load_View(NULL, NULL);
                else
                {
                    token = Load_View(&(agg_window.viewport), NULL);
                    if ( agg_window.view_widget &&
                         agg_window.viewport.scr_width )
                        XtVaSetValues(agg_window.view_widget,
                            XtNwidth, agg_window.viewport.scr_width,
                            XtNdesiredWidth, agg_window.viewport.scr_width,
                            XtNheight, agg_window.viewport.scr_height,
                            XtNdesiredHeight, agg_window.viewport.scr_height,
                            NULL);
                    agg_window.magnify = agg_window.viewport.magnify;
                }
                break;

            case VIEWPORT_TOKEN:
                if ( ( token = yylex() ) != STRING_TOKEN )
                    break;
                if ( merge )
                    token = Load_View(NULL, NULL);
                else
                    token = Load_View(NULL, lex_string);
                break;

            case LAYER_TOKEN:
                if ( merge )
                    layer_offset = Layer_Get_Num() - 1;
                else
                    layer_offset = 0;
                token = Load_Layers();
                break;

            case CAMERA_TOKEN:
                if ( merge )
                    token = Load_Camera(&dummy_cam, FALSE, &new_camera);
                else
                    token = Load_Camera(&camera, TRUE, &new_camera);
                break;

            case INCLUDES_TOKEN:
                token = Load_Includes();
                break;

            case DECLARE_TOKEN:
                token = Load_Declarations(merge);
                break;

            case BASEOBJECTS_TOKEN:
                token = Load_Basetypes();
                break;

            case INSTANCES_TOKEN:
                token = Load_Instances();
                break;

            case CSG_TOKEN:
                token = Load_CSG_Trees();
                break;

            case AGG_TOKEN:
                token = Load_Aggregate_Insts();
                break;

            case AMBIENT_TOKEN:
                token = Load_Lights(!merge);
                break;

            default:
                if ( merge )
                    fprintf(stderr, "Error token %d in file %s line %d\n",
                            token, merge_filename, line_num);
                else
                    fprintf(stderr, "Error token %d in file %s line %d\n",
                            token, io_file_name, line_num);
                token = yylex();
        }
    }

    Refresh_Instance_Pointers(!merge && new_camera);

    if ( ! merge && ! new_camera )
        Camera_Set_Object_From_Camera(FALSE);

    Hash_Free(load_hash);
    changed_scene = FALSE;

    version = VERSION_FLOAT;
}

static int
Load_Handle_Options()
{
    int     token, i;
    long    count;

    if ( ( token = yylex() ) != INT_TOKEN )
    {
        Input_Error;
        return token;
    }

    count = lex_int;
    for ( i = 0 ; i < count ; i++ )
        if ( ( token = yylex() ) == EOF_TOKEN )
            return token;
    return yylex();
}



/*  int
**  Load_View(Viewport *view, char *label)
**  Loads information into the viewport view. Also saves if required.
*/
int
Load_View(Viewport *view, char *label)
{
    Viewport    result;
    int         token;
    Boolean     finished = FALSE;

    /* Initialize the result in case of underspecification. */
    Viewport_Init(&result);

    while ( ! finished )
    {
        switch ( token = yylex() )
        {
            case LOOKFROM_TOKEN:
                Load_Vector_f(result.view_from);
                break;

            case LOOKAT_TOKEN:
                Load_Vector_f(result.view_at);
                break;

            case LOOKUP_TOKEN:
                Load_Vector_f(result.view_up);
                break;

            case VIEWDIST_TOKEN:
                Load_Float(result.view_distance);
                break;

            case EYEDIST_TOKEN:
                Load_Float(result.eye_distance);
                break;

            case MAGNIFY_TOKEN:
                if ( (token = yylex()) != INT_TOKEN )
                {
                    Input_Error;
                    finished = TRUE;
                }
                else
                    result.magnify = lex_int;
                break;

            case MODE_TOKEN:
                if ( ( token = yylex() ) != INT_TOKEN )
                {
                    Input_Error;
                    finished = TRUE;
                }
                else
                    result.draw_mode = (int)lex_int;
                break;

            case SCREEN_TOKEN:
                if ((token = yylex()) != INT_TOKEN )
                {
                    Input_Error;
                    return token;
                }
                result.scr_width = (Dimension)lex_int;

                if ( (token = yylex()) != INT_TOKEN )
                {
                    Input_Error;
                    return token;
                }
                result.scr_height = (Dimension)lex_int;
                break;

            case AXES_TOKEN:
                if ((token = yylex()) != INT_TOKEN )
                    Input_Error
                else
                {
                    if ( view == &(main_window.viewport) )
                        Hash_Insert(load_hash, (unsigned long)lex_int,
                                    (void*)&(main_window.axes));
                    else if ( view == &(csg_window.viewport) )
                        Hash_Insert(load_hash, (unsigned long)lex_int,
                                    (void*)&(csg_window.axes));
                    else if ( view == &(agg_window.viewport) )
                        Hash_Insert(load_hash, (unsigned long)lex_int,
                                    (void*)&(agg_window.axes));
                }
                break;

            default:
                finished = TRUE;
        }
    }

    result.is_default = FALSE;

    Build_Viewport_Transformation(&result);

    if ( view ) *view = result;

    if ( label ) View_Save(&result, label);

    return token;
}

/*  int
**  Load_Layers()
**  Loads a set of layer number-name pairs, and stores each.
*/
static int
Load_Layers()
{
    char    *name;
    int     num;
    int     token;

    while ( ( token = yylex() ) == STRING_TOKEN )
    {
        name = lex_string;
        if ( ( token = yylex() ) != INT_TOKEN )
            Input_Error
        else
            num = lex_int;
        if ( ( token = yylex() ) != INT_TOKEN )
            Input_Error;

        Add_New_Layer(num + layer_offset, name, lex_int ? TRUE : FALSE);
        free(name);
    }

    return token;
}


/*  int
**  Load_Camera(Camera* cam, Boolean set_target)
**  Loads camera information into the global structure "camera".
*/
static int
Load_Camera(Camera *cam, Boolean set_target, Boolean *new_camera)
{
    int token;
    Boolean     loc = FALSE;
    Boolean     at = FALSE;
    Boolean     up = FALSE;
    Boolean     hfov = FALSE;
    Boolean     vfov = FALSE;
    Boolean     eye = FALSE;
    Boolean     screen = FALSE;
    Boolean     finished = FALSE;
    int         zone_type, var, det, qual;
    char        *zone_str, *exp_str, *ind_str;
    ObjectInstance  new_object;
    Transformation  trans;

    *new_camera = FALSE;
    new_object.o_flags = 0;

    if ( (token = yylex()) == INT_TOKEN )
    {
        int i;

        if ( set_target )
        {
            if ( lex_int )
                new_object.o_flags |= ObjVisible;
            else
                new_object.o_flags &= ( ObjAll ^ ObjVisible );
        }

        if ((token = yylex()) != INT_TOKEN )
            Input_Error;

        Hash_Insert(load_hash, (unsigned long)lex_int,
                    (void*)&camera_object);

        if ( ( token = yylex() ) != TRANSFORM_TOKEN)
            Input_Error;
        Load_Vector(trans.matrix.x);
        Load_Vector(trans.matrix.y);
        Load_Vector(trans.matrix.z);
        Load_Vector(trans.displacement);

        new_object.o_num_features = 0;
        Load_Features(&new_object);
        token = yylex();

        if ( token != DEPENDENTS_TOKEN || (token = yylex()) != INT_TOKEN )
            Input_Error;
        new_object.o_num_depend = lex_int;
        if ( lex_int )
            new_object.o_dependents = New(Dependent, lex_int);
        else
            new_object.o_dependents = NULL;
        for ( i = 0 ; i < new_object.o_num_depend ; i++ )
        {
            if ( (token = yylex()) != INT_TOKEN )
                Input_Error;
            new_object.o_dependents[i].obj = (ObjectInstancePtr)lex_int;
            if ( (token = yylex()) != INT_TOKEN )
                Input_Error;
            new_object.o_dependents[i].count = (char)lex_int;
        }

        if ( set_target )
        {
            /* Replace the existing camera. */
            Instance_Destroy_Features(&camera_object);
            if ( camera_object.o_num_depend )
                free(camera_object.o_dependents);

            Transform_Instance(&camera_object, &trans, TRUE);

            camera_object.o_features = new_object.o_features;
            camera_object.o_num_depend = new_object.o_num_depend;
            camera_object.o_dependents = new_object.o_dependents;
            camera_object.o_flags = new_object.o_flags;
            *new_camera = TRUE;
        }
        else
            Instance_Destroy_Features(&new_object);
        token = yylex();
    }

    switch ( token )
    {
        case NONE_TOKEN:
            if ( set_target )
                target_renderer = NoTarget;
            break;

        case RAYSHADE_TOKEN:
            if ( set_target )
                target_renderer = Rayshade;
            break;

        case RENDERMAN_TOKEN:
            if ( set_target )
                target_renderer = Renderman;
            break;

        case POVRAY_TOKEN:
            if ( set_target )
                target_renderer = POVray;
            break;

        case LOAD_VRML_TOKEN:
            if ( set_target )
                target_renderer = VRML;
            break;

        case RADIANCE_TOKEN:
            if ( set_target )
                target_renderer = Radiance;
            if ( ( token = yylex() ) != INT_TOKEN )
                Input_Error;
            zone_type = (int)lex_int;
            if ( ( token = yylex() ) != STRING_TOKEN )
                Input_Error;
            zone_str = lex_string;
            if ( ( token = yylex() ) != STRING_TOKEN )
                Input_Error;
            exp_str = lex_string;
            if ( ( token = yylex() ) != INT_TOKEN )
                Input_Error;
            var = (int)lex_int;
            if ( ( token = yylex() ) != INT_TOKEN )
                Input_Error;
            det = (int)lex_int;
            if ( ( token = yylex() ) != INT_TOKEN )
                Input_Error;
            qual = (int)lex_int;
            if ( ( token = yylex() ) != STRING_TOKEN )
                Input_Error;
            ind_str = lex_string;
#if RADIANCE_SUPPORT
            Radiance_Set_Extras(zone_type, zone_str, exp_str, var, det, qual,
                                ind_str);
#endif
            free(zone_str);
            free(exp_str);
            free(ind_str);
            break;

        default:
            fprintf(stderr, "Error: Camera type expected file %s line %d\n",
                    io_file_name, line_num);
            return -1;
    }

    cam->defined = TRUE;

    while ( ! finished )
    {
        switch ( token = yylex() )
        {
            case DEFAULT_TOKEN:
                cam->defined = FALSE;
                break;
            case LOOKFROM_TOKEN:
                Load_Vector_f(cam->location);
                loc = TRUE;
                break;
            case LOOKAT_TOKEN:
                Load_Vector_f(cam->look_at);
                at = TRUE;
                break;
            case LOOKUP_TOKEN:
                Load_Vector_f(cam->look_up);
                up = TRUE;
                break;
            case UP_TOKEN:
            case RIGHT_TOKEN:
                token = yylex();
                break;
            case HFOV_TOKEN:
                Load_Float(cam->horiz_fov);
                hfov = TRUE;
                break;
            case VFOV_TOKEN:
                Load_Float(cam->vert_fov);
                vfov = TRUE;
                break;
            case EYEDIST_TOKEN:
                Load_Float(cam->eye_dist);
                eye = TRUE;
                break;
            case SCREEN_TOKEN:
                if ( (token = yylex()) != INT_TOKEN )
                {
                    finished = TRUE;
                    break;
                }
                else cam->scr_width = (Dimension)lex_int;
                if ( (token = yylex()) != INT_TOKEN )
                {
                    finished = TRUE;
                    break;
                }
                else
                {
                    cam->scr_height = (Dimension)lex_int;
                    screen = TRUE;
                }
                break;

            default:
                finished = TRUE;
        }
    }

    return token;
}

/*  Loads declarations for any target.
*/
void
Load_Declaration(Renderer target, Boolean append)
{
    int token;

    if ( ( token = yylex() ) != STRING_TOKEN )
        Input_Error;
    Change_Declarations(target, lex_string, append);
    free(lex_string);
}

static int
Load_Declarations(Boolean append)
{
    Renderer    i;

    for ( i = NoTarget + 1 ; i < LastTarget - 1 ; i++ )
        Load_Declaration(i, append);
    if ( version < 1.015 )
	Change_Declarations(i, "", append);
    else
	Load_Declaration(i, append);

    return yylex();
}


static int
Load_Includes()
{
    int     token;
    char    *declare_string;

    while ( ( token = yylex() ) == STRING_TOKEN )
    {
        declare_string = New(char, strlen(lex_string) + 16);
        sprintf(declare_string, "#include \"%s\"\n", lex_string);
        Change_Declarations(target_renderer, declare_string, TRUE);
        free(declare_string);
        free(lex_string);
    }
    return token;
}


/*  int
**  Load_Basetypes()
**  Loads base type information.
*/
static int
Load_Basetypes()
{
    char            *label;
    GenericObject   obj_type;
    CSGNodePtr      tree = NULL;
    InstanceList    agg_insts = NULL;
    WireframePtr*   wireframes;
    WireframePtr*   simple_wireframes;
    WireframePtr    triangle_wire;
    Boolean         use_full;
    unsigned long   hash_index;
    BaseObjectPtr   res;
    int max_density = 0;
    int ref;
    int token;
    int i;
    Boolean dummy;

    for ( i = 0 ; ( token = yylex() ) == INT_TOKEN ; i++ )
        Hash_Insert(load_hash, (unsigned long)lex_int,(void*)(base_objects[i]));

    while ( token == STRING_TOKEN )
    {
        label = lex_string;

        if ( ( token = yylex()) == CSG_TOKEN )
            obj_type = csg_obj;
        else if ( token == WIREFRAME_TOKEN )
            obj_type = wireframe_obj;
        else if ( token == AGG_TOKEN )
            obj_type = aggregate_obj;
        token = yylex();

        if ( token != INT_TOKEN )
            Input_Error;
        hash_index = (unsigned long)lex_int;

        if ( (token = yylex()) != REFERENCE_TOKEN ||
             (token = yylex()) != INT_TOKEN )
            Input_Error;
        ref = lex_int;

        if ( obj_type == csg_obj )
        {
            tree = Load_CSG_Tree(NULL);
            token = yylex();
        }
        else if ( obj_type == aggregate_obj )
        {
            if ( ( token = yylex() ) != INT_TOKEN )
                Input_Error;
            token = Load_Instance_List(&agg_insts, (int)lex_int);
        }
        else
            token = yylex();

        if ( obj_type != aggregate_obj )
        {
            if ( token == INT_TOKEN )
            {
                max_density = (int)lex_int;
                token = yylex();
            }
            else
                max_density = 0;

            wireframes = New(WireframePtr, max_density + 1);

            if ( token == WIREFRAME_TOKEN )
            {
                for ( i = 0 ; i < max_density + 1 ; i++ )
                    token = Load_Wireframe(wireframes + i, &use_full);
            }
            else
                Input_Error;
        }

        if ( obj_type == csg_obj )
        {
            if ( token == WIREFRAME_TOKEN )
            {
                simple_wireframes = New(WireframePtr, max_density + 1);
                for ( i = 0 ; i < max_density + 1 ; i++ )
                    token = Load_Wireframe(simple_wireframes + i, &dummy);
            }
            else if ( use_full )
            {
                simple_wireframes = New(WireframePtr, max_density + 1);
                for ( i = 0 ; i < max_density + 1 ; i++ )
                    simple_wireframes[i] = CSG_Generate_Full_Wireframe(tree, i);
            }
            else
                simple_wireframes = NULL;

            res = Add_CSG_Base_Object(tree, label, max_density, wireframes,
                                      simple_wireframes, ref, use_full);

            Hash_Insert(load_hash, hash_index, (void*)res);
            Base_Add_Select_Option(res);
        }
        else if ( obj_type == wireframe_obj )
        {
            if ( token == WIREFRAME_TOKEN )
                token = Load_Wireframe(&triangle_wire, &dummy);
            else
                triangle_wire = NULL;
            res = Add_Wireframe_Base_Object(label, wireframes[0], triangle_wire,
                                            ref);
            Hash_Insert(load_hash, hash_index, (void*)res);
            Base_Add_Select_Option(res);
            free(wireframes);
        }
        else if ( obj_type == aggregate_obj )
        {
            wireframes = New(WireframePtr, 1);
            *wireframes = Agg_Generate_Wireframe(agg_insts, 0, AGG_ALL);
            Edge_Table_Build(*wireframes);
            res = Add_Agg_Base_Object(Merge_Selection_Lists(agg_insts, NULL),
                                      label, *wireframes, NULL, ref);
            Hash_Insert(load_hash, hash_index, (void*)res);
            Base_Add_Select_Option(res);
            Free_Selection_List(agg_insts);
            agg_insts = NULL;
        }

        free(label);
    }
    
    return token;
}


static int
Load_Instance_List(InstanceList* list, int num_objs)
{
    char                *label;
    int                 token;
    ObjectInstancePtr   obj;
    int                 count = 0;

    token = yylex();
    while ( ( num_objs == -1 || num_objs > count) && token == STRING_TOKEN )
    {
        label = lex_string;
        obj = Load_Instance(lex_string);
        free(label);

        if ( obj )
        {
            Insert_Element(list, obj);

            /* Set visibility. */
            if ( Layer_Is_Visible(obj->o_layer) )
                obj->o_flags |= ObjVisible;

            Insert_Element(&(new_instances), obj);
        }

        count++;
        token = yylex();
    }

    return token;
}


/*  int
**  Load_Instances()
**  Loads instances.
*/
static int
Load_Instances()
{
    int token = Load_Instance_List(&(main_window.all_instances), -1);

    if ( main_window.view_widget )
    {
        View_Update(&main_window, main_window.all_instances, CalcView);
        Update_Projection_Extents(main_window.all_instances);
    }

    return token;

}



/*  ObjectInstancePtr
**  Load_Instance()
**  Loads a single instance. Returns NULL on error.
*/
static ObjectInstancePtr
Load_Instance(char *label)
{
    int                 token;
    void*               base_index;
    BaseObjectPtr       base;
    ObjectInstancePtr   obj;
    unsigned long       hash_index;
    Transformation      trans;
    Attributes          attribs = sced_preferences.default_attributes;
    int                 i;

#define Instance_Error(detail) \
    { \
        fprintf(stderr, "Malformed instance line %d: %s\n", line_num, detail); \
        return NULL; \
    }

#define Instance_Double(f) \
    { \
        if ((token = yylex()) == INT_TOKEN) \
            f = (double)lex_int; \
        else if (token == FLOAT_TOKEN) \
            f = lex_float; \
        else \
            Instance_Error("Expecting double") \
    }
#define Instance_Float(f) \
    { \
        if ((token = yylex()) == INT_TOKEN) \
            f = (float)lex_int; \
        else if (token == FLOAT_TOKEN) \
            f = (float)lex_float; \
        else \
            Instance_Error("Expecting float") \
    }

#define Instance_Vector(v) \
    { Instance_Double((v).x); Instance_Double((v).y); Instance_Double((v).z); }
#define Instance_Vector_f(v) \
    { Instance_Float((v).x); Instance_Float((v).y); Instance_Float((v).z); }

#define Load_Transformation(t) \
    { Instance_Vector((t).matrix.x); Instance_Vector((t).matrix.y); \
      Instance_Vector((t).matrix.z); Instance_Vector((t).displacement); }

    if ((token = yylex()) != INT_TOKEN )
        Instance_Error("Expecting id num")
    hash_index = (unsigned long)lex_int;

    if ((token = yylex()) != INT_TOKEN)
        Instance_Error("Expecting base index")
    base_index = (void*)lex_int;

    if ((base_index = Hash_Get_Value(load_hash, (unsigned long)base_index)) ==
        (void*)-1 )
    {
        fprintf(stderr, "Couldn't find base object %ld line %d\n",
                (long)base_index, line_num);
        return NULL;
    }
    base = (BaseObjectPtr)base_index;

    if ((obj = Create_Instance(base, label)) == NULL)
    {
        fprintf(stderr,"Couldn't create instance %s line %d\n",
                label, line_num);
        return NULL;
    }

    Hash_Insert(load_hash, hash_index, (void*)obj);

    if ( ( token = yylex() ) == DENSE_TOKEN )
    {
        if ( ( token = yylex() ) != INT_TOKEN )
            Instance_Error("Expecting dense wireframe level")
        if ( lex_int )
            Object_Change_Wire_Level(obj, (int)lex_int);
        token = yylex();
    }

    if ( Obj_Is_Torus(obj) )
    {
        if ( token != FLOAT_TOKEN && token != INT_TOKEN )
            Instance_Error("Expecting torus radius")
        ((TorusPtr)obj->o_hook)->major_radius =
            ( token == FLOAT_TOKEN ? lex_float : (double)lex_int );
        token = yylex();
    }

    if ( Obj_Is_Control(obj) )
    {
        if ( token != INT_TOKEN )
            Instance_Error("Expecting number of control vertices")
        control_part(obj)->num_control_verts = (int)lex_int;
        for ( i = 0 ; i < control_part(obj)->num_control_verts ; i++ )
            Instance_Vector(control_part(obj)->control_verts[i]);
        token = yylex();
    }

    if ( token != TRANSFORM_TOKEN)
        Instance_Error("Expecting transform token")
    Load_Transformation(trans);
    Transform_Instance(obj, &trans, TRUE);

    if ( (token = yylex()) != ATTRIBUTES_TOKEN)
        Instance_Error("Expecting attributes token")
    token = Load_Attributes(&attribs, obj->o_attribs);
    Modify_Instance_Attributes(obj, &attribs,
                ModSimple | ModExtend | ModLight | ModSpotlight | ModArealight);

    if ( token != ALIAS_TOKEN )
        Instance_Error("Expecting alias token")
    else
        token = Load_Aliases(&(obj->o_aliases));

    if ( token != LOD_TOKEN )
        Instance_Error("Expecting LOD token")
    else
        token = Load_LOD(&(obj->o_lods));

    if ( token != LAYER_TOKEN || (token = yylex()) != INT_TOKEN )
        Instance_Error("Expecting layer token and integer")
    else if ( lex_int )
    {
        Layer_Remove_Instance(NULL, obj->o_layer, obj);
        if ( lex_int == 1 )
            obj->o_layer = 1;
        else
            obj->o_layer = lex_int + layer_offset;
        Layer_Add_Instance(NULL, obj->o_layer, obj);
    }

    Load_Features(obj);
    token = yylex();

    if ( token != DEPENDENTS_TOKEN || (token = yylex()) != INT_TOKEN )
        Instance_Error("Expecting dependents token and number of dependents")
    obj->o_num_depend = lex_int;
    if ( lex_int )
        obj->o_dependents = New(Dependent, lex_int);
    else
        obj->o_dependents = NULL;
    for ( i = 0 ; i < obj->o_num_depend ; i++ )
    {
        if ( (token = yylex()) != INT_TOKEN )
            Instance_Error("Expecting dependent")
        obj->o_dependents[i].obj = (ObjectInstancePtr)lex_int;
        if ( (token = yylex()) != INT_TOKEN )
            Instance_Error("Expecting dependent count")
        obj->o_dependents[i].count = (char)lex_int;
    }

    return obj;
}


/*  int
**  Load_Attributes(AttributePtr a, AttributePtr target)
**  Loads what attributes it can find into a.  Will always read one past,
**  so returns the last token read.
**  Sets flags to indicate what was read.
*/
static int
Load_Attributes(AttributePtr a, AttributePtr target)
{
    Renderer    rend;
    int token;

    if ( ( token = yylex() ) != INT_TOKEN )
        Input_Error;
    Hash_Insert(load_hash, (unsigned long)lex_int, (void*)target);

    if ( ( token = yylex() ) != INT_TOKEN )
        Input_Error;
    a->defined = lex_int ? TRUE : FALSE;

    Load_Float(a->color.red);
    Load_Float(a->color.green);
    Load_Float(a->color.blue);
    Load_Float(a->diff_coef);
    Load_Float(a->spec_coef);
    Load_Float(a->spec_power);
    Load_Float(a->reflect_coef);
    Load_Float(a->transparency);
    Load_Float(a->refract_index);
    if ((token = yylex()) != INT_TOKEN)
        Input_Error;
    a->use_extension = lex_int ? TRUE : FALSE;
    for ( rend = NoTarget ; rend < LastTarget - 1 ; rend++ )
    {
        if ( ( token = yylex() ) != STRING_TOKEN )
            Input_Error;
        if ( lex_string[0] == '\0' )
        {
            a->extension[rend] = NULL;
            free(lex_string);
        }
        else
            a->extension[rend] = lex_string;
    }
    if ( /* version > 1.005 && */ version < 1.015 )
	a->extension[rend] = NULL;
    else
    {
        if ( ( token = yylex() ) != STRING_TOKEN )
            Input_Error;
        if ( lex_string[0] == '\0' )
        {
            a->extension[rend] = NULL;
            free(lex_string);
        }
        else
            a->extension[rend] = lex_string;
    }
    if ((token = yylex()) != INT_TOKEN)
        Input_Error;
    a->use_obj_trans = lex_int ? TRUE : FALSE;
    if ((token = yylex()) != INT_TOKEN)
        Input_Error;
    a->open = lex_int ? TRUE : FALSE;
    Load_Float(a->intensity.red);
    Load_Float(a->intensity.green);
    Load_Float(a->intensity.blue);
    Load_Float(a->radius);
    Load_Float(a->tightness);
    if ((token = yylex()) != INT_TOKEN)
        Input_Error;
    a->samples = lex_int;
    if ((token = yylex()) != INT_TOKEN)
        Input_Error;
    a->jitter = lex_int ? TRUE : FALSE;
    if ((token = yylex()) != INT_TOKEN)
        Input_Error;
    a->invert = lex_int ? TRUE : FALSE;
                
    /* To keep the compiler happy. */
    return yylex();
}


static int
Load_Aliases(void ***aliases)
{
    int token;
    int i;

    if ( ( token = yylex() ) != INT_TOKEN )
        Input_Error;
    if ( ! lex_int )
    {
        *aliases = NULL;
        return yylex();
    }

    *aliases = (void**)New(void*, LastTarget);
    for ( i = NoTarget ; i < LastTarget ; i++ )
        (*aliases)[i] = NULL;
    for ( i = 0 ; ( token = yylex() ) == STRING_TOKEN ; i++ )
        if ( lex_string[0] != '\0' )
            (*aliases)[i] = (void*)lex_string;
        else
            free(lex_string);

    return token;
}


static int
Load_LOD(LODInfoPtr *lods)
{
    int token;
    int i;

    if ( ( token = yylex() ) != INT_TOKEN )
        Input_Error;
    if ( ! lex_int )
    {
        *lods = NULL;
        return yylex();
    }

    *lods = New(LODInfo, 1);
    (*lods)->num_lods = (int)lex_int;
    (*lods)->lods = New(float, (*lods)->num_lods);
    for ( i = 0 ; i < (*lods)->num_lods ; i++ )
        Load_Float((*lods)->lods[i]);

    return yylex();
}


/*  int
**  Load_Lights(Boolean do_ambient)
**  Loads the lights.
*/
static int
Load_Lights(Boolean do_ambient)
{
    int     token;
    double  dummy;

    if ( do_ambient )
    {
        Load_Float(ambient_light.red);
        Load_Float(ambient_light.green);
        Load_Float(ambient_light.blue);
    }
    else
    {
        Load_Float(dummy);
        Load_Float(dummy);
        Load_Float(dummy);
    }

    return yylex();
}



static void
Load_Spec(ConstraintSpecPtr s)
{
    int token;

    if ( (token = yylex()) != INT_TOKEN )
        Input_Error;

    s->spec_type = (ConstraintSpecType)lex_int;

    if ( s->spec_type == reference_spec || s->spec_type == feature_spec ||
         s->spec_type == vertex_spec || s->spec_type == parameter_spec )
    {
        if ( (token = yylex()) != INT_TOKEN )
            Input_Error;
        s->spec_data = (void*)lex_int;
    }
    else
        s->spec_data = NULL;

    Load_Vector_f(s->spec_vector)
}

/*  int
**  Load_Constraint(ConstraintPtr feat)
**  Loads a single constraint feature structure.
*/
static int
Load_Constraint(ConstraintPtr feat)
{
    int         token;
    Boolean     reverse_specs = FALSE;
    int         i;

    VNew(0, 0, 0, feat->c_vector);
    VNew(0, 0, 0, feat->c_point);
    feat->c_value = feat->c_radius = feat->c_ratio = 0;
    feat->c_status = feat->c_forced = FALSE;

    if ( ( token = yylex() ) != INT_TOKEN )
        Input_Error
    feat->c_type = (int)lex_int;

    if ( ( token = yylex() ) != INT_TOKEN )
        Input_Error
    feat->c_spec_flags = (int)lex_int;

    if ( (token = yylex()) != STRING_TOKEN )
        Input_Error
    feat->c_label = lex_string;

    if ( ( token = yylex() ) != INT_TOKEN )
        Input_Error
    feat->c_status = lex_int ? TRUE : FALSE;

    if ( ( token = yylex() ) != INT_TOKEN )
        Input_Error
    feat->c_forced = lex_int ? TRUE : FALSE;

    switch ( feat->c_type )
    {
        case plane_feature:
            Load_Vector_f(feat->c_vector);
            Load_Vector_f(feat->c_point);
            feat->c_value = VDot(feat->c_vector, feat->c_point);
            if ( feat->c_spec_flags & RatioPlane )
            {
                Load_Double(feat->c_radius);
                Load_Double(feat->c_ratio);
            }
            break;

        case line_feature:
            Load_Vector_f(feat->c_vector);
            Load_Vector_f(feat->c_point);
            break;


        case sphere_feature:
            Load_Vector_f(feat->c_point);
            Load_Double(feat->c_radius);
            break;

        case circle_feature:
            Load_Vector_f(feat->c_vector);
            Load_Vector_f(feat->c_point);
            Load_Double(feat->c_radius);
            break;

        case point_feature:
            Load_Vector_f(feat->c_point);
            if ( feat->c_spec_flags & RatioPoint )
            {
                Load_Double(feat->c_radius);
                Load_Double(feat->c_ratio);
            }
            break;

        case axis_feature:
        case ratio_point_feature:
        case ratio_plane_feature:
        case null_feature:
        case inconsistent_feature:
            break;
    }
    if ( ( token = yylex() ) != INT_TOKEN )
        Input_Error
    feat->c_num_specs = (int)lex_int;

    feat->c_specs = New(ConstraintSpec, feat->c_num_specs);
    if ( reverse_specs )
        for ( i = feat->c_num_specs - 1 ; i >= 0 ; i-- )
            Load_Spec(feat->c_specs + i);
    else
        for ( i = 0 ; i < feat->c_num_specs ; i++ )
            Load_Spec(feat->c_specs + i);

    return token;
}


/*  int
**  Load_Constraints(ConstraintPtr*, int*)
**  Loads a set of Constraints and activity.
*/
static int
Load_Constraints(ConstraintPtr *ret_cons, int *ret_num)
{
    ConstraintData  current;
    int start_num = *ret_num;
    int token;
    int i;


    if ( (token = yylex()) != INT_TOKEN )
        Input_Error;
    for ( i = 0 ; i < start_num ; i++ )
    {
        free((*ret_cons)[i].c_specs);
        (*ret_cons)[i].c_specs = NULL;
    }
    *ret_num = lex_int;
    if ( *ret_num )
    {
        if ( start_num )
            *ret_cons = More(*ret_cons, ConstraintData, *ret_num);
        else
            *ret_cons = New(ConstraintData, *ret_num);
    }
    else
    {
        if ( start_num )
            free(*ret_cons);
        *ret_cons = NULL;
    }
    for ( i = 0 ; i < *ret_num ; i++ )
    {
        Load_Constraint(&current);
        (*ret_cons)[i] = current;
    }

    return 1;
}


static void
Load_Features(ObjectInstancePtr obj)
{
    int token;
    int i;

    if ( obj->o_features )
        Instance_Destroy_Features(obj);

    if ( yylex() != FEATURES_TOKEN )
        Input_Error

    if ( yylex() != INT_TOKEN )
        Input_Error

    obj->o_num_features = (int)lex_int;

    obj->o_features = New(Feature, obj->o_num_features);
    for ( i = 0 ; i < obj->o_num_features ; i++ )
    {
        if ( yylex() != INT_TOKEN )
            Input_Error
        obj->o_features[i].base = Feature_Base((int)lex_int);
        Load_Vector_f(obj->o_features[i].location);
        obj->o_features[i].num_constraints = 0;
        Load_Constraints(&(obj->o_features[i].constraints),
                         &(obj->o_features[i].num_constraints));
        if ( yylex() != INT_TOKEN )
            Input_Error
        obj->o_features[i].flags = (int)lex_int;
    }
}


/*  int
**  Load_Wireframe(WireframePtr*, Boolean *full)
**  Loads a wireframe and returns the next token.
**  It exits on error, because the base types must be entered properly.
*/
static int
Load_Wireframe(WireframePtr *wire, Boolean *full)
{
    int token;
    WireframePtr    res;
    int i, j;

    res = New(Wireframe, 1);

#define Wireframe_Error(detail) \
    { \
        fprintf(stderr, "Malformed wireframe line %d: %s\n", line_num, detail);\
        Sced_Exit(1); \
    }

#define Wireframe_Float(f) \
    { \
        if ((token = yylex()) == INT_TOKEN) \
            f = (double)lex_int; \
        else if (token == FLOAT_TOKEN) \
            f = lex_float; \
        else \
            Wireframe_Error("Expecting float") \
    }

#define Wireframe_Vector(v) \
    { Wireframe_Float((v).x); Wireframe_Float((v).y); Wireframe_Float((v).z); }


    if ( ( token = yylex() ) == FULL_TOKEN )
    {
        *full = TRUE;
        token = yylex();
    }
    else
        *full = FALSE;

    if ( token != INT_TOKEN )
        Wireframe_Error("Expecting number of attribute sets.")
    res->num_attribs = (int)lex_int;
    token = Load_Wireframe_Attributes(&(res->attribs), res->num_attribs);

    if ( token != INT_TOKEN )
        Wireframe_Error("Expecting number of real vertices")
    res->num_real_verts = lex_int;
    res->num_vertices = lex_int + 1;
    res->vertices = New(Vector, res->num_vertices);
    for ( i = 0 ; i < res->num_real_verts ; i++ )
        Wireframe_Vector(res->vertices[i])
    VNew(0, 0, 0, res->vertices[i]);

    if ( ( token = yylex() ) != INT_TOKEN )
        Wireframe_Error("Expecting number of normals")
    res->num_normals = lex_int;
    res->normals = New(Vector, res->num_normals);
    for ( i = 0 ; i < res->num_normals ; i++ )
        Wireframe_Vector(res->normals[i])

    if ( ( token = yylex() ) != INT_TOKEN )
        Wireframe_Error("Expecting number of faces")
    res->num_faces = lex_int;
    res->faces = New(Face, res->num_faces);
    for ( i = 0 ; i < res->num_faces ; i++ )
    {
        if ( ( token = yylex() ) != INT_TOKEN )
            Wireframe_Error("Expecting number of face verts")
        res->faces[i].num_vertices = lex_int;
        res->faces[i].vertices = New(int, res->faces[i].num_vertices);
        res->faces[i].normals = New(int, res->faces[i].num_vertices);
        for ( j = 0 ; j < res->faces[i].num_vertices ; j++ )
        {
            if ( ( token = yylex() ) != INT_TOKEN )
                Wireframe_Error("Expecting face vertex")
            res->faces[i].vertices[j] = lex_int;
            if ( ( token = yylex() ) != INT_TOKEN )
                Wireframe_Error("Expecting face normal")
            res->faces[i].normals[j] = lex_int;
        }
        
        if ((token = yylex()) == INT_TOKEN)
        {
            if ( ( res->faces[i].face_attribs =
                    Hash_Get_Value(load_hash, (unsigned long)lex_int) ) ==
                 (void*)-1 )
                res->faces[i].face_attribs = NULL;
            Wireframe_Vector(res->faces[i].normal);
        }
        else
        {
            res->faces[i].face_attribs = NULL;
            if ( token == FLOAT_TOKEN )
                res->faces[i].normal.x = lex_float;
            else
                Wireframe_Error("Expecting face normal")
            Wireframe_Float(res->faces[i].normal.y);
            Wireframe_Float(res->faces[i].normal.z);
        }
    }

    res->edges = NULL;

    *wire = res;
    return yylex();

#undef Wireframe_Error
#undef Wireframe_Float
#undef Wireframe_Vector

}


static int
Load_Wireframe_Attributes(AttributePtr **attribs, int num)
{
    int token;
    int i;

    *attribs = New(AttributePtr, num);

    token = yylex();
    for ( i = 0 ; i < num ; i++ )
    {
        (*attribs)[i] = Attribute_New(NULL, TRUE);
        if ( token != ATTRIBUTES_TOKEN )
            Input_Error;
        token = Load_Attributes((*attribs)[i], (*attribs)[i]);
    }

    return token;
}


static int
Load_Aggregate_Insts()
{
    int token;

    if ( ( token = yylex() ) != INT_TOKEN )
        Input_Error;

    token = Load_Instance_List(&(agg_window.all_instances), lex_int);

    if ( agg_window.view_widget )
    {
        View_Update(&agg_window, agg_window.all_instances, CalcView);
        Update_Projection_Extents(agg_window.all_instances);
    }

    return token;

}



/*  int
**  Load_CSG_Trees()
**  Loads a sequence of CSG trees and sets them up in the CSG window.
*/
static int
Load_CSG_Trees()
{
    CSGNodePtr  new_tree;
    int         token;
    int         num_trees;
    int         i;

    if ( ( token = yylex() ) != INT_TOKEN )
        Input_Error;

    num_trees = lex_int;

    if ( ! csg_window.shell )
        Create_CSG_Display();

    for ( i = 0 ; i < num_trees ; i++ )
    {
        new_tree = Load_CSG_Tree(NULL);

        if ( ( token = yylex() ) != INT_TOKEN )
            Input_Error;

        CSG_Insert_Existing_Tree(new_tree, lex_int ? TRUE : FALSE, NULL);
    }

    return yylex();
}


/*  CSGNodePtr
**  Loads a CSG tree spec. If it errors, it exits.
*/
static CSGNodePtr
Load_CSG_Tree(CSGNodePtr parent)
{
    char        *label;
    int         token;
    CSGNodePtr  res = New(CSGNode, 1);

    res->csg_parent = parent;
    res->csg_widget = NULL;

    if ( ( token = yylex() ) == STRING_TOKEN )
    {
        label = lex_string;
        res->csg_op = csg_leaf_op;
        res->csg_instance = Load_Instance(label);

        free(label);
        if ( ! res->csg_instance )
        {
            fprintf(stderr, "Unable to load CSG instance line %d\n", line_num);
            Sced_Exit(1);
        }

        Insert_Element(&(new_instances), res->csg_instance);

        res->csg_left_child = res->csg_right_child = NULL;
    }
    else
    {
        switch ( token )
        {
            case UNION_TOKEN:
                res->csg_op = csg_union_op;
                break;
            case INTERSECTION_TOKEN:
                res->csg_op = csg_intersection_op;
                break;
            case DIFFERENCE_TOKEN:
                res->csg_op = csg_difference_op;
                break;
            default:
                fprintf(stderr, "Unknown CSG Op token %d int %ld line %d\n",
                        token, lex_int, line_num);
                Sced_Exit(1);
        }
        res->csg_instance = NULL;

        res->csg_left_child = Load_CSG_Tree(res);
        res->csg_right_child = Load_CSG_Tree(res);
    }

    return res;
}


static void
Refresh_Spec(ConstraintSpecPtr spec, ObjectInstancePtr obj, void *ptr,
             void *ptr2, int num)
{
    if ( Spec_Is_Dependent(spec->spec_type) )
        spec->spec_data =
            Hash_Get_Value(load_hash, (unsigned long)spec->spec_data);
}

void
Refresh_Constraint_References(ConstraintPtr options, int num)
{
    int i;

    for ( i = 0 ; i < num ; i++ )
        Constraint_Manipulate_Specs(options + i,
                                    NULL, NULL, NULL, 0, Refresh_Spec);
}


static void
Refresh_Constraint_Pointers(ObjectInstancePtr obj)
{
    int i, j;

    for ( i = 0 ; i < obj->o_num_depend ; i++ )
        if ( ( obj->o_dependents[i].obj = (ObjectInstancePtr)
                Hash_Get_Value(load_hash,
                    (unsigned long)(obj->o_dependents[i].obj)) ) ==
                (void*)-1 )
        {
            for ( j = i + 1 ; j < obj->o_num_depend ; j++ )
                obj->o_dependents[j-1] = obj->o_dependents[j];
            obj->o_num_depend--;
            i--;
        }

    for ( i = 0 ; i < obj->o_num_features ; i++ )
        Refresh_Constraint_References(obj->o_features[i].constraints,
                                      obj->o_features[i].num_constraints);

}

/*  void
**  Refresh_Instance_Pointers()
**  Changes all the instance pointers which are stale (because they were
**  read from file) to new the current instance pointers.
*/
static void
Refresh_Instance_Pointers(Boolean new_cam)
{
    InstanceList    current;

    for ( current = new_instances ; current ; current = current->next )
        Refresh_Constraint_Pointers(current->the_instance);

    if ( new_cam )
        Refresh_Constraint_Pointers(&camera_object);

    Free_Selection_List(new_instances);
    new_instances = NULL;
}

