# STARTING TUTORIAL - The Basics.

This tutorial runs through the basic editing functions and constraint
management on a click by click basis. The aim is to make you a bit
familiar with the interface.

### It should take 0.5 - 1 hour (very rough estimate).

* Start up the program (the easy bit!). You should see a column of
buttons down the left, a big window with a set of axes in the middle and
a couple of things down the bottom.

* Create a Cube. Click on the New Object button and a dialog will appear
with lots of pretty pictures in it. From left to right, top then bottom
they are: Cube, Sphere, Cylinder, Cone, Square, Plane. The cancel button
is at the bottom - choosing it will pop down the dialog without doing
anything. There is also a desensitized CSG Object button. Ignore that for
the moment. Click on the Cube to create a cube. The cube you just created
will appear at the origin.

* The Edit button shold now be sensitive. It's actually a menu. Click
down on it and a menu containing Cube_0 will appear. All newly created
objects start on this menu, and objects selected for editing appear here
too. That is, their names appear here.

* Edit the cube. Choose it's name from the Edit menu and wait for the
interface to get its act together. Another window will pop up below the
Scene Window. You may need to place it. There are lots of buttons. I won't
describe them all here - check out the user's guide. The Scene window
will also change. The cube appears in blue centered on the origin. It has
a green point at its center - the Object Origin. It has a red point
at one corner - the Scaling Point. There is also a set of axes sticking
out of the Origin, but these are hidden by the world axes.

* Move the cursor around in the window a little, especially over the
Origin (green circle) and Scaling point (red circle). The cursor will
be a pair of arrows most places in the screen, and a circle over the
origin and scaling points. The arrows mean you can rotate the body by
dragging the mouse around in that region. The circles signify that you
can't move those points, in this case because they aren't constrained in
any way.

* Position the cube. Click on the X-Y Plane toggle middle left of the Edit
dialog. A grey square, representing the constraint you just selected
will appear. You chose the X-Y world plane as a constraint on the Origin,
so you can now move the Origin around in the X-Y plane, and the body will
follow it. You will note that the cursor is now a diamond cross while
over the Origin point. This signfies that you can move the origin.
To move, click down in the green circle and drag it where you
want. The Origin moves in the X-Y plane under the cursor.

* After moving the cube you should be able to see the axes sticking
out of the origin. The long red one is the Major Axis. The medium green
one is the Minor Axis and the blue one is the Other axis.

* Check out where you put it. Choose Viewpoint from the View menu in the
Edit Dialog. Everything will become insensitive, and the cursor when in
the Scene Window will be a cross. To change the view, push down with button
1 and move the mouse around. Objects in the foreground of the scene will
appear to follow the mouse. By moving around a bit you can get a good
idea of where the cube is in space. Now use button 2. You will notice
that you can only move around the scene. Using button 3 limits you to moving
up and down. Choose Apply in the Scene Window or Undo in the Edit Dialog
to complete the view change. Try to leave it somewhere near where it was
before, with the X axis to the left and the Y axis to the right.

* Position the cube a bit more. Choose the Z-X Plane constraint. the
rectangle should change to a line, indicating that the Origin constraint
is now a line. The 2 planes have combined to give a line. Dragging the
Origin now moves the object along the line, which is parallel to the world
X axis.

* Scale it a bit. Choose the Major-Other Pl constraint from the Scale
constraint options. A rectangle centered on the Scaling point and in
the Major-Other plane of the object will appear. The Major-Other plane is
that defined by the object's Major axis (the red one sticking out of
the Origin) and the Other Axis (the blue one). You can scale the object in
this plane by dragging the red Scaling point around. The Origin will
stay where it is, and the object will scale so that the Scaling point
stays on the same vertex. Note the cursor is now a pair of sizing arrows
while over the Scaling point.

* Rotate it a bit. First try unconstrained rotation. Click button 1 down
somewhere not on an Origin or Scaling point and drag the object around.
This is the Arcball interface, which is supposed to be intuitive so I won't
try to explain what happens. It will probably make things harder. Now is
probably a good time to introduce undo. Click once on Undo to undo the
rotation drag. Note also that the scaling constraint adjusts as the object
moves.

* Try constrained rotation. Choose Major Axis from the Rotate constraint
options. An arc will appear on the screen. Drag somewhere near that arc and
the object will rotate about its Major axis. This is a more controlled
way to rotate.

* That will do for now. Play a bit with undo. It will undo everything you
just did. You can redo if you undo more than you want.

* Choose Finish to complete editing. The Editing dialog box will desensitize
itself, but stay popped up. You can minimise it if it gets in the way.

* The block should now be sitting where you left it, in the shape you left it.

* Change the view a bit more. Choose Viewpoint from the View menu and move
around a bit.

* Change the At point. Choose LookAt from the View menu. The vertex closest
to the mouse will light up with a red square. As you move the mouse around
the square will follow you, always clickng to the nearest vertex. Note that
the world axes can supply vertices for selection. The cursor also changes
to tell you that the program is waiting for points.
Click inside a highlight square and it will become the
center of the world for viewing purposes. When you move the Viewpoint around
now it will rotate about the point you just selected. You could also type
in any point. Change the LookAt point again, but this time type "0 0 0"
in the text window at the bottom (without the quotes). Hit return to apply
the text you just typed. A dialog pops up asking whether you want the point
to be interpreted as absolute or offset. Choose absolute, and 0 0 0 will
be at the center again. Offset will add the vector you typed in to
the current look-at point, and use that, which in this case will do nothing.
So choose absolute.

* Change LookUp. Select LookUp from the View menu. This time you need to
select or enter 2 points. The direction FROM the first point TO the second
will become up in the scene. Try making the Y axis up, by choosing the point
on the negative end of the Y axis and then the one on the positive end. Y is
now up. Change it back by choosing LookAt again and selecting the negative and
then positive Z axis. You can type points in here again, and offsets are
interpreted as relative to the look-at point.

* Change the distance a bit. Choose distance from the View menu and
drag the distance in or out. Different buttons have different speeds -
1 is medium, 2 fast and 3 slow. Click on Apply to finish the distance
change.

* Change the eye. It's the same as changing the distance. Be careful with
eye and distance changes. Funny things happen when points go behind the
eye and viewplane.

* Change the Image Size. Choose Image Size from the View menu and enter 800
for the width and 600 for the height. Then click on Done. The image window
will resize within scrollbars. This will be the rendered image size if the
renderer supports it (POV doesn't). Choose image size again, this time
click on To Fit. The image will resize itself to match the window size.

* Change the zoom. Click on zoom, type in a new number, say 200, and hit
return. The view should get bigger. Do it again, this time choosing To Fit.
The image will rescale to fit the largest view possible. Go back to a zoom
of 100.

* Select the object. Drag over any (or all) edges with button 1 down. The
cube should highlight. Drag over it with button 2 - should unhighlight.
Reselect it with button 1.

* Choose New from the Layer menu. Enter a name (or use the default). The selected
object has been added to a new layer.

* Select Display from the Layer menu. Both "World" and "Layer 1" (or the name
you entered) are displayed. Click on "Layer 1" or whatever, and the cube
should disappear. Click again and it will reappear. Choose Finish.

* Select Merge from the Layer menu. Choose "Layer 1" first then "World".
Instances in "Layer 1" (the cube) are placed in the world layer, and "Layer 1"
is deleted.

* Go on to target. Click on it and a small dialog will appear. Choose your
favorite raytracer.

* Click on camera. A dialog containing all the features for your favorite
raytracer camera will appear. Edit the fields, or, more usefully, choose
Viewport. This will match the Camera specs to the current viewport. Choosing
Default will set the parameters to the the defaults for that raytracer,
which are generally useless. Click on Done when you've made up your mind.

* Create a light. Choose Create from the Lights menu. The Light appears at the
center of the world, and is edited just like the cube. Move the light
somewhere away from the center by choosing it from the Edit menu, then using
one of the Placement constraint planes to move it. Light colour (intensity)
is set by setting its attributes. The default will do for now.

* Set the ambient light level. Choose Ambient off the Lights menu and set
it to .2 .2 .2, or something like that.

* Select the object again. Choose the preview button and your favorite
raytracer. Now is as good a time as any to check out Genray. NOTE. You must
have set up your .scenerc file for this to work. When the raytracer is
chosen an acknowledge dialog will pop up telling you where the output is
going (in the current directory), and the trace will start. This will mean
different things for different raytracers, but it a seperate process and
won't affect Sced. ie you can work on while the trace is progressing.

* The object should still be selected. Choose Edit from the Object menu
and the object will be replaced on the edit list. Choose it from the Edit
menu and edit it a bit more, or just choose finish.

* It should still be selected (things don't get unselected in most cases).
Copy from the Object menu will copy the selected objects and put them on
the Edit list, while Delete will prompt for deletion. Copy the object. The
copy will be on the Edit menu and should be called Cube_0_1. It looks a bit
strange when editing because it's directly on top of the object it was copied
from. Move it away a bit by choosing one of the Position constraints and
dragging it away. Then finish it.

* Delete the new object. First unselect the old one (button 2 drag), then
select the new one (button 1 drag). Choose Delete form the Object menu
and confirm.

* Reselect the original object and choose Name from the Object menu.
Type in a new name - whatever you like, then hit return. If multiple objects
were selected you will prompted for a new name for each in turn.

* Choose Attribs from the Object menu. A big dialog with lots of attributes
will appear. Change what you like then hit Done. Default will set them to
the default as defined in the .scenerc file. RGB values are between 0 and 1.
Diffuse acts on top of the base color, as do all the rest.

* Save the scene. Choose Save off the File menu. A big dialog will appear,
with the default directory and extension at the top (.scenerc again).
replace the star with a name, say starting.scn, then hit return or choose
Save.

* Finally, Export it. Choose Export from the File menu and enter a filename
as before. The default extension will reflect what the Target raytracer is.
Change the target to export to a different renderer.

* Really finally, choose Quit from the File menu.

The file you exported should be suitable for input into the raytracer you
chose as a target. Try it and tell me if you have problems. NOTE POVray has
some patches which MUST be applied.

Tutorial **Blocks World** deals with constraints in far more detail, and
advanced use of the Scaling Point, Object Origin and Object Axes.
