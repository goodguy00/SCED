/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/load_old.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: load_old.c,v $
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
**  load_old.c : Code needed to load old version files.
**
**  External Functions:
**
**  void Load_O_Dialog_Func(Widget, XtPointer, XtPointer);
**  Puts up the load dialog box.
**
**  void Load_O_Internal_File(FILE*, Boolean)
**  Loads the contents of a file.
**
*/

#include <sced.h>
#include <add_constraint.h>
#include <attributes.h>
#include <base_objects.h>
#include <constraint.h>
#include <csg.h>
#include <csg_wire.h>
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

#define Load_O_Float(f) \
    { \
        if ((token = yylex()) == INT_TOKEN) \
            f = (float)lex_int; \
        else if (token == FLOAT_TOKEN) \
            f = (float)lex_float; \
        else \
            Input_Error; \
    }
#define Load_O_Double(f) \
    { \
        if ((token = yylex()) == INT_TOKEN) \
            f = (double)lex_int; \
        else if (token == FLOAT_TOKEN) \
            f = lex_float; \
        else \
            Input_Error; \
    }

#define Load_O_Vector(v) \
    { Load_O_Double((v).x); Load_O_Double((v).y); Load_O_Double((v).z); }
#define Load_O_Vector_f(v) \
    { Load_O_Float((v).x); Load_O_Float((v).y); Load_O_Float((v).z); }

#if RADIANCE_SUPPORT
extern void     Radiance_Set_Extras(int, char*, char*, int, int, int, char*);
#endif

typedef enum _OldSpec {
    old_absolute_spec, old_offset_spec, old_reference_spec, old_origin_spec,
    old_scaling_spec, old_major_spec, old_minor_spec, old_other_spec,
    old_radius_spec
    } OldSpec;
typedef enum _OldConstraintType {
    old_null_feature,
    old_plane_feature,
    old_line_feature,
    old_point_feature,
    old_sphere_feature,
    old_circle_feature,
    old_inconsistent_feature,
    old_midplane_feature,
    old_midpoint_feature,
    old_axis_plane_feature,
    old_axis_feature,
    old_orig_line_feature,
    old_ref_line_feature,
    old_orig_plane_feature,
    old_ref_plane_feature,
    old_dir_axis_feature,
    old_norm_plane_feature,
    old_const_norm_plane_feature,
    old_ratio_point_feature,
    old_ratio_plane_feature
    } OldConstraintType;

static int  Load_O_Handle_Options();
static int  Load_O_Declarations(Boolean);
static int  Load_O_Includes();
static int  Load_O_View(Viewport*, char*);
static int  Load_O_Layers();
static int  Load_O_Basetypes();
static int  Load_O_Instances();
static int  Load_O_Attributes(AttributePtr, AttributePtr);
static int  Load_O_Aliases(void***);
static int  Load_O_LOD(LODInfoPtr*);
static int  Load_O_Light_Info(AttributePtr, GenericObject);
static int  Load_O_Camera(Camera *, Boolean, Boolean*);
static int  Load_O_Lights(Boolean);
static int  Load_O_Constraint(ConstraintPtr);
static int  Load_O_Constraints(ConstraintPtr*, int*);
static void Load_O_Features(ObjectInstancePtr);
static int  Load_O_CSG_Trees();
static int  Load_O_Wireframe(WireframePtr*, Boolean*, Boolean, Boolean);
static int  Load_O_Wireframe_Attributes(AttributePtr*, int);
static void Load_O_Create_Square_Wireframe();

static ObjectInstancePtr    Load_O_Instance(char*);
static CSGNodePtr           Load_O_CSG_Tree(CSGNodePtr);

static void Old_Refresh_Instance_Pointers(Boolean);

static HashTable    load_hash;
static InstanceList new_instances = NULL;

static int          layer_offset;

static BaseObjectPtr    square_wire_base = NULL;

/*  void
**  Load_Old_Format(FILE *file, int merge)
**  Loads the information contained in file.
*/
void
Load_Old_Format(FILE *file, int merge)
{
    Camera  dummy_cam;
    Boolean new_camera;
    int     token;

    load_hash = Hash_New_Table();

    token = yylex();

    while ( token != EOF_TOKEN && token != END_TOKEN )
    {
        switch (token)
        {
            case OPTIONS_TOKEN:
                token = Load_O_Handle_Options();
                break;

            case MAINVIEW_TOKEN:
                if ( merge )
                    token = Load_O_View(NULL, NULL);
                else
                {
                    token = Load_O_View(&(main_window.viewport), NULL);
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
                    token = Load_O_View(NULL, NULL);
                else
                {
                    token = Load_O_View(&(csg_window.viewport), NULL);
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

            case VIEWPORT_TOKEN:
                if ( ( token = yylex() ) != STRING_TOKEN )
                    break;
                if ( merge )
                    token = Load_O_View(NULL, NULL);
                else
                    token = Load_O_View(NULL, lex_string);
                break;

            case LAYER_TOKEN:
                if ( merge )
                    layer_offset = Layer_Get_Num() - 1;
                else
                    layer_offset = 0;
                if ( version < 0.911 )
                    layer_offset++;
                token = Load_O_Layers();
                break;

            case CAMERA_TOKEN:
                if ( merge )
                    token = Load_O_Camera(&dummy_cam, FALSE, &new_camera);
                else
                    token = Load_O_Camera(&camera, TRUE, &new_camera);
                break;

            case INCLUDES_TOKEN:
                token = Load_O_Includes();
                break;

            case DECLARE_TOKEN:
                token = Load_O_Declarations(merge);
                break;

            case BASEOBJECTS_TOKEN:
                token = Load_O_Basetypes();
                break;

            case INSTANCES_TOKEN:
                token = Load_O_Instances();
                break;

            case CSG_TOKEN:
                token = Load_O_CSG_Trees();
                break;

            case AMBIENT_TOKEN:
                token = Load_O_Lights(!merge);
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

    Old_Refresh_Instance_Pointers(!merge && new_camera);

    if ( ! merge && ! new_camera )
        Camera_Set_Object_From_Camera(FALSE);

    Hash_Free(load_hash);
    changed_scene = FALSE;
}

static int
Load_O_Handle_Options()
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
**  Load_O_View(Viewport *view, char *label)
**  Loads information into the viewport view. Also saves if required.
*/
static int
Load_O_View(Viewport *view, char *label)
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
                Load_O_Vector_f(result.view_from);
                break;

            case LOOKAT_TOKEN:
                Load_O_Vector_f(result.view_at);
                break;

            case LOOKUP_TOKEN:
                Load_O_Vector_f(result.view_up);
                break;

            case VIEWDIST_TOKEN:
                Load_O_Float(result.view_distance);
                break;

            case EYEDIST_TOKEN:
                Load_O_Float(result.eye_distance);
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
**  Load_O_Layers()
**  Loads a set of layer number-name pairs, and stores each.
*/
static int
Load_O_Layers()
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
    }

    return token;
}


/*  int
**  Load_O_Camera(Camera* cam, Boolean set_target)
**  Loads camera information into the global structure "camera".
*/
static int
Load_O_Camera(Camera *cam, Boolean set_target, Boolean *new_camera)
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
    Vector      dummy_v;

    *new_camera = FALSE;
    new_object.o_flags = 0;

    if ( version > 0.81 )
    {
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
            Load_O_Vector(trans.matrix.x);
            Load_O_Vector(trans.matrix.y);
            Load_O_Vector(trans.matrix.z);
            Load_O_Vector(trans.displacement);

            if ( version < 0.924 )
            {
                new_object.o_num_features = 4;
                new_object.o_features = New(Feature, 4);

                new_object.o_features[origin_feature].base =
                    Feature_Base(origin_feature);
                new_object.o_features[scale_feature].base =
                    Feature_Base(scale_feature);
                new_object.o_features[major_feature].base =
                    Feature_Base(major_feature);
                new_object.o_features[minor_feature].base =
                    Feature_Base(minor_feature);
                new_object.o_features[origin_feature].flags =
                new_object.o_features[scale_feature].flags =
                new_object.o_features[major_feature].flags =
                new_object.o_features[minor_feature].flags = 0;

                if ( (token = yylex()) != AXES_TOKEN )
                    Input_Error;
                Load_O_Vector_f(new_object.o_features[major_feature].location);
                Load_O_Vector_f(new_object.o_features[minor_feature].location);
                Load_O_Vector(dummy_v);

                if ( (token = yylex()) != ORIGIN_TOKEN )
                    Input_Error;
                Load_O_Vector_f(new_object.o_features[origin_feature].location);
                if ( version > 0.92 )
                {
                    if ( ( token = yylex() ) != INT_TOKEN )
                        Input_Error;
                    if ( lex_int == 1 )
                        new_object.o_features[origin_feature].flags |=
                            FeaturePref1;
                    else if ( lex_int == 2 )
                        new_object.o_features[origin_feature].flags |=
                            FeaturePref2;
                }
                if ( (token = yylex()) != REFERENCE_TOKEN )
                    Input_Error;
                Load_O_Vector_f(new_object.o_features[scale_feature].location);
                if ( version > 0.92 )
                {
                    if ( ( token = yylex() ) != INT_TOKEN )
                        Input_Error;
                    if ( lex_int == 1 )
                        new_object.o_features[scale_feature].flags |=
                            FeaturePref1;
                    else if ( lex_int == 2 )
                        new_object.o_features[scale_feature].flags |=
                            FeaturePref2;
                }
                    
                if ((token = yylex()) != CONSTRAINTS_TOKEN)
                    Input_Error;
                new_object.o_features[origin_feature].constraints =
                new_object.o_features[scale_feature].constraints =
                new_object.o_features[major_feature].constraints =
                new_object.o_features[minor_feature].constraints = NULL;
                new_object.o_features[origin_feature].num_constraints =
                new_object.o_features[scale_feature].num_constraints =
                new_object.o_features[major_feature].num_constraints =
                new_object.o_features[minor_feature].num_constraints = 0;
                Load_O_Constraints(
                    &(new_object.o_features[origin_feature].constraints),
                    &(new_object.o_features[origin_feature].num_constraints));
                Load_O_Constraints(
                    &(new_object.o_features[scale_feature].constraints),
                    &(new_object.o_features[scale_feature].num_constraints));
                Load_O_Constraints(
                    &(new_object.o_features[major_feature].constraints),
                    &(new_object.o_features[major_feature].num_constraints));
                Load_O_Constraints(
                    &(new_object.o_features[minor_feature].constraints),
                    &(new_object.o_features[minor_feature].num_constraints));
            }
            else
            {
                new_object.o_num_features = 0;
                Load_O_Features(&new_object);
            }
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
    }
    else
        token = yylex();

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

        case GENSCAN_TOKEN:
	case GENRAY_TOKEN:
            if ( set_target )
                target_renderer = NoTarget;
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
                Load_O_Vector_f(cam->location);
                loc = TRUE;
                break;
            case LOOKAT_TOKEN:
                Load_O_Vector_f(cam->look_at);
                at = TRUE;
                break;
            case LOOKUP_TOKEN:
                Load_O_Vector_f(cam->look_up);
                up = TRUE;
                break;
            case UP_TOKEN:
            case RIGHT_TOKEN:
                token = yylex();
                break;
            case HFOV_TOKEN:
                Load_O_Float(cam->horiz_fov);
                hfov = TRUE;
                break;
            case VFOV_TOKEN:
                Load_O_Float(cam->vert_fov);
                vfov = TRUE;
                break;
            case EYEDIST_TOKEN:
                Load_O_Float(cam->eye_dist);
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
Load_O_Declaration(Renderer target, Boolean append)
{
    int token;

    if ( ( token = yylex() ) != STRING_TOKEN )
        Input_Error;
    if ( target != NoTarget )
	Change_Declarations(target, lex_string, append);
    free(lex_string);
}

static int
Load_O_Declarations(Boolean append)
{
    Renderer    i;
    int 	token;

    if ( version <= 0.82 )
        Load_O_Declaration(target_renderer, append);
    else if ( version < 0.931 )
    {
        for ( i = NoTarget + 1 ; i < Radiance ; i++ )
            Load_O_Declaration(i, append);
	if ( ( token = yylex() ) != STRING_TOKEN )
	    Input_Error;
	if ( ( token = yylex() ) != STRING_TOKEN )
	    Input_Error;
        for ( ; i < LastTarget ; i++ )
            Load_O_Declaration(i, append);
    }
    else
    {
        for ( i = NoTarget + 1 ; i < Radiance ; i++ )
            Load_O_Declaration(i, append);
	if ( ( token = yylex() ) != STRING_TOKEN )
	    Input_Error;
	if ( ( token = yylex() ) != STRING_TOKEN )
	    Input_Error;
        for ( ; i < LastTarget ; i++ )
            Load_O_Declaration(i, append);
    }

    return yylex();
}


static int
Load_O_Includes()
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
**  Load_O_Basetypes()
**  Loads base type information.
*/
static int
Load_O_Basetypes()
{
    char            *label;
    Boolean         doing_csg;
    CSGNodePtr      tree = NULL;
    WireframePtr*   wireframes;
    WireframePtr*   simple_wireframes;
    WireframePtr    triangle_wire;
    Boolean         use_full;
    unsigned long   hash_index;
    BaseObjectPtr   res;
    AttributePtr    *attribs;
    int             num_attribs;
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
        {
            doing_csg = TRUE;
            token = yylex();
        }
        else if ( token == WIREFRAME_TOKEN )
        {
            doing_csg = FALSE;
            token = yylex();
        }
        else
            doing_csg = TRUE;

        if ( token != INT_TOKEN )
            Input_Error;
        hash_index = (unsigned long)lex_int;

        if ( (token = yylex()) != REFERENCE_TOKEN ||
             (token = yylex()) != INT_TOKEN )
            Input_Error;
        ref = lex_int;

        if ( doing_csg )
            tree = Load_O_CSG_Tree(NULL);

        if ( ( token = yylex() ) == INT_TOKEN )
        {
            max_density = (int)lex_int;
            token = yylex();
        }

        wireframes = New(WireframePtr, max_density + 1);

        if ( token == WIREFRAME_TOKEN )
        {
            for ( i = 0 ; i < max_density + 1 ; i++ )
	       token = Load_O_Wireframe(wireframes + i, &use_full, FALSE, TRUE);
        }
        else if ( ! doing_csg && token == INT_TOKEN )
        {
            num_attribs = lex_int;
            attribs = New(AttributePtr, num_attribs);
            token = Load_O_Wireframe_Attributes(attribs, num_attribs);
            if ( token != WIREFRAME_TOKEN )
                Input_Error;
            token = Load_O_Wireframe(wireframes, &use_full, FALSE, TRUE);
            wireframes[0]->num_attribs = num_attribs;
            wireframes[0]->attribs = attribs;
        }
        else
            Input_Error;

        if ( doing_csg )
        {
            if ( token == WIREFRAME_TOKEN )
            {
                simple_wireframes = New(WireframePtr, max_density + 1);
                for ( i = 0 ; i < max_density + 1 ; i++ )
                    token = Load_O_Wireframe(simple_wireframes + i, &dummy,
					     TRUE, FALSE);
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
        else
        {
            if ( token == INT_TOKEN )
            {
                num_attribs = lex_int;
                attribs = New(AttributePtr, num_attribs);
                token = Load_O_Wireframe_Attributes(attribs, num_attribs);
                if ( token != WIREFRAME_TOKEN )
                    Input_Error;
                token = Load_O_Wireframe(&triangle_wire, &dummy, TRUE, FALSE);
            }
            else
                triangle_wire = NULL;
            res = Add_Wireframe_Base_Object(label, wireframes[0], triangle_wire,
                                            ref);
            Hash_Insert(load_hash, hash_index, (void*)res);
            Base_Add_Select_Option(res);
            free(wireframes);
        }

        free(label);
    }
    
    return token;
}


/*  int
**  Load_O_Instances()
**  Loads instances.
*/
static int
Load_O_Instances()
{
    char                *label;
    int                 token;
    ObjectInstancePtr   obj;

    token = yylex();
    while ( token == STRING_TOKEN )
    {
        label = lex_string;
        obj = Load_O_Instance(lex_string);
        free(label);

        if ( obj )
        {
            Insert_Element(&(main_window.all_instances), obj);

            /* Set visibility. */
            if ( Layer_Is_Visible(obj->o_layer) )
                obj->o_flags |= ObjVisible;

            Insert_Element(&(new_instances), obj);
        }

        token = yylex();

    }

    if ( main_window.view_widget )
    {
        View_Update(&main_window, new_instances, CalcView);
        Update_Projection_Extents(main_window.all_instances);
    }

    return token;

}



/*  ObjectInstancePtr
**  Load_O_Instance()
**  Loads a single instance. Returns NULL on error.
*/
static ObjectInstancePtr
Load_O_Instance(char *label)
{
    int                 token;
    void*               base_index;
    BaseObjectPtr       base;
    ObjectInstancePtr   obj;
    unsigned long       hash_index;
    Transformation      trans;
    Attributes          attribs = sced_preferences.default_attributes;
    Vector              dummy_v;
    ConstraintPtr       dummy_cons;
    int                 dummy_i;
    int                 i;

#define Instance_Error \
    { \
        fprintf(stderr, "Malformed instance line %d\n", line_num); \
        return NULL; \
    }

#define Instance_Double(f) \
    { \
        if ((token = yylex()) == INT_TOKEN) \
            f = (double)lex_int; \
        else if (token == FLOAT_TOKEN) \
            f = lex_float; \
        else \
            Instance_Error \
    }
#define Instance_Float(f) \
    { \
        if ((token = yylex()) == INT_TOKEN) \
            f = (float)lex_int; \
        else if (token == FLOAT_TOKEN) \
            f = (float)lex_float; \
        else \
            Instance_Error \
    }

#define Instance_Vector(v) \
    { Instance_Double((v).x); Instance_Double((v).y); Instance_Double((v).z); }
#define Instance_Vector_f(v) \
    { Instance_Float((v).x); Instance_Float((v).y); Instance_Float((v).z); }

#define Load_O_Transformation(t) \
    { Instance_Vector((t).matrix.x); Instance_Vector((t).matrix.y); \
      Instance_Vector((t).matrix.z); Instance_Vector((t).displacement); }

    if ((token = yylex()) != INT_TOKEN )
        Instance_Error;
    hash_index = (unsigned long)lex_int;

    if ((token = yylex()) != INT_TOKEN)
        Instance_Error;
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
            Instance_Error;
        if ( lex_int )
            Object_Change_Wire_Level(obj, (int)lex_int);
        token = yylex();
    }

    if ( Obj_Is_Torus(obj) )
    {
        if ( token != FLOAT_TOKEN && token != INT_TOKEN )
            Instance_Error;
        ((TorusPtr)obj->o_hook)->major_radius =
            ( token == FLOAT_TOKEN ? lex_float : (double)lex_int );
        if ( version < 0.924 )
        {
            Load_O_Vector(obj->o_features[radius_feature].location);
            if ( version > 0.92 )
            {
                if ( ( token = yylex() ) != INT_TOKEN )
                    Instance_Error;
                if ( lex_int == 1 )
                    obj->o_features[radius_feature].flags |= FeaturePref1;
                else if ( lex_int == 2 )
                    obj->o_features[radius_feature].flags |= FeaturePref2;
            }
        }
        token = yylex();
    }

    if ( Obj_Is_Control(obj) )
    {
        if ( version < 0.926 )
        {
            /* Must be a square. */
            if ( ! square_wire_base )
                Load_O_Create_Square_Wireframe();
            Base_Change(obj, square_wire_base, FALSE, NULL);
        }
        else
        {
            if ( token != INT_TOKEN )
                Instance_Error;
            control_part(obj)->num_control_verts = (int)lex_int;
            for ( i = 0 ; i < control_part(obj)->num_control_verts ; i++ )
                Instance_Vector(control_part(obj)->control_verts[i]);
            token = yylex();
        }
    }

    if ( token != TRANSFORM_TOKEN)
        Instance_Error;
    Load_O_Transformation(trans);
    Transform_Instance(obj, &trans, TRUE);

    if ( (token = yylex()) != ATTRIBUTES_TOKEN)
        Instance_Error;
    if ( base->b_class == light_obj ||
         base->b_class == spotlight_obj ||
         base->b_class == arealight_obj ||
         base->b_class == dirlight_obj )
        token = Load_O_Light_Info(obj->o_attribs, base->b_class);
    else
    {
        token = Load_O_Attributes(&attribs, (AttributePtr)obj->o_attribs);
        Modify_Instance_Attributes(obj, &attribs,
                ModSimple | ModExtend | ModLight | ModSpotlight | ModArealight);
    }

    if ( version > 0.921 )
    {
        if ( token != ALIAS_TOKEN )
            Instance_Error
        else
            token = Load_O_Aliases(&(obj->o_aliases));
    }

    if ( version > 0.929 )
    {
        if ( token != LOD_TOKEN )
            Instance_Error
        else
            token = Load_O_LOD(&(obj->o_lods));
    }

    if ( token != LAYER_TOKEN || (token = yylex()) != INT_TOKEN )
        Instance_Error
    else if ( lex_int )
    {
        Layer_Remove_Instance(NULL, obj->o_layer, obj);
        if ( lex_int == 1 && version > 0.91 )
            obj->o_layer = 1;
        else
            obj->o_layer = lex_int + layer_offset;
        Layer_Add_Instance(NULL, obj->o_layer, obj);
    }

    if ( version < 0.924 )
    {
        if ( (token = yylex()) != AXES_TOKEN )
            Instance_Error;
        if ( obj->o_num_features > minor_feature )
        {
            Instance_Vector_f(obj->o_features[major_feature].location);
            Instance_Vector_f(obj->o_features[minor_feature].location);
        }
        else
        {
            Instance_Vector(dummy_v);
            Instance_Vector(dummy_v);
        }
        Instance_Vector(dummy_v);

        if ( (token = yylex()) != ORIGIN_TOKEN )
            Instance_Error;
        Instance_Vector_f(obj->o_features[origin_feature].location);
        if ( version > 0.92 )
        {
            if ( ( token = yylex() ) != INT_TOKEN )
                Instance_Error;
            if ( lex_int == 1 )
                obj->o_features[origin_feature].flags |= FeaturePref1;
            else if ( lex_int == 2 )
                obj->o_features[origin_feature].flags |= FeaturePref2;
        }

        if ( (token = yylex()) != REFERENCE_TOKEN )
            Instance_Error;
        if ( obj->o_num_features > scale_feature )
            Instance_Vector_f(obj->o_features[scale_feature].location)
        else
            Instance_Vector(dummy_v);
        if ( version > 0.92 )
        {
            if ( ( token = yylex() ) != INT_TOKEN )
                Instance_Error;
            if ( obj->o_num_features > scale_feature )
            {
                if ( lex_int == 1 )
                    obj->o_features[scale_feature].flags |= FeaturePref1;
                else if ( lex_int == 2 )
                    obj->o_features[scale_feature].flags |= FeaturePref2;
            }
        }
                
        if ((token = yylex()) != CONSTRAINTS_TOKEN)
            Instance_Error;
        Load_O_Constraints(&(obj->o_features[origin_feature].constraints),
                         &(obj->o_features[origin_feature].num_constraints));
        if ( obj->o_num_features > scale_feature )
            Load_O_Constraints(&(obj->o_features[scale_feature].constraints),
                             &(obj->o_features[scale_feature].num_constraints));
        else
        {
            dummy_cons = NULL;
            dummy_i = 0;
            Load_O_Constraints(&dummy_cons, &dummy_i);
            for ( i = ( version == 0.8 ? 3 : 0 ) ; i < dummy_i ; i++ )
                free(dummy_cons[i].c_specs);
            free(dummy_cons);
        }
        if ( version == 0.8 )
        {
            ConstraintPtr   dummy_feats = New(ConstraintData, 1);
            int             dummy_int = 1;
            Vector          body_space_ref;
            Matrix          body_axes;

            /* Load the old rotation constraints. */
            Load_O_Constraints(&dummy_feats, &dummy_int);
            if ( dummy_feats )
                free(dummy_feats);

            if ( obj->o_num_features > scale_feature )
            {
                /* Need to check forced. */
                body_axes.x = obj->o_features[major_feature].location;
                body_axes.y = obj->o_features[minor_feature].location;
                VCross(body_axes.x, body_axes.y, body_axes.z);
                MVMul(body_axes, obj->o_features[scale_feature].location,
                      body_space_ref);

                obj->o_features[scale_feature].constraints[0].c_forced =
                    IsZero(body_space_ref.x);
                obj->o_features[scale_feature].constraints[1].c_forced =
                    IsZero(body_space_ref.y);
                obj->o_features[scale_feature].constraints[2].c_forced =
                    IsZero(body_space_ref.z);
            }

            /* Load major and minor axis constraints. */
            if ( (token = yylex()) == MAJOR_TOKEN )
            {
                if ( obj->o_num_features > minor_feature )
                {
                    obj->o_features[major_feature].constraints =
                        More(obj->o_features[major_feature].constraints,
                             ConstraintData,
                             obj->o_features[major_feature].num_constraints+1);
                    Load_O_Constraint(obj->o_features[major_feature].constraints +
                        obj->o_features[major_feature].num_constraints);
                    obj->o_features[major_feature].constraints[
                        obj->o_features[major_feature].num_constraints].c_status
                        = TRUE;
                    obj->o_features[major_feature].num_constraints++;
                }
                else
                {
                    dummy_cons = New(ConstraintData, 1);
                    Load_O_Constraint(dummy_cons);
                    free(dummy_cons->c_specs);
                    free(dummy_cons);
                }
                token = yylex();
            }
            if ( token == MINOR_TOKEN )
            {
                if ( obj->o_num_features > minor_feature )
                {
                    obj->o_features[minor_feature].constraints =
                        More(obj->o_features[minor_feature].constraints,
                             ConstraintData,
                             obj->o_features[minor_feature].num_constraints+1);
                    Load_O_Constraint(obj->o_features[minor_feature].constraints +
                        obj->o_features[minor_feature].num_constraints);
                    obj->o_features[minor_feature].constraints[
                        obj->o_features[minor_feature].num_constraints].c_status
                        = TRUE;
                    obj->o_features[minor_feature].num_constraints++;
                }
                else
                {
                    dummy_cons = New(ConstraintData, 1);
                    Load_O_Constraint(dummy_cons);
                    free(dummy_cons->c_specs);
                    free(dummy_cons);
                }
                token = yylex();
            }
        }
        else
        {
            if ( obj->o_num_features > minor_feature )
            {
                Load_O_Constraints(&(obj->o_features[major_feature].constraints),
                            &(obj->o_features[major_feature].num_constraints));
                Load_O_Constraints(&(obj->o_features[minor_feature].constraints),
                            &(obj->o_features[minor_feature].num_constraints));
            }
            else
            {
                dummy_cons = NULL;
                dummy_i = 0;
                Load_O_Constraints(&dummy_cons, &dummy_i);
                for ( i = ( version == 0.8 ? 3 : 0 ) ; i < dummy_i ; i++ )
                    free(dummy_cons[i].c_specs);
                free(dummy_cons);
                dummy_cons = NULL;
                dummy_i = 0;
                Load_O_Constraints(&dummy_cons, &dummy_i);
                for ( i = ( version == 0.8 ? 3 : 0 ) ; i < dummy_i ; i++ )
                    free(dummy_cons[i].c_specs);
                free(dummy_cons);
            }
            if ( Obj_Is_Torus(obj) )
                Load_O_Constraints(&(obj->o_features[radius_feature].constraints),
                        &(obj->o_features[radius_feature].num_constraints));
            token = yylex();
        }
    }
    else
    {
        Load_O_Features(obj);
        token = yylex();
    }

    if ( token != DEPENDENTS_TOKEN || (token = yylex()) != INT_TOKEN )
        Instance_Error;
    obj->o_num_depend = lex_int;
    if ( lex_int )
        obj->o_dependents = New(Dependent, lex_int);
    else
        obj->o_dependents = NULL;
    for ( i = 0 ; i < obj->o_num_depend ; i++ )
    {
        if ( (token = yylex()) != INT_TOKEN )
            Instance_Error;
        obj->o_dependents[i].obj = (ObjectInstancePtr)lex_int;
        if ( (token = yylex()) != INT_TOKEN )
            Instance_Error;
        obj->o_dependents[i].count = (char)lex_int;
    }

    return obj;
}


static int
Load_O_Light_Info(AttributePtr l, GenericObject obj_type)
{
    int     token;
    float   s1, s2;
    int     si1, si2;

    if ( version > 0.94 + 0.0001 )
    {
        if ( ( token = yylex() ) != INT_TOKEN )
            Input_Error;
        l->defined = lex_int ? TRUE : FALSE;
    }
    else
        l->defined = TRUE;

    Load_O_Float(l->intensity.red);
    Load_O_Float(l->intensity.green);
    Load_O_Float(l->intensity.blue);
    if ( obj_type == spotlight_obj )
    {
        Load_O_Float(l->radius);
        Load_O_Float(l->tightness);
        if ( ( token = yylex() ) != INT_TOKEN )
            Input_Error;
        l->invert = lex_int ? TRUE : FALSE;
    }
    else
    {
        Load_O_Float(s1);
        si1 = (int)s1;
        Load_O_Float(s2);
        si2 = (int)s2;
        l->samples = ( si1 & 0xFF ) | ( si2 << 8 );
        if ( ( token = yylex() ) != INT_TOKEN )
            Input_Error;
        l->jitter = lex_int ? TRUE : FALSE;
    }

    return yylex();
}


/*  int
**  Load_O_Attributes(AttributePtr a, AttributePtr target)
**  Loads what attributes it can find into a.  Will always read one past,
**  so returns the last token read.
**  Sets flags to indicate what was read.
*/
static int
Load_O_Attributes(AttributePtr a, AttributePtr target)
{
    Renderer    rend;
    int token;
    int first = TRUE;
    int second = FALSE;

    /* Force explicit definition of attributes. */
    a->defined = FALSE;

    while (TRUE)
    {
        switch (token = yylex())
        {
            case INT_TOKEN:
                if ( first )
                {
                    Hash_Insert(load_hash, (unsigned long)lex_int,
                                (void*)target);
                    second = TRUE;
                }
                else if ( second )
                    a->defined = ( lex_int ? TRUE : FALSE );
                else
                    return token;
                break;

            case COLOUR_TOKEN:
                if ( version >= 0.92 )
                {
                    Load_O_Float(a->color.red);
                    Load_O_Float(a->color.green);
                    Load_O_Float(a->color.blue);
                }
                else
                {
                    if ((token = yylex()) != INT_TOKEN)
                        Input_Error;
                    a->color.red = lex_int / (double)MAX_UNSIGNED_SHORT;
                    if ((token = yylex()) != INT_TOKEN)
                        Input_Error;
                    a->color.green = lex_int / (double)MAX_UNSIGNED_SHORT;
                    if ((token = yylex()) != INT_TOKEN)
                        Input_Error;
                    a->color.blue = lex_int / (double)MAX_UNSIGNED_SHORT;
                }
                break;

            case DIFFUSE_TOKEN:
                Load_O_Float(a->diff_coef);
                break;

            case SPECULAR_TOKEN:
                Load_O_Float(a->spec_coef);
                Load_O_Float(a->spec_power);
                break;

            case TRANSPARENCY_TOKEN:
                Load_O_Float(a->transparency);
                break;

            case REFLECT_TOKEN:
                Load_O_Float(a->reflect_coef);
                break;

            case REFRACT_TOKEN:
                Load_O_Float(a->refract_index);
                break;

            case EXTEND_TOKEN:
                if ((token = yylex()) != INT_TOKEN)
                    Input_Error;
                a->use_extension = lex_int ? TRUE : FALSE;
                if ( version < 0.9101 )
                {
                    if ( ( token = yylex() ) != STRING_TOKEN )
                        Input_Error;
                    a->extension[target_renderer] = lex_string;
		    break;
                }
                for ( rend = NoTarget ; rend < Radiance ; rend++ )
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
		if ( ( token = yylex() ) != STRING_TOKEN )
		    Input_Error;
		if ( ( token = yylex() ) != STRING_TOKEN )
		    Input_Error;
                for ( ; rend < Renderman ; rend++ )
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
                if ( version >= 0.93 )
                    for ( ; rend < LastTarget ; rend++ )
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
                else if ( version >= 0.92 )
                    for ( ; rend < LastTarget ; rend++ )
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
                else if ( version == 0.911 )
                    for ( ; rend < LastTarget - 2 ; rend++ )
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
                break;

            case TRANSFORM_TOKEN:
                if ((token = yylex()) != INT_TOKEN)
                    Input_Error;
                a->use_obj_trans = lex_int ? TRUE : FALSE;
                break;

            case OPEN_TOKEN:
                if ((token = yylex()) != INT_TOKEN)
                    Input_Error;
                a->open = lex_int ? TRUE : FALSE;
                break;
                
            default:
                return token;
        }

        first = FALSE;
    }

    /* To keep the compiler happy. */
    return token;
}


static int
Load_O_Aliases(void ***aliases)
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

    *aliases = (void**)New(void*, LastTarget + 1);
    for ( i = NoTarget ; i < LastTarget + 1 ; i++ )
        (*aliases)[i] = NULL;
    for ( i = 0 ; i < Renderman && ( token = yylex() ) == STRING_TOKEN ; i++ )
        if ( lex_string[0] != '\0' )
            (*aliases)[i] = (void*)lex_string;
        else
            free(lex_string);
    for ( ; ( token = yylex() ) == STRING_TOKEN ; i++ )
        if ( lex_string[0] != '\0' )
            (*aliases)[i] = (void*)lex_string;
        else
            free(lex_string);

    return token;
}


static int
Load_O_LOD(LODInfoPtr *lods)
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
        Load_O_Float((*lods)->lods[i]);

    return yylex();
}


/*  int
**  Load_O_Lights(Boolean do_ambient)
**  Loads the lights.
*/
static int
Load_O_Lights(Boolean do_ambient)
{
    int     token;
    double  dummy;

    if ( version >= 0.92 )
    {
        if ( do_ambient )
        {
            Load_O_Float(ambient_light.red);
            Load_O_Float(ambient_light.green);
            Load_O_Float(ambient_light.blue);
        }
        else
        {
            Load_O_Float(dummy);
            Load_O_Float(dummy);
            Load_O_Float(dummy);
        }
    }
    else
    {
        if ((token = yylex()) != INT_TOKEN)
            Input_Error;
        if ( do_ambient )
            ambient_light.red = lex_int / (double)MAX_UNSIGNED_SHORT;
        if ((token = yylex()) != INT_TOKEN)
            Input_Error;
        if ( do_ambient )
            ambient_light.green = lex_int / (double)MAX_UNSIGNED_SHORT;
        if ((token = yylex()) != INT_TOKEN)
            Input_Error;
        if ( do_ambient )
            ambient_light.blue = lex_int / (double)MAX_UNSIGNED_SHORT;
    }

    return yylex();
}


static void
Load_O_Old_Specs(ConstraintSpecPtr s)
{
    OldSpec spec_type;
    int token;

    if ( (token = yylex()) != INT_TOKEN )
        Input_Error;

    spec_type = (ConstraintSpecType)lex_int;

    if ( spec_type == old_reference_spec )
    {
        if ( (token = yylex()) != INT_TOKEN )
            Input_Error;
        s->spec_data = (void*)lex_int;
    }
    else
        s->spec_data = NULL;

    switch ( spec_type )
    {
      case old_absolute_spec:
        s->spec_type = absolute_spec;
        break;
      case old_offset_spec:
        s->spec_type = offset_spec;
        break;
      case old_reference_spec:
        s->spec_type = reference_spec;
        break;
      case old_origin_spec:
        s->spec_type = feature_spec;
        s->spec_data = (void*)origin_feature;
        break;
      case old_scaling_spec:
        s->spec_type = feature_spec;
        s->spec_data = (void*)scale_feature;
        break;
      case old_major_spec:
        s->spec_type = feature_spec;
        s->spec_data = (void*)major_feature;
        break;
      case old_minor_spec:
        s->spec_type = feature_spec;
        s->spec_data = (void*)minor_feature;
        break;
      case old_radius_spec:
        s->spec_type = feature_spec;
        s->spec_data = (void*)radius_feature;
        break;

      case old_other_spec:
        s->spec_type = other_spec;
        break;
    }

    Load_O_Vector_f(s->spec_vector)
}

static void
Load_O_Spec(ConstraintSpecPtr s)
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

    Load_O_Vector_f(s->spec_vector)
}

/*  int
**  Load_O_Constraint(ConstraintPtr feat)
**  Loads a single constraint feature structure.
*/
static int
Load_O_Constraint(ConstraintPtr feat)
{
    int         token;
    OldConstraintType   old_spec_type;
    Boolean     reverse_specs = FALSE;
    int         i;

    VNew(0, 0, 0, feat->c_vector);
    VNew(0, 0, 0, feat->c_point);
    feat->c_value = feat->c_radius = feat->c_ratio = 0;
    feat->c_status = feat->c_forced = FALSE;

    if ( version < 0.925 )
    {
        if ( version == 0.8 )
        {
            reverse_specs = TRUE;
            token = yylex();
            if ( token == AXES_TOKEN )
            {
                if ( ( token = yylex() ) == PLANE_TOKEN )
                {
                    feat->c_type = axis_feature;
                    feat->c_spec_flags = HavePt1 | HavePt2 | HavePt3 | Axis;
                    feat->c_num_specs = 3;
                }
                else if ( token == LINE_TOKEN )
                {
                    feat->c_spec_flags = HavePt1 | HavePt2 | Axis;
                    feat->c_num_specs = 2;
                }
                else
                    Input_Error;
                feat->c_type = line_feature;
            }
            else if ( token == MID_TOKEN )
            {
                feat->c_radius = feat->c_ratio = 1;
                if ( (token = yylex()) == PLANE_TOKEN )
                {
                    feat->c_type = plane_feature;
                    feat->c_spec_flags = HavePt1 | HavePt2 | RatioPlane;
                    feat->c_num_specs = 2;
                }
                else if ( token == POINT_TOKEN )
                {
                    feat->c_type = point_feature;
                    feat->c_spec_flags = HavePt1 | HavePt2 | RatioPoint;
                    feat->c_num_specs = 2;
                }
                else
                    Input_Error
            }
            else if ( token == PLANE_TOKEN )
            {
                feat->c_type = plane_feature;
                feat->c_spec_flags = HavePt1 | HavePt2 | HavePt3;
                feat->c_num_specs = 3;
            }
            else if ( token == LINE_TOKEN )
            {
                feat->c_type = line_feature;
                feat->c_spec_flags = HavePt1 | HavePt2;
                feat->c_num_specs = 2;
            }
            else if ( token == POINT_TOKEN )
            {
                feat->c_type = point_feature;
                feat->c_spec_flags = HavePt1;
                feat->c_num_specs = 1;
            }
            else if ( token == REFERENCE_TOKEN )
            {
                if ( (token = yylex()) == LINE_TOKEN )
                {
                    feat->c_type = line_feature;
                    feat->c_num_specs = 2;
                }
                else if ( token == PLANE_TOKEN )
                {
                    feat->c_type = plane_feature;
                    feat->c_num_specs = 3;
                }
                else
                    Input_Error;
            }
            else if ( token == ORIGIN_TOKEN )
            {
                if ( (token = yylex()) == LINE_TOKEN )
                {
                    feat->c_type = line_feature;
                    feat->c_num_specs = 2;
                }
                else if ( token == PLANE_TOKEN )
                {
                    feat->c_type = plane_feature;
                    feat->c_num_specs = 3;
                }
                else
                    Input_Error;
            }
            else
                Input_Error;

            if ( (token = yylex()) != STRING_TOKEN )
                Input_Error
            feat->c_label = lex_string;

            if ( feat->c_type != point_feature )
                Load_O_Vector_f(feat->c_vector);
            Load_O_Vector_f(feat->c_point);

            if ( feat->c_type == plane_feature )
                feat->c_value = VDot(feat->c_vector, feat->c_point);
        }
        else if ( version == 0.9 )
        {
            reverse_specs = TRUE;
            if ( ( token = yylex() ) != INT_TOKEN )
                Input_Error
            old_spec_type = (ConstraintType)lex_int;

            if ( (token = yylex()) != STRING_TOKEN )
                Input_Error
            feat->c_label = lex_string;

            if ( ( token = yylex() ) != INT_TOKEN )
                Input_Error
            feat->c_status = lex_int ? TRUE : FALSE;

            if ( ( token = yylex() ) != INT_TOKEN )
                Input_Error
            feat->c_forced = lex_int ? TRUE : FALSE;

            switch ( old_spec_type )
            {
                case old_midplane_feature:
                    feat->c_type = plane_feature;
                    feat->c_spec_flags = HavePt1 | HavePt2 | RatioPlane;
                    feat->c_num_specs = 2;
                    feat->c_radius = feat->c_ratio = 1;
                    Load_O_Vector_f(feat->c_vector);
                    VScalarMul(feat->c_vector, -1, feat->c_vector);
                    Load_O_Vector_f(feat->c_point);
                    feat->c_value = VDot(feat->c_vector, feat->c_point);
                    break;

                case old_plane_feature:
                case old_orig_plane_feature:
                case old_ref_plane_feature:
                    feat->c_type = plane_feature;
                    feat->c_spec_flags =
                        HavePt1 | HavePt2 | HavePt3 | TwoisPt | ThreeisPt;
                    feat->c_num_specs = 3;
                    Load_O_Vector_f(feat->c_vector);
                    Load_O_Vector_f(feat->c_point);
                    feat->c_value = VDot(feat->c_vector, feat->c_point);
                    break;

                case old_axis_plane_feature:
                    feat->c_type = line_feature;
                    feat->c_spec_flags = HavePt1 | HavePt2 | HavePt3 | Axis;
                    feat->c_num_specs = 3;
                    Load_O_Vector_f(feat->c_vector);
                    Load_O_Vector_f(feat->c_point);
                    feat->c_value = VDot(feat->c_vector, feat->c_point);
                    break;

                case old_norm_plane_feature:
                    reverse_specs = FALSE;
                    feat->c_type = plane_feature;
                    feat->c_spec_flags = HavePt1 | HavePt2 | NormPlane;
                    feat->c_num_specs = 2;
                    Load_O_Vector_f(feat->c_vector);
                    Load_O_Vector_f(feat->c_point);
                    feat->c_value = VDot(feat->c_vector, feat->c_point);
                    break;

                case old_const_norm_plane_feature:
                    feat->c_type = plane_feature;
                    feat->c_spec_flags = HavePt1 | ConstNorm;
                    feat->c_num_specs = 1;
                    Load_O_Vector_f(feat->c_vector);
                    Load_O_Vector_f(feat->c_point);
                    feat->c_value = VDot(feat->c_vector, feat->c_point);
                    break;

                case old_line_feature:
                case old_orig_line_feature:
                case old_ref_line_feature:
                    feat->c_type = line_feature;
                    feat->c_spec_flags = HavePt1 | HavePt2;
                    feat->c_num_specs = 2;
                    Load_O_Vector_f(feat->c_vector);
                    Load_O_Vector_f(feat->c_point);
                    break;


                case old_axis_feature:
                    feat->c_type = line_feature;
                    feat->c_spec_flags = HavePt1 | HavePt2 | Axis;
                    feat->c_num_specs = 2;
                    Load_O_Vector_f(feat->c_vector);
                    Load_O_Vector_f(feat->c_point);
                    break;

                case old_sphere_feature:
                    feat->c_type = sphere_feature;
                    feat->c_spec_flags = HavePt1 | HavePt2;
                    feat->c_num_specs = 2;
                    Load_O_Vector_f(feat->c_point);
                    Load_O_Double(feat->c_radius);
                    break;

                case old_circle_feature:
                    feat->c_type = circle_feature;
                    feat->c_spec_flags =
                        HavePt1 | HavePt2 | HavePt3 | TwoisPt | ThreeisPt;
                    feat->c_num_specs = 3;
                    Load_O_Vector_f(feat->c_vector);
                    Load_O_Vector_f(feat->c_point);
                    Load_O_Double(feat->c_radius);
                    break;

                case old_dir_axis_feature:
                    feat->c_type = line_feature;
                    feat->c_spec_flags = HavePt1 | Axis | DirAxis;
                    feat->c_num_specs = 1;
                    Load_O_Vector_f(feat->c_vector);
                    break;

                case old_midpoint_feature:
                    feat->c_type = point_feature;
                    feat->c_spec_flags = HavePt1 | HavePt2 | RatioPoint;
                    feat->c_num_specs = 2;
                    feat->c_radius = feat->c_ratio = 1;
                    Load_O_Vector_f(feat->c_point);
                    break;

                case old_point_feature:
                    feat->c_type = point_feature;
                    feat->c_spec_flags = HavePt1;
                    feat->c_num_specs = 1;
                    Load_O_Vector_f(feat->c_point);
                    break;

                case old_ratio_point_feature:
                case old_ratio_plane_feature:
                case old_null_feature:
                case old_inconsistent_feature:
                    break;
            }
        }
        else
        {
            if ( ( token = yylex() ) != INT_TOKEN )
                Input_Error
            old_spec_type = (ConstraintType)lex_int;
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

            switch ( old_spec_type )
            {
                case old_plane_feature:
                    Load_O_Vector_f(feat->c_vector);
                    Load_O_Vector_f(feat->c_point);
                    feat->c_value = VDot(feat->c_vector, feat->c_point);
                    if ( feat->c_spec_flags & RatioPlane )
                    {
                        Load_O_Double(feat->c_radius);
                        Load_O_Double(feat->c_ratio);
                    }
                    break;

                case old_line_feature:
                    Load_O_Vector_f(feat->c_vector);
                    Load_O_Vector_f(feat->c_point);
                    break;


                case old_sphere_feature:
                    Load_O_Vector_f(feat->c_point);
                    Load_O_Double(feat->c_radius);
                    break;

                case old_circle_feature:
                    Load_O_Vector_f(feat->c_vector);
                    Load_O_Vector_f(feat->c_point);
                    Load_O_Double(feat->c_radius);
                    break;

                case old_point_feature:
                    Load_O_Vector_f(feat->c_point);
                    if ( feat->c_spec_flags & RatioPoint )
                    {
                        Load_O_Double(feat->c_radius);
                        Load_O_Double(feat->c_ratio);
                    }
                    break;

                case old_axis_plane_feature:
                case old_const_norm_plane_feature:
                case old_orig_line_feature:
                case old_ref_line_feature:
                case old_axis_feature:
                case old_dir_axis_feature:
                case old_norm_plane_feature:
                case old_midplane_feature:
                case old_orig_plane_feature:
                case old_ref_plane_feature:
                case old_midpoint_feature:
                case old_ratio_point_feature:
                case old_ratio_plane_feature:
                case old_null_feature:
                case old_inconsistent_feature:
                    break;
            }
            if ( ( token = yylex() ) != INT_TOKEN )
                Input_Error
            feat->c_num_specs = (int)lex_int;

        }

        feat->c_specs = New(ConstraintSpec, feat->c_num_specs);
        if ( reverse_specs )
            for ( i = feat->c_num_specs - 1 ; i >= 0 ; i-- )
                Load_O_Old_Specs(feat->c_specs + i);
        else
            for ( i = 0 ; i < feat->c_num_specs ; i++ )
                Load_O_Old_Specs(feat->c_specs + i);
    }
    else
    {
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
                Load_O_Vector_f(feat->c_vector);
                Load_O_Vector_f(feat->c_point);
                feat->c_value = VDot(feat->c_vector, feat->c_point);
                if ( feat->c_spec_flags & RatioPlane )
                {
                    Load_O_Double(feat->c_radius);
                    Load_O_Double(feat->c_ratio);
                }
                break;

            case line_feature:
                Load_O_Vector_f(feat->c_vector);
                Load_O_Vector_f(feat->c_point);
                break;


            case sphere_feature:
                Load_O_Vector_f(feat->c_point);
                Load_O_Double(feat->c_radius);
                break;

            case circle_feature:
                Load_O_Vector_f(feat->c_vector);
                Load_O_Vector_f(feat->c_point);
                Load_O_Double(feat->c_radius);
                break;

            case point_feature:
                Load_O_Vector_f(feat->c_point);
                if ( feat->c_spec_flags & RatioPoint )
                {
                    Load_O_Double(feat->c_radius);
                    Load_O_Double(feat->c_ratio);
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
                Load_O_Spec(feat->c_specs + i);
        else
            for ( i = 0 ; i < feat->c_num_specs ; i++ )
                Load_O_Spec(feat->c_specs + i);
    }

    return token;
}


/*  int
**  Load_O_Constraints(ConstraintPtr*, int*)
**  Loads a set of Constraints and activity.
*/
static int
Load_O_Constraints(ConstraintPtr *ret_cons, int *ret_num)
{
    ConstraintData  current;
    int start_num = *ret_num;
    int token;
    int i;


    if ( (token = yylex()) != INT_TOKEN )
        Input_Error;
    if ( version == 0.8 )
    {
        for ( i = 3 ; i < start_num ; i++ )
        {
            free((*ret_cons)[i].c_specs);
            (*ret_cons)[i].c_specs = NULL;
        }
        *ret_num = lex_int + 3;
    }
    else
    {
        for ( i = 0 ; i < start_num ; i++ )
        {
            free((*ret_cons)[i].c_specs);
            (*ret_cons)[i].c_specs = NULL;
        }
        *ret_num = lex_int;
    }
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
    for ( i = ( version == 0.8 ? 3 : 0 ) ; i < *ret_num ; i++ )
    {
        Load_O_Constraint(&current);
        (*ret_cons)[i] = current;
    }

    if ( version == 0.8 )
    {
        /* Need to load active list. */
        if ( ( token = yylex() ) != ACTIVE_TOKEN )
            Input_Error
        for ( i = 0 ; i < *ret_num ; i++ )
        {
            if ( ( token = yylex() ) != INT_TOKEN )
                Input_Error
            (*ret_cons)[i].c_status = lex_int ? TRUE : FALSE;
        }
    }

    return 1;
}


static void
Load_O_Features(ObjectInstancePtr obj)
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
        Load_O_Vector_f(obj->o_features[i].location);
        obj->o_features[i].num_constraints = 0;
        Load_O_Constraints(&(obj->o_features[i].constraints),
                         &(obj->o_features[i].num_constraints));
        if ( yylex() != INT_TOKEN )
            Input_Error
        obj->o_features[i].flags = (int)lex_int;
    }
}


/*  int
**  Load_O_Wireframe(WireframePtr*, Boolean *full, Boolean simple)
**  Loads a wireframe and returns the next token.
**  It exits on error, because the base types must be entered properly.
*/
static int
Load_O_Wireframe(WireframePtr *wire, Boolean *full, Boolean simple,
		 Boolean triangulate)
{
    HashTable	attrib_hash = Hash_New_Table();
    AttributePtr    attribs;
    int token;
    WireframePtr    res;
    int i, j;

    res = New(Wireframe, 1);

#define Wireframe_Error \
    { \
        fprintf(stderr, "Malformed wireframe line %d\n", line_num); \
        Sced_Exit(1); \
    }

#define Wireframe_Float(f) \
    { \
        if ((token = yylex()) == INT_TOKEN) \
            f = (double)lex_int; \
        else if (token == FLOAT_TOKEN) \
            f = lex_float; \
        else \
            Wireframe_Error \
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
        Wireframe_Error
    if ( simple && version < 0.9209 )
    {
        res->num_vertices = lex_int;
        res->num_real_verts = res->num_vertices - 1;
    }
    else
    {
        res->num_vertices = lex_int + 1;
        res->num_real_verts = lex_int;
    }
    res->vertices = New(Vector, res->num_vertices);
    if ( simple && version < 0.9209 )
        for ( i = 0 ; i < res->num_vertices ; i++ )
            Wireframe_Vector(res->vertices[i])
    else
    {
        for ( i = 0 ; i < res->num_real_verts ; i++ )
            Wireframe_Vector(res->vertices[i])
        VNew(0, 0, 0, res->vertices[i]);
    }


    if ( ( token = yylex() ) != INT_TOKEN )
        Wireframe_Error
    res->num_faces = lex_int;
    res->faces = New(Face, res->num_faces);
    for ( i = 0 ; i < res->num_faces ; i++ )
    {
        if ( ( token = yylex() ) != INT_TOKEN )
            Wireframe_Error
        res->faces[i].num_vertices = lex_int;
        res->faces[i].vertices = New(int, res->faces[i].num_vertices);
        res->faces[i].normals = New(int, res->faces[i].num_vertices);
        for ( j = 0 ; j < res->faces[i].num_vertices ; j++ )
        {
            if ( ( token = yylex() ) != INT_TOKEN )
                Wireframe_Error
            res->faces[i].vertices[j] = lex_int;
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
                Wireframe_Error
            Wireframe_Float(res->faces[i].normal.y);
            Wireframe_Float(res->faces[i].normal.z);
        }
    }

    res->num_attribs = 0;
    res->attribs = NULL;
    for ( i = 0 ; i < res->num_faces ; i++ )
    {
	if ( res->faces[i].face_attribs )
	{
	    attribs = res->faces[i].face_attribs;
	    if ( ( res->faces[i].face_attribs =
		    Hash_Get_Value(attrib_hash,
				   (unsigned long)res->faces[i].face_attribs) )
		    == (void*)-1 )
	    {
		res->num_attribs++;
		res->attribs = More(res->attribs, AttributePtr,
				    res->num_attribs);
		res->attribs[res->num_attribs-1] = New(Attributes, 1);
		Attribute_Copy(res->attribs[res->num_attribs-1], attribs);
		Hash_Insert(attrib_hash, (unsigned long)attribs,
			    (void*)res->attribs[res->num_attribs-1]);
		res->faces[i].face_attribs = res->attribs[res->num_attribs-1];
	    }
	}
    }

    if ( ( token = yylex() ) == NORMAL_TOKEN )
    {
        res->num_normals = res->num_real_verts;
        res->normals = New(Vector, res->num_normals);
        for ( i = 0 ; i < res->num_normals ; i++ )
            Wireframe_Vector(res->normals[i])
        if ( simple && version < 0.9209 )
        {
            Vector  dummy;
            Wireframe_Vector(dummy);
        }
        token = yylex();
        for ( i = 0 ; i < res->num_faces ; i++ )
            for ( j = 0 ; j < res->faces[i].num_vertices ; j++ )
                res->faces[i].normals[j] = res->faces[i].vertices[j];
    }
    else
        Wireframe_Assign_Vertex_Normals(res, 45.0 * M_PI / 180.0);

    res->edges = NULL;

    if ( triangulate )
    {
	*wire = Wireframe_Triangulate(res);
	Wireframe_Destroy(res);
    }
    else
	*wire = res;
    Hash_Free(attrib_hash);

    return token;

#undef Wireframe_Error
#undef Wireframe_Float
#undef Wireframe_Vector

}


static int
Load_O_Wireframe_Attributes(AttributePtr *attribs, int num)
{
    int token;
    int i;

    token = yylex();
    for ( i = 0 ; i < num ; i++ )
    {
        attribs[i] = Attribute_New(NULL, TRUE);
        if ( token != ATTRIBUTES_TOKEN )
            Input_Error;
        token = Load_O_Attributes(attribs[i], attribs[i]);
    }

    return token;
}



/*  int
**  Load_O_CSG_Trees()
**  Loads a sequence of CSG trees and sets them up in the CSG window.
*/
static int
Load_O_CSG_Trees()
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
        new_tree = Load_O_CSG_Tree(NULL);

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
Load_O_CSG_Tree(CSGNodePtr parent)
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
        res->csg_instance = Load_O_Instance(label);

        Insert_Element(&(new_instances), res->csg_instance);

        free(label);
        if ( ! res->csg_instance )
        {
            fprintf(stderr, "Unable to load CSG instance line %d\n", line_num);
            Sced_Exit(1);
        }

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

        res->csg_left_child = Load_O_CSG_Tree(res);
        res->csg_right_child = Load_O_CSG_Tree(res);
    }

    return res;
}


static void
Old_Refresh_Spec(ConstraintSpecPtr spec, ObjectInstancePtr obj, void *ptr,
             void *ptr2, int num)
{
    if ( Spec_Is_Dependent(spec->spec_type) )
        spec->spec_data =
            Hash_Get_Value(load_hash, (unsigned long)spec->spec_data);
}

void
Old_Refresh_Constraint_References(ConstraintPtr options, int num)
{
    int i;

    for ( i = 0 ; i < num ; i++ )
        Constraint_Manipulate_Specs(options + i,
                                    NULL, NULL, NULL, 0, Old_Refresh_Spec);
}


static void
Old_Refresh_Constraint_Pointers(ObjectInstancePtr obj)
{
    Resultant   res;
    Vector      pos;
    Vector      obj_axes_z;
    int preference;
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
        Old_Refresh_Constraint_References(obj->o_features[i].constraints,
                                      obj->o_features[i].num_constraints);

    if ( version == 0.8 )
    {
        /* Must update constraints too, because scaling and origin points
        ** will be wrong.
        */
        if ( obj->o_num_features >= minor_feature )
            VCross(obj->o_features[major_feature].location,
                   obj->o_features[minor_feature].location, obj_axes_z);
        else
            VNew(0, 1, 0, obj_axes_z);
        for ( i = 0 ; i < obj->o_num_features ; i++ )
            for ( j = 0 ; j < obj->o_features[i].num_constraints ; j++ )
                Edit_Update_Constraint_Specs(obj->o_features[i].constraints + j,
                            obj->o_world_verts + (obj->o_num_vertices - 1),
                            obj->o_features, &obj_axes_z);
    }

    if ( version < 0.921 )
    {
        /* Need to set preferences for constraint soluion. */
        Constraint_Solve_System(obj,
                        obj->o_features[origin_feature].constraints,
                        obj->o_features[origin_feature].num_constraints, &res);
        if ( res.feature_1.c_type == point_feature &&
             res.feature_2.c_type == point_feature )
        {
            VAdd(obj->o_features[origin_feature].location,
                 obj->o_world_verts[obj->o_num_vertices - 1], pos);
            Find_Required_Motion(&pos, &res, FALSE, &preference);
            if ( preference == 1 )
                obj->o_features[origin_feature].flags |= FeaturePref1;
            else
                obj->o_features[origin_feature].flags |= FeaturePref2;
        }
        if ( obj->o_num_features > scale_feature )
        {
            Constraint_Solve_System(obj,
                        obj->o_features[scale_feature].constraints,
                        obj->o_features[scale_feature].num_constraints, &res);
            if ( res.feature_1.c_type == point_feature &&
                 res.feature_2.c_type == point_feature )
            {
                VAdd(obj->o_features[scale_feature].location,
                     obj->o_world_verts[obj->o_num_vertices - 1], pos);
                Find_Required_Motion(&pos, &res, FALSE, &preference);
                if ( preference == 1 )
                    obj->o_features[scale_feature].flags |= FeaturePref1;
                else
                    obj->o_features[scale_feature].flags |= FeaturePref2;
            }
        }
        if ( obj->o_num_features > radius_feature )
        {
            Constraint_Solve_System(obj,
                        obj->o_features[radius_feature].constraints,
                        obj->o_features[radius_feature].num_constraints, &res);
            if ( res.feature_1.c_type == point_feature &&
                 res.feature_2.c_type == point_feature )
            {
                VAdd(obj->o_features[radius_feature].location,
                     obj->o_world_verts[obj->o_num_vertices - 1], pos);
                Find_Required_Motion(&pos, &res, FALSE, &preference);
                if ( preference == 1 )
                    obj->o_features[radius_feature].flags |= FeaturePref1;
                else
                    obj->o_features[radius_feature].flags |= FeaturePref2;
            }
        }
    }
}

/*  void
**  Old_Refresh_Instance_Pointers()
**  Changes all the instance pointers which are stale (because they were
**  read from file) to new the current instance pointers.
*/
static void
Old_Refresh_Instance_Pointers(Boolean new_cam)
{
    InstanceList    current;

    for ( current = new_instances ; current ; current = current->next )
        Old_Refresh_Constraint_Pointers(current->the_instance);

    if ( new_cam )
        Old_Refresh_Constraint_Pointers(&camera_object);

    Free_Selection_List(new_instances);
    new_instances = NULL;
}


/* Create a wireframe object to use for squares, if required.
*/
static void
Load_O_Create_Square_Wireframe()
{
    WireframePtr    wire;
    WireframePtr    tri_wire;
    int             i;

    wire = Generic_Square_Wireframe();

    tri_wire = New(Wireframe, 1);
    tri_wire->num_vertices = 5;
    tri_wire->num_real_verts = 4;
    tri_wire->vertices = New(Vector, 5);
    VNew(1, 1, 0, tri_wire->vertices[0]);
    VNew(-1, 1, 0, tri_wire->vertices[1]);
    VNew(-1, -1, 0, tri_wire->vertices[2]);
    VNew(1, -1, 0, tri_wire->vertices[3]);
    VNew(0, 0, 0, tri_wire->vertices[4]);
    tri_wire->num_normals = 1;
    tri_wire->normals = New(Vector, tri_wire->num_normals);
    VNew(0, 0, 1, tri_wire->normals[0]);
    tri_wire->edges = NULL;
    tri_wire->faces = New(Face, 2);
    tri_wire->num_faces = 2;
    tri_wire->faces[0].num_vertices = tri_wire->faces[1].num_vertices = 3;
    tri_wire->faces[0].vertices = New(int, 3);
    tri_wire->faces[0].normals = New(int, 3);
    tri_wire->faces[0].vertices[0] = 0;
    tri_wire->faces[0].vertices[1] = 3;
    tri_wire->faces[0].vertices[2] = 1;
    tri_wire->faces[1].vertices = New(int, 3);
    tri_wire->faces[1].normals = New(int, 3);
    tri_wire->faces[1].vertices[0] = 1;
    tri_wire->faces[1].vertices[1] = 3;
    tri_wire->faces[1].vertices[2] = 2;
    for ( i = 0 ; i < 3 ; i++ )
    {
        tri_wire->faces[0].normals[i] = 0;
        tri_wire->faces[1].normals[i] = 0;
    }
    VNew(0, 0, 1, tri_wire->faces[0].normal);
    VNew(0, 0, 1, tri_wire->faces[1].normal);
    tri_wire->faces[0].face_attribs = tri_wire->faces[1].face_attribs = NULL;
    tri_wire->attribs = NULL;
    tri_wire->num_attribs = 0;

    square_wire_base = Add_Wireframe_Base_Object("square", wire, tri_wire, 0);
    Base_Add_Select_Option(square_wire_base);
}

