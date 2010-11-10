/* Compiz unity plugin
 * unity.cpp
 *
 * Copyright (c) 2010 Sam Spilsbury <smspillaz@gmail.com>
 *                    Jason Smith <jason.smith@canonical.com>
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


#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/BaseWindow.h>
#include <Nux/WindowCompositor.h>

#include "Launcher.h"
#include "LauncherIcon.h"
#include "LauncherController.h"
#include "PluginAdapter.h"
#include "unity.h"

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <gtk/gtk.h>

#include <core/atoms.h>

/* You must also call this. This creates a proper vTable for a plugin name in the 
 * first argument (it's a macro, so you don't need a string). This changes from
 * time to time as the vTable changes, so it is a good way of ensure plugins keep
 * current */

COMPIZ_PLUGIN_20090315 (unity, UnityPluginVTable);

/* This is the function that is called before the screen is 're-painted'. It is used for animation
 * and such because it gives you a time difference between when the last time the screen was repainted
 * and the current time of the execution of the functions in milliseconds). It's part of the composite
 * plugin's interface
 */
 
static bool paint_required = false;
static UnityScreen *uScreen = 0;

void
UnityScreen::preparePaint (int ms)
{
    /* At the end of every function, you must call BaseClass->functionName (args) in order to pass on
     * the call chain */
    cScreen->preparePaint (ms);
    paint_required = true;
}

void
UnityScreen::nuxPrologue ()
{
    glPushAttrib (GL_VIEWPORT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT);
    
    glMatrixMode (GL_PROJECTION);
    glPushMatrix ();
    
    glMatrixMode (GL_MODELVIEW);
    glPushMatrix ();

    glGetError();
}

void
UnityScreen::nuxEpilogue ()
{
    (*GL::bindFramebuffer) (GL_FRAMEBUFFER_EXT, 0);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();
    glDepthRange (0, 1);
    glViewport (-1, -1, 2, 2);
    glRasterPos2f (0, 0);

    gScreen->resetRasterPos ();

    glMatrixMode (GL_PROJECTION);
    glPopMatrix ();
    glMatrixMode (GL_MODELVIEW);
    glPopMatrix ();

    glDrawBuffer (GL_BACK);
    glReadBuffer (GL_BACK);

    glPopAttrib ();
}

void
UnityScreen::paintDisplay (const CompRegion &region)
{
    nuxPrologue ();
    wt->RenderInterfaceFromForeignCmd ();
    nuxEpilogue ();
}

/* This is the guts of the paint function. You can transform the way the entire output is painted
 * or you can just draw things on screen with openGL. The unsigned int here is a mask for painting
 * the screen, see opengl/opengl.h on how you can change it */

bool
UnityScreen::glPaintOutput (const GLScreenPaintAttrib &attrib, // Some basic attribs on how the screen should be painted
			      const GLMatrix		&transform, // Screen transformation matrix
			      const CompRegion		&region, // Region of screen being painted
			      CompOutput 		*output, // Output properties. Use this to the get output width and height for the output being painted
			      unsigned int		mask /* Some other paint properties, see opengl.h */)
{
    bool ret;

    /* glPaintOutput is part of the opengl plugin, so we need the GLScreen base class. */
    ret = gScreen->glPaintOutput (attrib, transform, region, output, mask);
    
    if (paint_required)
    {
        paintDisplay (region);
        paint_required = false;
    }
    
    return ret;
}

/* This is called when the output is transformed by a matrix. Basically the same, but core has
 * done a few things for us like clip planes etc */

void
UnityScreen::glPaintTransformedOutput (const GLScreenPaintAttrib &attrib, // Some basic attribs on how the screen should be painted
			      		 const GLMatrix		&transform, // Screen transformation matrix
			      		 const CompRegion	&region, // Region of screen being painted
			      		 CompOutput 		*output, // Output properties. Use this to the get output width and height for the output being painted
			      		 unsigned int		mask /* Some other paint properties, see opengl.h */)
{
    /* glPaintOutput is part of the opengl plugin, so we need the GLScreen base class. */
    gScreen->glPaintOutput (attrib, transform, region, output, mask);
}

/* This is called after screen painting is finished. It gives you a chance to do any cleanup and or
 * call another repaint with screen->damageScreen (). It's also part of the composite plugin's
 * interface.
 */

void
UnityScreen::donePaint ()
{
    cScreen->donePaint ();
}

void
UnityScreen::damageNuxRegions ()
{
    CompRegion region;
    std::vector<nux::Geometry>::iterator it;
    std::vector<nux::Geometry> dirty = wt->GetDrawList ();
    nux::Geometry geo;
	    
    for (it = dirty.begin (); it != dirty.end (); it++)
    {
       	geo = *it;
      	cScreen->damageRegion (CompRegion (geo.x, geo.y, geo.width, geo.height));
    }
	    
    geo = wt->GetWindowCompositor ().GetTooltipMainWindowGeometry();
    cScreen->damageRegion (CompRegion (geo.x, geo.y, geo.width, geo.height));
    cScreen->damageRegion (CompRegion (lastTooltipArea.x, lastTooltipArea.y, lastTooltipArea.width, lastTooltipArea.height));
	    
    lastTooltipArea = geo;
    
    wt->ClearDrawList ();
}

/* This is our event handler. It directly hooks into the screen's X Event handler and allows us to handle
 * our raw X Events
 */

void
UnityScreen::handleEvent (XEvent *event)
{
    screen->handleEvent (event);
    
    if (screen->otherGrabExist ("deco", "move", NULL))
    {
      printf ("Process Foreign!\n");
      wt->ProcessForeignEvent (event, NULL);
    }
}

bool
UnityScreen::initPluginForScreen (CompPlugin *p)
{
    if (g_strcmp0 (p->vTable->name ().c_str (), "expo") == 0)
    {
        CompOption::Vector &options = p->vTable->getOptions ();
        
        foreach (CompOption& option, options)
        {
          if (g_strcmp0 (option.name ().c_str () , "expo_key") == 0)
          {
              CompAction *expoAction = &option.value ().action ();
              
              PluginAdapter::Default ()->SetExpoAction (expoAction);
              break;
          }
        }
    }
    
    if (g_strcmp0 (p->vTable->name ().c_str (), "scale") == 0)
    {
        CompOption::Vector &options = p->vTable->getOptions ();
        
        foreach (CompOption& option, options)
        {
          if (g_strcmp0 (option.name ().c_str () , "initiate_all_key") == 0)
          {
              CompAction *scaleAction = &option.value ().action ();
              
              PluginAdapter::Default ()->SetScaleAction (scaleAction);
              break;
          }
        }
    }

    screen->initPluginForScreen (p);
    return true;
}

/* This gets called whenever the window needs to be repainted. WindowPaintAttrib gives you some
 * attributes like brightness/saturation etc to play around with. GLMatrix is the window's
 * transformation matrix. the unsigned int is the mask, have a look at opengl.h on what you can do
 * with it */

bool
UnityWindow::glPaint (const GLWindowPaintAttrib &attrib, // Brightness, Saturation, Opacity etc
      const GLMatrix &transform, // Transformation Matrix
      const CompRegion &region, // Repaint region
      unsigned int mask) // Other flags. See opengl.h
{
    bool ret;
    ret = gWindow->glPaint (attrib, transform, region, mask);
    return ret;
}

bool 
UnityWindow::glDraw (const GLMatrix 	&matrix,
			     GLFragment::Attrib &attrib,
			     const CompRegion 	&region,
			     unsigned int	mask)
{
    bool ret;
    
    if (paint_required && uScreen && window->type () & (CompWindowTypeMenuMask | 
                                                        CompWindowTypeDropdownMenuMask | 
                                                        CompWindowTypePopupMenuMask |
                                                        CompWindowTypeComboMask |
                                                        CompWindowTypeTooltipMask |
                                                        CompWindowTypeDndMask
                                                        ))
    {
        uScreen->paintDisplay (region);
        paint_required = false;
    }

    ret = gWindow->glDraw (matrix, attrib, region, mask);

    return ret;
}
/* This get's called whenever a window's rect is damaged. You can do stuff here or you can adjust the damage
 * rectangle so that the window will update properly. Part of the CompositeWindowInterface.
 */

bool
UnityWindow::damageRect (bool initial, // initial damage?
    const CompRect &rect) // The actual rect. Modifyable.
{
    bool ret;

    ret = cWindow->damageRect (initial, rect);

    return ret;
}

/* This is called whenever the window is focussed */

bool
UnityWindow::focus ()
{
    bool ret;

    ret = window->focus ();

    return ret;
}

/* This is called whenever the window is activated */

void
UnityWindow::activate ()
{
    window->activate ();
}

/* This is called whenever the window must be placed. You can alter the placement position by altering
 * the CompPoint & */

bool
UnityWindow::place (CompPoint &point)
{
    bool ret;

    ret = window->place (point);
    return ret;
}

/* This is called whenever the window is moved. It tells you how far it's been moved and whether that
 * move should be immediate or animated. (Immediate is usually for a sudden move, false for that
 * usually means the user moved the window */

void
UnityWindow::moveNotify (int dx,
            int dy,
            bool immediate)
{
    window->moveNotify (dx, dy, immediate);
}

/* This is called whenever the window is resized. It tells you how much the window has been resized */

void
UnityWindow::resizeNotify (int dx,
              int dy,
              int dwidth,
              int dheight)
{
    window->resizeNotify (dx, dy, dwidth, dheight);
}

/* This is called when another plugin deems a window to be 'grabbed'. You can't actually 'grab' a
 * window in X, but it is useful in case you need to know when a user has grabbed a window for some reason */

void
UnityWindow::grabNotify (int x,
            int y,
            unsigned int state,
            unsigned int mask)
{
    window->grabNotify (x, y, state, mask);
}

/* This is called when a window in released from a grab. See above */

void
UnityWindow::ungrabNotify ()
{
    window->ungrabNotify ();
}

/* This is called when another misc notification arises, such as Map/Unmap etc */

void
UnityWindow::windowNotify (CompWindowNotify n)
{
    window->windowNotify (n);
}

/* This is an action. It is called on a keybinding as set in the options. It is binded when
 *  the class is constructed
 */

bool
UnityScreen::unityInitiate (CompAction         *action,
                CompAction::State  state,
                CompOption::Vector &options)
{
    return false;
}

void 
UnityScreen::launcherWindowConfigureCallback(int WindowWidth, int WindowHeight, nux::Geometry& geo, void *user_data)
{
    int OurWindowHeight = WindowHeight - 24;
    geo = nux::Geometry(0, 24, geo.width, OurWindowHeight);
}

void 
UnityScreen::panelWindowConfigureCallback(int WindowWidth, int WindowHeight, nux::Geometry& geo, void *user_data)
{
    geo = nux::Geometry(0, 0, WindowWidth, 24);
}


void 
UnityScreen::initUnity(nux::NThread* thread, void* InitData)
{
    initLauncher(thread, InitData);
      
    nux::ColorLayer background(nux::Color(0x00000000));
    static_cast<nux::WindowThread*>(thread)->SetWindowBackgroundPaintLayer(&background);
}

void 
UnityScreen::onRedrawRequested ()
{
  damageNuxRegions ();
}

UnityScreen::UnityScreen (CompScreen *screen) :// The constructor takes a CompScreen *,
    PluginClassHandler <UnityScreen, CompScreen> (screen), // Initiate PluginClassHandler class template
    screen (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen))
{
    int (*old_handler) (Display *, XErrorEvent *);
    old_handler = XSetErrorHandler (NULL);
    
    g_thread_init (NULL);
    dbus_g_thread_init ();
    gtk_init (NULL, NULL);
    
    XSetErrorHandler (old_handler);
    
    ScreenInterface::setHandler (screen); // Sets the screen function hook handler
    CompositeScreenInterface::setHandler (cScreen); // Ditto for cScreen
    GLScreenInterface::setHandler (gScreen); // Ditto for gScreen

    optionSetUnityInitiate (unityInitiate); // Initiate handler for 'unity' action

    nux::NuxInitialize (0);
    wt = nux::CreateFromForeignWindow (cScreen->output (), 
                                       glXGetCurrentContext (),	
                                       &UnityScreen::initUnity,
                                       this);
    
    wt->RedrawRequested.connect (sigc::mem_fun (this, &UnityScreen::onRedrawRequested));
    
    wt->Run (NULL);
    uScreen = this;
    
    PluginAdapter::Initialize (screen);
}

/* This is the destructor. It is called when the class is destroyed. If you allocated any
 * memory or need to do some cleanup, here is where you do it. Note the tilde (~)?
 */

UnityScreen::~UnityScreen ()
{
}

gboolean UnityScreen::strutHackTimeout (gpointer data)
{
    UnityScreen *self = (UnityScreen*) data;  
    
    if (!self->launcher->AutohideEnabled ())
    {
        self->launcherWindow->InputWindowEnableStruts(false);
        self->launcherWindow->InputWindowEnableStruts(true);
    }
    
    self->panelWindow->InputWindowEnableStruts(false);
    self->panelWindow->InputWindowEnableStruts(true);
    
    return FALSE;
}

void UnityScreen::initLauncher (nux::NThread* thread, void* InitData)
{
  UnityScreen *self = (UnityScreen*) InitData;
  
  self->launcherWindow = new nux::BaseWindow(TEXT(""));
  self->launcher = new Launcher(self->launcherWindow);

  nux::HLayout* layout = new nux::HLayout();

  layout->AddView(self->launcher, 1);
  layout->SetContentDistribution(nux::eStackLeft);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);

  self->controller = new LauncherController (self->launcher, self->screen, self->launcherWindow);

  self->launcherWindow->SetConfigureNotifyCallback(&UnityScreen::launcherWindowConfigureCallback, self);
  self->launcherWindow->SetLayout(layout);
  self->launcherWindow->SetBackgroundColor(nux::Color(0x00000000));
  self->launcherWindow->SetBlurredBackground(false);
  self->launcherWindow->ShowWindow(true);
  self->launcherWindow->EnableInputWindow(true);
  self->launcherWindow->InputWindowEnableStruts(true);

  self->launcher->SetIconSize (54, 48);

  /* Setup panel */
  self->panelView = new PanelView ();

  layout = new nux::HLayout();

  self->panelView->SetMaximumHeight(24);
  layout->AddView(self->panelView, 1);
  layout->SetContentDistribution(nux::eStackLeft);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);

  self->panelWindow = new nux::BaseWindow("");

  self->panelWindow->SetConfigureNotifyCallback(&UnityScreen::panelWindowConfigureCallback, self);
  self->panelWindow->SetLayout(layout);
  self->panelWindow->SetBackgroundColor(nux::Color(0x00000000));
  self->panelWindow->SetBlurredBackground(false);
  self->panelWindow->ShowWindow(true);
  self->panelWindow->EnableInputWindow(true);  
  self->panelWindow->InputWindowEnableStruts(true);
  
  g_timeout_add (2000, &UnityScreen::strutHackTimeout, self);
  
  /*self->launcher->SetAutohide (true, (nux::View*) self->panelView->HomeButton ());
  self->launcher->SetFloating (true);*/
}

UnityWindow::UnityWindow (CompWindow *window) :
    PluginClassHandler <UnityWindow, CompWindow> (window), // Initiate our PrivateHandler class template
    window (window), 
    cWindow (CompositeWindow::get (window)),
    gWindow (GLWindow::get (window))
{
    WindowInterface::setHandler (window); // Sets the window function hook handler
    CompositeWindowInterface::setHandler (cWindow); // Ditto for cWindow
    GLWindowInterface::setHandler (gWindow); // Ditto for gWindow
}

UnityWindow::~UnityWindow ()
{
}

/* This is the very first function compiz calls. It kicks off plugin initialization */

bool
UnityPluginVTable::init ()
{
    /* Calls to checkPluginABI check the ABI of a particular plugin to see if it is loaded
     * and in sync with your current build. If it fails the plugin will not load otherwise
     * compiz will crash.
     */

    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	 return false;
    if (!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI))
	 return false;
    if (!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	 return false;

    return true;
}

