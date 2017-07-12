/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/utils.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: utils.c,v $
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
**  utils.c: Utility functions.
**
**  Created: 05/03/94
**
**  External functions:
**
**  char*
**  EMalloc(unsigned int size);
**  Tries to malloc size bytes, exiting on failure.
**
**  char*
**  WMalloc(unsigned int size);
**  Tries to malloc size bytes, warning and returning NULL on failure.
**
**  char*
**  ERealloc(char *ptr, unsigned int size);
**  Tries to realloc size bytes, exiting on failure.
**
**  char*
**  WRealloc(char *ptr, unsigned int size);
**  Tries to realloc size bytes, warning and returning NULL on failure.
**
**  void
**  Popup_Error(char *text, Widget parent, char *title)
**  Pops up an error dialog box.
**
**  Dimension
**  Match_Widths(Widget *widgets, int num)
**  Makes all the widgets the width of the widest.
**  Returns the maximum width.
**
**  void
**  Destroy_World(Boolean)
**  Destroys the world (really!).
**  Actually just frees the instance list and any non-default base types.
**  Also cameras, light sources and the other nitty gritty bits.
**
**  Boolean
**  Check_Rectangle_Intersection(XPoint, XPoint, XPoint, XPoint)
**  Returns TRUE if the 2 rectangles defined by the points intersect.
**
**  void
**  Set_Main_Prompt(String new)
**  Sets the main prompt string.
**
**  double
**  Round_To_Snap(double orig, double snap)
**  Rounds orig to the nearest multiple of snap.
**
**  Boolean
**  Points_Colinear(Vector p1, Vector p2, Vector p3)
**  Returns TRUE if the three screen points are colinear.
**
**  void
**  Set_WindowInfo(WindowInfo *inf)
**  Initializes all the fields of a WindowInfo structure.
**
**  Vector
**  Extract_Euler_Angles(Matrix)
**  Extracts the three euler angles from a rotation matrix.
**
**  void
**  Copy_Objects_Callback(Widget w, XtPointer cl, XtPointer ca)
**  The callback for the copy function.
*/

#include <stdlib.h>
#include <math.h>
#include <sced.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <base_objects.h>
#include <instance_list.h>
#include <layers.h>
#if HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <X11/Shell.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Text.h>
#include <X11/Xaw/Command.h>

#define DEBUG_MALLOC

extern void Save_Func(FILE*);

/* The popup error message widget. */
static Widget   popup_err_shell;

/* The popup error acknowledge callback. */
static Boolean  error_down;
static void     Acknowledge_Error(Widget, XtPointer, XtPointer);

void
Remove_Temporaries()
{
    int i;

    /* Delete the temporary files. */
    for ( i = 0 ; i < num_temp_files ; i++ )
        unlink(temp_filenames[i]);
    num_temp_files = 0;
}

void
Sced_Exit(int code)
{
    /* Do any extra work here, if required. */
    Remove_Temporaries();
    if ( code != -1 )
        exit(code);
    else
        abort();
}

void
Sced_Abort(Boolean signaled)
{
    FILE    *save_file;

    if ( ! signaled )
        fprintf(stderr, "Saving to sced.save\n");

    if ( ( save_file = fopen("sced.save", "w") ) == NULL )
    {
        if ( ! signaled )
            fprintf(stderr, "sced: Could not open emergency save file\n");
    }
    else
        Save_Func(save_file);

#ifdef DEBUG_MALLOC
    abort();
#else
    Sced_Exit(1);
#endif
}


/*  char*
**  emalloc(unsigned int size);
**  Tries to malloc size bytes, exiting on failure.
*/
char*
EMalloc(unsigned int size)
{
    char    *res;

    if ( size == 0 )
        return NULL;

    if ((res = malloc(size)) == NULL)
    {
        fprintf(stderr, "Unable to malloc %d bytes.  Exiting.\n", size);
        Sced_Abort(FALSE);
    }

    return res;
}


/*  char*
**  wmalloc(unsigned int size);
**  Tries to malloc size bytes, warning and returning NULL on failure.
*/
char*
WMalloc(unsigned int size)
{
    char    *res;

    if ((res = malloc(size)) == NULL)
        fprintf(stderr, "Unable to malloc %d bytes.\n", size);

    return res;
}


/*  char*
**  erealloc(char *ptr, unsigned int size);
**  Tries to realloc size bytes, exiting on failure.
*/
char*
ERealloc(char *ptr, unsigned int size)
{
    char    *res;

    if ( size == 0 )
    {
        if ( ptr )
            free(ptr);
        return NULL;
    }

    if ( ! ptr )
        return EMalloc(size);

    if ((res = realloc(ptr, size)) == NULL)
    {
        fprintf(stderr, "Unable to realloc %d bytes.  Exiting.\n", size);
        Sced_Abort(FALSE);
    }

    return res;
}


/*  char*
**  wrealloc(char *ptr, unsigned int size);
**  Tries to realloc size bytes, warning and returning NULL on failure.
*/
char*
WRealloc(char *ptr, unsigned int size)
{
    char    *res;

    if ((res = realloc(ptr, size)) == NULL)
        fprintf(stderr, "Unable to realloc %d bytes.\n", size);

    return res;
}



/*  void
**  Popup_Error(char *text, Widget parent, char *title)
**  Pops up an error dialog box.
*/
void
Popup_Error(char *text, Widget parent, char *title)
{
    XtAppContext    context;
    XEvent          event;
    static Widget   err_dialog = NULL;
    Arg     arg;

    popup_err_shell = XtCreatePopupShell(title, transientShellWidgetClass,
                                        main_window.shell, NULL, 0);

    XtSetArg(arg, XtNlabel, text);
    err_dialog = XtCreateManagedWidget("errDialog", dialogWidgetClass,
                                        popup_err_shell, &arg, 1);

    XawDialogAddButton(err_dialog, "Acknowledge", Acknowledge_Error, NULL);

    XtVaSetValues(XtNameToWidget(err_dialog, "label"),
                  XtNborderWidth, 0, NULL);

    XtRealizeWidget(popup_err_shell);

    Position_Shell(popup_err_shell, FALSE);

    XtPopup(popup_err_shell, XtGrabExclusive);

    error_down = FALSE;
    context = XtWidgetToApplicationContext(main_window.shell);
    while ( ! error_down )
    {
        XtAppNextEvent(context, &event);
        XtDispatchEvent(&event);
    }
}


static void
Acknowledge_Error(Widget w, XtPointer cl, XtPointer ca)
{
    XtPopdown(popup_err_shell);

    XtDestroyWidget(popup_err_shell);

    error_down = TRUE;
}



/*  Dimension
**  Match_Widths(Widget *widgets, int num)
**  Makes all the widgets the width of the widest.
**  Returns the maximum width.
*/
Dimension
Match_Widths(Widget *widgets, int num)
{
    Dimension   max_width;
    Dimension   width;
    Arg         arg;
    int         i;

    max_width = 0;
    XtSetArg(arg, XtNwidth, &width);
    for ( i = 0 ; i < num ; i++ )
    {
        XtGetValues(widgets[i], &arg, 1);
        if ( width > max_width ) max_width = width;
    }

    XtSetArg(arg, XtNwidth, max_width);
    for ( i = 0 ; i < num ; i++ )
        XtSetValues(widgets[i], &arg, 1);

    return max_width;
}


/*  void
**  Destroy_World(Boolean destroy_bases)
**  Destroys the world and all that's in it.
*/
void
Destroy_World(Boolean destroy_bases)
{
    InstanceList    inst, temp_inst;

    Free_Selection_List(main_window.selected_instances);
    main_window.selected_instances = NULL;

    for ( inst = main_window.edit_instances ; inst ; inst = temp_inst )
    {
        temp_inst = inst->next;
        Delete_Edit_Instance(&main_window, inst);
    }
    main_window.edit_instances = NULL;

    /* Destroy all the instances first. */
    for ( inst = main_window.all_instances ; inst != NULL ; inst = temp_inst )
    {
        temp_inst = inst->next;
        Destroy_Instance(Delete_Instance(inst));
    }
    main_window.all_instances = NULL;

    if ( csg_window.shell )
        CSG_Reset();

    if ( agg_window.shell )
        Agg_Reset();

    if ( destroy_bases )
    {
        Destroy_All_Base_Objects();
        Reset_Layers();
        View_Reset();
    }

    changed_scene = FALSE;

    /* Redraw an empty world. */
    View_Update(&main_window, main_window.all_instances, ViewNone);

    XFlush(XtDisplay(main_window.shell));
}


/*  Boolean
**  Check_Rectangle_Intersection(XPoint min1, XPoint max1,
**                               XPoint min2, XPoint max2)
**  Returns TRUE if the 2 rectangles defined by the points intersect.
*/
Boolean
Check_Rectangle_Intersection(XPoint min1, XPoint max1, XPoint min2, XPoint max2)
{
    XPoint  min_int, max_int;

    /* Intersect them, and see if the result is valid. */
    min_int.x = max(min1.x, min2.x);
    max_int.x = min(max1.x, max2.x);

    if ( max_int.x < min_int.x )
        return FALSE;

    min_int.y = max(min1.y, min2.y);
    max_int.y = min(max1.y, max2.y);

    if ( max_int.y < min_int.y )
        return FALSE;

    return TRUE;
}


/*  void
**  Set_Prompt(WindowInfoPtr window, String new)
**  Sets the main prompt string.
*/
void
Set_Prompt(WindowInfoPtr window, String new)  
{
    XawTextBlock    text_block;
    int old_length;

    old_length = strlen(window->text_string);
    text_block.format = FMT8BIT;
    text_block.firstPos = 0;
    strcpy(window->text_string, new);
    text_block.length = strlen(window->text_string);
    text_block.ptr = window->text_string;
    XawTextReplace(window->text_widget, 0, old_length + 1, &text_block);
}



/*  Boolean
**  Points_Colinear(Vector p1, Vector p2, Vector p3)
**  Returns TRUE if the three screen points are colinear.
*/
Boolean
Points_Colinear(Vector p1, Vector p2, Vector p3)
{
    Vector temp1, temp2, temp3;

    VSub(p1, p2, temp1);
    VSub(p1, p3, temp2);
    VCross(temp1, temp2, temp3);
    return VZero(temp3);
}

Boolean
Points_Colinear_f(Vector p1, Vector p2, Vector p3)
{
    Vector temp1, temp2, temp3;

    VSub(p1, p2, temp1);
    VSub(p1, p3, temp2);
    VCross(temp1, temp2, temp3);
    return VZero(temp3);
}

/*  void
**  Set_WindowInfo(WindowInfo *inf)
**  Initializes all the fields of a window info structure.
*/
void
Set_WindowInfo(WindowInfo *inf)
{
    inf->shell = NULL;
    inf->view_widget = NULL;
    inf->text_string = New(char, ENTRY_STRING_LENGTH);
    inf->all_instances = NULL;
    inf->edit_instances = NULL;
    inf->current_state = SelectionState;
    inf->off_screen = 0;
    inf->width = inf->height = 0;
}



void
Save_Temp_Filename(char *name)
{
    if ( num_temp_files == 0 )
        temp_filenames = New(char*, 1);
    else
        temp_filenames = More(temp_filenames, char*, num_temp_files + 1);
    temp_filenames[num_temp_files++] = Strdup(name);
}


void
Vector_To_Rotation_Matrix(Vector *vect, Matrix *res)
{
    Matrix  y_rot, z_rot;
    Vector  v1, v2, v3;
    double  cos_a, sin_a;
    double  rads;

    rads = vect->x * M_PI / 180.0;
    cos_a = cos(rads);
    sin_a = sin(rads);
    VNew(1, 0, 0, v1);
    VNew(0, cos_a, -sin_a, v2);
    VNew(0, sin_a, cos_a, v3);
    MNew(v1, v2, v3, *res);

    rads = vect->y * M_PI / 180.0;
    cos_a = cos(rads);
    sin_a = sin(rads);
    VNew(cos_a, 0, sin_a, v1);
    VNew(0, 1, 0, v2);
    VNew(-sin_a, 0, cos_a, v3);
    MNew(v1, v2, v3, y_rot);
    *res = MMMul(&y_rot, res);

    rads = vect->z * M_PI / 180.0;
    cos_a = cos(rads);
    sin_a = sin(rads);
    VNew(cos_a, -sin_a, 0, v1);
    VNew(sin_a, cos_a, 0, v2);
    VNew(0, 0, 1, v3);
    MNew(v1, v2, v3, z_rot);
    *res = MMMul(&z_rot, res);
}


/* Popup shell positioning routine. */
void
Position_Shell(Widget w, Boolean offset)
{
    Dimension       width, height, b_width;
    int             x, y, max_x, max_y;
    Window          root, child;
    int             dummyx, dummyy;
    unsigned int    dummymask;
    Arg             args[3];
    int             n;
    
    XQueryPointer(XtDisplay(w), XtWindow(w), &root, &child, &x, &y, &dummyx,
                  &dummyy, &dummymask);
    n = 0;
    XtSetArg(args[n], XtNwidth, &width);            n++;
    XtSetArg(args[n], XtNheight, &height);          n++;
    XtSetArg(args[n], XtNborderWidth, &b_width);    n++;
    XtGetValues(w, args, n);

    width += 2 * b_width;
    height += 2 * b_width;

    x -= ( (Position) width/2 );
    if ( offset )
        x += (Position)width;
    if (x < 0) x = 0;
    if ( x > (max_x = (Position) (XtScreen(w)->width - width)) ) x = max_x;

    y -= ( (Position) height/2 );
    if ( offset )
        y += (Position)height;
    if (y < 0) y = 0;
    if ( y > (max_y = (Position) (XtScreen(w)->height - height)) ) y = max_y;
    
    n = 0;
    XtSetArg(args[n], XtNx, x); n++;
    XtSetArg(args[n], XtNy, y); n++;
    XtSetValues(w, args, n);
}


/*  void
**  Window_Axes_Initialize(...);
**  Sets up endpoints for each axis.
*/
extern Wireframe    axes_wireframe;
void
Window_Axes_Initialize(WindowInfoPtr window)
{
    int i;

    NewIdentityMatrix(window->axes.o_transform.matrix);
    VNew(0, 0, 0, window->axes.o_transform.displacement);
    window->axes.o_num_vertices = 7;
    window->axes.o_num_real = 7;
    window->axes.o_world_verts = New(Vector, 7);
    window->axes.o_main_verts = New(Vertex, 7);
    window->axes.o_num_faces = 0;
    window->axes.o_flags |= ObjVisible;
    window->axes.o_wireframe = &axes_wireframe;
    for ( i = 0 ; i < axes_wireframe.num_vertices ; i++ )
        window->axes.o_world_verts[i] = axes_wireframe.vertices[i];
}


#if ! HAVE_STRERROR
char*
StrError(int num)
{
    if ( num < 0 )
        return "";
    return sys_errlist[num];
}
#endif
