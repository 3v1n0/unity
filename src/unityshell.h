// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#ifndef UNITYSHELL_H
#define UNITYSHELL_H

#include <core/core.h>
#include <core/pluginclasshandler.h>
#include <composite/composite.h>
#include <opengl/opengl.h>

#include "unityshell_options.h"

#include "Introspectable.h"
#include "Launcher.h"
#include "LauncherController.h"
#include "PanelView.h"
#include "PanelHomeButton.h"
#include "PlacesController.h"
#include "DebugDBusInterface.h"
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

	/* Pop our InputOutput windows from the paint list */
	const CompWindowList & getWindowPaintList ();

	/* handle X11 events */
	void handleEvent (XEvent *);
    void handleCompizEvent (const char          *plugin,
                            const char          *event,
                            CompOption::Vector  &option);
	
	bool
	showLauncherKeyInitiate (CompAction *action, CompAction::State state,
                             CompOption::Vector &options);

	bool
	showLauncherKeyTerminate (CompAction *action, CompAction::State state,
                              CompOption::Vector &options);

    bool
    showPanelFirstMenuKeyInitiate (CompAction         *action,
                                   CompAction::State   state,
                                   CompOption::Vector &options);
  bool
  showPanelFirstMenuKeyTerminate (CompAction         *action,
                                  CompAction::State   state,
                                  CompOption::Vector &options);
                                  
  bool
  executeCommand (CompAction*         action,
                  CompAction::State   state,
                  CompOption::Vector& options);
  bool
  setKeyboardFocusKeyInitiate (CompAction*         action,
                               CompAction::State   state,
                               CompOption::Vector& options);

	/* handle option changes and change settings inside of the
	 * panel and dock views */
	void optionChanged (CompOption *, Options num);
	
	/* Handle changes in the number of workspaces by showing the switcher
     * or not showing the switcher */
	bool setOptionForPlugin(const char *plugin, const char *name, 
                            CompOption::Value &v);

	/* init plugin actions for screen */
	bool initPluginForScreen (CompPlugin *p);

  void NeedsRelayout ();

  void ScheduleRelayout (guint timeout);

    protected:

	const gchar* GetName ();

	void AddProperties (GVariantBuilder *builder);

    private:

	static gboolean
	initPluginActions (gpointer data);

	static void
	initLauncher (nux::NThread* thread, void* InitData);

	void
	damageNuxRegions();

	void
	onRedrawRequested ();

	void Relayout ();

	static gboolean
	RelayoutTimeout (gpointer data);

	static void
	launcherWindowConfigureCallback(int WindowWidth, int WindowHeight, nux::Geometry& geo, void* user_data);

	static void
	panelWindowConfigureCallback(int WindowWidth, int WindowHeight, nux::Geometry& geo, void* user_data);

	static void
	initUnity(nux::NThread* thread, void* InitData);

	static gboolean
	strutHackTimeout (gpointer data);

  static void
  OnStartKeyNav (GVariant* data, void* value);

  static void
  OnExitKeyNav (GVariant* data, void* value);

  void
  startLauncherKeyNav ();

  void
  restartLauncherKeyNav ();

  static void
  OnQuicklistEndKeyNav (GVariant* data, void* value);

  static void
  OnLauncherStartKeyNav (GVariant* data, void* value);

  static void
  OnLauncherEndKeyNav (GVariant* data, void* value);

	Launcher               *launcher;
	LauncherController     *controller;
	PanelView              *panelView;
  PanelHomeButton        *panelHomeButton;
	PlacesController 			 *placesController;
	nux::WindowThread      *wt;
	nux::BaseWindow        *launcherWindow;
	nux::BaseWindow        *panelWindow;
	nux::Geometry           lastTooltipArea;
	DebugDBusInterface 		 *debugger;
  bool                   needsRelayout;

  /* keyboard-nav mode */
  CompWindow* newFocusedWindow;
  CompWindow* lastFocusedWindow;

	/* handle paint order */
	bool	  doShellRepaint;
	bool    allowWindowPaint;
	bool    damaged;
	CompWindowList _withRemovedNuxWindows;

  GdkRectangle _primary_monitor;

	friend class UnityWindow;
};

class UnityWindow :
    public WindowInterface,
    public GLWindowInterface,
    public PluginClassHandler <UnityWindow, CompWindow>
{

    public:

        /* Constructors, destructors etc */
	UnityWindow (CompWindow *);
	~UnityWindow ();

	CompWindow      *window;
	GLWindow	*gWindow;

	/* basic window draw function */
	bool
	glDraw (const GLMatrix 	&matrix,
	        GLFragment::Attrib &attrib,
	        const CompRegion 	&region,
	        unsigned int	mask);

	void windowNotify (CompWindowNotify n);

  void moveNotify (int x, int y, bool immediate);

  void resizeNotify (int x, int y, int w, int h);
  
  void stateChangeNotify (unsigned int lastState);
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

#endif // UNITYSHELL_H

