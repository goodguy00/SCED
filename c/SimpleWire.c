/*
 * $Header: /home/barad-dur/vision/forsyth/schenney/sced-0.94/c/RCS/SimpleWire.c,v 1.0 1997/05/06 20:30:22 schenney Exp $
 *
 * $Log: SimpleWire.c,v $
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
**  SimpleWire.c : C file for the SimpleWire widget class.
**
**  Created: 19/03/94
*/

#include <X11/IntrinsicP.h>
#include <math.h>
#include <sced.h>
#include <SimpleWireP.h>

static XtResource resources[] = {
#define offset(field) XtOffsetOf(SimpleWireRec, simpleWire.field)
    /* {name, class, type, size, offset, default_type, default_addr}, */
    { XtNbasePtr, XtCValue, XtRPointer, sizeof(XtPointer),
      offset(base_ptr), XtRPointer, NULL },
    { XtNforeground, XtCColor, XtRPixel, sizeof(Pixel), offset(drawing_color),
      XtRString, XtDefaultForeground },
    { XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct*), offset(font),
      XtRString, XtDefaultFont },
    { XtNcallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
      offset(select_callback), XtRCallback, NULL }
#undef offset
};


/* Prototypes for various procedures. */
static void InitializeSimpleWire(Widget request, Widget new, ArgList args,
                                 Cardinal *num_args);
static void DestroySimpleWire(Widget w);
static Boolean SetSimpleWire(Widget old, Widget request, Widget new,
                             ArgList args, Cardinal *num_args);
static void Resize(Widget w);
static void Redisplay(Widget w, XEvent *event, Region region);
static void Select_Wireframe_Mouse_Action(Widget, XEvent*, String*, Cardinal*);
static void Select_Wireframe_Key_Action(Widget, XEvent*, String*, Cardinal*);


static XtActionsRec actions[] =
{
  /* {name, procedure}, */
    {"selectWireframe", Select_Wireframe_Mouse_Action},
    {"keypressWireframe", Select_Wireframe_Key_Action}
};


static char translations[] = "<BtnUp> : selectWireframe()\n\
                              <Key>0 : keypressWireframe(0)\n\
                              <Key>1 : keypressWireframe(1)\n\
                              <Key>2 : keypressWireframe(2)\n\
                              <Key>3 : keypressWireframe(3)\n\
                              <Key>4 : keypressWireframe(4)\n\
                              <Key>5 : keypressWireframe(5)\n\
                              <Key>6 : keypressWireframe(6)\n\
                              <Key>7 : keypressWireframe(7)\n\
                              <Key>8 : keypressWireframe(8)\n\
                              <Key>9 : keypressWireframe(9)";


SimpleWireClassRec simpleWireClassRec = {
  { /* core fields */
    /* superclass       */  (WidgetClass) &widgetClassRec,
    /* class_name       */  "SimpleWire",
    /* widget_size      */  sizeof(SimpleWireRec),
    /* class_initialize */  NULL,
    /* class_part_initialize    */  NULL,
    /* class_inited     */  FALSE,
    /* initialize       */  InitializeSimpleWire,
    /* initialize_hook  */  NULL,
    /* realize          */  XtInheritRealize,
    /* actions          */  actions,
    /* num_actions      */  XtNumber(actions),
    /* resources        */  resources,
    /* num_resources    */  XtNumber(resources),
    /* xrm_class        */  NULLQUARK,
    /* compress_motion  */  TRUE,
    /* compress_exposure*/  TRUE,
    /* compress_enterleave  */  TRUE,
    /* visible_interest */  FALSE,
    /* destroy          */  DestroySimpleWire,
    /* resize           */  Resize,
    /* expose           */  Redisplay,
    /* set_values       */  SetSimpleWire,
    /* set_values_hook  */  NULL,
    /* set_values_almost*/  XtInheritSetValuesAlmost,
    /* get_values_hook  */  NULL,
    /* accept_focus     */  NULL,
    /* version          */  XtVersion,
    /* callback_private */  NULL,
    /* tm_table         */  translations,
    /* query_geometry   */  XtInheritQueryGeometry,
    /* display_accelerator  */  XtInheritDisplayAccelerator,
    /* extension        */  NULL
  },
  { /* simpleWire fields */
    /* empty            */  0
  }
};

WidgetClass simpleWireWidgetClass = (WidgetClass)&simpleWireClassRec;

static unsigned char	desensitized_stipple_bits[] = { 0x01, 0x02 };
static Pixmap		desensitized_stipple = 0;

/*  void
**  InitializeSimpleWire( ... )
**  The initialization procedure for SimpleWire widgets.
**  Does anything that can be done with the given args, in particular
**  sets up the wireframe (through its base object).
**  Also sets up the drawing_gc.
*/
static void
InitializeSimpleWire(Widget request, Widget new, ArgList args,
                     Cardinal *num_args)
{
    SimpleWireWidget    sww = (SimpleWireWidget)new;
    XGCValues           gc_vals;

    sww->simpleWire.wireframe = NULL;
    sww->simpleWire.scale = 1;
    if (sww->simpleWire.base_ptr != NULL)
    {
        BaseObjectPtr   base = (BaseObjectPtr)sww->simpleWire.base_ptr;

        /* A base object name has been specified. */
        sww->simpleWire.wireframe = base->b_wireframes[0];

    }

    /* Allocate a gc for drawing with. */
    gc_vals.foreground = sww->simpleWire.drawing_color;
    gc_vals.background = sww->core.background_pixel;
    gc_vals.font = sww->simpleWire.font->fid;
    sww->simpleWire.drawing_gc =
            XtGetGC((Widget)sww, GCForeground|GCBackground|GCFont, &gc_vals);
    gc_vals.fill_style = FillOpaqueStippled;
    if ( ! desensitized_stipple )
	desensitized_stipple = XCreateBitmapFromData(XtDisplay(new),
				    DefaultRootWindow(XtDisplay(new)),
				    desensitized_stipple_bits, 2, 2);
    gc_vals.stipple = desensitized_stipple;
    sww->simpleWire.desensitized_gc =
            XtGetGC((Widget)sww,
		    GCForeground|GCBackground|GCFillStyle|GCStipple, &gc_vals);

    sww->simpleWire.off_screen = 0;
}


/*  void
**  DestroySimpleWire(Widget w)
**  Frees memory and the drawing_gc.
*/
static void
DestroySimpleWire(Widget w)
{
    SimpleWireWidget    sww = (SimpleWireWidget)w;

    if ( sww->simpleWire.off_screen )
        XFreePixmap(XtDisplay(w), sww->simpleWire.off_screen);
    XtReleaseGC((Widget)sww, sww->simpleWire.drawing_gc);
    XtReleaseGC((Widget)sww, sww->simpleWire.desensitized_gc);
}


static void
Redraw(SimpleWireWidget sww)
{
    WireframePtr    wireframe = (WireframePtr)sww->simpleWire.wireframe;
    double  	    max_x, max_y;
    Vertex  	    *these_vertices;
    Vector  	    *normals;
    Vector  	    *world_verts;
    Viewport        vp;
    int     	    i;
    Vector  	    sum;
    GC      	    clear_gc;
    XGCValues       gc_vals;
    Cuboid  	    bound;
    int     	    x_char_pos, y_char_pos;

    if ( ! XtIsRealized((Widget)sww) ) return;
    if ( ! sww->simpleWire.off_screen )
	return;
    if ( wireframe == NULL ) return;

    /* Need a group of vertices to work with. */
    these_vertices = New(Vertex, wireframe->num_real_verts);

    /* Convert them to view. */
    /* Need a viewport to use. */
    /* View at is the centroid. Need to calculate it. */
    bound = Calculate_Bounds(wireframe->vertices, wireframe->num_real_verts);
    VAdd(bound.min, bound.max, sum);
    VScalarMul(sum, 0.5, vp.view_at);

    VNew(5, 4, 3, vp.view_from);
    VNew(0, 0, 1, vp.view_up);
    vp.view_distance = 500;
    vp.eye_distance = 250;
    Build_Viewport_Transformation(&vp);
    world_verts = New(Vector, wireframe->num_real_verts);
    for ( i = 0 ; i < wireframe->num_real_verts ; i++ )
        world_verts[i] = wireframe->vertices[i];
    Convert_World_To_View(world_verts, these_vertices,
                            wireframe->num_real_verts, &vp);

    max_x = max_y = 0.0;
    for ( i = 0 ; i < wireframe->num_real_verts ; i++ )
    {
        if ( these_vertices[i].view.z < 0 )
            continue;
        if (fabs(these_vertices[i].view.x) > max_x)
            max_x = fabs(these_vertices[i].view.x);
        if (fabs(these_vertices[i].view.y) > max_y)
            max_y = fabs(these_vertices[i].view.y);
    }

    if ( max_x == 0.0 && max_y == 0.0 )
        if ( max_y == 0.0 )
            sww->simpleWire.scale = 50;
        else
            sww->simpleWire.scale = (sww->core.height - 5) / max_y;
    else
        if ( max_y == 0.0 )
            sww->simpleWire.scale = (sww->core.width - 5) / max_x;
        else
            sww->simpleWire.scale =
                    (int)min(((sww->core.width - 5) / max_x ),
                             ((sww->core.height - 5) / max_y ));

    /* Recalculate screen co-ords for each vertex. */
    Convert_View_To_Screen(these_vertices, wireframe->num_real_verts, &vp,
        (short)sww->core.width, (short)sww->core.height,
        (double)(sww->simpleWire.scale));

    /* Also need a set of normals. */
    /* These come from the wireframe specs. */
    normals = New(Vector, wireframe->num_faces);
    for ( i = 0 ; i < wireframe->num_faces ; i++ )
        normals[i] = wireframe->faces[i].normal;

    gc_vals.function = GXcopy;
    gc_vals.foreground = sww->core.background_pixel;
    clear_gc = XtGetGC((Widget)sww, GCFunction | GCForeground, &gc_vals);
    XFillRectangle(XtDisplay((Widget)sww), sww->simpleWire.off_screen, clear_gc,
                    0, 0, sww->core.width, sww->core.height);
    XtReleaseGC((Widget)sww, clear_gc);

    if ( sww->core.sensitive && sww->core.ancestor_sensitive )
	Draw_Visible_Edges(XtDisplay((Widget)sww), sww->simpleWire.off_screen,
		    sww->simpleWire.drawing_gc,
		    wireframe, these_vertices, world_verts, normals, &vp);
    else
	Draw_Visible_Edges(XtDisplay((Widget)sww), sww->simpleWire.off_screen,
		    sww->simpleWire.desensitized_gc,
		    wireframe, these_vertices, world_verts, normals, &vp);
    
    x_char_pos = sww->simpleWire.font->min_bounds.lbearing + 5;
    y_char_pos =
        sww->core.height - sww->simpleWire.font->max_bounds.descent - 5;

    if ( sww->core.sensitive && sww->core.ancestor_sensitive )
	XDrawString(XtDisplay((Widget)sww), sww->simpleWire.off_screen,
		    sww->simpleWire.drawing_gc, x_char_pos, y_char_pos,
		    ((BaseObjectPtr)sww->simpleWire.base_ptr)->b_label,
		    strlen(((BaseObjectPtr)sww->simpleWire.base_ptr)->b_label));
    else
	XDrawString(XtDisplay((Widget)sww), sww->simpleWire.off_screen,
		    sww->simpleWire.desensitized_gc, x_char_pos, y_char_pos,
		    ((BaseObjectPtr)sww->simpleWire.base_ptr)->b_label,
		    strlen(((BaseObjectPtr)sww->simpleWire.base_ptr)->b_label));

    free(these_vertices);
    free(world_verts);
    free(normals);
}


/*  Boolean
**  SetSimpleWire( ... )
**  The SetValues procedure for SimpleWire widgets.
*/
static Boolean
SetSimpleWire(Widget old, Widget request, Widget new, ArgList args,
                                                        Cardinal *num_args)
{
    SimpleWireWidget    newsww = (SimpleWireWidget)new;
    SimpleWireWidget    oldsww = (SimpleWireWidget)old;
    XGCValues           gc_vals;
    Boolean             need_redraw = FALSE;


#define NE(field) (newsww->simpleWire.field != oldsww->simpleWire.field)
#define NECore(field) (newsww->core.field != oldsww->core.field)

    /* If drawing_color or font have changed, need new GC. */
    if (NE(font) || NE(drawing_color))
    {
        XtReleaseGC((Widget)oldsww, oldsww->simpleWire.drawing_gc);
        gc_vals.foreground = newsww->simpleWire.drawing_color;
        gc_vals.background = newsww->core.background_pixel;
        gc_vals.font = newsww->simpleWire.font->fid;
        newsww->simpleWire.drawing_gc =
            XtGetGC((Widget)newsww, GCForeground|GCBackground|GCFont, &gc_vals);
	gc_vals.fill_style = FillOpaqueStippled;
	if ( ! desensitized_stipple )
	    desensitized_stipple = XCreateBitmapFromData(XtDisplay(new),
				    DefaultRootWindow(XtDisplay(new)),
				    desensitized_stipple_bits, 2, 2);
	gc_vals.stipple = desensitized_stipple;
	newsww->simpleWire.desensitized_gc =
		XtGetGC((Widget)newsww,
			GCForeground|GCBackground|GCFillStyle|GCStipple,
			&gc_vals);

        need_redraw = TRUE;
    }

    if ( NECore(sensitive) )
    {
        Redraw(newsww);
	need_redraw = TRUE;
    }

    return need_redraw;
}



/*  void Resize(Widget w)
**  Returns if the widget is unrealized.
**  Clears the window with exposures TRUE.
*/
static void
Resize(Widget w)
{
    SimpleWireWidget    sww = (SimpleWireWidget)w;

    if ( ! XtIsRealized(w) ) return;
    if ( sww->core.width == 0 ||  sww->core.height == 0 ) return;

    if ( sww->simpleWire.off_screen )
        XFreePixmap(XtDisplay(w), sww->simpleWire.off_screen);
    sww->simpleWire.off_screen = XCreatePixmap(XtDisplay(w), XtWindow(w),
                                    sww->core.width, sww->core.height,
                                    DefaultDepthOfScreen(XtScreen(w)));

    Redraw(sww);
}


/*  void Redisplay(Widget w, XEvent *event, Region region);
**  The expose procedure.
**  Converts all the points for the wireframe to screen, then draws the edges.
*/
static void
Redisplay(Widget w, XEvent *event, Region region)
{
    SimpleWireWidget    sww = (SimpleWireWidget)w;

    if ( ! sww->simpleWire.off_screen )
        Resize(w);

    /* Copy the off screen on. */
    XCopyArea(XtDisplay(w), sww->simpleWire.off_screen,
	      XtWindow(w), sww->simpleWire.drawing_gc, 0, 0,
	      sww->core.width, sww->core.height, 0, 0);
}


static void
Select_Wireframe_Mouse_Action(Widget w, XEvent *event, String *params,
                              Cardinal* num_params)
{
    XtCallCallbacks(w, XtNcallback, (XtPointer)1);
}

static void
Select_Wireframe_Key_Action(Widget w, XEvent *event, String *params,
                            Cardinal* num_params)
{
    int count = (int)((*params)[0]) - '0';
    if ( count == 0 ) count = 10;
    XtCallCallbacks(w, XtNcallback, (XtPointer)count);
}



/*  void
**  Update_SimpleWire_Wireframe(Widget widget, WireframePtr wireframe)
**  Updates the widget the represent the new wireframe.
*/
void
Update_SimpleWire_Wireframe(Widget widget, WireframePtr wireframe)
{
    XtCheckSubclass(widget, SimpleWireClass, "Update_SimpleWire_Wireframe");
    ((SimpleWireWidget)widget)->simpleWire.wireframe = wireframe;
    Redraw((SimpleWireWidget)widget);
}


