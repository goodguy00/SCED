/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/sced.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: sced.c,v $
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
**  sced.c : The master file.  Actually, it doesn't do too much.  Just sets
**              up a few widgets and lets it run.
**
**  Created: 20/03/94
*/

#include <sced.h>
#include <layers.h>
#include <select_point.h>
#include <signal.h>
#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/param.h>
#include <X11/cursorfont.h>
#include <X11/Shell.h>
#include <X11/Xaw/SimpleMenu.h>

#if ELK_SUPPORT
#include <elk.h>
#endif /* ELK_SUPPORT */

#if ( ! defined(MAXPATHLEN) )
#define MAXPATHLEN 1024
#endif


/*
**  Declaration of global variables.
*/
XtAppContext    app_context;    /* The application context for the program. */
WindowInfo      main_window;
WindowInfo      csg_window;
WindowInfo      agg_window;
ScedResources   sced_resources; /* Application resources. */

double  version = VERSION_FLOAT;

Renderer    target_renderer = NoTarget; /* The target renderer. */

Boolean changed_scene = FALSE;  /* Whether or not the scene has been modified.*/

/* Various file names. */
char    start_directory_name[MAXPATHLEN];
char    *io_file_name = NULL;

/* GCs for drawing the axes. */
GC              axis_gcs[3];

/* Preferences. */
ScedPreferences sced_preferences;

/* Temporary filenames (for removal). */
char    **temp_filenames = NULL;
int     num_temp_files = 0;

/* Translations for top level windows, those that stay open. */
char    *top_level_translations = "<Message>:Handle_Client_Message()\n";

extern void Create_Main_Display();

extern void Main_Button_Down_Action(Widget, XEvent*, String*, Cardinal*);
extern void Main_Motion_Action(Widget, XEvent*, String*, Cardinal*);
extern void Main_Button_Up_Action(Widget, XEvent*, String*, Cardinal*);
extern void Main_Double_Click_Action(Widget, XEvent*, String*, Cardinal*); // @suppress("Unused function declaration")

extern Widget   edit_shell;

static int  Sced_X_Error_Handler(Display*, XErrorEvent*);
static void Set_Defaults();
static void Initialize_Sced(int, char**, char**);
static void Set_Signal_Handlers();
static void Handle_Client_Messages(Widget, XEvent*, String*, Cardinal*);

static char *defaults_filename = NULL;


/* Fallback resources. Geometries. */
String  fallbacks[] = {
    "Sced.geometry: 800x600",
    "Sced.csgShell.geometry: 800x600",
    "Sced.aggShell.geometry: 800x600",
    "Sced.editShell.geometry: 800x180",
    "Sced.elkShell.geometry: 600x400",
    "Sced.newObject.geometry: 540x300",
    "Sced.wireSelectShell.geometry: 400x300",
    "Sced.aggSelectShell.geometry: 400x300",
    "Sced.csgSelectShell.geometry: 400x300",
    "Sced.csgReferenceShell.geometry: 400x400",
    "Sced.specAttributesShell.geometry: 400x300",
    "Sced.aliasShell.geometry: 300x200",
    "Sced.fileShell.geometry: 200x400",
    NULL
    };

/* Actions. */
XtActionsRec    actions[] = {
#if ELK_SUPPORT
    { "elk_eval_action", Elk_Eval_Action },
#endif /* ELK_SUPPORT */
    { "Handle_Client_Message", Handle_Client_Messages },
    { "Main_Button_Down", Main_Button_Down_Action },
    { "Main_Motion", Main_Motion_Action },
    { "Main_Button_Up", Main_Button_Up_Action },
    { "Highlight_Object", Select_Highlight_Object },
    { "Select_Point", Select_Point_Action },
    { "Highlight_Closest", Select_Highlight_Action },
    { "Rename_Object", Rename_Action_Func },
    { "Apply_Button", Apply_Button_Action },
    { "Zoom_Action", Zoom_Action_Func },
    { "Defaults_Action", Defaults_Pathname_Action },
    { "Copy_Action", Copy_Action_Func },
    { "Change_Magnify", Magnify_Step_Action_Func },
    { "Ambient_Action", Ambient_Action_Func },
    { "Light_Action", Light_Action_Func },
    { "View_Name_Action", View_Name_Action_Func },
    { "csg_tree_notify", CSG_Tree_Notify_Func },
    { "menu_notify", CSG_Menu_Button_Up_Func },
    { "csg_motion", CSG_Tree_Motion_Func },
    { "csg_complete", CSG_Complete_Action_Func },
#if RENDERMAN_SUPPORT
    { "Renderman_File_Action", Renderman_Action_Func },
#endif
    { "New_Layer_Action", New_Layer_Action_Function },
    { "File_Action", File_Action_Function }
    };

/* Resources. */
#define Offset(field)   XtOffsetOf(ScedResources, field)
XtResource  resources[] = {
    {"xAxisColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(x_axis_color),
     XtRString, "red"},
    {"yAxisColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(y_axis_color),
     XtRString, "green"},
    {"zAxisColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(z_axis_color),
     XtRString, "blue"},
    {"axisWidth", XtCWidth, XtRInt, sizeof(int), Offset(axis_width),
     XtRImmediate, (XtPointer)2},
    {"xAxisLength", XtCLength, XtRInt, sizeof(int), Offset(x_axis_length),
     XtRImmediate, (XtPointer)2},
    {"yAxisLength", XtCLength, XtRInt, sizeof(int), Offset(y_axis_length),
     XtRImmediate, (XtPointer)2},
    {"zAxisLength", XtCLength, XtRInt, sizeof(int), Offset(z_axis_length),
     XtRImmediate, (XtPointer)2},
    {"axisDenom", XtCLength, XtRInt, sizeof(int), Offset(axis_denom),
     XtRImmediate, (XtPointer)1},
    {"axisFont", XtCFont, XtRFontStruct, sizeof(XFontStruct*),
     Offset(axis_font), XtRString, XtDefaultFont },
    {"majorAxisColor", XtCColor, XtRPixel, sizeof(Pixel),
     Offset(obj_x_axis_color), XtRString, "red"},
    {"minorAxisColor", XtCColor, XtRPixel, sizeof(Pixel),
     Offset(obj_y_axis_color), XtRString, "green"},
    {"otherAxisColor", XtCColor, XtRPixel, sizeof(Pixel),
     Offset(obj_z_axis_color), XtRString, "blue"},
    {"editAxisWidth", XtCWidth, XtRInt, sizeof(int), Offset(obj_axis_width),
     XtRImmediate, (XtPointer)2},
    {"majorAxisLength", XtCLength, XtRInt, sizeof(int),
     Offset(obj_x_axis_length), XtRImmediate, (XtPointer)4},
    {"minorAxisLength", XtCLength, XtRInt, sizeof(int),
     Offset(obj_y_axis_length), XtRImmediate, (XtPointer)3},
    {"otherAxisLength", XtCLength, XtRInt, sizeof(int),
     Offset(obj_z_axis_length), XtRImmediate, (XtPointer)2},
    {"editAxisDenom", XtCLength, XtRInt, sizeof(int), Offset(obj_axis_denom),
     XtRImmediate, (XtPointer)2},
    {"editPointRadius", XtCWidth, XtRInt, sizeof(int), Offset(edit_pt_rad),
     XtRImmediate, (XtPointer)10},
    {"controlPointRadius", XtCWidth, XtRInt, sizeof(int),
     Offset(control_pt_rad), XtRImmediate, (XtPointer)4},
    {"scalingColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(scaling_color),
     XtRString, "red"},
    {"originColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(origin_color),
     XtRString, "green"},
    {"torusRadColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(torus_color),
     XtRString, "purple"},
    {"controlColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(control_color),
     XtRString, "green"},
    {"controlTextColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(ctext_color),
     XtRString, "DarkGreen"},
    {"controlFont", XtCFont, XtRFontStruct, sizeof(XFontStruct*),
     Offset(control_font), XtRString, XtDefaultFont },
    {"objectColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(object_color),
     XtRString, "blue"},
    {"selectColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(selected_color),
     XtRString, "red"},
    {"selectWidth", XtCWidth, XtRInt, sizeof(int), Offset(selected_width),
     XtRImmediate, (XtPointer)0},
    {"lightColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(light_color),
     XtRString, "yellow"},
    {"lightPointRadius", XtCWidth, XtRInt, sizeof(int), Offset(light_pt_rad),
     XtRImmediate, (XtPointer)12},
    {"cameraColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(camera_color),
     XtRString, "midnightblue"},
    {"constraintColor", XtCColor, XtRPixel, sizeof(Pixel),
     Offset(constraint_color), XtRString, "grey"},
    {"planeConLength", XtCLength, XtRInt, sizeof(int), Offset(plane_con_length),
     XtRImmediate, (XtPointer)6},
    {"lineConLength", XtCLength, XtRInt, sizeof(int), Offset(line_con_length),
     XtRImmediate, (XtPointer)6},
    {"pointConWidth", XtCWidth, XtRInt, sizeof(int), Offset(point_con_rad),
     XtRImmediate, (XtPointer)15},
    {"inconConLength", XtCLength, XtRInt, sizeof(int), Offset(incon_con_length),
     XtRImmediate, (XtPointer)15},
    {"originConWidth", XtCLength, XtRInt, sizeof(int), Offset(origin_con_width),
     XtRImmediate, (XtPointer)7},
    {"scaleConWidth", XtCLength, XtRInt, sizeof(int), Offset(scale_con_width),
     XtRImmediate, (XtPointer)5},
    {"torusRadConWidth", XtCLength, XtRInt, sizeof(int),Offset(torus_con_width),
     XtRImmediate, (XtPointer)9},
    {"controlConWidth", XtCLength, XtRInt, sizeof(int),
     Offset(control_con_width), XtRImmediate, (XtPointer)11},
    {"rotateConWidth", XtCLength, XtRInt, sizeof(int), Offset(rotate_con_width),
     XtRImmediate, (XtPointer)3},
    {"referencedColor", XtCColor, XtRPixel, sizeof(Pixel),
     Offset(referenced_color), XtRString, "red"},
    {"activeColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(active_color),
     XtRString, "red"},
    {"selectPointColor", XtCColor, XtRPixel, sizeof(Pixel),
     Offset(selected_pt_color), XtRString, "blue"},
    {"selectPointWidth", XtCWidth, XtRInt, sizeof(int), Offset(select_pt_width),
     XtRImmediate, (XtPointer)8},
    {"selectPointLineWidth", XtCWidth, XtRInt, sizeof(int),
     Offset(select_pt_line_width), XtRImmediate, (XtPointer)2},
    {"absoluteColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(absolute_color),
     XtRString, "green"},
    {"offsetColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(offset_color),
     XtRString, "blue"},
    {"referenceColor", XtCColor, XtRPixel, sizeof(Pixel),
     Offset(reference_color), XtRString, "red"},
    {"vertexColor", XtCColor, XtRPixel, sizeof(Pixel),
     Offset(vertex_color), XtRString, "red"},
    {"arcballColor", XtCColor, XtRPixel, sizeof(Pixel), Offset(arcball_color),
     XtRString, "grey"},
    {"constructColor", XtCColor, XtRPixel, sizeof(Pixel),
     Offset(construct_color), XtRString, "green4"},
    {"aliasColor", XtCColor, XtRPixel, sizeof(Pixel),
     Offset(aliased_color), XtRString, "maroon"},
    {"editOrientation", XtCOrientation, XtROrientation, sizeof(XtOrientation),
     Offset(edit_orientation), XtRString, "horizontal" },
    {"editRows", XtCWidth, XtRInt, sizeof(int), Offset(edit_rows),
     XtRImmediate, (XtPointer)5 },
    {"editColumns", XtCWidth, XtRInt, sizeof(int), Offset(edit_columns),
     XtRImmediate, (XtPointer)5 }
    };
#undef Offset


/* Icon bitmap. */
#define icon_width 43
#define icon_height 44
static char icon_bits[] = {
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xdb,0xfe,0xff,0xff,0xff,0xff,0xfe,0xb7,0xff,0xff,0xff,
 0xc7,0x6f,0xef,0xff,0xff,0xff,0x1f,0xfe,0xd5,0xdf,0xff,0xff,0x67,0xb1,0xda,
 0x9f,0xff,0xff,0x8f,0xca,0xd6,0x9f,0xff,0xff,0x57,0x52,0xb5,0x1f,0xfe,0xff,
 0x57,0xa5,0xad,0x1f,0xfc,0xff,0x4f,0x4a,0xd5,0x3f,0xf4,0xff,0x97,0xd4,0xda,
 0x1f,0xf8,0xff,0x57,0xa9,0xb6,0x1f,0xe0,0xff,0xaf,0x52,0xd5,0x3f,0xc0,0xff,
 0x4f,0x4a,0xeb,0x5f,0x88,0xfe,0x9f,0xd4,0xda,0x1f,0x40,0xff,0xaf,0x92,0xd6,
 0x3f,0x00,0xff,0x5f,0x65,0xf5,0x5f,0x80,0xff,0xff,0x54,0xfb,0x1f,0xe1,0xff,
 0xff,0xcb,0xfe,0x3f,0xf8,0xff,0xff,0xbf,0xff,0x5f,0xff,0xff,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
 0x7f,0xfe,0xff,0xff,0xff,0xff,0xbf,0xf0,0xff,0x17,0xfe,0xff,0xbf,0x00,0xff,
 0x4b,0xf9,0xff,0x3f,0x02,0xf8,0x01,0xfa,0xff,0xbf,0x00,0xf5,0x8a,0xf0,0xff,
 0x5f,0x00,0xf8,0x02,0xe4,0xff,0x3f,0x44,0x7a,0x01,0xd0,0xff,0x9f,0x00,0xf4,
 0x00,0xf4,0xff,0x5f,0x00,0xfd,0x12,0xe0,0xff,0x2f,0x00,0x7a,0x01,0xd4,0xff,
 0x1f,0xa0,0xfc,0x02,0xf1,0xff,0x4f,0x02,0xfd,0x15,0xf4,0xff,0x2f,0x00,0xfe,
 0x45,0xf9,0xff,0x17,0xa0,0xfe,0x2b,0xfe,0xff,0x0f,0x00,0xfd,0xaf,0xfd,0xff,
 0x97,0x40,0xff,0xff,0xff,0xff,0x0f,0x08,0xfd,0xff,0xff,0xff,0x7f,0x40,0xff,
 0xff,0xff,0xff,0xff,0xa3,0xfe,0xff,0xff,0xff};
Pixmap  icon_bitmap;


void
main(int argc, char *argv[], char *envp[])
{
    FILE    *infile = NULL;
    Cursor  time_cursor;

    Set_WindowInfo(&main_window);
    Set_WindowInfo(&csg_window);
    Set_WindowInfo(&agg_window);

    /* Create the application shell. */
    /* This gets rid of X command line arguments. */
    main_window.shell = XtVaAppInitialize(&app_context, "Sced",
                (XrmOptionDescList)NULL, 0, &argc, argv, fallbacks,
                XtNtitle, "Scene Window", NULL);

    /* Set up the icon. */
    icon_bitmap = XCreateBitmapFromData(XtDisplay(main_window.shell),
                                RootWindowOfScreen(XtScreen(main_window.shell)),
                                icon_bits, icon_width, icon_height);
    XtVaSetValues(main_window.shell, XtNiconPixmap, icon_bitmap, NULL);

    XtAppAddConverter(app_context, XtRString, XtROrientation, XmuCvtStringToOrientation, NULL, 0);
    XtGetApplicationResources(main_window.shell, (XtPointer)&sced_resources, resources, XtNumber(resources), (Arg*)NULL, 0);

    /* Global initialization routine. */
    Initialize_Sced(argc, argv, envp);

    /* Register actions. */
    XtAppAddActions(app_context, actions, XtNumber(actions));
    XawSimpleMenuAddGlobalActions(app_context);

    /* Add error handler. */
    XSetErrorHandler(Sced_X_Error_Handler);
    XtAugmentTranslations(main_window.shell, XtParseTranslationTable(top_level_translations));

    /* Create the contents. */
    Create_Main_Display();

    /* Realize it all. */
    XtRealizeWidget(main_window.shell);

    /* Init drawing. */
    Draw_Initialize();

    /* Set the signal handlers. */
    Set_Signal_Handlers();

    /* Load the defaults file, if specified. */
    Load_Defaults_File(defaults_filename);

    /* Load the scene file, if specified. */
    if ( io_file_name )
    {
        /* Try to open the file in the current directory. */
        if ((infile = Open_Load_File_Name(&io_file_name)) == NULL && sced_preferences.scene_path)
        {
            /* Couldn't open it. Try opening in the scenes directory. */
            char    *new_filename = New(char, strlen(io_file_name) + strlen(sced_preferences.scene_path) + 4);

            sprintf(new_filename, "%s/%s", sced_preferences.scene_path, io_file_name);
            if ( ( infile = Open_Load_File_Name(&new_filename) ) == NULL )
                fprintf(stderr, "Unable to open file %s\n", io_file_name);
            else
            {
                free(io_file_name);
                io_file_name = new_filename;
            }
        }
        /* Check the first character. We want it to be an absolute pathname,
        ** since SelFile screws up the current dir otherwise.
        ** This in turn screws previewing.
        */
        if ( io_file_name[0] != '/' )
        {
            char    current_dir[MAXPATHLEN];
            char    *temp;

#if HAVE_GETCWD
            getcwd(current_dir, MAXPATHLEN);
#else
            getwd(current_dir);
#endif
            temp = New(char, strlen(io_file_name) + strlen(current_dir) + 5);
            sprintf(temp, "%s/%s", current_dir, io_file_name);
            free(io_file_name);
            io_file_name = temp;
        }
    }
    if ( infile )
    {
        time_cursor = XCreateFontCursor(XtDisplay(main_window.shell), XC_watch);
        XDefineCursor(XtDisplay(main_window.shell),
                      XtWindow(main_window.shell), time_cursor);

        Sced_Load_File(infile, FALSE);

        XDefineCursor(XtDisplay(main_window.shell), XtWindow(main_window.shell),
                      None);
        XFreeCursor(XtDisplay(main_window.shell), time_cursor);
    }

#if ELK_SUPPORT
    /*
     * Fire up scheme interpretor
     */
    Elk_Sced_Init(argc, argv);
#endif /* ELK_SUPPORT */
    
    /* Loop for events. */
    XtAppMainLoop(app_context);

}

static int
Sced_X_Error_Handler(Display *display, XErrorEvent *error)
{
    char    msg[128];
    XGetErrorText(display, error->error_code, msg, 128);
    fprintf(stderr, "X Error code %s\n", msg);
    Sced_Abort(FALSE);
    /* Never returns. */
    return 0;
}


/*  void
**  Initialize_Sced(int argc, char *argv[], char *envp[])
**  Parses the command line, then calls initialization functions.
*/
static void
Initialize_Sced(int argc, char *argv[], char *envp[])
{
    int     i;
    char    *temp_s;
    int     w, h;
    Boolean have_filename = FALSE;

    /* Any options left on the command line are mine.   */
    /* Options at the moment are:
    ** -I WxH
    ** -F filename
    ** -D defaults_file
    */
    for ( i = 1 ; i < argc ; i++ )
    {
        if ( argv[i][0] != '-' )
        {
            /* Assume it's a filename. */
            if ( have_filename )
                printf("Unexpected argument: %s\n", argv[i]);
            else
            {
                io_file_name = Strdup(argv[i]);
                have_filename = TRUE;
            }
        }
        else
        {
            switch ( argv[i][1] )
            {
                case 'F':
                    if ( argv[i][2] != '\0' )
                        io_file_name = Strdup(argv[i] + 2);
                    else
                    {
                        i++;
                        io_file_name = Strdup(argv[i]);
                    }
                    have_filename = TRUE;
                    break;

                case 'D':
                    if ( argv[i][2] != '\0' )
                        defaults_filename = Strdup(argv[i] + 2);
                    else
                    {
                        i++;
                        defaults_filename = Strdup(argv[i]);
                    }
                    break;

                case 'I':
                    if ( argv[i][2] == '\0' )
                    {
                        i++;
                        temp_s = argv[i];
                    }
                    else
                        temp_s = argv[i] + 2;
                    if ( ( sscanf(temp_s, "%dx%d", &w, &h) == 2 ) ||
                         ( sscanf(temp_s, "%dX%d", &w, &h) == 2 ) )
                    {
                        camera.scr_width = (Dimension)w;
                        camera.scr_height = (Dimension)h;
                    }
                    else
                        printf("Invalid geometry string: %s\n", temp_s);
                    break;

                default:
                    printf("Unknown argument: %s\n", argv[i]);
            }
        }
    }

    /* Set defaults for everything that needs one, such as the viewports. */
    Set_Defaults();

    /* Initialize the default base objects. */
    Initialize_Base_Objects();

    /* Initialize constrained features. */
    Feature_Create_Bases();

    /* Initialize visibility layer info. */
    Layers_Init();

    /* Store the start directory, because SelFile tends to trash it, but
    ** preview needs it. */
#if HAVE_GETCWD
    getcwd(start_directory_name, MAXPATHLEN);
#else
    getwd(start_directory_name);
#endif



}


/*  void
**  Set_Defaults()
**  Sets default values for:
**  - preferences
**  - main_viewport
**  - csg_viewport
*/
static void
Set_Defaults()
{
    Renderer    i;

    /* Set the default attributes for all new objects. */
    sced_preferences.default_attributes.defined = TRUE;
    sced_preferences.default_attributes.color.red = DEFAULT_RED;
    sced_preferences.default_attributes.color.green = DEFAULT_GREEN;
    sced_preferences.default_attributes.color.blue = DEFAULT_BLUE;
    sced_preferences.default_attributes.diff_coef = DEFAULT_DIFFUSE;
    sced_preferences.default_attributes.spec_coef = DEFAULT_SPECULAR;
    sced_preferences.default_attributes.spec_power = DEFAULT_SPECULAR * 100;
    sced_preferences.default_attributes.reflect_coef = DEFAULT_REFLECT;
    sced_preferences.default_attributes.refract_index = DEFAULT_REFRACT;
    sced_preferences.default_attributes.transparency = DEFAULT_TRANSPARENCY;
    sced_preferences.default_attributes.use_extension = FALSE;
    for ( i = NoTarget ; i < LastTarget ; i++ )
        sced_preferences.default_attributes.extension[i] = NULL;
    sced_preferences.default_attributes.use_obj_trans = FALSE;
    sced_preferences.default_attributes.open = FALSE;
    sced_preferences.default_attributes.intensity.red = DEFAULT_I_RED;
    sced_preferences.default_attributes.intensity.green = DEFAULT_I_GREEN;
    sced_preferences.default_attributes.intensity.blue = DEFAULT_I_BLUE;
    sced_preferences.default_attributes.radius = DEFAULT_RADIUS;
    sced_preferences.default_attributes.tightness = DEFAULT_TIGHTNESS;
    sced_preferences.default_attributes.samples = DEFAULT_SAMPLES;
    sced_preferences.default_attributes.jitter = DEFAULT_JITTER;
    sced_preferences.default_attributes.invert = DEFAULT_INVERT;

    sced_preferences.scene_path = NULL;
    sced_preferences.compress_output = FALSE;
    sced_preferences.save_simple_wires = FALSE;
    sced_preferences.autosave_time = 0;
    sced_preferences.povray_v3 = TRUE;

    /* Build the default viewports. */
    Viewport_Init(&(main_window.viewport));
    Viewport_Init(&(csg_window.viewport));
    Viewport_Init(&(agg_window.viewport));

}


static RETSIGTYPE
Sced_Signal_Handler(int sig_num)
{
    Sced_Abort(TRUE);
}

static void
Sced_Handle_Save_Yourself()
{
    FILE    *save_file;
    char    current_dir[MAXPATHLEN];
    char    *temp;
    char    *argv[2];


#if HAVE_GETCWD
    getcwd(current_dir, MAXPATHLEN);
#else
    getwd(current_dir);
#endif
    temp = New(char, strlen(current_dir) + 12);
    sprintf(temp, "%s/sced.save", current_dir);

    /* Clean up and save state. */
    Remove_Temporaries();
    if ( ( save_file = fopen(temp, "w") ) )
        Save_Func(save_file);

    /* Set new arguments and register them. */
    argv[0] = "sced";
    argv[1] = temp;
    XSetCommand(XtDisplay(main_window.shell), XtWindow(main_window.shell),
                argv, 2);
}

static void
Set_Signal_Handlers()
{
#if ( ! MISSING_ATOMS )
    Atom    atoms[2];
#endif

    signal(SIGHUP, Sced_Signal_Handler);
    signal(SIGTERM, Sced_Signal_Handler);

#if ( ! MISSING_ATOMS )
    atoms[0] = XInternAtom(XtDisplay(main_window.shell), "WM_SAVE_YOURSELF",
                           FALSE);
    atoms[1] = XInternAtom(XtDisplay(main_window.shell), "WM_DELETE_WINDOW",
                           FALSE);
    XSetWMProtocols(XtDisplay(main_window.shell), XtWindow(main_window.shell), 
                    atoms, 2);
#endif
}


static void
Handle_Client_Messages(Widget w, XEvent *event, String *s, Cardinal *n)
{
#if ( ! MISSING_ATOMS )
    char    *message_name =
        XGetAtomName(XtDisplay(main_window.shell), event->xclient.data.l[0]);

    if ( ! message_name )
        return;

    if ( ! strcmp(message_name, "WM_DELETE_WINDOW") )
    {
        if ( w == main_window.shell )
            Quit_Dialog_Func(w, NULL, NULL);
        else if ( w == csg_window.shell )
            CSG_Window_Delete_Message();
        else if ( w == agg_window.shell )
            Agg_Window_Delete_Message();
        else if ( w == edit_shell )
            Edit_Window_Delete_Message();
#if ELK_SUPPORT
        else if ( w == elk_shell )
            Elk_Window_Delete_Message();
#endif
    }
    else if ( ! strcmp(message_name, "WM_SAVE_YOURSELF") )
        Sced_Handle_Save_Yourself();
#endif
}

