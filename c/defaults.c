/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/defaults.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: defaults.c,v $
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
**  defaults.c : Loads a default specification file, if it exists.
*/

#include <sced.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <attributes.h>
#include <ctype.h>
#include <errno.h>
#include <load.h>
#if ( HAVE_STRING_H )
#include <string.h>
#elif ( HAVE_STRINGS_H )
#include <strings.h>
#endif
#include <time.h>
#include <View.h>
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Label.h>


extern char *rayshade_path;
extern char *rayshade_options;
extern char *povray_path;
extern char *povray_options;
extern char *radiance_path;
extern char *radiance_options;
extern char *renderman_path;
extern char *renderman_options;
extern char *vrml_path;
extern char *vrml_options;

static void Load_Defaults(FILE*);
static int  Load_Default_Declarations();
static int  Load_Default_View(Viewport*, char*);
static int  Load_Renderer_Defs(char**, char**);
static int  Load_Default_Attributes();

#define Error fprintf(stderr, "Error in defaults file line %d\n", line_num)
#define Load_Float(f) \
    { \
        if ((token = yylex()) == INT_TOKEN) \
            f = (double)lex_int; \
        else if (token == FLOAT_TOKEN) \
            f = lex_float; \
        else { \
            Error; \
            return token; \
        } \
    }
#define Load_Vector(v) \
    { Load_Float((v).x); Load_Float((v).y); Load_Float((v).z); }

#define CONFIRM_YES 2
#define CONFIRM_NO 1
#define CONFIRM_NONE 0

#define PATH_DONE 2
#define PATH_CANCEL 1
#define PATH_NONE 0

static char     *def_filename = NULL;
static int      path_result = PATH_NONE;
static Widget   pathname_shell = NULL;
static int      confirm_result = CONFIRM_NONE;

static FILE*
Defaults_Open_Filename(char *filename)
{
    char    *home_dir;

    if ( ! filename )
    {
        /* Look for a file called .scenerc in the users home directory. */
        home_dir = getenv("HOME");
        if ( ! home_dir )
            filename = Strdup(".scenrc");
        else
        {
            filename = New(char, strlen(home_dir) + 12);
            sprintf(filename, "%s/.scenerc", home_dir);
        }
        def_filename = filename;
        return fopen(filename, "r");
    }
    else
        def_filename = Strdup(filename);

    return fopen(def_filename, "r");
}

void
Load_Defaults_File(char *filename)
{
    FILE    *defs_file;

    defs_file = Defaults_Open_Filename(filename);

    if ( ! defs_file )
    {
        if ( filename )
            fprintf(stderr, "Couldn't open defaults file %s\n", filename);
        return;
    }

    Load_Defaults(defs_file);

    fclose(defs_file);
}


static void
Load_Defaults(FILE *file)
{
    int token;

#if FLEX
    if ( yyin ) yyrestart(yyin);
#endif /* FLEX */
    yyin = file;

    token = yylex();
    while ( token != EOF_TOKEN )
    {
        switch ( token )
        {
            case MAINVIEW_TOKEN:
                token = Load_Default_View(&(main_window.viewport),
                                          "Main_Default");
                if ( main_window.view_widget && main_window.viewport.scr_width )
                    XtVaSetValues(main_window.view_widget,
                            XtNdesiredWidth, main_window.viewport.scr_width,
                            XtNwidth, main_window.viewport.scr_width,
                            XtNdesiredHeight, main_window.viewport.scr_height,
                            XtNheight, main_window.viewport.scr_height,
                            NULL);
                main_window.magnify = main_window.viewport.magnify;
                break;

            case CSGVIEW_TOKEN:
                token = Load_Default_View(&(csg_window.viewport),
                                          "CSG_Default");
                break;

            case AGGVIEW_TOKEN:
                token = Load_Default_View(&(agg_window.viewport),
                                          "Agg_Default");
                break;

            case VIEWPORT_TOKEN:
                if ( ( token = yylex() ) != STRING_TOKEN )
                {
                    Error;
                    break;
                }
                token = Load_Default_View(NULL, lex_string);
                free(lex_string);
                break;

            case RAYSHADE_TOKEN:
                token = Load_Renderer_Defs(&rayshade_path, &rayshade_options);
                break;

            case POVRAY_TOKEN:
                token = Load_Renderer_Defs(&povray_path, &povray_options);
                if ( token == INT_TOKEN )
                {
                    if ( lex_int == 3 )
                        sced_preferences.povray_v3 = TRUE;
                    else
                        sced_preferences.povray_v3 = FALSE;
                    token = yylex();
                }
                else if ( token == FLOAT_TOKEN )
                {
                    if ( lex_float == 3.0 )
                        sced_preferences.povray_v3 = TRUE;
                    else
                        sced_preferences.povray_v3 = FALSE;
                    token = yylex();
                }
                else
                    Error;
                break;

            case RADIANCE_TOKEN:
                token = Load_Renderer_Defs(&radiance_path, &radiance_options);
                break;

            case RENDERMAN_TOKEN:
                token = Load_Renderer_Defs(&renderman_path,
                                            &renderman_options);
                break;

            case LOAD_VRML_TOKEN:
                token = Load_Renderer_Defs(&vrml_path, &vrml_options);
                break;

            case DECLARE_TOKEN:
                token = Load_Default_Declarations();
                break;

            case ATTRIBUTES_TOKEN:
                token = Load_Default_Attributes();
                break;

            case AUTOSAVE_TOKEN:
                if ( ( token = yylex() ) == INT_TOKEN )
                {
                    sced_preferences.autosave_time = (int)lex_int;
                    Save_Auto_Startup();
                    token = yylex();
                }
                else
                    Error;
                break;

            case COMPRESS_TOKEN:
                sced_preferences.compress_output = TRUE;
                token = yylex();
                break;

            case WIREFRAME_TOKEN:
                if ( ( token = yylex() ) == FULL_TOKEN )
                {
                    sced_preferences.save_simple_wires = TRUE;
                    token = yylex();
                }
                break;

            case SCENEDIR_TOKEN:
                if ((token = yylex()) != STRING_TOKEN)
                    Error;
                else
                    sced_preferences.scene_path = lex_string;
                token = yylex();
                break;

            case TARGET_TOKEN:
                if ( ( token = yylex() ) == POVRAY_TOKEN )
                    target_renderer = POVray;
                else if ( token == RAYSHADE_TOKEN )
                    target_renderer = Rayshade;
                else if ( token == RADIANCE_TOKEN )
                    target_renderer = Radiance;
                else if ( token == RENDERMAN_TOKEN )
                    target_renderer = Renderman;
                else if ( token == LOAD_VRML_TOKEN )
                    target_renderer = VRML;
                else
                    Error;
                token = yylex();
                break;

            default:
                Error;
                token = yylex();
        }
    }
}


static int
Load_Default_View(Viewport *viewport, char *label)
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
                Load_Vector(result.view_from);
                break;

            case LOOKAT_TOKEN:
                Load_Vector(result.view_at);
                break;

            case LOOKUP_TOKEN:
                Load_Vector(result.view_up);
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
                    Error;
                    finished = TRUE;
                }
                else
                    result.magnify = lex_int;
                break;

            case MODE_TOKEN:
                if ( (token = yylex()) != INT_TOKEN )
                {
                    Error;
                    finished = TRUE;
                }
                else
                    result.draw_mode = (int)lex_int;
                break;

            case SCREEN_TOKEN:
                if ((token = yylex()) != INT_TOKEN )
                {
                    Error;
                    return token;
                }
                result.scr_width = (Dimension)lex_int;

                if ( (token = yylex()) != INT_TOKEN )
                {
                    Error;
                    return token;
                }
                result.scr_height = (Dimension)lex_int;
                break;

            default:
                finished = TRUE;
        }
    }

    result.is_default = TRUE;

    Build_Viewport_Transformation(&result);

    if ( viewport ) *viewport = result;

    View_Save(&result, label);

    return token;
}


static int
Load_Renderer_Defs(char **path, char **options)
{
    int         token;

    if ( (token = yylex()) != STRING_TOKEN )
    {
        Error;
        return token;
    }
    *path = lex_string;

    if ( (token = yylex()) != STRING_TOKEN )
    {
        Error;
        return token;
    }
    *options = lex_string;

    return yylex();
}


static int
Load_Default_Attributes()
{
    int     token;
    float   r, g, b;
    int     s1, s2;

    while (TRUE)
    {
        switch ( token = yylex() )
        {
            case COLOUR_TOKEN:
                Load_Float(r);  Load_Float(g);  Load_Float(b);
                sced_preferences.default_attributes.color.red = r;
                sced_preferences.default_attributes.color.green = g;
                sced_preferences.default_attributes.color.blue = b;
                break;

            case INTENSITY_TOKEN:
                Load_Float(r);  Load_Float(g);  Load_Float(b);
                sced_preferences.default_attributes.intensity.red = r;
                sced_preferences.default_attributes.intensity.green = g;
                sced_preferences.default_attributes.intensity.blue = b;
                break;

            case DIFFUSE_TOKEN:
                Load_Float(sced_preferences.default_attributes.diff_coef);
                break;

            case SPECULAR_TOKEN:
                Load_Float(sced_preferences.default_attributes.spec_coef);
                Load_Float(sced_preferences.default_attributes.spec_power);
                break;

            case SAMPLE_TOKEN:
                if ( ( token = yylex() ) != INT_TOKEN )
                    Error;
                s1 = lex_int;
                if ( ( token = yylex() ) != INT_TOKEN )
                    Error;
                s2 = lex_int;
                sced_preferences.default_attributes.samples =
                    ( s1 & 0xFF ) | ( s2 << 8 );
                break;

            case TRANSPARENCY_TOKEN:
                Load_Float(sced_preferences.default_attributes.transparency);
                break;

            case REFLECT_TOKEN:
                Load_Float(sced_preferences.default_attributes.reflect_coef);
                break;

            case REFRACT_TOKEN:
                Load_Float(sced_preferences.default_attributes.refract_index);
                break;

            case RADIUS_TOKEN:
                Load_Float(sced_preferences.default_attributes.radius);
                break;

            case TIGHTNESS_TOKEN:
                Load_Float(sced_preferences.default_attributes.tightness);
                break;

            case JITTER_TOKEN:
                sced_preferences.default_attributes.jitter = TRUE;
                break;

            case TRANSFORM_TOKEN:
                sced_preferences.default_attributes.use_obj_trans = TRUE;
                break;

            case OPEN_TOKEN:
                sced_preferences.default_attributes.open = TRUE;
                break;

            case INVERT_TOKEN:
                sced_preferences.default_attributes.invert = TRUE;
                break;

            default: return token;
        }
    }
    return token;
}


static int
Load_Default_Declarations()
{
    int         token;
    Renderer    target;

    switch ( token = yylex() )
    {
        case POVRAY_TOKEN:
            target = POVray;
            break;
        case RADIANCE_TOKEN:
            target = Radiance;
            break;
        case RAYSHADE_TOKEN:
            target = Rayshade;
            break;
        case RENDERMAN_TOKEN:
            target = Renderman;
            break;
        case LOAD_VRML_TOKEN:
            target = VRML;
            break;
        default: return token;
    }

    if ( ( token = yylex() ) != STRING_TOKEN )
    {
        Error;
        return token;
    }

    Change_Declarations(target, lex_string, FALSE);
    free(lex_string);

    return yylex();
}


static void
Defaults_Pathname_Cancel_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    path_result = PATH_CANCEL;
}


static void
Defaults_Pathname_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    path_result = PATH_DONE;
}


void
Defaults_Pathname_Action(Widget w, XEvent *e, String *s, Cardinal *n)
{
    Defaults_Pathname_Callback(w, NULL, NULL);
}


static void
Defaults_Create_Pathname_Dialog()
{
    Widget  dialog;
    Arg     args[3];
    int     n;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Confirm");         n++;
    XtSetArg(args[n], XtNallowShellResize, TRUE);   n++;
    pathname_shell = XtCreatePopupShell("defaultsConfirm",
                        transientShellWidgetClass, main_window.shell, args, n);

    n = 0;
    XtSetArg(args[n], XtNlabel, "Defaults file name:"); n++;
    XtSetArg(args[n], XtNvalue, "");                    n++;
    dialog = XtCreateManagedWidget("pathDialog", dialogWidgetClass,
                                   pathname_shell, args, n);

    XawDialogAddButton(dialog, "Done", Defaults_Pathname_Callback, NULL);
    XawDialogAddButton(dialog, "Cancel", Defaults_Pathname_Cancel_Callback,
                       NULL);

    XtOverrideTranslations(XtNameToWidget(dialog, "value"),
        XtParseTranslationTable(":<Key>Return: Defaults_Action()"));

    XtVaSetValues(XtNameToWidget(dialog, "label"), XtNborderWidth, 0, NULL);

    XtRealizeWidget(pathname_shell);
}


static char*
Defaults_Request_Pathname(char *initial)
{
    XEvent  event;
    char    *name;

    if ( ! pathname_shell )
        Defaults_Create_Pathname_Dialog();

    XtVaSetValues(XtNameToWidget(pathname_shell, "pathDialog"),
                  XtNvalue, initial, NULL);

    Position_Shell(pathname_shell, FALSE);
    XtPopup(pathname_shell, XtGrabExclusive);

    path_result = PATH_NONE;
    while ( path_result == PATH_NONE )
    {
        XtAppNextEvent(app_context, &event);
        XtDispatchEvent(&event);
    }

    XtPopdown(pathname_shell);

    if ( path_result == PATH_DONE )
    {
        XtVaGetValues(XtNameToWidget(pathname_shell, "pathDialog"),
                      XtNvalue, &name, NULL);
        return Strdup(name);
    }

    return NULL;
}


static void
Defaults_Confirm_Create_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    confirm_result = CONFIRM_YES;
}


static void
Defaults_Confirm_New_Callback(Widget w, XtPointer cl, XtPointer ca)
{
    confirm_result = CONFIRM_NO;
}


static Widget
Defaults_Confirm_Create_Dialog(char *name)
{
    Widget  shell;
    Widget  dialog;
    char    *label;
    Arg     args[3];
    int     n;

    n = 0;
    XtSetArg(args[n], XtNtitle, "Confirm"); n++;
    shell = XtCreatePopupShell("defaultsConfirm", transientShellWidgetClass,
                               main_window.shell, args, n);

    label = New(char, strlen(name) + 64);
    sprintf(label, "Error: Cannot open %s", name);

    n = 0;
    XtSetArg(args[n], XtNlabel, label); n++;
    dialog = XtCreateManagedWidget("confirmDialog", dialogWidgetClass,
                                   shell, args, n);

    XawDialogAddButton(dialog, "Create", Defaults_Confirm_Create_Callback,NULL);
    XawDialogAddButton(dialog, "New File", Defaults_Confirm_New_Callback, NULL);

    XtVaSetValues(XtNameToWidget(dialog, "label"), XtNborderWidth, 0, NULL);

    free(label);

    XtRealizeWidget(shell);

    return shell;
}

static int
Defaults_Confirm_Write(char *filename)
{
    XEvent  event;
    Widget  confirm_shell;

    confirm_shell = Defaults_Confirm_Create_Dialog(filename);

    confirm_result = CONFIRM_NONE;

    Position_Shell(confirm_shell, FALSE);
    XtPopup(confirm_shell, XtGrabExclusive);

    while ( confirm_result == CONFIRM_NONE )
    {
        XtAppNextEvent(app_context, &event);
        XtDispatchEvent(&event);
    }

    XtPopdown(confirm_shell);
    XtDestroyWidget(confirm_shell);

    if ( confirm_result == CONFIRM_YES )
        return TRUE;

    return FALSE;
}


static void
Defaults_Dump_Preferences(FILE *outfile, ScedPreferencesPtr prefs)
{
    fprintf(outfile, "Attributes\n");
    fprintf(outfile, "\tColor %g %g %g\n",
            prefs->default_attributes.color.red,
            prefs->default_attributes.color.green,
            prefs->default_attributes.color.blue);
    fprintf(outfile, "\tDiffuse %g\n", prefs->default_attributes.diff_coef);
    fprintf(outfile, "\tSpecular %g %g\n", prefs->default_attributes.spec_coef,
            prefs->default_attributes.spec_power);
    fprintf(outfile, "\tReflect %g\n", prefs->default_attributes.reflect_coef);
    fprintf(outfile, "\tRefract %g\n", prefs->default_attributes.refract_index);
    fprintf(outfile, "\tTransparency %g\n",
            prefs->default_attributes.transparency);
    if ( prefs->default_attributes.use_obj_trans )
        fprintf(outfile, "\tTransform\n");
    if ( prefs->default_attributes.open )
        fprintf(outfile, "\tOpen\n");
    fprintf(outfile, "\tIntensity %g %g %g\n",
            prefs->default_attributes.intensity.red,
            prefs->default_attributes.intensity.green,
            prefs->default_attributes.intensity.blue);
    fprintf(outfile, "\tRadius %g\n", prefs->default_attributes.radius);
    fprintf(outfile, "\tTightness %g\n", prefs->default_attributes.tightness);
    fprintf(outfile, "\tSamples %d %d\n",
            prefs->default_attributes.samples & 0xFF,
            ( prefs->default_attributes.samples & 0xFF00 ) >> 8);
    if ( prefs->default_attributes.invert )
        fprintf(outfile, "\tInvert\n");
    if ( prefs->default_attributes.jitter )
        fprintf(outfile, "\tJitter\n");

    fprintf(outfile, "Directory \"%s\"\n", prefs->scene_path);

    if ( prefs->compress_output )
        fprintf(outfile, "Compress\n");

    if ( prefs->save_simple_wires )
        fprintf(outfile, "Wireframe Full\n");

    if ( prefs->autosave_time )
        fprintf(outfile, "Autosave %d\n", prefs->autosave_time);
}


static int
Defaults_Read_Word(FILE *src, FILE *dest, char *buf, int max_l)
{
    int ch;
    int num;

    max_l -= 4; /* Safety */

    while ( ( ch = fgetc(src) ) != EOF && isspace(ch) )
        if ( ch == '\n' )
        {
            buf[0] = '\n';
            buf[1] = '\0';
            return 1;
        }

    if ( ch == EOF )
        return -1;

    num = 0;
    buf[num++] = ch;
    while ( ( ch = fgetc(src) ) != EOF && ! isspace(ch) && num < max_l )
        buf[num++] = ch;

    buf[num++] = ch;
    buf[num] = '\0';

    return num;
}


static void
Defaults_Transfer_Data(FILE *src, FILE *dest, ScedPreferencesPtr prefs)
{
    char    buffer[256];

    while ( Defaults_Read_Word(src, dest, buffer, 256) != -1 )
    {
        if ( ! strncmp(buffer, "Attributes", 10) ||
             ! strncmp(buffer, "Compress", 8) ||
             ! strncmp(buffer, "Invert", 6) ||
             ! strncmp(buffer, "Transform", 9) ||
             ! strncmp(buffer, "Open", 4) ||
             ! strncmp(buffer, "Jitter", 6) )
            continue;

        if ( ! strncmp(buffer, "Color", 5) ||
             ! strncmp(buffer, "Intensity", 9) )
        {
            Defaults_Read_Word(src, dest, buffer, 256);
            Defaults_Read_Word(src, dest, buffer, 256);
            Defaults_Read_Word(src, dest, buffer, 256);
            continue;
        }

        if ( ! strncmp(buffer, "Diffuse", 7) ||
             ! strncmp(buffer, "Reflect", 7) ||
             ! strncmp(buffer, "Refract", 7) ||
             ! strncmp(buffer, "Transparency", 12) ||
             ! strncmp(buffer, "Radius", 6) ||
             ! strncmp(buffer, "Tightness", 9) ||
             ! strncmp(buffer, "Directory", 9) ||
             ! strncmp(buffer, "Wireframe", 9) ||
             ! strncmp(buffer, "Autosave", 8) )
        {
            Defaults_Read_Word(src, dest, buffer, 256);
            continue;
        }

        if ( ! strncmp(buffer, "Specular", 8) ||
             ! strncmp(buffer, "Samples", 7) )
        {
            Defaults_Read_Word(src, dest, buffer, 256);
            Defaults_Read_Word(src, dest, buffer, 256);
            continue;
        }

        fputs(buffer, dest);
    }
}


static void
Defaults_Update_Old_File(char *filename, ScedPreferencesPtr prefs)
{
    FILE    *src;
    FILE    *dest;
    char    *temp_file;

    temp_file = New(char, strlen(filename) + 16);
    sprintf(temp_file, "%s.transient", filename);

    src = fopen(filename, "r");
    dest = fopen(temp_file, "w");

    if ( ! src || ! dest )
    {
        Popup_Error("Error opening file: Defaults not saved", main_window.shell,
                    "Error");
        if ( src )
            fclose(src);
        if ( dest )
            fclose(dest);
        return;
    }

    Defaults_Transfer_Data(src, dest, prefs);
    Defaults_Dump_Preferences(dest, prefs);

    fclose(src);
    fclose(dest);

    /* Do the relinking. */
    if ( unlink(filename) == -1 )
    {
#if HAVE_STRERROR
        Popup_Error(strerror(errno), main_window.shell, "Error");
#else
        Popup_Error(StrError(errno), main_window.shell, "Error");
#endif
        unlink(temp_file);
        return;
    }

    if ( link(temp_file, filename) == -1 )
#if HAVE_STRERROR
        Popup_Error(strerror(errno), main_window.shell, "Error");
#else
        Popup_Error(StrError(errno), main_window.shell, "Error");
#endif

    unlink(temp_file);

    if ( def_filename )
        free(def_filename);
    def_filename = filename;
}


static void
Defaults_Create_New_File(char *filename, ScedPreferencesPtr prefs)
{
    FILE    *outfile;
    time_t  current_time;

    time(&current_time);

    outfile = fopen(filename, "w");

    if ( ! outfile )
    {
        /* Shouldn't be here. */
        Popup_Error("Error opening file: Defaults not saved", main_window.shell,
                    "Error");
        return;
    }

    fprintf(outfile, "# Sced defaults. Created %s\n", ctime(&current_time));

    Defaults_Dump_Preferences(outfile, prefs);

    fclose(outfile);

    if ( def_filename )
        free(def_filename);
    def_filename = filename;
}


void
Defaults_Save_Preferences(ScedPreferencesPtr prefs)
{
    char    *new_filename;
    char    *home_dir;
    char    *temp_s;
    Boolean creating;
    Boolean have_file = FALSE;

    /* Get a filename. */
    do
    {
        new_filename = Defaults_Request_Pathname(def_filename);

        if ( ! new_filename )
            return;

        if ( new_filename[0] == '~' )
        {
            home_dir = getenv("HOME");
            if ( home_dir )
            {
                temp_s = New(char, strlen(new_filename) + strlen(home_dir) + 4);
                sprintf(temp_s, "%s%s", home_dir, new_filename);
                free(new_filename);
                new_filename = temp_s;
            }
        }

        if ( ! access(new_filename, R_OK | W_OK ) )
        {
            creating = FALSE;
            have_file = TRUE;
        }
        else
        {
            if ( Defaults_Confirm_Write(new_filename) )
            {
                creating = TRUE;
                have_file = TRUE;
            }
            else
                free(new_filename);
        }
    } while ( ! have_file );

    if ( creating )
        Defaults_Create_New_File(new_filename, prefs);
    else
        Defaults_Update_Old_File(new_filename, prefs);
}

