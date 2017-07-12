/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/events.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: events.c,v $
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
**  events.c : event handling functions.
**
*/

#include <sced.h>
#include <select_point.h>
#include <View.h>
#include <X11/cursorfont.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Text.h>


static Cursor   viewpoint_cursor = 0;
static Cursor   distance_cursor = 0;
static Cursor   eye_cursor = 0;

/* Translation tables. */
String  selection_string =
        "Ctrl <BtnDown> : Main_Button_Down(ctrl)\n"
        "Shift <BtnDown> : Main_Button_Down(shift)\n"
        "Meta <BtnDown> : Main_Button_Down(meta)\n"
        "Alt <BtnDown> : Main_Button_Down(meta)\n"
        "<BtnDown> : Main_Button_Down()\n"
        "<PtrMoved> : Main_Motion()\n"
        "<BtnUp> : Main_Button_Up()\n"
        "<Key>+ : Change_Magnify(plus)\n"
        "<Key>- : Change_Magnify(minus)";

/* Apply 'callback' function. */
extern void Apply_Viewfrom_Text(WindowInfoPtr);
extern void Apply_Distance_Text(WindowInfoPtr, Boolean);
extern void Prepare_Change_Look(WindowInfoPtr);

void    Cancel_Change_Look_Event();


/* A function to clear the prompt at the bottom of the screen. */
static void Reset_Prompt(WindowInfoPtr);

#define Transl(s, t)    ( t ? t : ( t = XtParseTranslationTable(s) ) )

void
Cancel_Viewport_Change(WindowInfoPtr window)
{
    if ( window->current_state & ViewpointState )
        window->current_state ^= ViewpointState;
    else if ( window->current_state & PanState )
        window->current_state ^= PanState; 
    else if ( window->current_state & DistanceState )
        window->current_state ^= DistanceState;
    else if ( window->current_state & EyeState )
        window->current_state ^= EyeState;

    if ( window->current_state & EditState )
    {
        if ( window == &main_window )
            Sensitize_Main_Buttons(TRUE,
                ( SenEdit|SenObject|SenView|SenWindow|SenLights ));
        else if ( window == &csg_window )
            Sensitize_CSG_Buttons(TRUE,
                ( SenCSGEdit | SenCSGObject | SenCSGView | SenCSGWindow) );
        else if ( window == &agg_window )
            Sensitize_Agg_Buttons(TRUE,
                ( SenAggEdit | SenAggObject | SenAggView | SenAggWindow) );
        Edit_Sensitize_Buttons(TRUE, SenEditAll);
    }
    else
    {
        Sensitize_Main_Buttons(TRUE, SenAll);
        Sensitize_CSG_Buttons(TRUE, SenCSGAll);
        Sensitize_Agg_Buttons(TRUE, SenAggAll);
    }

    /* Change the cursor back. */
    XDefineCursor(XtDisplay(window->shell),
                XtWindow(window->view_widget), None);

    Reset_Prompt(window);
}


static void
Init_View_Change(WindowInfoPtr window)
{
    char        prompt_string[ENTRY_STRING_LENGTH];

    /* Change the cursor. */
    if ( ! viewpoint_cursor )
        viewpoint_cursor = XCreateFontCursor(XtDisplay(window->shell),XC_fleur);
    XDefineCursor(XtDisplay(window->shell),
                    XtWindow(window->view_widget), viewpoint_cursor);

    /* Set the label at the bottom of the screen. */
    XtVaSetValues(window->text_label,
                XtNlabel, "Viewpoint:", XtNjustify, XtJustifyRight, NULL);

    /* Set the text at the bottom of the screen. */
    sprintf(prompt_string, "%1.3g %1.3g %1.3g",
            window->viewport.view_from.x,
            window->viewport.view_from.y,
            window->viewport.view_from.z);
    Set_Prompt(window, prompt_string);

    Sensitize_Main_Buttons(FALSE, SenAll);
    Sensitize_CSG_Buttons(FALSE, SenCSGAll);
    Sensitize_Agg_Buttons(FALSE, SenAggAll);
    if ( window->current_state & EditState )
        Edit_Sensitize_Buttons(FALSE, SenEditAll ^ SenEditUndo);
}



/*  void
**  Initiate_Viewfrom_Change(Widget w, XtPointer data, XtPointer call_data)
**  Sets up a view change operation by adding translations to the
**  appropriate widget.
*/
void
Initiate_Viewfrom_Change(Widget w, XtPointer cl_data, XtPointer call_data)
{
    ((WindowInfoPtr)cl_data)->current_state |= ViewpointState;

    Init_View_Change((WindowInfoPtr)cl_data);
}


/*  void
**  Initiate_Pan_Change(Widget w, XtPointer data, XtPointer call_data)
**  Sets up a pan operation by adding translations to the
**  appropriate widget.
*/
void
Initiate_Pan_Change(Widget w, XtPointer cl_data, XtPointer call_data)
{
    ((WindowInfoPtr)cl_data)->current_state |= PanState;

    Init_View_Change((WindowInfoPtr)cl_data);
}


/*  void
**  Initiate_Distance_Change(WindowInfoPtr w, Boolean do_view)
**  Sets up a view distance change operation by adding translations to the
**  appropriate widget.
*/
void
Initiate_Distance_Change(WindowInfoPtr w, Boolean do_view)
{
    char    prompt_string[ENTRY_STRING_LENGTH];
    Arg     args[5];
    int     n;

    n = 0;
    /* Install the new translations. */
    if (do_view)
    {
        w->current_state |= DistanceState;
        if ( ! distance_cursor )
            distance_cursor =
                    XCreateFontCursor(XtDisplay(w->shell), XC_center_ptr);
        XDefineCursor(XtDisplay(w->shell), XtWindow(w->view_widget),
                      distance_cursor);
        XtSetArg(args[n], XtNlabel, "Distance:");   n++;
        sprintf(prompt_string, "%1.3g"HPFIX, w->viewport.view_distance);
    }
    else
    {
        w->current_state |= EyeState;
        if ( ! eye_cursor )
            eye_cursor = XCreateFontCursor(XtDisplay(w->shell), XC_target);
        XDefineCursor(XtDisplay(w->shell), XtWindow(w->view_widget),eye_cursor);
        XtSetArg(args[n], XtNlabel, "Eye:");    n++;
        sprintf(prompt_string, "%1.3g"HPFIX, w->viewport.eye_distance);
    }

    /* Set the label at the bottom of the screen. */
    XtSetArg(args[n], XtNjustify, XtJustifyRight);  n++;
    XtSetValues(w->text_label, args, n);

    /* Force an update. */
    Set_Prompt(w, prompt_string);

    Sensitize_Main_Buttons(FALSE, SenAll);
    Sensitize_CSG_Buttons(FALSE, SenCSGAll);
    Sensitize_Agg_Buttons(FALSE, SenAggAll);
}



void
Apply_Button_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
    WindowInfoPtr   window = (WindowInfoPtr)cl_data;

    if ( window->current_state & ( ViewpointState | PanState ) )
        Apply_Viewfrom_Text(window);
    else if ( window->current_state & ( DistanceState | EyeState ) )
        Apply_Distance_Text(window, window->current_state & DistanceState);
}


void
Apply_Button_Action(Widget w, XEvent *e, String *s, Cardinal *c)
{
    if ( w == main_window.text_widget )
        XtCallCallbacks(main_window.apply_button, XtNcallback, NULL);
    else if ( w == csg_window.text_widget )
        XtCallCallbacks(csg_window.apply_button, XtNcallback, NULL);
    else if ( w == agg_window.text_widget )
        XtCallCallbacks(agg_window.apply_button, XtNcallback, NULL);
}


static void
Reset_Prompt(WindowInfoPtr window)
{
    Dimension   width;

    /* Set the label at the bottom of the screen. */
    XtVaGetValues(window->apply_button, XtNwidth, &width, NULL);
    XtVaSetValues(window->text_label, XtNwidth, width,
                XtNjustify, XtJustifyCenter, XtNlabel, "Ready", NULL);

    /* Clear the text entry. */
    Set_Prompt(window, "");
}


/*  void
**  Redraw_Main_View(Widget w, XtPointer client_data, XtPointer call_data)
**  The exposure callback for the screens.
*/
void
Redraw_Main_View(Widget w, XtPointer client_data, XtPointer call_data)
{
    WindowInfoPtr       window = (WindowInfoPtr)client_data;
    Dimension           new_width, new_height;

    XtVaGetValues(window->view_widget,
                  XtNwidth, &new_width,
                  XtNheight, &new_height, NULL);

    if ( new_height == window->height && new_width == window->width )
        View_Update(window, window->all_instances, JustExpose);
    else
    {
        window->height = new_height;
        window->width = new_width;
        View_Update(window, window->all_instances, CalcScreen | NewSize);
        Update_Projection_Extents(window->all_instances);
    }
}



/*  void
**  Initiate_Object_Edit(WindowInfoPtr window)
**  Prepares the widget for a placement sequence.
**  This means drawing everything in a new way and desensitizing most of the
**  buttons on the screen.
*/
void
Initiate_Object_Edit(WindowInfoPtr window)
{
    window->current_state &= ( AllState ^ SelectionState );
    window->current_state |= EditState;

    if ( window == &main_window )
    {
        Sensitize_Main_Buttons(FALSE,
          SenAll ^ ( SenEdit | SenObject | SenView | SenWindow | SenLights ));
        Sensitize_CSG_Buttons(FALSE, SenCSGAll);
        Sensitize_Agg_Buttons(FALSE, SenAggAll);
    }
    else if ( window == &csg_window )
    {
        Sensitize_Main_Buttons(FALSE, SenAll);
        Sensitize_CSG_Buttons(FALSE,
            SenCSGAll^( SenCSGEdit | SenCSGObject | SenCSGView | SenCSGWindow));
        Sensitize_Agg_Buttons(FALSE, SenAggAll);
    }
    else if ( window == &agg_window )
    {
        Sensitize_Main_Buttons(FALSE, SenAll);
        Sensitize_CSG_Buttons(FALSE, SenCSGAll);
        Sensitize_Agg_Buttons(FALSE,
            SenAggAll^( SenAggEdit | SenAggObject | SenAggView | SenAggWindow));
    }
}


void
Cancel_Object_Edit(WindowInfoPtr window)
{
    window->current_state ^= EditState;

    Sensitize_Main_Buttons(TRUE, SenAll);
    Sensitize_CSG_Buttons(TRUE, SenCSGAll);
    Sensitize_Agg_Buttons(TRUE, SenAggAll);

    window->current_state |= SelectionState;
}



/*  void
**  Change_Lookat_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
**  Initiates a change to the lookat point in the window passed as cl_data.
*/
void
Change_Lookat_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
    WindowInfoPtr   window = (WindowInfoPtr)cl_data;

    window->current_state |= LookState;

    Sensitize_Main_Buttons(FALSE, SenAll);
    Sensitize_CSG_Buttons(FALSE, SenCSGAll);
    Sensitize_Agg_Buttons(FALSE, SenAggAll);
    if ( window->current_state & EditState )
        Edit_Sensitize_Buttons(FALSE, SenEditAll ^ SenEditUndo);

    select_window = window;
    select_highlight = FALSE;
    prompt_offset = TRUE;
    select_center = window->viewport.view_at;
    select_callback = Change_Lookat_Point_Callback;
    Prepare_Change_Look(window);

    Register_Select_Operation(window);
}


/*  void
**  Change_Lookup_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
**  Initiates a change to the lookup vector in the window passed as cl_data.
*/
void
Change_Lookup_Callback(Widget w, XtPointer cl_data, XtPointer ca_data)
{
    WindowInfoPtr   window = (WindowInfoPtr)cl_data; 

    window->current_state |= LookState;

    Sensitize_Main_Buttons(FALSE, SenAll);
    Sensitize_CSG_Buttons(FALSE, SenCSGAll);
    Sensitize_Agg_Buttons(FALSE, SenAggAll);

    select_window = window;
    select_highlight = FALSE;
    prompt_offset = FALSE;
    select_center = window->viewport.view_at;
    select_callback = Change_Lookup_Point_Callback;
    Prepare_Change_Look(window);

    Register_Select_Operation(window);
}

/*  void
**  Cancel_Change_Look_Event()
**  Cancels a currently active look_ event.
*/
void
Cancel_Change_Look_Event(WindowInfoPtr window)
{
    Cancel_Select_Operation(window);

    window->current_state ^= LookState;

    if ( window->current_state == SelectionState )
    {
        Sensitize_Main_Buttons(TRUE, SenAll);
        Sensitize_CSG_Buttons(TRUE, SenCSGAll);
        Sensitize_Agg_Buttons(TRUE, SenAggAll);
    }
    else if ( window->current_state & EditState )
    {
        if ( window == &main_window )
            Sensitize_Main_Buttons(TRUE,
                SenEdit | SenObject | SenView | SenWindow | SenLights);
        else if ( window == &csg_window )
            Sensitize_CSG_Buttons(TRUE,
                SenCSGEdit | SenCSGObject | SenCSGView | SenCSGWindow);
        else if ( window == &agg_window )
            Sensitize_Agg_Buttons(TRUE,
                SenAggEdit | SenAggObject | SenAggView | SenAggWindow);
        Edit_Sensitize_Buttons(TRUE, SenEditAll ^ SenEditUndo);
    }
}


void
Register_Select_Operation(WindowInfoPtr window)
{
    window->current_state |= SelectPointState;
}


void
Cancel_Select_Operation(WindowInfoPtr window)
{
    Cleanup_Selection();

    window->current_state &= ( AllState ^ SelectPointState);
}


#define ButtonsDown(e) \
    ( ( e->type == ButtonPress && \
      ( e->xbutton.button == 1 || e->xbutton.button == 2 || \
        e->xbutton.button == 3 ) ) || \
      ( e->type == MotionNotify && \
        ( e->xmotion.state & ( Button1 | Button2 | Button3 ) ) ) )

/*
**  Intermediary action functions for each state. generally append extra
**  arguments.
*/
static void
Viewpoint_Down_Action(Widget w, XEvent *e, String *s, Cardinal *c, Boolean pan)
{
    String      new_args[2];
    Cardinal    n = 0;

    if ( pan )
        new_args[n++] = "pan";

    if ( e->xbutton.button == 1 )
        new_args[n++] = "both";
    else if ( e->xbutton.button == 2 )
        new_args[n++] = "horiz";
    else if ( e->xbutton.button == 3 )
        new_args[n++] = "vert";

    Start_Newview_Rotation(w, e, new_args, &n);
}


static void
Distance_Down_Action(Widget w, XEvent *e, String *s, Cardinal *c, Boolean eye)
{
    String      new_args[2];
    Cardinal    n = 0;

    if ( eye )
        new_args[n++] = "eye";
    else
        new_args[n++] = "view";

    if ( e->xbutton.button == 1 )
        new_args[n++] = "med";
    else if ( e->xbutton.button == 2 )
        new_args[n++] = "fast";
    else if ( e->xbutton.button == 3 )
        new_args[n++] = "slow";

    Start_Distance_Change(w, e, new_args, &n);
}

static void
Selection_Up_Action(Widget w, XEvent *e, String *s, Cardinal *c, int mask,
                    int ctrl)
{
    String      new_args[2];
    Cardinal    n = 0;

    if ( ctrl )
        new_args[n++] = "edit";

    if ( mask == 1 )
        new_args[n++] = "add";
    else
        new_args[n++] = "delete";

    Finish_Selection_Drag(w, e, new_args, &n);
}


/*
**  Action functions intended to be installed on view widgets in the csg and
**  main windows. It looks at the current window state, the modifiers and
**  buttons in effect and then decides where to send it and with which
**  arguments.
*/

void
Main_Button_Down_Action(Widget w, XEvent *e, String *s, Cardinal *c)
{
    WindowInfoPtr   window = WindowFromWidget(w);
    Boolean         meta = FALSE;
    Boolean         shift = FALSE;
    Boolean         ctrl = FALSE;
    int i;

    for ( i = 0 ; i < *c ; i++ )
    {
        if ( ! strcmp(s[i], "meta") )
            meta = TRUE;
        else if ( ! strcmp(s[i], "shift") )
            shift = TRUE;
        else if ( ! strcmp(s[i], "ctrl") )
            ctrl = TRUE;
    }

    /* Test states in order of precedence. */
    if ( ( window->current_state & ( ViewpointState | PanState ) ) &&
         ButtonsDown(e) )
    {
        Viewpoint_Down_Action(w, e, s, c, window->current_state & PanState);
        window->current_state |= ViewpointDragState;
        return;
    }

    if ( window->current_state & ( DistanceState | EyeState ) )
    {
        Distance_Down_Action(w, e, s, c, window->current_state & EyeState);
        window->current_state |= DistanceDragState;
        return;
    }

    if ( ( shift || meta ) && ButtonsDown(e) )
    {
        Viewpoint_Down_Action(w, e, s, c, meta);
        window->current_state |= ViewpointDragState;
        return;
    }

    if ( window->current_state & SelectPointState )
    {
        Select_Highlight_Object(w, e, s, c);
        return;
    }

    if ( window->current_state & EditState )
    {
        Edit_Start_Drag(w, e, s, c);
        return;
    }

    if ( ( window->current_state & SelectionState ) && ButtonsDown(e) )
    {
        Start_Selection_Drag(w, e, s, c);
        window->current_state |= SelectionDragState;
        if ( ctrl )
            window->current_state |= CtrlState;
        return;
    }
}


void
Main_Motion_Action(Widget w, XEvent *e, String *s, Cardinal *c)
{
    WindowInfoPtr   window = WindowFromWidget(w);

    if ( window->current_state & ViewpointDragState )
    {
        Newview_Rotation(w, e, s, c);
        return;
    }

    if ( window->current_state & DistanceDragState )
    {
        Distance_Change(w, e, s, c);
        return;
    }

    if ( window->current_state &
         ( ViewpointState | PanState | DistanceState | EyeState ) )
        return;

    if ( window->current_state & SelectPointState )
    {
        Select_Highlight_Action(w, e, s, c);
        return;
    }

    if ( window->current_state & EditState )
    {
        Edit_Continue_Drag(w, e, s, c);
        return;
    }

    if ( window->current_state & SelectionDragState )
        Continue_Selection_Drag(w, e, s, c);

}


void
Main_Button_Up_Action(Widget w, XEvent *e, String *s, Cardinal *c)
{
    WindowInfoPtr   window = WindowFromWidget(w);

    if ( window->current_state & ViewpointDragState )
    {
        Stop_Newview_Rotation(w, e, s, c);
        window->current_state ^= ViewpointDragState;
        return;
    }

    if ( window->current_state & DistanceDragState )
    {
        Stop_Distance_Change(w, e, s, c);
        window->current_state ^= DistanceDragState;
        return;
    }

    if ( window->current_state & SelectPointState )
    {
        Select_Point_Action(w, e, s, c);
        return;
    }

    if ( window->current_state & EditState )
    {
        Edit_Finish_Drag(w, e, s, c);
        return;
    }

    if ( window->current_state & SelectionDragState )
    {
        Selection_Up_Action(w, e, s, c, e->xbutton.button,
                            window->current_state & CtrlState);
        window->current_state ^= SelectionDragState;
        if ( window->current_state & CtrlState )
            window->current_state ^= CtrlState;
    }
}


