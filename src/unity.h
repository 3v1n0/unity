/* Compiz unity plugin
 * unity.h
 *
 * Copyright (c) 2008 Sam Spilsbury <smspillaz@gmail.com>
 * Copyright (c) 2010 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <core/core.h>
#include <core/pluginclasshandler.h>
#include <composite/composite.h>
#include <opengl/opengl.h>

#include "unityshell_options.h"

#include "Introspectable.h"
#include "Launcher.h"
#include "LauncherController.h"
#include "PanelView.h"
#include "IntrospectionDBusInterface.h"
#include <Nux/WindowThread.h>
#include <sigc++/sigc++.h>

/* base screen class */
class UnityScreen : 
    public Introspectable,
    public sigc::trackable,
    public ScreenInterface,
    public CompositeScreenInterface,
    public GLScreenInterface,
    public PluginClassHandler <UnityScreen, CompScreen>,
    public UnityshellOptions
{

    public:

	/* Init */
	UnityScreen (CompScreen *s);

	/* Cleanup */
	~UnityScreen ();

	/* We store these  to avoid unecessary calls to ::get */
	CompScreen      *screen;
	CompositeScreen *cScreen;
        GLScreen        *gScreen;

	/* hook this function to determine animations before paint */
	void
	preparePaint (int);

	/* prepares nux for drawing */	 
	void
	nuxPrologue ();

	/* pops nux draw stack */
	void
	nuxEpilogue ();

	/* nux draw wrapper */
	void paintDisplay (const CompRegion 	&region);

	/* paint on top of all windows if we could not find a window
	 * to paint underneath */
	bool glPaintOutput (const GLScreenPaintAttrib &,
			    const GLMatrix &,
			    const CompRegion &,
		       	    CompOutput *,
			    unsigned int);

	/* paint in the special case that the output is transformed */
	void glPaintTransformedOutput (const GLScreenPaintAttrib &,
		       		       const GLMatrix &,
				       const CompRegion &,
		       		       CompOutput *,
				       unsigned int);


	/* cleanup after painting */
	void donePaint ();

	/* handle X11 events */
	void handleEvent (XEvent *);

	/* handle option changes and change settings inside of the
	 * panel and dock views */
	void optionChanged (CompOption *, Options num);

	/* init plugin actions for screen */
	bool initPluginForScreen (CompPlugin *p);

    protected:

	const gchar* GetName ();

	void AddProperties (GVariantBuilder *builder);
	
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
	IntrospectionDBusInterface 		*debugger;

	/* handle paint order */
	bool	  doShellRepaint;
	bool      allowWindowPaint;

	friend class UnityWindow;
};

class UnityWindow :
    public WindowInterface,
    public CompositeWindowInterface,
    public GLWindowInterface,
    public PluginClassHandler <UnityWindow, CompWindow>
{

    public:

        /* Constructors, destructors etc */
	UnityWindow (CompWindow *);
	~UnityWindow ();

	CompWindow      *window;
	CompositeWindow *cWindow;
        GLWindow        *gWindow;

	/* basic window paint function */
	bool
	glPaint (const GLWindowPaintAttrib &,
		 const GLMatrix &,
		 const CompRegion &,
		 unsigned int);
	
	/* basic window draw function */
	bool 
	glDraw (const GLMatrix 	&matrix,
	        GLFragment::Attrib &attrib,
	        const CompRegion 	&region,
	        unsigned int	mask);

	/* handle damages on windows */
	bool
	damageRect (bool, const CompRect &);

	/* This is called whenever the window is focussed */
	bool
	focus ();

	/* This is called whenever the window is activated */
	void
	activate ();

	/* handle placement */
	bool
	place (CompPoint &);

	/* handle move */
	void
	moveNotify (int, int, bool);

	/* handle resize */
	void
	resizeNotify (int, int, int, int);

	/* handle grabs */
	void
	grabNotify (int, int, unsigned int, unsigned int);

	/* handle ungrabs */
	void
	ungrabNotify ();

	/* handle minimize, unmap, map, etc */
	void
	windowNotify (CompWindowNotify);

};

#define EX_SCREEN (screen) \
UnityScreen *es = UnityScreen::get (screen);

#define EX_WINDOW (window) \
UnityWindow *ew = UnityWindow::get (window);

/* Your vTable class is some basic info about the plugin that core uses.
 */

class UnityPluginVTable :
    public CompPlugin::VTableForScreenAndWindow<UnityScreen, UnityWindow>
{
    public:

	/* kickstart initialization */
	bool init ();
};
