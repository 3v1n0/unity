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
#include "PlacesSettings.h"
#include "PluginAdapter.h"
#include "StartupNotifyService.h"
#include "unityshell.h"

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <core/atoms.h>

#include "perf-logger-utility.h"
#include "unitya11y.h"

#include "ubus-server.h"
#include "UBusMessages.h"

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
  bool skip_other_plugins = false;
  
  switch (event->type)
  {
    case FocusIn:
    case FocusOut:
      if (event->xfocus.mode == NotifyGrab)
        PluginAdapter::Default ()->OnScreenGrabbed ();
      else if (event->xfocus.mode == NotifyUngrab)
        PluginAdapter::Default ()->OnScreenUngrabbed ();
      break;
    case KeyPress:
      KeySym key_sym;
      if (XLookupString (&(event->xkey), NULL, 0, &key_sym, 0) > 0)
        skip_other_plugins = launcher->CheckSuperShortcutPressed (key_sym, event->xkey.keycode, event->xkey.state);
      break;
  }

  // avoid further propagation (key conflict for instance)
  if (!skip_other_plugins)
    screen->handleEvent (event);

  if (!skip_other_plugins && screen->otherGrabExist ("deco", "move", "wall", "switcher", NULL))
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

bool
UnityScreen::showPanelFirstMenuKeyInitiate (CompAction         *action,
                                            CompAction::State   state,
                                            CompOption::Vector &options)
{
  // to receive the Terminate event
  if (state & CompAction::StateInitKey)
    action->setState (action->state () | CompAction::StateTermKey);
  
  panelView->StartFirstMenuShow ();
  return false;
}

bool
UnityScreen::showPanelFirstMenuKeyTerminate (CompAction         *action,
                                             CompAction::State   state,
                                             CompOption::Vector &options)
{
  panelView->EndFirstMenuShow ();
  return false;
}

bool
UnityScreen::executeCommand (CompAction         *action,
                             CompAction::State   state,
                             CompOption::Vector &options)
{
  ubus_server_send_message (ubus_server_get_default (),
                            UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                            g_variant_new ("(sus)",
                                           "/com/canonical/unity/applicationsplace/runner",
                                           0,
                                           ""));
  return false;
}

void
UnityScreen::restartLauncherKeyNav ()
{
  // set input-focus on launcher-window and start key-nav mode
  if (newFocusedWindow != NULL)
  {
    newFocusedWindow->moveInputFocusTo ();
    launcher->startKeyNavMode ();
  }
}

void
UnityScreen::startLauncherKeyNav ()
{
  // get CompWindow* of launcher-window
  newFocusedWindow = screen->findWindow (launcherWindow->GetInputWindowId ());

  // check if currently focused window isn't the launcher-window
  if (newFocusedWindow != screen->findWindow (screen->activeWindow ()))
    lastFocusedWindow = screen->findWindow (screen->activeWindow ());

  // set input-focus on launcher-window and start key-nav mode
  if (newFocusedWindow != NULL)
  {
    // Put the launcher BaseWindow at the top of the BaseWindow stack. The input focus coming
    // from the XinputWindow will be processed by the launcher BaseWindow only. Then the Launcher
    // BaseWindow will decide which View will get the input focus.
    launcherWindow->PushToFront ();
    newFocusedWindow->moveInputFocusTo ();
  }
}

bool
UnityScreen::setKeyboardFocusKeyInitiate (CompAction         *action,
                                          CompAction::State  state,
                                          CompOption::Vector &options)
{
  launcher->startKeyNavMode ();

  return false;
}

void
UnityScreen::OnLauncherStartKeyNav (GVariant* data, void* value)
{
  UnityScreen *self = (UnityScreen*) value;

  self->startLauncherKeyNav ();
}

void
UnityScreen::OnLauncherEndKeyNav (GVariant* data, void* value)
{
  UnityScreen* self           = (UnityScreen*) value;
  bool         preserve_focus = false;

  if (data)
  {
    preserve_focus = g_variant_get_boolean (data);
  }

  // return input-focus to previously focused window (before key-nav-mode was
  // entered)
  if (preserve_focus)
  {
    if (self->lastFocusedWindow != NULL)
      self->lastFocusedWindow->moveInputFocusTo ();
  }
}

void
UnityScreen::OnQuicklistEndKeyNav (GVariant* data,
                                   void*     value)
{
  UnityScreen *self = (UnityScreen*) value;

  self->restartLauncherKeyNav ();
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
  UnityScreen *self = static_cast<UnityScreen *> (user_data);
  geo = nux::Geometry(self->_primary_monitor.x, self->_primary_monitor.y + 24,
                      geo.width, self->_primary_monitor.height - 24);
}

/* Configure callback for the panel window */
void
UnityScreen::panelWindowConfigureCallback(int WindowWidth, int WindowHeight, nux::Geometry& geo, void *user_data)
{
  UnityScreen *self = static_cast<UnityScreen *> (user_data);
  geo = nux::Geometry(self->_primary_monitor.x, self->_primary_monitor.y,
                      self->_primary_monitor.width, 24);
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
    case UnityshellOptions::LauncherHideMode:
      launcher->SetHideMode ((Launcher::LauncherHideMode) optionGetLauncherHideMode ());
      break;
    case UnityshellOptions::BacklightMode:
      launcher->SetBacklightMode ((Launcher::BacklightMode) optionGetBacklightMode ());
      break;
    case UnityshellOptions::LaunchAnimation:
      launcher->SetLaunchAnimation ((Launcher::LaunchAnimation) optionGetLaunchAnimation ());
      break;
    case UnityshellOptions::UrgentAnimation:
      launcher->SetUrgentAnimation ((Launcher::UrgentAnimation) optionGetUrgentAnimation ());
      break;
    case UnityshellOptions::PanelOpacity:
      panelView->SetOpacity (optionGetPanelOpacity ());
      break;
    case UnityshellOptions::IconSize:
      panelHomeButton->SetButtonWidth (optionGetIconSize()+18);
      launcher->SetIconSize (optionGetIconSize()+6, optionGetIconSize());
      PlacesController::SetLauncherSize (optionGetIconSize()+18);
      
      break;
    case UnityshellOptions::AutohideAnimation:
      launcher->SetAutoHideAnimation ((Launcher::AutoHideAnimation) optionGetAutohideAnimation ());
      break;

    case UnityshellOptions::DashBlurExperimental:
      PlacesSettings::GetDefault ()->SetDashBlurType ((PlacesSettings::DashBlurType)optionGetDashBlurExperimental ());
      break;
    default:
      break;
  }
}

void
UnityScreen::NeedsRelayout ()
{
  needsRelayout = true;
}

void
UnityScreen::ScheduleRelayout (guint timeout)
{
  g_timeout_add (timeout, &UnityScreen::RelayoutTimeout, this);
}

void
UnityScreen::Relayout ()
{
  GdkScreen *scr;
  GdkRectangle rect;
  nux::Geometry lCurGeom, pCurGeom;
  gint primary_monitor;

  if (!needsRelayout)
    return;

  scr = gdk_screen_get_default ();
  primary_monitor = gdk_screen_get_primary_monitor (scr);
  gdk_screen_get_monitor_geometry (scr, primary_monitor, &rect);
  _primary_monitor = rect;

  wt->SetWindowSize (rect.width, rect.height);

  pCurGeom = panelWindow->GetGeometry(); 
  lCurGeom = launcherWindow->GetGeometry(); 

  panelWindow->EnableInputWindow(false);
  launcherWindow->EnableInputWindow(false);
  launcherWindow->InputWindowEnableStruts(false);
  panelWindow->InputWindowEnableStruts(false);

  panelView->SetMaximumWidth(rect.width);
  launcher->SetMaximumHeight(rect.height - pCurGeom.height);

  g_debug ("setting to primary screen rect: x=%d y=%d w=%d h=%d",
           rect.x,
           rect.y,
           rect.width,
           rect.height);

  panelWindow->SetGeometry(nux::Geometry(rect.x,
					rect.y,
					rect.width,
					pCurGeom.height));
  panelView->SetGeometry(nux::Geometry(rect.x,
					rect.y,
					rect.width,
					pCurGeom.height));

  launcherWindow->SetGeometry(nux::Geometry(rect.x,
					rect.y + pCurGeom.height,
					lCurGeom.width,
					rect.height - pCurGeom.height));
  launcher->SetGeometry(nux::Geometry(rect.x,
					rect.y + pCurGeom.height,
					lCurGeom.width,
					rect.height - pCurGeom.height));

  panelWindow->EnableInputWindow(true);
  launcherWindow->EnableInputWindow(true);
  launcherWindow->InputWindowEnableStruts(true);
  panelWindow->InputWindowEnableStruts(true);

  needsRelayout = false;
}

gboolean
UnityScreen::RelayoutTimeout (gpointer data)
{
  UnityScreen *uScr = (UnityScreen*) data;

  uScr->NeedsRelayout ();
  uScr->Relayout();

  return FALSE;
}

/* Handle changes in the number of workspaces by showing the switcher
 * or not showing the switcher */
bool
UnityScreen::setOptionForPlugin(const char *plugin, const char *name, 
                                CompOption::Value &v)
{
  bool status;
  status = screen->setOptionForPlugin (plugin, name, v);
  if (status)
  {
    if (strcmp (plugin, "core") == 0 && strcmp (name, "hsize") == 0)
    {
      controller->UpdateNumWorkspaces(screen->vpSize ().width ());
    }
  }
  return status;
}

static gboolean
write_logger_data_to_disk (gpointer data)
{
  LOGGER_WRITE_LOG ("/tmp/unity-perf.log");
  return FALSE;
}

void
OnMonitorChanged (GdkScreen* screen,
                  gpointer   data)
{
  UnityScreen* uscreen = (UnityScreen*) data;
  uscreen->ScheduleRelayout (500);
}

void
OnSizeChanged (GdkScreen* screen,
               gpointer   data)
{
  UnityScreen* uscreen = (UnityScreen*) data;
  uscreen->ScheduleRelayout (500);
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

  newFocusedWindow  = NULL;
  lastFocusedWindow = NULL;

  /* i18n init */
  bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

  wt->Run (NULL);
  uScreen = this;

  debugger = new DebugDBusInterface (this);

  optionSetLauncherHideModeNotify (boost::bind (&UnityScreen::optionChanged, this, _1, _2));
  optionSetBacklightModeNotify    (boost::bind (&UnityScreen::optionChanged, this, _1, _2));
  optionSetLaunchAnimationNotify  (boost::bind (&UnityScreen::optionChanged, this, _1, _2));
  optionSetUrgentAnimationNotify  (boost::bind (&UnityScreen::optionChanged, this, _1, _2));
  optionSetPanelOpacityNotify     (boost::bind (&UnityScreen::optionChanged, this, _1, _2));
  optionSetIconSizeNotify         (boost::bind (&UnityScreen::optionChanged, this, _1, _2));
  optionSetAutohideAnimationNotify (boost::bind (&UnityScreen::optionChanged, this, _1, _2));
  optionSetDashBlurExperimentalNotify (boost::bind (&UnityScreen::optionChanged, this, _1, _2));
  optionSetShowLauncherInitiate   (boost::bind (&UnityScreen::showLauncherKeyInitiate, this, _1, _2, _3));
  optionSetShowLauncherTerminate  (boost::bind (&UnityScreen::showLauncherKeyTerminate, this, _1, _2, _3));
  optionSetKeyboardFocusInitiate  (boost::bind (&UnityScreen::setKeyboardFocusKeyInitiate, this, _1, _2, _3));
  //optionSetKeyboardFocusTerminate (boost::bind (&UnityScreen::setKeyboardFocusKeyTerminate, this, _1, _2, _3));
  optionSetExecuteCommandInitiate  (boost::bind (&UnityScreen::executeCommand, this, _1, _2, _3));
  optionSetPanelFirstMenuInitiate (boost::bind (&UnityScreen::showPanelFirstMenuKeyInitiate, this, _1, _2, _3));
  optionSetPanelFirstMenuTerminate(boost::bind (&UnityScreen::showPanelFirstMenuKeyTerminate, this, _1, _2, _3));

  UBusServer* ubus = ubus_server_get_default ();
  ubus_server_register_interest (ubus,
                                 UBUS_LAUNCHER_START_KEY_NAV,
                                 (UBusCallback)&UnityScreen::OnLauncherStartKeyNav,
                                 this);

  ubus_server_register_interest (ubus,
                                 UBUS_LAUNCHER_END_KEY_NAV,
                                 (UBusCallback)&UnityScreen::OnLauncherEndKeyNav,
                                 this);

  ubus_server_register_interest (ubus,
                                 UBUS_QUICKLIST_END_KEY_NAV,
                                 (UBusCallback)&UnityScreen::OnQuicklistEndKeyNav,
                                 this);

  g_timeout_add (0, &UnityScreen::initPluginActions, this);
  g_timeout_add (5000, (GSourceFunc) write_logger_data_to_disk, NULL);

  g_signal_connect (gdk_screen_get_default (),
                    "monitors-changed",
                     G_CALLBACK (OnMonitorChanged),
                     this);
  g_signal_connect (gdk_screen_get_default (),
                    "size-changed",
                    G_CALLBACK (OnSizeChanged),
                    this);

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

  if (self->launcher->GetHideMode () == Launcher::LAUNCHER_HIDE_NEVER)
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
  self->launcherWindow = new nux::BaseWindow(TEXT("LauncherWindow"));
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
  self->launcherWindow->ShowWindow(true);
  self->launcherWindow->EnableInputWindow(true, "launcher", false, false);
  self->launcherWindow->InputWindowEnableStruts(true);
  self->launcherWindow->SetEnterFocusInputArea (self->launcher);

  /* FIXME: this should not be manual, should be managed with a
     show/hide callback like in GAIL*/
  if (unity_a11y_initialized () == TRUE)
    unity_util_accessible_add_window (self->launcherWindow);

  self->launcher->SetIconSize (54, 48);
  self->launcher->SetBacklightMode (Launcher::BACKLIGHT_ALWAYS_ON);
  LOGGER_END_PROCESS ("initLauncher-Launcher");

  /* Setup panel */
  LOGGER_START_PROCESS ("initLauncher-Panel");
  self->panelView = new PanelView ();
  self->AddChild (self->panelView);

  self->panelHomeButton = self->panelView->HomeButton ();

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
  self->panelWindow->ShowWindow(true);
  self->panelWindow->EnableInputWindow(true, "panel", false, false);
  self->panelWindow->InputWindowEnableStruts(true);

  /* FIXME: this should not be manual, should be managed with a
     show/hide callback like in GAIL*/
  if (unity_a11y_initialized () == TRUE)
    unity_util_accessible_add_window (self->panelWindow);

  LOGGER_END_PROCESS ("initLauncher-Panel");

  /* Setup Places */
  self->placesController = new PlacesController ();

  /* FIXME: this should not be manual, should be managed with a
     show/hide callback like in GAIL*/
  if (unity_a11y_initialized () == TRUE)
    unity_util_accessible_add_window (self->placesController->GetWindow ());

  self->launcher->SetHideMode (Launcher::LAUNCHER_HIDE_DODGE_WINDOWS);
  self->launcher->SetLaunchAnimation (Launcher::LAUNCH_ANIMATION_PULSE);
  self->launcher->SetUrgentAnimation (Launcher::URGENT_ANIMATION_WIGGLE);
  self->ScheduleRelayout (2000);
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
  UnityScreen *us = UnityScreen::get (screen);
  if (us->newFocusedWindow && (UnityWindow::get (us->newFocusedWindow) == this))
    us->newFocusedWindow = NULL;
  if (us->lastFocusedWindow && (UnityWindow::get (us->lastFocusedWindow) == this))
    us->lastFocusedWindow = NULL;
}

namespace {

/* Checks whether an extension is supported by the GLX or OpenGL implementation
 * given the extension name and the list of supported extensions. */
gboolean
is_extension_supported (const gchar *extensions, const gchar *extension)
{
  if (extensions != NULL && extension != NULL)
  {
    const gsize len = strlen (extension);
    gchar* p = (gchar*) extensions;
    gchar* end = p + strlen (p);

    while (p < end)
    {
      const gsize size = strcspn (p, " ");
      if (len == size && strncmp (extension, p, size) == 0)
        return TRUE;
      p += size + 1;
    }
  }

  return FALSE;
}

} /* anonymous namespace */

/* vtable init */
bool
UnityPluginVTable::init ()
{
  gchar* extensions;

  if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
    return false;
  if (!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI))
    return false;
  if (!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
    return false;

  /* Ensure OpenGL extensions required by the Unity plugin are available. */
  extensions = (gchar*) glGetString (GL_EXTENSIONS);
  if (!is_extension_supported (extensions, "GL_ARBB_vertex_program"))
  {
    compLogMessage ("unityshell", CompLogLevelError,
                    "GL_ARB_vertex_program not supported\n");
    return false;
  }
  if (!is_extension_supported (extensions, "GL_ARB_fragment_program"))
  {
    compLogMessage ("unityshell", CompLogLevelError,
                    "GL_ARB_fragment_program not supported\n");
    return false;
  }
  if (!is_extension_supported (extensions, "GL_ARB_vertex_buffer_object"))
  {
    compLogMessage ("unityshell", CompLogLevelError,
                    "GL_ARB_vertex_buffer_object not supported\n");
    return false;
  }
  if (!is_extension_supported (extensions, "GL_ARB_framebuffer_object"))
  {
    if (!is_extension_supported (extensions, "GL_EXT_framebuffer_object"))
    {
      compLogMessage ("unityshell", CompLogLevelError,
                      "GL_ARB_framebuffer_object or GL_EXT_framebuffer_object "
                      "not supported\n");
      return false;
    }
  }
  if (!is_extension_supported (extensions, "GL_ARB_texture_non_power_of_two"))
  {
    if (!is_extension_supported (extensions, "GL_ARB_texture_rectangle"))
    {
      compLogMessage ("unityshell", CompLogLevelError,
                      "GL_ARB_texture_non_power_of_two or "
                      "GL_ARB_texture_rectangle not supported\n");
      return false;
    }
  }

  return true;
}

