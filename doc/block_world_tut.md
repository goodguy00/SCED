# BLOCK WORLD TUTORIAL

In true computer science style I'll present blocks world as another
tutorial.
You should have done "starting" first, and it would help if you read the
guide before trying this one, particularly the part on the Edit Interface.

This tutorial covers more advanced editing, and constraint creation
and management. The aim is to create a blocks "pyramid".

It is recommended, to frequently save, after every edit op, for instance.

* Start up the program (easy, isn't it).

* Create a plane, edit it but leave it at the origin (ie just Finish it).
Note in passing that a scaling constraint is in force, even though you
didn't choose it. This is an example of a forced constraint, where the
Scaling Point and Origin have a dimension in common, in this case
they have equal Major coords. There is a section on forced constraints
in the user's guide.

* Create a cube, and edit it.

* We want the cube to sit on the plane, and stay there. So what we need
is a constraint that specifies the plane. It's probably best to choose
the most general constraint that satifies the relationship you want.
So in this case we want to specify a plane constraint that corresponds
to the actual plane in the world.

* To specify a plane constraint, choose Plane from the Add menu in the
Position box. A red square will appear on the vertex closest to the
mouse, and then follow the mouse around by snapping to the nearest vertex.
Note also the the cursor has changed to a pointing hand, indicating point
selection.

* Now the next issue. We want the box to stay on the plane
even if we move the plane, so we want Reference points, or points that
move with the object they lie on. Use Button 1 to choose 3 non-colinear
points from the plane. Lets take the origin and 2 points from extremes of
the cross type representation. If you have the defaults set up, some of
the plane points are off-screen. Lets fix this.

* Hit undo to cancel the point selection,
then choose Zoom from the View menu (Edit dialog) and set it to a smaller
value (80). Now you should be able to see most if not all of the plane. Choose
Add Plane from Position again.

* To choose the origin, push down with button 1 when the little red square is
on the origin. When you push the button down, something will change colour.
Either the object will go black, or the plane will go red or the axes will
change. The object that changed colour is the one referenced at the moment
by the point you have the mouse down over. Move the mouse around a bit while
the button is down. The colour change should cycle around the objects
just mentioned as the system cycles through possible choices for the
referenced object. You want the plane, so when it's red, let go of the
button. The complication is that the mouse also has to be inside the square,
so you have to be careful to get the right referenced object while still
being close to the square. Choose 2 other plane points using button 1.
Try the one to the left in the foreground and to the right in the foreground.
If at any time you get it wrong, choose undo and try again.

* You will be asked for a name for the constraint. Choose something
useful so you can remember what it is. I'll use BasePlane. After entering
the name a BasePlane button should appear as an available Position
constraint. If you accidentally selected the object as one of the defining
points, you will be warned that the specifier was changed to an absolute
point. This is because most constraints can't reference the object itself,
so the system changes them to something more appropriate. Anyway, none of
that should have happened.

* For the sake of example, choose the Remove button and click on the
constraint that you just created. It should disappear again. We actually want
it, so click on Undo and it will be put back.

* Select the BasePlane constraint, and move the box around a bit. You may
be wondering why we didn't just use the X-Y Plane constraint. The reason is
that the X-Y Plane constraint is tied to the Origin. It moves with the
Origin. It doesn't express the "On" relation that we want.

* But, you say, the Cube is not on the plane, it's in it. Now things get
interesting. What we really want is the base of the cube to lie in the
plane - ie we want to constrain the base of the cube, not its center.
Since the Origin is the thing we constrain, this equates to putting
the Origin on the base of the Cube. So do it. Choose Origin from the
dialog box and pick one of the
points on the base of the cube, say the point at the center of the base.

* A couple of things should happen quickly. Most apparent will be the
Cube jumping up so that it sits on the BasePlane. This is so that the
constraint you specified (the BasePlane) is satisfied by the new Origin.
You'll see that the Origin and Axes have moved onto the base of the Cube.
The constraint specifying points all have little red squares around them
too. The size says they're Origin constraints and the colour means they're
reference points. The grey square representing the resultant constraint also
appears.

* Change the Origin again, this time putting it on the corner diagonally
opposite the Scaling Point. Notice that the Cube doesn't move this time,
because the new Origin already satisfied the constraint.

* To build the pyramid we want 4 cubes arranged around the z axis,
and another on top. So we want to position the cube with one corner
at the world origin. So add another constraint, this time a line
corresponding to the world z-axis. Choose Add Line from the Position box,
and select 2 points. We want this constraint to be fixed in space - in
other words it's an absolute constraint. So use button 3 to choose the
2 points on either end of the world z axis. Call it ZAxis.

* Select the ZAxis constraint. The cube will jump so that the Origin is
at the world origin (where the axes meet). Note also the circle around the
Origin point. This indicates that it is point constrained, so you can't
move it (the line and plane constraints combine to give a point). The cursor
is now a circle when over the Origin. There are also specifier squares drawn
in green on the ends of the axes, indicating they are absolute specifying
points.

* Finish it there for the moment. Just choose Finish, WITHOUT deselecting
any of the constraints.

* Now edit the Plane again. Select it (by dragging over it with button 1)
and choose Edit from the Object menu. Then select the plane off the Edit
menu. When the Plane is drawn for editing, notice that the cube you were
just editing is drawn dashed. THis is because it depends on the plane
for its definition. Remember the reference points used to select a constraint
for the cube? These imply that the cube depends on the plane.

* Move the plane up a bit. Choose the Y-Z Plane and Z-X Plane Position
constraints. Together they combine to give the Z axis. Just move the plane
up a bit. The cube moves to follow the plane. This is constraint maintenance,
where the dependancies are re-evaluated and the cube adjusted to satisfy the
new constraints.

* Click on the Maintain toggle in the main window to turn interactive
maintenance off. Now as you move the plane the cube stays where it is. Its
constraints now won't be updated until you finish editing the plane.
Turn maintenance back on again. The only reason you would want it off is if
the evaluation of dependants was making the system too slow. The cube will
snap to its proper location.

* Edit the Cube again. Note that all the constraints come up as they were
before, still selected.

* Scale the cube a bit. Choose the Minor-Other Pl constraint and
drag the Scaling Point around a bit. Notice that the Origin stays
fixed ie the Origin is the center of the scaling. Use undo to get back
the unscaled state. Note that if you go far enough Undo will deselect the
constraint also.

* Choose the Uniform Scaling constraint. It's always a line from the
Origin to the Scaling point. Scaling along this line
will give equal scaling in all directions. Try it and see. Use undo to
get back to the unscaled state.

* Now lets get powerful - arbitrary axis scaling. Change the Minor Axis
of the Object. Choose "Redefine 1" from the Minor menu
in the dialog. Then choose the point on the base of the cube diagonally
opposite the Origin. A few things will happen. The Axes at the Origin
will rotate, so that the green axis is pointing from the Origin out to
the point you just selected. The Major axis stayed where it was (it will
unless you change it) and the Other (blue) axis moved to be perpendicular
to the other two. At the same time, the Major-Minor Scale constraint
activated. This is because, in terms of the new axes, the Origin and
Scaling point have the same Other Axis Coordinate. If you didn't get
what I just said ignore it - the program enforces it to save itself from
impossible states (but its not sensible to enter those states anyway).

* Given the forced constraint, select the Minor-Other Pl Scale constraint
also. These combine to give a line constraint. Scale along this constraint.
Notice how the Cube shears. Pull it out until you get a nice diamond
shape.

* Now rotate the Cube, but in a predictable way. We want the diamond to
point out along the X axis of the world. We'll use Allignment constraints to
do this.

* First decide how you want the cube to orient itself with respect to the
plane. We want the major axis, the red one, always perpendicular to the
plane. To fix this, select Allign Plane from the Major menu. Choose 3 points
on the plane using button 1. Make sure that you reference the plane when you
select them (it should highlight red for each point you choose) and
that they aren't colinear. The order of selection matter too. It defines
which normal to the plane you get (there are 2 possibilities). The program
takes the normal that is parallel to the cross product of a line passing from
the first point to the second point and a line passing from the first point
to the third point. When the points are selected, the constraint is
immediately enforced. The body doesn't change because it already satisfies
the constraint. The Major Axis rotation constraint becomes forced to stop
you breaking the constraint, and the points you used to define the plane
have small red squares around them. The small size indicates they are
allignment constraints, the red means they are reference points.

* Now we want to allign the Minor axis so that it stays parallel to the X axis.
The do this, choose Allign Line from the Minor menu, and select, by reference
or absolute (button 1 or 3), the negative then positive X axis endpoints.
Negative has to be chosen first because the order gives the line direction,
and we want the Minor Axis to point in the positive X direction. When the
points are chosen the cube will reallign itself to satisfy the constraint.
The Minor rotation constraint will also be enforced. Note that because of
this the resultant rotation constraint is not an axis any more, so you
can't freely rotate the cube. the cursor is also a closed circle when in
the window, indicating that rotation is not possible.

* Finish editing the Cube here. Go back and edit the plane a bit. Notice that
even if you rotate the plane, the cube will always stay perpendicular to
it and sitting on top. Undo everything so that the plane ends up where
it started.

* We want 3 more cubes, the same size and shape as the first, but arranged
around the center. So select the cube (not a cube any more), and choose
Copy from the Object menu, to make one copy (for the moment). The copy will
go on the Edit menu. Deselect the original.

* Edit the Copy. Note a few things. It's in the same position as the,
with the same size and shape, and the same constraints. The constraints
are copied because it's easier to Remove than Add constraints. Constraints
that depended on the object itself have however been changed to reference the
copy.

* We want to rotate the new object by 90 degrees, so that it lines up
with the positive Y axis. Do this simply by choosing Allign Line
from the Minor menu and choosing the negative then positive ends of the
Y axis. The cube will rotate to satisfy the new constraint.

* Copy two more cubes, and allign them with the negative X and negative Y
axes respectively. You should then have 4 diamond sort of shapes
with one common edge and each 90 degrees from its neighbour.

* Edit the Plane again. Move it around and rotate it and watch the cubes
follow.

* Finally, a cone for the top (so much for blocks world!). Create a cone,
and edit it to put it on top of the cubes. The easiest way to do this
is to add a point constraint for where the cubes meet in the middle, set
the origin of the Cone to be the center of its base, and select the point
constraint. Try it yourself. The next item does it in detail.

* Choose Add Point from the Position Add menu, then select the point at the
top where all the cubes meet. Use button 1, because we want the cone to move
with the rest of the structure. Make sure that one of the cubes is
highlighted when you let go of the button. It doesn't really matter which one.
Then choose Origin, and set the Origin to
be the center of the base of the cube (a forced scaling constraint will
activate). Then select the Point you added a second ago. The Cone will jump
to the top of the pyramid.

* Make sure the cone stays alligned. Choose appropriate allignment constraints
to do so. Use Allign Line from the Major menu to allign the Major axis
with the common edge of the cubes. Select 2 points to define the line,
the first where the cubes meets at the bottom, the second from where they
meet at the top. One of the cubes should highlight each time you choose
a point. Use Allign Point form the Minor menu to allign the Minor axis.
Select a refernence point (button 1) from the tip of on of the cubes
(not where the origin is, preferably in the direction the minor axis is
already pointing). The Minor Axis is now constrained to always point
toward the vertex you just chose.

* Scale the cone to make it taller. Change the Scaling point by selecting the
Scaling button and then choosing the vertex at the peak of the cone.
2 scaling constraints will enforce, giving a line. Scale the cone up along
this line. Finish editing the cone.

* What you've now done is create a fully constrained pyramid. Moving the
BasePlane will cause the whole thing to readjust itself to keep the
relationships you specified accurate.

* Set the names of things, just for good style. Select all the objects in the
scene (drag across the whole window with button 1). Choose Name form the
Object menu, and enter a new name for each object in turn.

* Set the attributes of the objects. Try making the blocks a bit reflective.
You can set the attributes of all the blocks at once by selecting all of them,
then choosing Attribs from Object just once. All the blocks will get the
same attributes.

* Create a light or 2, and set the ambient. Lights are edited the same as
everything else, so you can constrain the light to always stay at a
particular spot if you like. Set the colour of lights through their
Attributes. Only Colour has any relevence, and it controls both colour and
brightness.

* Choose a target, your favorite raytracer.

* Set the Camera by adjusting the View parameters until it looks like what
you want, then set the Camera to "Viewport".

* Export it and save and quit.

Raytrace the result and enjoy.

