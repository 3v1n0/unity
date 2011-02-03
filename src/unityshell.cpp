// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/* Compiz unity plugin
 * unity.cpp
 *
 * Copyright (c) 2010 Sam Spilsbury <smspillaz@gmail.com>
 *                    Jason Smith <jason.smith@canonical.com>
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


#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/BaseWindow.h>
#include <Nux/WindowCompositor.h>

#include "Launcher.h"
#include "LauncherIcon.h"
#include "LauncherController.h"
#include "PluginAdapter.h"
#include "StartupNotifyService.h"
#include "unityshell.h"

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <core/atoms.h>

#include "perf-logger-utility.h"
#include "unitya11y.h"

#include "config.h"

/* FIXME: once we get a better method to add the toplevel windows to
   the accessible root object, this include would not be required */
#include "unity-util-accessible.h"

/* Set up vtable symbols */
COMPIZ_PLUGIN_20090315 (unityshell, UnityPluginVTable);

static UnityScreen *uScreen = 0;

void
UnityScreen::nuxPrologue ()
{
  /* Vertex lighting isn't used in Unity, we disable that state as it could have
   * been leaked by another plugin. That should theoretically be switched off
   * right after PushAttrib since ENABLE_BIT is meant to restore the LIGHTING
   * bit, but we do that here in order to workaround a bug (?) in the NVIDIA
   * drivers (lp:703140). */
  glDisable (GL_LIGHTING);

  /* reset matrices */
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

  doShellRepaint = false;
}

/* called whenever we need to repaint parts of the screen */
bool
UnityScreen::glPaintOutput (const GLScreenPaintAttrib   &attrib,
                            const GLMatrix              &transform,
                            const CompRegion            &region,
                            CompOutput                  *output,
                            unsigned int                mask)
{
  bool ret;

  doShellRepaint = true;
  allowWindowPaint = true;

  /* glPaintOutput is part of the opengl plugin, so we need the GLScreen base class. */
  ret = gScreen->glPaintOutput (attrib, transform, region, output, mask);

  if (doShellRepaint)
    paintDisplay (region);

  return ret;
}

/* called whenever a plugin needs to paint the entire scene
 * transformed */

void
UnityScreen::glPaintTransformedOutput (const GLScreenPaintAttrib &attrib,
                                       const GLMatrix            &transform,
                                       const CompRegion          &region,
                                       CompOutput                *output,
                                       unsigned int              mask)
{
  allowWindowPaint = false;
  gScreen->glPaintOutput (attrib, transform, region, output, mask);
}

/* Grab changed nux regions and add damage rects for them */
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

/* handle X Events */
void
UnityScreen::handleEvent (XEvent *event)
{
  switch (event->type)
  {
    case FocusIn:
    case FocusOut:
      if (event->xfocus.mode == NotifyGrab)
        PluginAdapter::Default ()->OnScreenGrabbed ();
      else if (event->xfocus.mode == NotifyUngrab)
        PluginAdapter::Default ()->OnScreenUngrabbed ();

      break;
  }

  screen->handleEvent (event);

  if (screen->otherGrabExist ("deco", "move", NULL))
  {
    wt->ProcessForeignEvent (event, NULL);
  }
}

bool
UnityScreen::showLauncherKeyInitiate (CompAction         *action,
                                      CompAction::State   state,
                                      CompOption::Vector &options)
{
  // to receive the Terminate event
  if (state & CompAction::StateInitKey)
    action->setState (action->state () | CompAction::StateTermKey);
  
  launcher->StartKeyShowLauncher ();
  return false;
}

bool
UnityScreen::showLauncherKeyTerminate (CompAction         *action,
                                       CompAction::State   state,
                                       CompOption::Vector &options)
{
  launcher->EndKeyShowLauncher ();
  return false;
}

gboolean
UnityScreen::initPluginActions (gpointer data)
{
  CompPlugin *p;

  p = CompPlugin::find ("expo");

  if (p)
  {
    MultiActionList expoActions (0);

    foreach (CompOption &option, p->vTable->getOptions ())
    {
      if (option.name () == "expo_key" ||
          option.name () == "expo_button" ||
          option.name () == "expo_edge")
      {
        CompAction *action = &option.value ().action ();
        expoActions.AddNewAction (action, false);
        break;
      }
    }
    
    PluginAdapter::Default ()->SetExpoAction (expoActions);
  }

  p = CompPlugin::find ("scale");

  if (p)
  {
    MultiActionList scaleActions (0);

    foreach (CompOption &option, p->vTable->getOptions ())
    {
      if (option.name () == "initiate_all_key" ||
          option.name () == "initiate_all_edge" ||
          option.name () == "initiate_key" ||
          option.name () == "initiate_button" ||
          option.name () == "initiate_edge" ||
          option.name () == "initiate_group_key" ||
          option.name () == "initiate_group_button" ||
          option.name () == "initiate_group_edge" ||
          option.name () == "initiate_output_key" ||
          option.name () == "initiate_output_button" ||
          option.name () == "initiate_output_edge")
      {
        CompAction *action = &option.value ().action ();
        scaleActions.AddNewAction (action, false);
      }
      else if (option.name () == "initiate_all_button")
      {
        CompAction *action = &option.value ().action ();
        scaleActions.AddNewAction (action, true);
      }
    }
    
    PluginAdapter::Default ()->SetScaleAction (scaleActions);
  }

  return FALSE;
}

/* Set up expo and scale actions on the launcher */
bool
UnityScreen::initPluginForScreen (CompPlugin *p)
{
  if (p->vTable->name () == "expo" ||
      p->vTable->name () == "scale")
  {
    initPluginActions ((void *) this);
  }

  return screen->initPluginForScreen (p);
}

void
UnityScreen::AddProperties (GVariantBuilder *builder)
{
}

const gchar*
UnityScreen::GetName ()
{
  return "Unity";
}

const CompWindowList &
UnityScreen::getWindowPaintList ()
{
  CompWindowList &pl = _withRemovedNuxWindows = cScreen->getWindowPaintList ();
  CompWindowList::iterator it = pl.end ();
  const std::list <Window> &xwns = nux::XInputWindow::NativeHandleList ();

  while (it != pl.begin ())
  {
    it--;

    if (std::find (xwns.begin (), xwns.end (), (*it)->id ()) != xwns.end ())
    {
      it = pl.erase (it);
    }
  }

  return pl;
}

/* handle window painting in an opengl context
 *
 * we want to paint underneath other windows here,
 * so we need to find if this window is actually
 * stacked on top of one of the nux input windows
 * and if so paint nux and stop us from painting
 * other windows or on top of the whole screen */
bool
UnityWindow::glDraw (const GLMatrix     &matrix,
                     GLFragment::Attrib &attrib,
                     const CompRegion   &region,
                     unsigned int       mask)
{
  if (uScreen->doShellRepaint && uScreen->allowWindowPaint)
  {
    const std::list <Window> &xwns = nux::XInputWindow::NativeHandleList ();

    for (CompWindow *w = window; w && uScreen->doShellRepaint; w = w->prev)
    {
      if (std::find (xwns.begin (), xwns.end (), w->id ()) != xwns.end ())
      {
        uScreen->paintDisplay (region);
      }
    }
  }

  bool ret = gWindow->glDraw (matrix, attrib, region, mask);

  return ret;
}

/* Called whenever a window is mapped, unmapped, minimized etc */
void
UnityWindow::windowNotify (CompWindowNotify n)
{
  PluginAdapter::Default ()->Notify (window, n);
  window->windowNotify (n);
}

void 
UnityWindow::stateChangeNotify (unsigned int lastState)
{
  PluginAdapter::Default ()->NotifyStateChange (window, window->state (), lastState);
  window->stateChangeNotify (lastState);
}

void
UnityWindow::moveNotify (int x, int y, bool immediate)
{
  PluginAdapter::Default ()->NotifyMoved (window, x, y);
  window->moveNotify (x, y, immediate);
}

void
UnityWindow::resizeNotify (int x, int y, int w, int h)
{
  PluginAdapter::Default ()->NotifyResized (window, x, y, w, h);
  window->resizeNotify (x, y, w, h);
}

/* Configure callback for the launcher window */
void
UnityScreen::launcherWindowConfigureCallback(int WindowWidth, int WindowHeight, nux::Geometry& geo, void *user_data)
{
  int OurWindowHeight = WindowHeight - 24;
  geo = nux::Geometry(0, 24, geo.width, OurWindowHeight);
}

/* Configure callback for the panel window */
void
UnityScreen::panelWindowConfigureCallback(int WindowWidth, int WindowHeight, nux::Geometry& geo, void *user_data)
{
  geo = nux::Geometry(0, 0, WindowWidth, 24);
}

/* Start up nux after OpenGL is initialized */
void
UnityScreen::initUnity(nux::NThread* thread, void* InitData)
{
  START_FUNCTION ();
  initLauncher(thread, InitData);

  nux::ColorLayer background(nux::Color(0x00000000));
  static_cast<nux::WindowThread*>(thread)->SetWindowBackgroundPaintLayer(&background);
  END_FUNCTION ();
}

void
UnityScreen::onRedrawRequested ()
{
  damageNuxRegions ();
}

/* Handle option changes and plug that into nux windows */
void
UnityScreen::optionChanged (CompOption            *opt,
                            UnityshellOptions::Options num)
{
  switch (num)
  {
    case UnityshellOptions::LauncherAutohide:
      launcher->SetAutohide (optionGetLauncherAutohide ());
      break;
    case UnityshellOptions::BacklightAlwaysOn:
      launcher->SetBacklightAlwaysOn (optionGetBacklightAlwaysOn ());
      break;
    case UnityshellOptions::LaunchAnimation:
      launcher->SetLaunchAnimation ((Launcher::LaunchAnimation) optionGetLaunchAnimation ());
      break;
    case UnityshellOptions::UrgentAnimation:
      launcher->SetUrgentAnimation ((Launcher::UrgentAnimation) optionGetUrgentAnimation ());
      break;
    default:
      break;
  }
}

static gboolean
write_logger_data_to_disk (gpointer data)
{
  LOGGER_WRITE_LOG ("/tmp/unity-perf.log");
  return FALSE;
}

UnityScreen::UnityScreen (CompScreen *screen) :
    PluginClassHandler <UnityScreen, CompScreen> (screen),
    screen (screen),
    cScreen (CompositeScreen::get (screen)),
    gScreen (GLScreen::get (screen)),
    doShellRepaint (false)
{
  START_FUNCTION ();
  int (*old_handler) (Display *, XErrorEvent *);
  old_handler = XSetErrorHandler (NULL);

  g_thread_init (NULL);
  dbus_g_thread_init ();

  unity_a11y_preset_environment ();

  gtk_init (NULL, NULL);

  XSetErrorHandler (old_handler);

  /* Wrap compiz interfaces */
  ScreenInterface::setHandler (screen);
  CompositeScreenInterface::setHandler (cScreen);
  GLScreenInterface::setHandler (gScreen);

  PluginAdapter::Initialize (screen);
  WindowManager::SetDefault (PluginAdapter::Default ());

  StartupNotifyService::Default ()->SetSnDisplay (screen->snDisplay (), screen->screenNum ());

  nux::NuxInitialize (0);
  wt = nux::CreateFromForeignWindow (cScreen->output (),
                                     glXGetCurrentContext (),
                                     &UnityScreen::initUnity,
                                     this);

  wt->RedrawRequested.connect (sigc::mem_fun (this, &UnityScreen::onRedrawRequested));

  unity_a11y_init ();

  /* i18n init */
  bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

  wt->Run (NULL);
  uScreen = this;

  debugger = new DebugDBusInterface (this);

  optionSetLauncherAutohideNotify  (boost::bind (&UnityScreen::optionChanged, this, _1, _2));
  optionSetBacklightAlwaysOnNotify (boost::bind (&UnityScreen::optionChanged, this, _1, _2));
  optionSetLaunchAnimationNotify   (boost::bind (&UnityScreen::optionChanged, this, _1, _2));
  optionSetUrgentAnimationNotify   (boost::bind (&UnityScreen::optionChanged, this, _1, _2));
  optionSetShowLauncherInitiate (boost::bind (&UnityScreen::showLauncherKeyInitiate, this, _1, _2, _3));
  optionSetShowLauncherTerminate (boost::bind (&UnityScreen::showLauncherKeyTerminate, this, _1, _2, _3));

  g_timeout_add (0, &UnityScreen::initPluginActions, this);
  g_timeout_add (5000, (GSourceFunc) write_logger_data_to_disk, NULL);
  END_FUNCTION ();
}

UnityScreen::~UnityScreen ()
{
  unity_a11y_finalize ();
}

/* Can't create windows until after we have initialized everything */
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

/* Start up the launcher */
void UnityScreen::initLauncher (nux::NThread* thread, void* InitData)
{
  START_FUNCTION ();

  UnityScreen *self = (UnityScreen*) InitData;

  LOGGER_START_PROCESS ("initLauncher-Launcher");
  self->launcherWindow = new nux::BaseWindow(TEXT(""));
  self->launcher = new Launcher(self->launcherWindow, self->screen);
  self->AddChild (self->launcher);

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

  /* FIXME: this should not be manual, should be managed with a
     show/hide callback like in GAIL*/
  if (unity_a11y_initialized () == TRUE)
    unity_util_accessible_add_window (self->launcherWindow);

  self->launcher->SetIconSize (54, 48);
  self->launcher->SetBacklightAlwaysOn (true);
  LOGGER_END_PROCESS ("initLauncher-Launcher");

  /* Setup panel */
  LOGGER_START_PROCESS ("initLauncher-Panel");
  self->panelView = new PanelView ();
  self->AddChild (self->panelView);

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
  LOGGER_END_PROCESS ("initLauncher-Panel");

  /* Setup Places */
  self->placesController = new PlacesController ();

  self->launcher->SetAutohide (true);
  self->launcher->SetLaunchAnimation (Launcher::LAUNCH_ANIMATION_PULSE);
  self->launcher->SetUrgentAnimation (Launcher::URGENT_ANIMATION_WIGGLE);
  g_timeout_add (2000, &UnityScreen::strutHackTimeout, self);

  END_FUNCTION ();
}

/* Window init */
UnityWindow::UnityWindow (CompWindow *window) :
    PluginClassHandler <UnityWindow, CompWindow> (window),
    window (window),
    gWindow (GLWindow::get (window))
{
  WindowInterface::setHandler (window);
  GLWindowInterface::setHandler (gWindow);
}

UnityWindow::~UnityWindow ()
{
}

/* vtable init */
bool
UnityPluginVTable::init ()
{
  if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
    return false;
  if (!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI))
    return false;
  if (!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
    return false;

  return true;
}

