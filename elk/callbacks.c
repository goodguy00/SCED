/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/elk/RCS/callbacks.c,v 1.0 1997/05/06 20:43:53 schenney Exp $
 *
 * $Log: callbacks.c,v $
 * Revision 1.0  1997/05/06 20:43:53  schenney
 * Initial revision
 *
 */
#include "elk_private.h"
#include <View.h>

int T_Object3d;
int T_Viewport;
int T_CSGNode;

Object  Sym_Rayshade;
Object  Sym_Radiance;
Object  Sym_POVray;
Object  Sym_Renderman;
Object  Sym_VRML;
Object	Sym_OFF;
Object	Sym_OpenGL;
Object	Sym_Sced;

Object	Sym_Global;
Object	Sym_Local;

InstanceList
Elk_List_To_Instances(Object listobj)
{
    Object          list_elmt_obj;
    InstanceList    result = NULL;

    Check_List(listobj);

    if ( Nullp(listobj) )
        return NULL;

    for ( list_elmt_obj = listobj ;
          ! Nullp(list_elmt_obj) ;
          list_elmt_obj = Cdr(list_elmt_obj) )
    {
        Check_Type(Car(list_elmt_obj), T_Object3d);
        Insert_Element(&result, ELKOBJECT3(Car(list_elmt_obj))->object3);
    }

    return result;
}


/* These window setting functions make the appropriate window active
 * for elk commands.
 */
static Object
Elk_Set_Window_CSG()
{
    if ( ! csg_window.shell )
        Create_CSG_Display();

    elk_active = ELK_CSG_WINDOW;
    elk_window = &csg_window;

    return Void;
}

static Object
Elk_Set_Window_Scene()
{
    elk_active = ELK_MAIN_WINDOW;
    elk_window = &main_window;

    return Void;
}

static Object
Elk_Set_Window_Agg()
{
    if ( ! agg_window.shell )
        Create_Agg_Display();

    elk_active = ELK_AGG_WINDOW;
    elk_window = &agg_window;

    return Void;
}


/***********************************************************************
 *
 * Description: elk_csg_open() is the C callback for the scheme function
 *              "csg-open", which opens the CSG window with a list of
 *              instances.
 *
 * Scheme Example: (csg-open '())
 *
 **********************************************************************/
static Object
Elk_CSG_Open(Object listobj)
{
    InstanceList    instances = Elk_List_To_Instances(listobj);

    CSG_Window_Popup(NULL, &instances, NULL);

    return Void;
}


static Object
Elk_Agg_Open(Object listobj)
{
    InstanceList    instances = Elk_List_To_Instances(listobj);

    Agg_Window_Popup(NULL, &instances, NULL);

    return Void;
}


/***********************************************************************
 *
 * Description: elk_get_selectlist() is the C callback for the scheme
 *      function "get-selectlist". This function is used to
 *      get a list of objects currently selected.
 *
 * Scheme example: (get-selectlist)
 *
 * Return value: Returns the a scheme list object containing all
 *       the objects currently selected.
 *
 ***********************************************************************/
static Object
Elk_Get_Selectlist()
{
    Object obj;
    InstanceList elmt;
    Object list;

    list = Null;
    for (elmt = elk_window->selected_instances ; elmt; elmt = elmt->next)
    {
        obj = Alloc_Object(sizeof(Elkobject3), T_Object3d, 0);
        ELKOBJECT3(obj)->object3 = elmt->the_instance;
        list = Cons(obj, list);
    }
    return list;
}


/***********************************************************************
 *
 * Description: elk_clear() is the c callback for the scheme function
 *              "clear", which clears the screens.
 *
 **********************************************************************/
static Object
Elk_Clear()
{
    Destroy_World(FALSE);

    return Void;
}

/***********************************************************************
 *
 * Description: elk_reset() is the C callback for the scheme function
 *              "reset", which clears the screen and any base objects.
 *
 **********************************************************************/
static Object
Elk_Reset()
{
    Destroy_World(TRUE);

    return Void;
}


/**********************************************************************
 *
 * Description: elk_preview is the C callback for the scheme function
 *              preview, which causes a scene to be previewed.
 *              rayobj must be one of 'rayshade, 'genray, 'povray,
 *              'genscan or 'radiance.
 *              If listobj is empty, all instances are previewed.
 *              If width or height is 0, the current screen size is used.
 *
 * Parameters: rayobj: A symbol for the target renderer.
 *             listobj: A list of object to render, or the empty list.
 *             widthobj, heightobj: The width and height of the output.
 *
 *********************************************************************/
static Object
Elk_Preview(Object rayobj, Object instobj, Object widthobj, Object heightobj,
            Object cobj)
{
    Renderer        target;
    InstanceList    instances;
    int             width, height;
    Dimension       temp_dim;

    /* Check types. */
    Check_Type(rayobj, T_Symbol);
    Check_List(instobj);
    Check_Integer(widthobj);
    Check_Integer(heightobj);
    Check_Type(cobj, T_Symbol);

    /* Determine target. */
    if ( EQ(rayobj, Sym_Rayshade) )
        target = Rayshade;
    else if ( EQ(rayobj, Sym_Radiance) )
        target = Radiance;
    else if ( EQ(rayobj, Sym_Renderman) )
        target = Renderman;
    else if ( EQ(rayobj, Sym_VRML) )
        target = VRML;
    else if ( EQ(rayobj, Sym_POVray) )
        target = POVray;
    else
        Primitive_Error("Invalid Renderer: ~s", rayobj);

    /* Get instances. */
    if ( Nullp(instobj) )
        instances = Merge_Selection_Lists(elk_window->all_instances, NULL);
    else
        instances = Elk_List_To_Instances(instobj);

    /* Get sizes. */
    width = Get_Integer(widthobj);
    if ( ! width )
    {
        XtVaGetValues(elk_window->view_widget, XtNwidth, &temp_dim, NULL);
        width = (int)temp_dim;
    }
    height = Get_Integer(heightobj);
    if ( ! height )
    {
        XtVaGetValues(elk_window->view_widget, XtNheight, &temp_dim, NULL);
        height = (int)temp_dim;
    }

    Perform_Preview(elk_window, target, instances, width, height, Truep(cobj));

    return Void;
}


static Object
Elk_Target(Object rayobj)
{
    /* Check types. */
    Check_Type(rayobj, T_Symbol);

    /* Determine target. */
    if ( EQ(rayobj, Sym_Rayshade) )
        target_renderer = Rayshade;
    else if ( EQ(rayobj, Sym_Radiance) )
        target_renderer = Radiance;
    else if ( EQ(rayobj, Sym_Renderman) )
        target_renderer = Renderman;
    else if ( EQ(rayobj, Sym_VRML) )
        target_renderer = VRML;
    else if ( EQ(rayobj, Sym_POVray) )
        target_renderer = POVray;
    else
        Primitive_Error("Invalid Renderer: ~s", rayobj);

    return Void;
}

static Object
Elk_Export(Object strobj)
{
    SceneStruct export_scene;
    FILE        *outfile;

    if ( target_renderer == NoTarget )
        return Void;

    Check_Type(strobj, T_String);

    outfile = fopen(STRING(strobj)->data, "w");
    if ( ! outfile )
    {
        Primitive_Error("Cannot open file: ~s", STRING(strobj)->data);
	return Void;
    }

    export_scene.camera = camera;
    export_scene.light = NULL;
    export_scene.ambient = ambient_light;
    export_scene.instances = main_window.all_instances;
    if ( outfile )
        Export_File(outfile, STRING(strobj)->data, &export_scene, FALSE);

    return Void;
}


static Object
Elk_Export_Base(Object base_obj, Object rend_obj, Object file_obj)
{
    BaseObjectPtr   base;

    Check_Type(base_obj, T_String);
    Check_Type(rend_obj, T_Symbol);
    Check_Type(file_obj, T_String);

    if ( ! ( base = Get_Base_Object_From_Label(STRING(base_obj)->data) ) )
    {
        Primitive_Error("Unknown base object: ~s", base_obj);
        return Void;    /* unknown object type */
    }

    if ( EQ(rend_obj, Sym_Sced) )
	Save_Base(base, STRING(file_obj)->data);
#if VRML_SUPPORT
    else if ( EQ(rend_obj, Sym_VRML) )
	VRML_Export_Base(base, STRING(file_obj)->data);
#endif
    else if ( EQ(rend_obj, Sym_OpenGL) )
	OpenGL_Export_Base(base, STRING(file_obj)->data);
    else if ( EQ(rend_obj, Sym_OFF) )
	OFF_Export_Base(base, STRING(file_obj)->data);
    else
        Primitive_Error("Invalid target: ~s", rend_obj);

    return Void;
}


static Object
Elk_Quit()
{
    Sced_Exit(0);

    return Void;
}


/***********************************************************************
 *
 * Description: init_callbacks() is the main function for initializing
 *      the extended types in the scheme interpreter.
 *
 ***********************************************************************/
int
Elk_Init_Callbacks()
{
    T_Object3d = Define_Type(0, "object3d", NOFUNC,
               sizeof(Elkobject3),
               Elk_Object3d_Equiv, Elk_Object3d_Equal,
               Elk_Object3d_Print, NOFUNC);
    T_Viewport = Define_Type(0, "viewport", NOFUNC,
               sizeof(Elkviewport),
               Elk_Viewport_Equiv, Elk_Viewport_Equal,
               Elk_Viewport_Print, NOFUNC);
    T_CSGNode = Define_Type(0, "csgnode", NOFUNC,
               sizeof(Elkcsgnode),
               Elk_CSG_Equiv, Elk_CSG_Equal,
               Elk_CSG_Print, NOFUNC);

    /*
     * Symbols for csg ops.
     */
    Define_Symbol(&Sym_Union, "union");
    Define_Symbol(&Sym_Intersection, "intersection");
    Define_Symbol(&Sym_Difference, "difference");

    /*
     * Renderer symbols
     */
    Define_Symbol(&Sym_POVray, "povray");
    Define_Symbol(&Sym_Rayshade, "rayshade");
    Define_Symbol(&Sym_Radiance, "radiance");
    Define_Symbol(&Sym_Renderman, "renderman");
    Define_Symbol(&Sym_VRML, "vrml");
    Define_Symbol(&Sym_OFF, "off");
    Define_Symbol(&Sym_OpenGL, "opengl");
    Define_Symbol(&Sym_Sced, "sced");

    /*
     * Symbols for coord-system references.
     */
    Define_Symbol(&Sym_Global, "global");
    Define_Symbol(&Sym_Local, "local");

    /*
     * Callbacks for setting the current window.
     */
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Set_Window_Agg,
                     "set-window-aggregate", 0, 0, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Set_Window_CSG,
                     "set-window-csg", 0, 0, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Set_Window_Scene,
                     "set-window-scene", 0, 0, EVAL);

    /*
     * define callbacks for manipulating objects
     */
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Object3d_Create,
                     "object3d-create", 1, 1, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Object3d_Destroy,
                     "object3d-destroy", 1, 1, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Object3d_Position,
                     "object3d-position", 4, 4, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Object3d_Displace,
                     "object3d-displace", 4, 4, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Object3d_Scale,
                     "object3d-scale", 4, 4, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Object3d_Rotate,
                     "object3d-rotate", 4, 4, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Object3d_Set_Control,
		     "object3d-set-control", 6, 6, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Object3d_Wireframe_Query,
                     "object3d-wireframe-query", 1, 1, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Object3d_Wireframe_Level,
                     "object3d-wireframe-level", 2, 2, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Object3d_Attribs_Define,
                     "object3d-attribs-define", 2, 2, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Object3d_Color,
                     "object3d-color", 4, 4, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Object3d_Diffuse,
                     "object3d-diffuse", 2, 2, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Object3d_Specular,
                     "object3d-specular", 3, 3, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Object3d_Reflect,
                     "object3d-reflect", 2, 2, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Object3d_Transparency,
                     "object3d-transparency", 3, 3, EVAL);

    /*
     * Selection list interface.
     */
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Get_Selectlist,
                     "get-selectlist", 0, 0, EVAL);

    /*
     * defined callbacks for manipulating the viewport
     */
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Viewport_Create,
                     "viewport-create", 0, 0, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Viewport_Destroy,
                     "viewport-destroy", 1, 1, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Viewport_Lookat,
                     "viewport-lookat", 4, 4, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Viewport_Position,
                     "viewport-position", 4, 4, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Viewport_Upvector,
                     "viewport-upvector", 4, 4, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Viewport_Distance,
                     "viewport-distance", 2, 2, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Viewport_Eye,
                     "viewport-eye", 2, 2, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Viewport_Setup,
                     "viewport-setup", 1, 1, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Viewport_Zoom,
                     "viewport-zoom", 2, 2, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Viewport_To_Camera,
                     "viewport-to-camera", 1, 1, EVAL);

    /* CSG callbacks. */
    Define_Primitive((Object(*)(ELLIPSIS))Elk_CSG_Open,
                     "csg-open", 1, 1, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_CSG_Node,
                     "csg-node", 1, 1, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_CSG_Hide,
                     "csg-hide", 1, 1, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_CSG_Display,
                     "csg-display", 1, 1, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_CSG_Attach,
                     "csg-attach", 3, 3, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_CSG_Complete,
                     "csg-complete", 2, 2, EVAL);

    /* Aggreate callbacks. */
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Agg_Open,
		     "aggregate-open", 1, 1, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Agg_Complete,
		     "aggregate-complete", 2, 2, EVAL);

    /* Destroying the world. */
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Clear, "clear", 0, 0, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Reset, "reset", 0, 0, EVAL);

    /* Preview. */
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Preview, "preview", 4, 4, EVAL);

    /* Target. */
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Target, "target", 1, 1, EVAL);

    /* Export. */
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Export, "export", 1, 1, EVAL);
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Export_Base,
		     "export-base", 3, 3, EVAL);

    /* Quit. */
    Define_Primitive((Object(*)(ELLIPSIS))Elk_Quit, "quit-sced", 0, 0, EVAL);

    return 0;
}
