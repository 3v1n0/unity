/* Compiz unity plugin
 * unity.h
 *
 * Copyright (c) 2008 Sam Spilsbury <smspillaz@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Your own copyright notice would go above. You are free to choose whatever
 * licence you want, just take note that some compiz code is GPL and you will
 * not be able to re-use it if you want to use a different licence.
 */

/* The include path is set to /PREFIX/include/compiz in the Makefile, so you
 * don't need to worry about not being able to find compiz headers */

#include <core/core.h>

/* You need to include this in order to get access to the CompPrivate templates */

#include <core/pluginclasshandler.h>

/* Compositing is no longer built in by-default, so you need to include the interface
 * of the composite plugin.
 */

#include <composite/composite.h>

/* OpenGL support is now no longer built in by-default as well. You have to use the
 * interface of the opengl plugin if you want to do anything with that. */

#include <opengl/opengl.h>

/* This is the options class header created for us automatically on build with BCOP */
#include "unity_options.h"

#include "Launcher.h"
#include "LauncherController.h"
#include "PanelView.h"

#include <Nux/WindowThread.h>
#include <sigc++/sigc++.h>

/* This is your base screen class. This class definition can be considered the top most
 * as there is no multi-screen or multi-display support in compiz++, only one compiz
 * instance per screen.
 */

class UnityScreen :
    public sigc::trackable,
    /* You should inherit the ScreenInterface class if you want to dynamically hook
     * screen-level core functions. */
    public ScreenInterface,
    /* Same goes for screen functions in the composite plugin */
    public CompositeScreenInterface,
    /* And opengl plugin */
    public GLScreenInterface,
    /* This sets up a privates handler, where you can access an instance of your class for
     * every CompScreen object (same goes for UnityWindow and CompWindow). It adds the 
     * member function ::get, which will allow you get an instance of this class from a
     * CompScreen * object. Oh, and the memory management is done for you now ^_^
     */
    public PluginClassHandler <UnityScreen, CompScreen>,
    /* And finally, because we are using BCOP to generate our options, it creates a class
     * with a bunch of getters and setters that are added to out class when we inherit it. Yay.
     */
    public UnityOptions
{

    public:

	/* Lets get into buisness. This is the constructor, it replaces the initScreen, initEtc
         * things that used to be in compiz and is called automatically when an instance of this
         * class is created. Notice how it is the same name as the class name. It has to be that way.
         */

	UnityScreen (CompScreen *s);

	/* This is the destructor. It is called when the class is destroyed. If you allocated any
	 * memory or need to do some cleanup, here is where you do it. Note the tilde (~)?
	 */

	~UnityScreen ();

	/* We store CompositeScreen, GLScreen, and CompScreen to avoid unecessary calls to ::get */
	CompScreen      *screen;
	CompositeScreen *cScreen;
        GLScreen        *gScreen;

	/* Now. Because we imported ScreenInterface and CompositeScreenInterface, we get all the
	 * core functions in there for free. Here's the twist. Those functions are stored as 'virtual',
	 * so they can be overridden. That is what we do here. So instead of a core function being called,
	 * our own is
	 */

	/* This is the function that is called before the screen is 're-painted'. It is used for animation
	 * and such because it gives you a time difference between when the last time the screen was repainted
	 * and the current time of the execution of the functions in milliseconds). It's part of the composite
	 * plugin's interface
	 */

	void
	preparePaint (int);

	/* This is the guts of the paint function. You can transform the way the entire output is painted
	 * or you can just draw things on screen with openGL. The unsigned int here is a mask for painting
	 * the screen, see opengl/opengl.h on how you can change it */
	 
	void
	nuxPrologue ();
	 
	void
	nuxEpilogue ();
	
	void
	paintDisplay (const CompRegion 	&region);

	bool
	glPaintOutput (const GLScreenPaintAttrib &,
		       const GLMatrix &, const CompRegion &,
		       CompOutput *, unsigned int);

	/* This is called when the output is transformed by a matrix. Basically the same, but core has
	 * done a few things for us like clip planes etc */

	void
	glPaintTransformedOutput (const GLScreenPaintAttrib &,
		       		  const GLMatrix &, const CompRegion &,
		       		  CompOutput *, unsigned int);


	/* This is called after screen painting is finished. It gives you a chance to do any cleanup and or
	 * call another repaint with screen->damageScreen (). It's also part of the composite plugin's
	 * interface.
	 */

	void
	donePaint ();

	/* This is our event handler. It directly hooks into the screen's X Event handler and allows us to handle
	 * our raw X Events
	 */

	void
	handleEvent (XEvent *);

	void optionChanged (CompOption *, Options num);

	
	bool
	initPluginForScreen (CompPlugin *p);
	
    private:
    
	static void
	initLauncher (nux::NThread* thread, void* InitData);
  
	void
	damageNuxRegions();
	
	void 
	onRedrawRequested ();
	
	static void 
	launcherWindowConfigureCallback(int WindowWidth, int WindowHeight, nux::Geometry& geo, void* user_data);

	static void 
	panelWindowConfigureCallback(int WindowWidth, int WindowHeight, nux::Geometry& geo, void* user_data);

	static void 
	initUnity(nux::NThread* thread, void* InitData);
	
	static gboolean 
	strutHackTimeout (gpointer data);
	
	Launcher               *launcher;
	LauncherController     *controller;
	PanelView              *panelView;
	nux::WindowThread      *wt;
	nux::BaseWindow        *launcherWindow;
	nux::BaseWindow        *panelWindow;
	nux::Geometry           lastTooltipArea;
};

class UnityWindow :
    /* Same for Unity Screen, inherit interfaces, private handlers, options */
    public WindowInterface,
    public CompositeWindowInterface,
    public GLWindowInterface,
    public PluginClassHandler <UnityWindow, CompWindow>
{

    public:

        /* Constructors, destructors etc */

	UnityWindow (CompWindow *);
	~UnityWindow ();

	/* We store CompositeWindow, GLWindow, and CompWindow to avoid unecessary calls to ::get */
	CompWindow      *window;
	CompositeWindow *cWindow;
        GLWindow        *gWindow;

	/* This gets called whenever the window needs to be repainted. WindowPaintAttrib gives you some
	 * attributes like brightness/saturation etc to play around with. GLMatrix is the window's
	 * transformation matrix. the unsigned int is the mask, have a look at opengl.h on what you can do
	 * with it */

	bool
	glPaint (const GLWindowPaintAttrib &, const GLMatrix &,
		 const CompRegion &, unsigned int);
	
	bool 
	glDraw (const GLMatrix 	&matrix,
	        GLFragment::Attrib &attrib,
	        const CompRegion 	&region,
	        unsigned int	mask);

	/* This get's called whenever a window's rect is damaged. You can do stuff here or you can adjust the damage
	 * rectangle so that the window will update properly. Part of the CompositeWindowInterface.
	 */

	bool
	damageRect (bool, const CompRect &);

	/* This is called whenever the window is focussed */
	bool
	focus ();

	/* This is called whenever the window is activated */
	void
	activate ();

	/* This is called whenever the window must be placed. You can alter the placement position by altering
	 * the CompPoint & */
	bool
	place (CompPoint &);

	/* This is called whenever the window is moved. It tells you how far it's been moved and whether that
	 * move should be immediate or animated. (Immediate is usually for a sudden move, false for that
	 * usually means the user moved the window */
	void
	moveNotify (int, int, bool);

	/* This is called whenever the window is resized. It tells you how much the window has been resized */
	void
	resizeNotify (int, int, int, int);

	/* This is called when another plugin deems a window to be 'grabbed'. You can't actually 'grab' a
	 * window in X, but it is useful in case you need to know when a user has grabbed a window for some reason */
	void
	grabNotify (int, int, unsigned int, unsigned int);

	/* This is called when a window in released from a grab. See above */
	void
	ungrabNotify ();

	/* This is called when another misc notification arises, such as Map/Unmap etc */
	void
	windowNotify (CompWindowNotify);

};

/* Most plugins set up macros to access their Screen and Window classes based on a CompScreen * or CompWindow * */

#define EX_SCREEN (screen) \
UnityScreen *es = UnityScreen::get (screen);

#define EX_WINDOW (window) \
UnityWindow *ew = UnityWindow::get (window);

/* Your vTable class is some basic info about the plugin that core uses.
 */

class UnityPluginVTable :
    /* Inheriting CompPlugin::VTableForScreenAndWindow <ScreenClass, WindowClass>
     * automatically does most of the vTable setup for you */
    public CompPlugin::VTableForScreenAndWindow<UnityScreen, UnityWindow>
{
    public:

	/* You MUST have this function. Compiz calls this function first off and that
	 * is what kick-starts your initialization procedure. It must be named
	 * _exactly_ init too */

	bool init ();
};
