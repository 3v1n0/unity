// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/* Compiz unity plugin
 * unity.cpp
 *
 * Copyright (c) 2010-11 Canonical Ltd.
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

#include <NuxCore/Logger.h>
#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/BaseWindow.h>
#include <Nux/WindowCompositor.h>

#include "IconRenderer.h"
#include "Launcher.h"
#include "LauncherIcon.h"
#include "LauncherController.h"
#include "GeisAdapter.h"
#include "DevicesSettings.h"
#include "PluginAdapter.h"
#include "QuicklistManager.h"
#include "StartupNotifyService.h"
#include "Timer.h"
#include "KeyboardUtil.h"
#include "unityshell.h"
#include "BackgroundEffectHelper.h"

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <libnotify/notify.h>

#include <sstream>

#include <core/atoms.h>

#include "unitya11y.h"

#include "ubus-server.h"
#include "UBusMessages.h"
#include "UScreen.h"

#include "config.h"

/* FIXME: once we get a better method to add the toplevel windows to
   the accessible root object, this include would not be required */
#include "unity-util-accessible.h"

/* Set up vtable symbols */
COMPIZ_PLUGIN_20090315(unityshell, unity::UnityPluginVTable);

namespace unity
{
using namespace launcher;
using launcher::AbstractLauncherIcon;
using launcher::Launcher;
using ui::KeyboardUtil;
using ui::LayoutWindow;
using ui::LayoutWindowList;
using util::Timer;

namespace
{

nux::logging::Logger logger("unity.shell");

UnityScreen* uScreen = 0;

void configure_logging();
void capture_g_log_calls(const gchar* log_domain,
                         GLogLevelFlags log_level,
                         const gchar* message,
                         gpointer user_data);
gboolean is_extension_supported(const gchar* extensions, const gchar* extension);
gfloat get_opengl_version_f32(const gchar* version_string);

}

UnityScreen::UnityScreen(CompScreen* screen)
  : BaseSwitchScreen (screen)
  , PluginClassHandler <UnityScreen, CompScreen> (screen)
  , screen(screen)
  , cScreen(CompositeScreen::get(screen))
  , gScreen(GLScreen::get(screen))
  , enable_shortcut_overlay_(true)
  , gestureEngine(nullptr)
  , wt(nullptr)
  , panelWindow(nullptr)
  , debugger(nullptr)
  , needsRelayout(false)
  , _in_paint(false)
  , relayoutSourceId(0)
  , _redraw_handle(0)
  , newFocusedWindow(nullptr)
  , doShellRepaint(false)
  , allowWindowPaint(false)
  , damaged(false)
  , _key_nav_mode_requested(false)
  , _last_output(nullptr)
#ifndef USE_GLES
  , _active_fbo (0)
#endif
  , dash_is_open_ (false)
  , grab_index_ (0)
  , painting_tray_ (false)
{
  Timer timer;
  gfloat version;
  gchar* extensions;
  bool  failed = false;
  configure_logging();
  LOG_DEBUG(logger) << __PRETTY_FUNCTION__;
  int (*old_handler)(Display*, XErrorEvent*);
  old_handler = XSetErrorHandler(NULL);

#ifndef USE_GLES
  /* Ensure OpenGL version is 1.4+. */
  version = get_opengl_version_f32((const gchar*) glGetString(GL_VERSION));
  if (version < 1.4f)
  {
    compLogMessage("unityshell", CompLogLevelError,
                   "OpenGL 1.4+ not supported\n");
    setFailed ();
    failed = true;
  }

  /* Ensure OpenGL extensions required by the Unity plugin are available. */
  extensions = (gchar*) glGetString(GL_EXTENSIONS);
  if (!is_extension_supported(extensions, "GL_ARB_vertex_program"))
  {
    compLogMessage("unityshell", CompLogLevelError,
                   "GL_ARB_vertex_program not supported\n");
    setFailed ();
    failed = true;
  }
  if (!is_extension_supported(extensions, "GL_ARB_fragment_program"))
  {
    compLogMessage("unityshell", CompLogLevelError,
                   "GL_ARB_fragment_program not supported\n");
    setFailed ();
    failed = true;
  }
  if (!is_extension_supported(extensions, "GL_ARB_vertex_buffer_object"))
  {
    compLogMessage("unityshell", CompLogLevelError,
                   "GL_ARB_vertex_buffer_object not supported\n");
    setFailed ();
    failed = true;
  }
  if (!is_extension_supported(extensions, "GL_ARB_framebuffer_object"))
  {
    if (!is_extension_supported(extensions, "GL_EXT_framebuffer_object"))
    {
      compLogMessage("unityshell", CompLogLevelError,
                     "GL_ARB_framebuffer_object or GL_EXT_framebuffer_object "
                     "not supported\n");
      setFailed();
      failed = true;
    }
  }
  if (!is_extension_supported(extensions, "GL_ARB_texture_non_power_of_two"))
  {
    if (!is_extension_supported(extensions, "GL_ARB_texture_rectangle"))
    {
      compLogMessage("unityshell", CompLogLevelError,
                     "GL_ARB_texture_non_power_of_two or "
                     "GL_ARB_texture_rectangle not supported\n");
      setFailed ();
      failed = true;
    }
  }
#endif

  if (!failed)
  {
     notify_init("unityshell");

     dbus_g_thread_init();

     unity_a11y_preset_environment();

     XSetErrorHandler(old_handler);

     /* Wrap compiz interfaces */
     ScreenInterface::setHandler(screen);
     CompositeScreenInterface::setHandler(cScreen);
     GLScreenInterface::setHandler(gScreen);

#ifdef USE_GLES
     gScreen->glPaintCompositedOutputSetEnabled (this, true);
#endif

     PluginAdapter::Initialize(screen);
     WindowManager::SetDefault(PluginAdapter::Default());

     StartupNotifyService::Default()->SetSnDisplay(screen->snDisplay(), screen->screenNum());

     nux::NuxInitialize(0);
#ifndef USE_GLES
     wt = nux::CreateFromForeignWindow(cScreen->output(),
				       glXGetCurrentContext(),
				       &UnityScreen::initUnity,
				       this);
#else
     wt = nux::CreateFromForeignWindow(cScreen->output(),
				       eglGetCurrentContext(),
				       &UnityScreen::initUnity,
				       this);
#endif

     wt->RedrawRequested.connect(sigc::mem_fun(this, &UnityScreen::onRedrawRequested));

     unity_a11y_init(wt);

     /* i18n init */
     bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
     bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

     wt->Run(NULL);
     uScreen = this;

     debugger = new unity::debug::DebugDBusInterface(this, this->screen);

     _in_paint = false;

#ifndef USE_GLES
    void *dlhand = dlopen ("libunityshell.so", RTLD_LAZY);

    if (dlhand)
    {
      dlerror ();
      glXGetProcAddressP = (ScreenEffectFramebufferObject::GLXGetProcAddressProc) dlsym (dlhand, "glXGetProcAddress");
      if (dlerror () != NULL)
        glXGetProcAddressP = NULL;
    }

     if (GL::fbo)
     {
       nux::Geometry geometry (0, 0, screen->width (), screen->height ());
       uScreen->_fbo = ScreenEffectFramebufferObject::Ptr (new ScreenEffectFramebufferObject (glXGetProcAddressP, geometry));
       uScreen->_fbo->onScreenSizeChanged (geometry);
     }
#endif

     optionSetBackgroundColorNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetLauncherHideModeNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetBacklightModeNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetLaunchAnimationNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetUrgentAnimationNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetPanelOpacityNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetPanelOpacityMaximizedToggleNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetMenusFadeinNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetMenusFadeoutNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetMenusDiscoveryDurationNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetMenusDiscoveryFadeinNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetMenusDiscoveryFadeoutNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetLauncherOpacityNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetIconSizeNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetAutohideAnimationNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetDashBlurExperimentalNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetDevicesOptionNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetShortcutOverlayNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetShowDesktopIconNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetShowLauncherInitiate(boost::bind(&UnityScreen::showLauncherKeyInitiate, this, _1, _2, _3));
     optionSetShowLauncherTerminate(boost::bind(&UnityScreen::showLauncherKeyTerminate, this, _1, _2, _3));
     optionSetKeyboardFocusInitiate(boost::bind(&UnityScreen::setKeyboardFocusKeyInitiate, this, _1, _2, _3));
     //optionSetKeyboardFocusTerminate (boost::bind (&UnityScreen::setKeyboardFocusKeyTerminate, this, _1, _2, _3));
     optionSetExecuteCommandInitiate(boost::bind(&UnityScreen::executeCommand, this, _1, _2, _3));
     optionSetPanelFirstMenuInitiate(boost::bind(&UnityScreen::showPanelFirstMenuKeyInitiate, this, _1, _2, _3));
     optionSetPanelFirstMenuTerminate(boost::bind(&UnityScreen::showPanelFirstMenuKeyTerminate, this, _1, _2, _3));
     optionSetAutomaximizeValueNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetAltTabTimeoutNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetAltTabBiasViewportNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));

     optionSetAltTabForwardInitiate(boost::bind(&UnityScreen::altTabForwardInitiate, this, _1, _2, _3));
     optionSetAltTabForwardTerminate(boost::bind(&UnityScreen::altTabTerminateCommon, this, _1, _2, _3));
     optionSetAltTabPrevInitiate(boost::bind(&UnityScreen::altTabPrevInitiate, this, _1, _2, _3));

     optionSetAltTabDetailStartInitiate(boost::bind(&UnityScreen::altTabDetailStartInitiate, this, _1, _2, _3));
     optionSetAltTabDetailStopInitiate(boost::bind(&UnityScreen::altTabDetailStopInitiate, this, _1, _2, _3));

     optionSetAltTabNextWindowInitiate(boost::bind(&UnityScreen::altTabNextWindowInitiate, this, _1, _2, _3));
     optionSetAltTabNextWindowTerminate(boost::bind(&UnityScreen::altTabTerminateCommon, this, _1, _2, _3));

     optionSetAltTabPrevWindowInitiate(boost::bind(&UnityScreen::altTabPrevWindowInitiate, this, _1, _2, _3));

     optionSetAltTabLeftInitiate(boost::bind (&UnityScreen::altTabPrevInitiate, this, _1, _2, _3));
     optionSetAltTabRightInitiate(boost::bind (&UnityScreen::altTabForwardInitiate, this, _1, _2, _3));

     optionSetLauncherSwitcherForwardInitiate(boost::bind(&UnityScreen::launcherSwitcherForwardInitiate, this, _1, _2, _3));
     optionSetLauncherSwitcherPrevInitiate(boost::bind(&UnityScreen::launcherSwitcherPrevInitiate, this, _1, _2, _3));
     optionSetLauncherSwitcherForwardTerminate(boost::bind(&UnityScreen::launcherSwitcherTerminate, this, _1, _2, _3));

     optionSetStopVelocityNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetRevealPressureNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetEdgeResponsivenessNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetOvercomePressureNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetDecayRateNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetShowMinimizedWindowsNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));

     ubus_manager_.RegisterInterest(UBUS_LAUNCHER_START_KEY_NAV,
                   sigc::mem_fun(this, &UnityScreen::OnLauncherStartKeyNav));

     ubus_manager_.RegisterInterest(UBUS_LAUNCHER_END_KEY_NAV,
                   sigc::mem_fun(this, &UnityScreen::OnLauncherEndKeyNav));

     g_idle_add_full (G_PRIORITY_DEFAULT, &UnityScreen::initPluginActions, this, NULL);
     super_keypressed_ = false;

     GeisAdapter::Default()->Run();
     gestureEngine = new GestureEngine(screen);

     CompString name(PKGDATADIR"/panel-shadow.png");
     CompString pname("unityshell");
     CompSize size(1, 20);
     _shadow_texture = GLTexture::readImageToTexture(name, pname, size);

     BackgroundEffectHelper::updates_enabled = true;

     ubus_manager_.RegisterInterest(UBUS_PLACE_VIEW_SHOWN, [&](GVariant * args) { 
       dash_is_open_ = true; 
       dash_monitor_ = g_variant_get_int32(args);
       RaiseInputWindows(); 
     });
     ubus_manager_.RegisterInterest(UBUS_PLACE_VIEW_HIDDEN, [&](GVariant * args) { dash_is_open_ = false; });
      LOG_INFO(logger) << "UnityScreen constructed: " << timer.ElapsedSeconds() << "s";
  }
}

UnityScreen::~UnityScreen()
{
  notify_uninit();

  unity_a11y_finalize();

  if (relayoutSourceId != 0)
    g_source_remove(relayoutSourceId);

  ::unity::ui::IconRenderer::DestroyTextures();
  QuicklistManager::Destroy();

  delete wt;
}

void UnityScreen::initAltTabNextWindow()
{
  KeyboardUtil key_util (screen->dpy());
  guint above_tab_keycode = key_util.GetKeycodeAboveKeySymbol (XStringToKeysym("Tab"));
  KeySym above_tab_keysym = XkbKeycodeToKeysym (screen->dpy(), above_tab_keycode, 0, 0);

  if (above_tab_keysym != NoSymbol)
  {
    {
      std::ostringstream sout;
      sout << "<Alt>" << XKeysymToString(above_tab_keysym);

      screen->removeAction(&optionGetAltTabNextWindow());
      
      CompAction action = CompAction();
      action.keyFromString(sout.str());
      action.setState (CompAction::StateInitKey | CompAction::StateAutoGrab);
      mOptions[UnityshellOptions::AltTabNextWindow].value().set (action);
      screen->addAction (&mOptions[UnityshellOptions::AltTabNextWindow].value ().action ());

      optionSetAltTabNextWindowInitiate(boost::bind(&UnityScreen::altTabNextWindowInitiate, this, _1, _2, _3));
      optionSetAltTabNextWindowTerminate(boost::bind(&UnityScreen::altTabTerminateCommon, this, _1, _2, _3));
    }
    {
      std::ostringstream sout;
      sout << "<Alt><Shift>" << XKeysymToString(above_tab_keysym);

      screen->removeAction(&optionGetAltTabPrevWindow());
      
      CompAction action = CompAction();
      action.keyFromString(sout.str());
      action.setState (CompAction::StateInitKey | CompAction::StateAutoGrab);
      mOptions[UnityshellOptions::AltTabPrevWindow].value().set (action);
      screen->addAction (&mOptions[UnityshellOptions::AltTabPrevWindow].value ().action ());

      optionSetAltTabPrevWindowInitiate(boost::bind(&UnityScreen::altTabPrevWindowInitiate, this, _1, _2, _3));
    }
  }
  else
  {
    printf ("Could not find key above tab!\n");
  }

}

void UnityScreen::EnsureSuperKeybindings()
{
  for (auto action : _shortcut_actions)
    screen->removeAction(action.get());

  _shortcut_actions.clear ();

  for (auto shortcut : launcher_controller_->GetAllShortcuts())
  {
    CreateSuperNewAction(shortcut);
    CreateSuperNewAction(shortcut, true);
    CreateSuperNewAction(shortcut, false, true);
  }

  for (auto shortcut : dash_controller_->GetAllShortcuts())
    CreateSuperNewAction(shortcut);
}

void UnityScreen::CreateSuperNewAction(char shortcut, bool use_shift, bool use_numpad)
{
    CompActionPtr action(new CompAction());

    CompAction::KeyBinding binding;
    std::ostringstream sout;
    if (use_shift)
      sout << "<Shift><Super>"  << shortcut;
    else
      sout << "<Super>" << ((use_numpad) ? "KP_" : "") << shortcut;

    binding.fromString(sout.str());

    action->setKey(binding);

    screen->addAction(action.get());
    _shortcut_actions.push_back(action);
}

void UnityScreen::nuxPrologue()
{
#ifndef USE_GLES
  /* Vertex lighting isn't used in Unity, we disable that state as it could have
   * been leaked by another plugin. That should theoretically be switched off
   * right after PushAttrib since ENABLE_BIT is meant to restore the LIGHTING
   * bit, but we do that here in order to workaround a bug (?) in the NVIDIA
   * drivers (lp:703140). */
  glDisable(GL_LIGHTING);

  /* reset matrices */
  glPushAttrib(GL_VIEWPORT_BIT | GL_ENABLE_BIT |
               GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT | GL_SCISSOR_BIT);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
#endif

  glGetError();
}

void UnityScreen::nuxEpilogue()
{
#ifndef USE_GLES
  (*GL::bindFramebuffer)(GL_FRAMEBUFFER_EXT, _active_fbo);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glDepthRange(0, 1);
  glViewport(-1, -1, 2, 2);
  glRasterPos2f(0, 0);

  gScreen->resetRasterPos();

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glDrawBuffer(GL_BACK);
  glReadBuffer(GL_BACK);

  glPopAttrib();
#else
  glDepthRangef(0, 1);
  //glViewport(-1, -1, 2, 2);
  gScreen->resetRasterPos();
#endif

  glDisable(GL_SCISSOR_TEST);
}

void UnityScreen::paintPanelShadow(const GLMatrix& matrix)
{
#ifndef USE_GLES
  if (relayoutSourceId > 0)
    return;

  if (PluginAdapter::Default()->IsExpoActive())
    return;

  CompOutput* output = _last_output;
  float vc[4];
  float h = 20.0f;
  float w = 1.0f;
  float panel_h = 24.0f;

  float x1 = output->x();
  float y1 = output->y() + panel_h;
  float x2 = x1 + output->width();
  float y2 = y1 + h;
  GLMatrix sTransform = GLMatrix ();

  sTransform.toScreenSpace(output, -DEFAULT_Z_CAMERA);

  vc[0] = x1;
  vc[1] = x2;
  vc[2] = y1;
  vc[3] = y2;

  // compiz doesn't use the same method of tracking monitors as our toolkit
  // we need to make sure we properly associate with the right monitor
  int current_monitor = -1;
  auto monitors = UScreen::GetDefault()->GetMonitors();
  int i = 0;
  for (auto monitor : monitors)
  {
    if (monitor.x == output->x() && monitor.y == output->y())
    {
      current_monitor = i;
      break;
    }
    i++;
  }

  if (!(dash_is_open_ && current_monitor == dash_monitor_) && panel_controller_->opacity() > 0.0f)
  {
    foreach(GLTexture * tex, _shadow_texture)
    {
      glEnable(GL_BLEND);
      glColor4f(1.0f, 1.0f, 1.0f, panel_controller_->opacity());
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

      GL::activeTexture(GL_TEXTURE0_ARB);
      tex->enable(GLTexture::Fast);

      glTexParameteri(tex->target(), GL_TEXTURE_WRAP_S, GL_REPEAT);

      glBegin(GL_QUADS);
      {
        glTexCoord2f(COMP_TEX_COORD_X(tex->matrix(), 0), COMP_TEX_COORD_Y(tex->matrix(), 0));
        glVertex2f(vc[0], vc[2]);

        glTexCoord2f(COMP_TEX_COORD_X(tex->matrix(), 0), COMP_TEX_COORD_Y(tex->matrix(), h));
        glVertex2f(vc[0], vc[3]);

        glTexCoord2f(COMP_TEX_COORD_X(tex->matrix(), w), COMP_TEX_COORD_Y(tex->matrix(), h));
        glVertex2f(vc[1], vc[3]);

        glTexCoord2f(COMP_TEX_COORD_X(tex->matrix(), w), COMP_TEX_COORD_Y(tex->matrix(), 0));
        glVertex2f(vc[1], vc[2]);
      }
      glEnd();

      tex->disable();
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      glDisable(GL_BLEND);
    }
  }
#else
#warning Panel shadow not properly implemented for GLES2
  return;

  if (relayoutSourceId > 0)
    return;

  if (PluginAdapter::Default()->IsExpoActive())
    return;

  nuxPrologue();

  CompOutput* output = _last_output;
  float vc[4];
  float h = 20.0f;
  float w = 1.0f;
  float panel_h = 24.0f;

  float x1 = output->x();
  float y1 = output->y() + panel_h;
  float x2 = x1 + output->width();
  float y2 = y1 + h;

  vc[0] = x1;
  vc[1] = x2;
  vc[2] = y1;
  vc[3] = y2;

  // compiz doesn't use the same method of tracking monitors as our toolkit
  // we need to make sure we properly associate with the right monitor
  int current_monitor = -1;
  auto monitors = UScreen::GetDefault()->GetMonitors();
  int i = 0;
  for (auto monitor : monitors)
  {
    if (monitor.x == output->x() && monitor.y == output->y())
    {
      current_monitor = i;
      break;
    }
    i++;
  }

  if (!(dash_is_open_ && current_monitor == dash_monitor_) && panel_controller_->opacity() > 0.0f)
  {
    foreach(GLTexture * tex, _shadow_texture)
    {
      std::vector<GLfloat>  vertexData;
      std::vector<GLfloat>  textureData;
      std::vector<GLushort> colorData;
      GLVertexBuffer       *streamingBuffer = GLVertexBuffer::streamingBuffer();
      bool                  wasBlend = glIsEnabled(GL_BLEND);

      if (!wasBlend)
        glEnable(GL_BLEND);

      GL::activeTexture(GL_TEXTURE0);
      tex->enable(GLTexture::Fast);

      glTexParameteri(tex->target(), GL_TEXTURE_WRAP_S, GL_REPEAT);

      colorData = { 0xFFFF, 0xFFFF, 0xFFFF,
                    (GLushort)(panel_controller_->opacity() * 0xFFFF)
                  };

      vertexData = {
        vc[0], vc[2], 0,
        vc[0], vc[3], 0,
        vc[1], vc[2], 0,
        vc[1], vc[3], 0,
      };

      textureData = {
        COMP_TEX_COORD_X(tex->matrix(), 0), COMP_TEX_COORD_Y(tex->matrix(), 0),
        COMP_TEX_COORD_X(tex->matrix(), 0), COMP_TEX_COORD_Y(tex->matrix(), h),
        COMP_TEX_COORD_X(tex->matrix(), w), COMP_TEX_COORD_Y(tex->matrix(), 0),
        COMP_TEX_COORD_X(tex->matrix(), w), COMP_TEX_COORD_Y(tex->matrix(), h),
      };

      streamingBuffer->begin(GL_TRIANGLE_STRIP);

      streamingBuffer->addColors(1, &colorData[0]);
      streamingBuffer->addVertices(4, &vertexData[0]);
      streamingBuffer->addTexCoords(0, 4, &textureData[0]);

      streamingBuffer->end();
      streamingBuffer->render(matrix);

      tex->disable();
      if (!wasBlend)
        glDisable(GL_BLEND);
    }
  }
  nuxEpilogue();
#endif
}

void
UnityWindow::updateIconPos (int   &wx,
                            int   &wy,
                            int   x,
                            int   y,
                            float width,
                            float height)
{
  wx = x + (last_bound.width - width) / 2;
  wy = y + (last_bound.height - height) / 2;
}

#ifdef USE_GLES
void UnityScreen::paintDisplay()
#else
void UnityScreen::paintDisplay(const CompRegion& region, const GLMatrix& transform, unsigned int mask)
#endif
{
  CompOutput *output = _last_output;
  Window     tray_xid = panel_controller_->GetTrayXid ();

#ifndef USE_GLES
  bool was_bound = _fbo->bound ();
  _fbo->unbind ();

  /* Draw the bit of the relevant framebuffer for each output */

  if (was_bound)
  {
    GLMatrix sTransform;
    sTransform.toScreenSpace (&screen->fullscreenOutput (), -DEFAULT_Z_CAMERA);
    glPushMatrix ();
    glLoadMatrixf (sTransform.getMatrix ());
    _fbo->paint (nux::Geometry (output->x (), output->y (), output->width (), output->height ()));
    glPopMatrix ();
  }

  nux::ObjectPtr<nux::IOpenGLTexture2D> device_texture =
      nux::GetGraphicsDisplay()->GetGpuDevice()->CreateTexture2DFromID(_fbo->texture(),
                                                                       screen->width (), screen->height(), 1, nux::BITFMT_R8G8B8A8);
#else
  nux::ObjectPtr<nux::IOpenGLTexture2D> device_texture = 
    nux::GetGraphicsDisplay()->GetGpuDevice()->CreateTexture2DFromID(gScreen->fbo ()->tex ()->name (),
      output->width(), output->height(), 1, nux::BITFMT_R8G8B8A8);
#endif

  nux::GetGraphicsDisplay()->GetGpuDevice()->backup_texture0_ = device_texture;

  nux::Geometry geo = nux::Geometry (0, 0, screen->width (), screen->height ());
  nux::Geometry oGeo = nux::Geometry (output->x (), output->y (), output->width (), output->height ());
  BackgroundEffectHelper::monitor_rect_ = geo;

#ifdef USE_GLES
  GLint fboID;
  // Nux renders to the referenceFramebuffer when it's embedded.
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fboID);
  wt->GetWindowCompositor().SetReferenceFramebuffer(fboID, geo);
#endif

  nuxPrologue();
  _in_paint = true;
  wt->RenderInterfaceFromForeignCmd (&oGeo);
  _in_paint = false;
  nuxEpilogue();

  if (tray_xid && !allowWindowPaint)
  {
    CompWindow *tray = screen->findWindow (tray_xid);

    if (tray)
    {
      GLMatrix oTransform;
      UnityWindow  *uTrayWindow = UnityWindow::get (tray);
#ifndef USE_GLES
      GLFragment::Attrib attrib (uTrayWindow->gWindow->lastPaintAttrib());
#else
      GLWindowPaintAttrib attrib (uTrayWindow->gWindow->lastPaintAttrib());
#endif
      unsigned int oldGlAddGeometryIndex = uTrayWindow->gWindow->glAddGeometryGetCurrentIndex ();
      unsigned int oldGlDrawIndex = uTrayWindow->gWindow->glDrawGetCurrentIndex ();
#ifndef USE_GLES
      unsigned int oldGlDrawGeometryIndex = uTrayWindow->gWindow->glDrawGeometryGetCurrentIndex ();
#endif

#ifndef USE_GLES
      attrib.setOpacity (OPAQUE);
      attrib.setBrightness (BRIGHT);
      attrib.setSaturation (COLOR);
#else
      attrib.opacity = OPAQUE;
      attrib.brightness = BRIGHT;
      attrib.saturation = COLOR;
#endif

      oTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

#ifndef USE_GLES
      glPushMatrix ();
      glLoadMatrixf (oTransform.getMatrix ());
#endif

      painting_tray_ = true;

      /* force the use of the core functions */
      uTrayWindow->gWindow->glDrawSetCurrentIndex (MAXSHORT);
      uTrayWindow->gWindow->glAddGeometrySetCurrentIndex ( MAXSHORT);
#ifndef USE_GLES
      uTrayWindow->gWindow->glDrawGeometrySetCurrentIndex (MAXSHORT);
#endif
      uTrayWindow->gWindow->glDraw (oTransform, attrib, infiniteRegion,
				     PAINT_WINDOW_TRANSFORMED_MASK |
				     PAINT_WINDOW_BLEND_MASK |
				     PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK);
#ifndef USE_GLES
      uTrayWindow->gWindow->glDrawGeometrySetCurrentIndex (oldGlDrawGeometryIndex);
#endif
      uTrayWindow->gWindow->glAddGeometrySetCurrentIndex (oldGlAddGeometryIndex);
      uTrayWindow->gWindow->glDrawSetCurrentIndex (oldGlDrawIndex);
      painting_tray_ = false;

#ifndef USE_GLES
      glPopMatrix ();
#endif
    }
  }

 if (switcher_controller_->Visible ())
  {
    LayoutWindowList targets = switcher_controller_->ExternalRenderTargets ();

    for (LayoutWindow::Ptr target : targets)
    {
      CompWindow* window = screen->findWindow(target->xid);
      if (window)
      {
        UnityWindow *unity_window = UnityWindow::get (window);

        unity_window->paintThumbnail (target->result, target->alpha);
      }
    }
  }

  doShellRepaint = false;
  damaged = false;
}

bool UnityScreen::forcePaintOnTop ()
{
    return !allowWindowPaint ||
	    ((switcher_controller_->Visible() ||
	      dash_is_open_) && !fullscreen_windows_.empty () && (!(screen->grabbed () && !screen->otherGrabExist (NULL))));
}

void UnityWindow::paintThumbnail (nux::Geometry const& bounding, float alpha)
{
  GLMatrix matrix;
  matrix.toScreenSpace (UnityScreen::get (screen)->_last_output, -DEFAULT_Z_CAMERA);

  nux::Geometry geo = bounding;
  last_bound = geo;

  GLWindowPaintAttrib attrib = gWindow->lastPaintAttrib ();
  attrib.opacity = (GLushort) (alpha * G_MAXUSHORT);

  paintThumb (attrib,
              matrix,
              0,
              geo.x,
              geo.y,
              geo.width,
              geo.height,
              geo.width,
              geo.height);
}

void UnityScreen::EnableCancelAction(bool enabled, int modifiers)
{
  if (enabled)
  {
    /* Create a new keybinding for the Escape key and the current modifiers */
    CompAction::KeyBinding binding(9, modifiers);

    _escape_action = CompActionPtr(new CompAction());
    _escape_action->setKey(binding);

    screen->addAction(_escape_action.get());
  }
  else if (!enabled && _escape_action.get())
  {
    screen->removeAction(_escape_action.get());
    _escape_action = nullptr;
  }
}

void UnityScreen::enterShowDesktopMode ()
{
  for (CompWindow *w : screen->windows ())
  {
    if (UnityShowdesktopHandler::shouldHide (w))
    {
      UnityWindow::get (w)->enterShowDesktop ();
      // the animation plugin does strange things here ...
      // if this notification is sent
      // w->windowNotify (CompWindowNotifyEnterShowDesktopMode);
    }
    if (w->type() & CompWindowTypeDesktopMask)
      w->moveInputFocusTo();
  }

  PluginAdapter::Default()->OnShowDesktop();

  /* Disable the focus handler as we will report that
   * minimized windows can be focused which will
   * allow them to enter showdesktop mode. That's
   * no good */
  for (CompWindow *w : screen->windows ())
  {
    UnityWindow *uw = UnityWindow::get (w);
    w->focusSetEnabled (uw, false);
  }

  screen->enterShowDesktopMode ();

  for (CompWindow *w : screen->windows ())
  {
    UnityWindow *uw = UnityWindow::get (w);
    w->focusSetEnabled (uw, true);
  }
}

void UnityScreen::leaveShowDesktopMode (CompWindow *w)
{
  /* Where a window is inhibiting, only allow the window
   * that is inhibiting the leave show desktop to actually
   * fade in again - all other windows should remain faded out */
  if (!UnityShowdesktopHandler::inhibitingXid ())
  {
    for (CompWindow *cw : screen->windows ())
    {
      if (cw->inShowDesktopMode ())
      {
	UnityWindow::get (cw)->leaveShowDesktop ();
	// the animation plugin does strange things here ...
	// if this notification is sent
	//cw->windowNotify (CompWindowNotifyLeaveShowDesktopMode);
      }
    }

    PluginAdapter::Default()->OnLeaveDesktop();

    screen->leaveShowDesktopMode (w);
  }
  else
  {
    CompWindow *cw = screen->findWindow (UnityShowdesktopHandler::inhibitingXid ());
    if (cw)
    {
      if (cw->inShowDesktopMode ())
      {
	UnityWindow::get (cw)->leaveShowDesktop ();
      }
    }
  }
}

void UnityWindow::enterShowDesktop ()
{
  if (!mShowdesktopHandler)
    mShowdesktopHandler = new UnityShowdesktopHandler (window);

  window->setShowDesktopMode (true);
  mShowdesktopHandler->fadeOut ();
}

void UnityWindow::leaveShowDesktop ()
{
  if (mShowdesktopHandler)
  {
    mShowdesktopHandler->fadeIn ();
    window->setShowDesktopMode (false);
    delete mShowdesktopHandler;
    mShowdesktopHandler = NULL;
  }
}

void UnityWindow::activate ()
{
  UnityShowdesktopHandler::inhibitLeaveShowdesktopMode (window->id ());
  window->activate ();
  UnityShowdesktopHandler::allowLeaveShowdesktopMode (window->id ());
}

bool UnityWindow::handleAnimations (unsigned int ms)
{
  if (mShowdesktopHandler)
    if (mShowdesktopHandler->animate (ms))
    { 
      delete mShowdesktopHandler;
      mShowdesktopHandler = NULL;
      return true;
    }

  return false;
}

/* 300 ms */
const unsigned int UnityShowdesktopHandler::fade_time = 300;
CompWindowList UnityShowdesktopHandler::animating_windows (0);

bool UnityShowdesktopHandler::shouldHide (CompWindow *w)
{
  if (w->overrideRedirect ())
    return false;

  if (!w->managed ())
    return false;

  if (w->grabbed ())
    return false;

  if (w->wmType () & (CompWindowTypeDesktopMask |
                      CompWindowTypeDockMask))
   return false;

  if (w->state () & (CompWindowStateSkipPagerMask |
		     CompWindowStateSkipTaskbarMask))
    return false;

  if ((w->state () & CompWindowStateHiddenMask))
    if (!(w->inShowDesktopMode () || w->shaded ()))
      return false;

  return true;
}

guint32 UnityShowdesktopHandler::mInhibitingXid = 0;

void
UnityShowdesktopHandler::inhibitLeaveShowdesktopMode (guint32 xid)
{
  if (!mInhibitingXid)
    mInhibitingXid = xid;
}

void
UnityShowdesktopHandler::allowLeaveShowdesktopMode (guint32 xid)
{
  if (mInhibitingXid == xid)
    mInhibitingXid = 0;
}

guint32
UnityShowdesktopHandler::inhibitingXid ()
{
  return mInhibitingXid;
}

UnityShowdesktopHandler::UnityShowdesktopHandler (CompWindow *w) :
  mWindow (w),
  mRemover (new compiz::WindowInputRemover (screen->dpy (), w->id ())),
  mState (Visible),
  mProgress (0.0f)
{
}

UnityShowdesktopHandler::~UnityShowdesktopHandler ()
{
  if (mRemover)
    delete mRemover;
}

void UnityShowdesktopHandler::fadeOut ()
{
  mState = UnityShowdesktopHandler::FadeOut;
  mProgress = 1.0f;

  mWasHidden = mWindow->state () & CompWindowStateHiddenMask;

  if (!mWasHidden)
  {
    mWindow->changeState (mWindow->state () | CompWindowStateHiddenMask);
    mWindow->windowNotify (CompWindowNotifyHide);
    mRemover->save ();
    mRemover->remove ();
  }

  CompositeWindow::get (mWindow)->addDamage ();

  if (std::find (animating_windows.begin(),
                 animating_windows.end(),
                 mWindow) == animating_windows.end())
    animating_windows.push_back(mWindow);
}

void UnityShowdesktopHandler::fadeIn ()
{
  mState = UnityShowdesktopHandler::FadeIn;

  if (!mWasHidden)
  {
    mWindow->changeState (mWindow->state () & ~CompWindowStateHiddenMask);
    mWindow->windowNotify (CompWindowNotifyShow);
    mRemover->restore ();
  }

  CompositeWindow::get (mWindow)->addDamage ();
}

bool UnityShowdesktopHandler::animate (unsigned int ms)
{
  float inc = fade_time / (float) ms;

  if (mState == UnityShowdesktopHandler::FadeOut)
  {
    mProgress -= inc;
    if (mProgress <= 0.0f)
    {
      mProgress = 0.0f;
      mState = Invisible;
    }
    else
      CompositeWindow::get (mWindow)->addDamage ();
  }
  else if (mState == FadeIn)
  {
    mProgress += inc;
    if (mProgress >= 1.0f)
    {
      mProgress = 1.0f;
      mState = Visible;

      return true;
    }
    else
      CompositeWindow::get (mWindow)->addDamage ();
  }

  return false;
}

void UnityShowdesktopHandler::paintAttrib (GLWindowPaintAttrib &attrib)
{
  attrib.opacity = static_cast <int> (static_cast <float> (attrib.opacity) * mProgress);
}

unsigned int UnityShowdesktopHandler::getPaintMask ()
{
    return 0;
}

void UnityShowdesktopHandler::handleEvent (XEvent *event)
{
  /* Ignore sent events from the InputRemover */
  if (screen->XShape () && event->type ==
      screen->shapeEvent () + ShapeNotify &&
      !event->xany.send_event)
  {
    if (mRemover)
    {
      mRemover->save ();
      mRemover->remove ();
    }
  }
}

void UnityShowdesktopHandler::windowNotify (CompWindowNotify n)
{
  if (n == CompWindowNotifyFocusChange && mWindow->minimized ())
  {
    for (CompWindow *w : animating_windows)
      w->focusSetEnabled (UnityWindow::get (w), false);

    mWindow->moveInputFocusToOtherWindow ();

    for (CompWindow *w : animating_windows)
      w->focusSetEnabled (UnityWindow::get (w), true);
  }
}

void UnityShowdesktopHandler::updateFrameRegion (CompRegion &r)
{
  unsigned int oldUpdateFrameRegionIndex;
  r = CompRegion ();

  /* Ensure no other plugins can touch this frame region */
  oldUpdateFrameRegionIndex = mWindow->updateFrameRegionGetCurrentIndex ();
  mWindow->updateFrameRegionSetCurrentIndex (MAXSHORT);
  mWindow->updateFrameRegion (r);
  mWindow->updateFrameRegionSetCurrentIndex (oldUpdateFrameRegionIndex);
}

/* called whenever we need to repaint parts of the screen */
bool UnityScreen::glPaintOutput(const GLScreenPaintAttrib& attrib,
                                const GLMatrix& transform,
                                const CompRegion& region,
                                CompOutput* output,
                                unsigned int mask)
{
  bool ret;

  doShellRepaint = true;
  allowWindowPaint = true;
  _last_output = output;

#ifndef USE_GLES
  /* bind the framebuffer here
   * - it will be unbound and flushed
   *   to the backbuffer when some
   *   plugin requests to draw a
   *   a transformed screen or when
   *   we have finished this draw cycle.
   *   once an fbo is bound any further
   *   attempts to bind it will only increment
   *   its bind reference so make sure that
   *   you always unbind as much as you bind */
  _fbo->bind (nux::Geometry (output->x (), output->y (), output->width (), output->height ()));
#endif

  /* glPaintOutput is part of the opengl plugin, so we need the GLScreen base class. */
  ret = gScreen->glPaintOutput(attrib, transform, region, output, mask);

#ifndef USE_GLES
  if (doShellRepaint)
    paintDisplay(region, transform, mask);
#endif

  return ret;
}

#ifdef USE_GLES
void UnityScreen::glPaintCompositedOutput (const CompRegion &region,
                                           GLFramebufferObject *fbo,
                                           unsigned int        mask)
{
  bool useFbo = false;

  if (doShellRepaint)
  {
    oldFbo = fbo->bind ();
    useFbo = fbo->checkStatus () && fbo->tex ();
    if (!useFbo) {
	printf ("bailing from UnityScreen::glPaintCompositedOutput");
	GLFramebufferObject::rebind (oldFbo);
	return;
    }
    paintDisplay();
    GLFramebufferObject::rebind (oldFbo);
  }

  gScreen->glPaintCompositedOutput(region, fbo, mask);
}
#endif

/* called whenever a plugin needs to paint the entire scene
 * transformed */

void UnityScreen::glPaintTransformedOutput(const GLScreenPaintAttrib& attrib,
                                           const GLMatrix& transform,
                                           const CompRegion& region,
                                           CompOutput* output,
                                           unsigned int mask)
{
  allowWindowPaint = false;
  gScreen->glPaintTransformedOutput(attrib, transform, region, output, mask);

}

void UnityScreen::preparePaint(int ms)
{
  CompWindowList remove_windows;

  cScreen->preparePaint(ms);

  for (CompWindow *w : UnityShowdesktopHandler::animating_windows)
    if (UnityWindow::get (w)->handleAnimations (ms))
      remove_windows.push_back(w);

  for (CompWindow *w : remove_windows)
    UnityShowdesktopHandler::animating_windows.remove (w);

  if (damaged)
  {
    damaged = false;
    damageNuxRegions();
  }

}

/* Grab changed nux regions and add damage rects for them */
void UnityScreen::damageNuxRegions()
{
  CompRegion nux_damage;

  if (damaged)
    return;

  std::vector<nux::Geometry> dirty = wt->GetDrawList();
  damaged = true;

  for (std::vector<nux::Geometry>::iterator it = dirty.begin(), end = dirty.end();
       it != end; ++it)
  {
    nux::Geometry const& geo = *it;
    nux_damage = CompRegion(geo.x, geo.y, geo.width, geo.height);
    cScreen->damageRegion(nux_damage);
  }

  nux::Geometry geo = wt->GetWindowCompositor().GetTooltipMainWindowGeometry();
  nux_damage = CompRegion(geo.x, geo.y, geo.width, geo.height);
  cScreen->damageRegion(nux_damage);

  geo = lastTooltipArea;
  nux_damage = CompRegion(lastTooltipArea.x, lastTooltipArea.y,
                          lastTooltipArea.width, lastTooltipArea.height);
  cScreen->damageRegion(nux_damage);

  wt->ClearDrawList();

  lastTooltipArea = geo;
}

/* handle X Events */
void UnityScreen::handleEvent(XEvent* event)
{
  bool skip_other_plugins = false;
  switch (event->type)
  {
    case FocusIn:
    case FocusOut:
      if (event->xfocus.mode == NotifyGrab)
        PluginAdapter::Default()->OnScreenGrabbed();
      else if (event->xfocus.mode == NotifyUngrab)
        PluginAdapter::Default()->OnScreenUngrabbed();
#ifndef USE_GLES
      cScreen->damageScreen();  // evil hack
#endif
      if (_key_nav_mode_requested)
        launcher_controller_->KeyNavGrab();
      _key_nav_mode_requested = false;
      break;
    case ButtonPress:
      if (super_keypressed_)
      {
        launcher_controller_->KeyNavTerminate(false);
        EnableCancelAction(false);
      }
      break;
    case KeyPress:
    {
      KeySym key_sym;
      char key_string[2];
      int result = XLookupString(&(event->xkey), key_string, 2, &key_sym, 0);
      if (result > 0)
      {
        // NOTE: does this have the potential to do an invalid write?  Perhaps
        // we should just say "key_string[1] = 0" because that is the only
        // thing that could possibly make sense here.
        key_string[result] = 0;
        if (super_keypressed_ && key_sym != XK_Escape)
        {
          g_idle_add([] (gpointer data) -> gboolean {
            auto self = static_cast<UnityScreen*>(data);
            if (!self->launcher_controller_->KeyNavIsActive())
            {
              self->shortcut_controller_->SetEnabled(false);
              self->shortcut_controller_->Hide();
            }
            return FALSE;
          }, this);

          skip_other_plugins = launcher_controller_->HandleLauncherKeyEvent(screen->dpy(), key_sym, event->xkey.keycode, event->xkey.state, key_string);
          if (!skip_other_plugins)
            skip_other_plugins = dash_controller_->CheckShortcutActivation(key_string);

          if (skip_other_plugins && launcher_controller_->KeyNavIsActive())
          {
            launcher_controller_->KeyNavTerminate(false);
            EnableCancelAction(false);
          }
        }
      }
      break;
    }
    default:
        if (screen->shapeEvent () + ShapeNotify == event->type)
        {
          Window xid = event->xany.window;
          CompWindow *w = screen->findWindow(xid);

          if (w)
          {
            UnityWindow *uw = UnityWindow::get (w);

            if (uw->mShowdesktopHandler)
              uw->mShowdesktopHandler->handleEvent(event);
          }
        }
      break;
  }

  compiz::CompizMinimizedWindowHandler<UnityScreen, UnityWindow>::handleEvent (event);

  // avoid further propagation (key conflict for instance)
  if (!skip_other_plugins)
    screen->handleEvent(event);

  if (event->type == PropertyNotify)
  {
    if (event->xproperty.atom == Atoms::mwmHints)
    {
      PluginAdapter::Default ()->NotifyNewDecorationState(event->xproperty.window);
    }
  }

  if (!skip_other_plugins &&
      screen->otherGrabExist("deco", "move", "switcher", "resize", NULL) &&
      !switcher_controller_->Visible())
  {
    wt->ProcessForeignEvent(event, NULL);
  }

  if (event->type == cScreen->damageEvent() + XDamageNotify)
  {
    XDamageNotifyEvent *de = (XDamageNotifyEvent *) event;
    CompWindow* w = screen->findWindow (de->drawable);
    std::vector<Window> const& xwns = nux::XInputWindow::NativeHandleList();
    CompWindow* lastNWindow = screen->findWindow (xwns.back ());
    bool        processDamage = true;

    if (w)
    {
      if (!w->overrideRedirect () &&
          w->isViewable () &&
          !w->invisible ())
      {

        for (; lastNWindow != NULL; lastNWindow = lastNWindow->next)
        {
          if (lastNWindow == w)
          {
            processDamage = false;
            break;
          }
        }

        if (processDamage)
        {
          nux::Geometry damage (de->area.x, de->area.y, de->area.width, de->area.height);

          const CompWindow::Geometry &geom = w->geometry ();
          damage.x += geom.x () + geom.border ();
          damage.y += geom.y () + geom.border ();

          BackgroundEffectHelper::ProcessDamage(damage);
        }
      }
    }
  }
}

void UnityScreen::handleCompizEvent(const char* plugin,
                                    const char* event,
                                    CompOption::Vector& option)
{
  PluginAdapter::Default()->NotifyCompizEvent(plugin, event, option);
  compiz::CompizMinimizedWindowHandler<UnityScreen, UnityWindow>::handleCompizEvent (plugin, event, option);

  if (dash_is_open_ && g_strcmp0(event, "start_viewport_switch") == 0)
  {
    ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
  }

  screen->handleCompizEvent(plugin, event, option);
}

bool UnityScreen::showLauncherKeyInitiate(CompAction* action,
                                          CompAction::State state,
                                          CompOption::Vector& options)
{
  // to receive the Terminate event
  if (state & CompAction::StateInitKey)
    action->setState(action->state() | CompAction::StateTermKey);

  super_keypressed_ = true;
  launcher_controller_->HandleLauncherKeyPress();
  EnsureSuperKeybindings ();

  if (!shortcut_controller_->Visible() && shortcut_controller_->IsEnabled())
  {
    static nux::Geometry last_geo;
    UScreen* uscreen = UScreen::GetDefault();
    int primary_monitor = uscreen->GetMonitorWithMouse();
    auto monitor_geo = uscreen->GetMonitorGeometry(primary_monitor);

    int width = 970;
    int height =  680;
    int launcher_width = optionGetIconSize() + 18;
    int panel_height = 24;
    int x = monitor_geo.x + launcher_width + (monitor_geo.width - launcher_width- width) / 2;
    int y = monitor_geo.y + panel_height + (monitor_geo.height - panel_height - height) / 2;

    nux::Geometry geo (x, y, width, height);

    if (last_geo != geo)
    {
      shortcut_controller_->SetWorkspace(geo);
      last_geo = geo;
    }

    if (last_geo.x > monitor_geo.x and last_geo.y > monitor_geo.y)
      shortcut_controller_->Show();
   }

  return false;
}

bool UnityScreen::showLauncherKeyTerminate(CompAction* action,
                                           CompAction::State state,
                                           CompOption::Vector& options)
{
  if (state & CompAction::StateCancel)
    return false;

  bool accept_state = (state & CompAction::StateCancel) == 0;

  super_keypressed_ = false;
  launcher_controller_->KeyNavTerminate(accept_state);
  launcher_controller_->HandleLauncherKeyRelease();
  EnableCancelAction(false);

  shortcut_controller_->SetEnabled(enable_shortcut_overlay_);
  shortcut_controller_->Hide();
  action->setState (action->state() & (unsigned)~(CompAction::StateTermKey));
  return false;
}

bool UnityScreen::showPanelFirstMenuKeyInitiate(CompAction* action,
                                                CompAction::State state,
                                                CompOption::Vector& options)
{
  grab_index_ = screen->pushGrab (None, "unityshell");
  // to receive the Terminate event
  action->setState(action->state() | CompAction::StateTermKey);
  panel_controller_->StartFirstMenuShow();
  return false;
}

bool UnityScreen::showPanelFirstMenuKeyTerminate(CompAction* action,
                                                 CompAction::State state,
                                                 CompOption::Vector& options)
{
  screen->removeGrab(grab_index_, NULL);
  action->setState (action->state() & (unsigned)~(CompAction::StateTermKey));
  panel_controller_->EndFirstMenuShow();
  return false;
}

void UnityScreen::SendExecuteCommand()
{
  ubus_manager_.SendMessage(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                            g_variant_new("(sus)", "commands.lens", 0, ""));
}

bool UnityScreen::executeCommand(CompAction* action,
                                 CompAction::State state,
                                 CompOption::Vector& options)
{
  SendExecuteCommand();
  return false;
}

void UnityScreen::startLauncherKeyNav()
{
  // get CompWindow* of launcher-window
  newFocusedWindow = screen->findWindow(launcher_controller_->KeyNavLauncherInputWindowId());

  // check if currently focused window isn't the launcher-window
  if (newFocusedWindow != screen->findWindow(screen->activeWindow()))
    PluginAdapter::Default()->saveInputFocus();

  // set input-focus on launcher-window and start key-nav mode
  if (newFocusedWindow)
  {
    // Put the launcher BaseWindow at the top of the BaseWindow stack. The
    // input focus coming from the XinputWindow will be processed by the
    // launcher BaseWindow only. Then the Launcher BaseWindow will decide
    // which View will get the input focus.
    launcher_controller_->PushToFront();
    newFocusedWindow->moveInputFocusTo();
  }
}

bool UnityScreen::setKeyboardFocusKeyInitiate(CompAction* action,
                                              CompAction::State state,
                                              CompOption::Vector& options)
{
  _key_nav_mode_requested = true;
  return false;
}

bool UnityScreen::altTabInitiateCommon(CompAction *action,
                                      CompAction::State state,
                                      CompOption::Vector& options)
{
  if (!grab_index_)
    grab_index_ = screen->pushGrab (screen->invisibleCursor(), "unity-switcher");
  if (!grab_index_)
    return false;
  
  screen->addAction(&optionGetAltTabRight());
  screen->addAction(&optionGetAltTabDetailStart());
  screen->addAction(&optionGetAltTabDetailStop());
  screen->addAction(&optionGetAltTabLeft());

  // maybe check launcher position/hide state?

  int device = screen->outputDeviceForPoint (pointerX, pointerY);
  switcher_controller_->SetWorkspace(nux::Geometry(screen->outputDevs()[device].x1() + 100,
                                                 screen->outputDevs()[device].y1() + 100,
                                                 screen->outputDevs()[device].width() - 200,
                                                 screen->outputDevs()[device].height() - 200), device);

  switcher::ShowMode show_mode = optionGetAltTabBiasViewport() ? switcher::ShowMode::CURRENT_VIEWPORT : switcher::ShowMode::ALL;

  RaiseInputWindows();

  int show_monitor = (show_mode == switcher::ShowMode::CURRENT_VIEWPORT) ? device : -1;

  std::vector<unity::launcher::AbstractLauncherIcon*> results = launcher_controller_->GetAltTabIcons(show_monitor);

  if (!(results.size() == 1 && results[0]->Type() == AbstractLauncherIcon::IconType::TYPE_BEGIN))
    switcher_controller_->Show(show_mode, switcher::SortMode::FOCUS_ORDER, false, results);

  return true;
}

bool UnityScreen::altTabTerminateCommon(CompAction* action,
                                       CompAction::State state,
                                       CompOption::Vector& options)
{
  if (grab_index_)
  {
    // remove grab before calling hide so workspace switcher doesn't fail
    screen->removeGrab(grab_index_, NULL);
    grab_index_ = 0;

    screen->removeAction (&optionGetAltTabRight ());
    screen->removeAction (&optionGetAltTabDetailStart ());
    screen->removeAction (&optionGetAltTabDetailStop ());
    screen->removeAction (&optionGetAltTabLeft ());

    bool accept_state = (state & CompAction::StateCancel) == 0;
    switcher_controller_->Hide(accept_state);
  }

  action->setState (action->state() & (unsigned)~(CompAction::StateTermKey));
  return true;
}

bool UnityScreen::altTabForwardInitiate(CompAction* action,
                                        CompAction::State state,
                                        CompOption::Vector& options)
{
  if (switcher_controller_->Visible())
    switcher_controller_->Next();
  else
    altTabInitiateCommon(action, state, options);

  action->setState(action->state() | CompAction::StateTermKey);
  return false;
}


bool UnityScreen::altTabPrevInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  if (switcher_controller_->Visible())
    switcher_controller_->Prev();

  return false;
}

bool UnityScreen::altTabDetailStartInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  if (switcher_controller_->Visible())
    switcher_controller_->SetDetail(true);

  return false;
}

bool UnityScreen::altTabDetailStopInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  if (switcher_controller_->Visible())
    switcher_controller_->SetDetail(false);

  return false;
}

bool UnityScreen::altTabNextWindowInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  if (!switcher_controller_->Visible())
  {
    altTabInitiateCommon(action, state, options);
    switcher_controller_->Select(1); // always select the current application
  }
  
  switcher_controller_->NextDetail();

  action->setState(action->state() | CompAction::StateTermKey);
  return false;
}

bool UnityScreen::altTabPrevWindowInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  if (switcher_controller_->Visible())
    switcher_controller_->PrevDetail();
  
  return false;
}

bool UnityScreen::launcherSwitcherForwardInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  if (!launcher_controller_->KeyNavIsActive())
  {
    launcher_controller_->KeyNavActivate();
    EnableCancelAction(true, action->key().modifiers());
  }
  else
  {
    launcher_controller_->KeyNavNext();
  }

  action->setState(action->state() | CompAction::StateTermKey);
  return false;
}
bool UnityScreen::launcherSwitcherPrevInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  launcher_controller_->KeyNavPrevious();

  return false;
}
bool UnityScreen::launcherSwitcherTerminate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  bool accept_state = (state & CompAction::StateCancel) == 0;
  launcher_controller_->KeyNavTerminate(accept_state);

  EnableCancelAction(false);
  action->setState (action->state() & (unsigned)~(CompAction::StateTermKey));
  return false;
}

void UnityScreen::OnLauncherStartKeyNav(GVariant* data)
{
  startLauncherKeyNav();
}

void UnityScreen::OnLauncherEndKeyNav(GVariant* data)
{
  bool preserve_focus = false;

  if (data)
  {
    preserve_focus = g_variant_get_boolean(data);
  }

  // Return input-focus to previously focused window (before key-nav-mode was
  // entered)
  if (preserve_focus)
    PluginAdapter::Default ()->restoreInputFocus ();
}

gboolean UnityScreen::initPluginActions(gpointer data)
{
  CompPlugin* p = CompPlugin::find("expo");

  if (p)
  {
    MultiActionList expoActions(0);

    foreach(CompOption & option, p->vTable->getOptions())
    {
      if (option.name() == "expo_key" ||
          option.name() == "expo_button" ||
          option.name() == "expo_edge")
      {
        CompAction* action = &option.value().action();
        expoActions.AddNewAction(action, false);
        break;
      }
    }

    PluginAdapter::Default()->SetExpoAction(expoActions);
  }

  p = CompPlugin::find("scale");

  if (p)
  {
    MultiActionList scaleActions(0);

    foreach(CompOption & option, p->vTable->getOptions())
    {
      if (option.name() == "initiate_all_key" ||
          option.name() == "initiate_all_edge" ||
          option.name() == "initiate_key" ||
          option.name() == "initiate_button" ||
          option.name() == "initiate_edge" ||
          option.name() == "initiate_group_key" ||
          option.name() == "initiate_group_button" ||
          option.name() == "initiate_group_edge" ||
          option.name() == "initiate_output_key" ||
          option.name() == "initiate_output_button" ||
          option.name() == "initiate_output_edge")
      {
        CompAction* action = &option.value().action();
        scaleActions.AddNewAction(action, false);
      }
      else if (option.name() == "initiate_all_button")
      {
        CompAction* action = &option.value().action();
        scaleActions.AddNewAction(action, true);
      }
    }

    PluginAdapter::Default()->SetScaleAction(scaleActions);
  }

  p = CompPlugin::find("unitymtgrabhandles");

  if (p)
  {
    foreach(CompOption & option, p->vTable->getOptions())
    {
      if (option.name() == "show_handles_key")
        PluginAdapter::Default()->SetShowHandlesAction(&option.value().action());
      else if (option.name() == "hide_handles_key")
        PluginAdapter::Default()->SetHideHandlesAction(&option.value().action());
      else if (option.name() == "toggle_handles_key")
        PluginAdapter::Default()->SetToggleHandlesAction(&option.value().action());
    }
  }

  return FALSE;
}

/* Set up expo and scale actions on the launcher */
bool UnityScreen::initPluginForScreen(CompPlugin* p)
{
  if (p->vTable->name() == "expo" ||
      p->vTable->name() == "scale")
  {
    initPluginActions(this);
  }

  bool result = screen->initPluginForScreen(p);
  if (p->vTable->name() == "unityshell")
    initAltTabNextWindow();
    
  return result;
}

void UnityScreen::AddProperties(GVariantBuilder* builder)
{
}

std::string UnityScreen::GetName() const
{
  return "Unity";
}

bool isNuxWindow (CompWindow* value) 
{ 
  std::vector<Window> const& xwns = nux::XInputWindow::NativeHandleList();
  auto id = value->id();

  // iterate loop by hand rather than use std::find as this is considerably faster
  // we care about performance here becuase of the high frequency in which this function is
  // called (nearly every frame)
  unsigned int size = xwns.size();
  for (unsigned int i = 0; i < size; ++i)
  {
    if (xwns[i] == id)
      return true;
  }
  return false;
}

const CompWindowList& UnityScreen::getWindowPaintList()
{
  CompWindowList& pl = _withRemovedNuxWindows = cScreen->getWindowPaintList();
  pl.remove_if(isNuxWindow);

  return pl;
}

void UnityScreen::RaiseInputWindows()
{
  std::vector<Window> const& xwns = nux::XInputWindow::NativeHandleList();
  
  for (auto window : xwns)
  {
    CompWindow* cwin = screen->findWindow(window);
    if (cwin)
      cwin->raise();
  }
}

/* detect occlusions
 *
 * core passes down the PAINT_WINDOW_OCCLUSION_DETECTION
 * mask when it is doing occlusion detection, so use that
 * order to fill our occlusion buffer which we'll flip
 * to nux later
 */
bool UnityWindow::glPaint(const GLWindowPaintAttrib& attrib,
                          const GLMatrix& matrix,
                          const CompRegion& region,
                          unsigned int mask)
{
  GLWindowPaintAttrib wAttrib = attrib;

  if (mMinimizeHandler)
  {
    mask |= mMinimizeHandler->getPaintMask ();
  }
  else if (mShowdesktopHandler)
  {
    mShowdesktopHandler->paintAttrib (wAttrib);
    mask |= mShowdesktopHandler->getPaintMask ();
  }

  if (uScreen->panel_controller_->GetTrayXid () == window->id () && !uScreen->allowWindowPaint)
  {
    if (!uScreen->painting_tray_)
    {
      uScreen->tray_paint_mask_ = mask;
      mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;
    }
  }

  return gWindow->glPaint(wAttrib, matrix, region, mask);
}

/* handle window painting in an opengl context
 *
 * we want to paint underneath other windows here,
 * so we need to find if this window is actually
 * stacked on top of one of the nux input windows
 * and if so paint nux and stop us from painting
 * other windows or on top of the whole screen */
bool UnityWindow::glDraw(const GLMatrix& matrix,
#ifndef USE_GLES
                         GLFragment::Attrib& attrib,
#else
                         const GLWindowPaintAttrib& attrib,
#endif
                         const CompRegion& region,
                         unsigned int mask)
{
  if (uScreen->doShellRepaint && !uScreen->forcePaintOnTop ())
  {
    std::vector<Window> const& xwns = nux::XInputWindow::NativeHandleList();
    unsigned int size = xwns.size();

    for (CompWindow* w = window; w && uScreen->doShellRepaint; w = w->prev)
    {
      auto id = w->id();
      for (unsigned int i = 0; i < size; ++i)
      {
        if (xwns[i] == id)
        {
#ifdef USE_GLES
          uScreen->paintDisplay();
#else
          uScreen->paintDisplay(region, matrix, mask);
#endif          
          break;
        }
      }
    }
  }

  bool ret = gWindow->glDraw(matrix, attrib, region, mask);

  if (window->type() == CompWindowTypeDesktopMask)
  {
    uScreen->paintPanelShadow(matrix);
  }

  return ret;
}

void
UnityWindow::minimize ()
{
  if (!window->managed ())
    return;

  if (!mMinimizeHandler)
  {
    mMinimizeHandler = new UnityMinimizedHandler (window);
    mMinimizeHandler->minimize ();
  }
}

void
UnityWindow::unminimize ()
{
  if (mMinimizeHandler)
  {
    mMinimizeHandler->unminimize ();
    delete mMinimizeHandler;
    mMinimizeHandler = nullptr;
  }
}

bool
UnityWindow::focus ()
{
  if (!mMinimizeHandler)
    return window->focus ();

  if (window->overrideRedirect ())
    return false;

  if (!window->managed ())
    return false;

  if (!window->onCurrentDesktop ())
    return false;

  /* Only withdrawn windows 
   * which are marked hidden
   * are excluded */
  if (!window->shaded () &&
      !window->minimized () &&
      (window->state () & CompWindowStateHiddenMask))
    return false;

  if (window->geometry ().x () + window->geometry ().width ()  <= 0	||
      window->geometry ().y () + window->geometry ().height () <= 0	||
      window->geometry ().x () >= (int) screen->width ()||
      window->geometry ().y () >= (int) screen->height ())
    return false;

  return true;
}

bool
UnityWindow::minimized ()
{
  return mMinimizeHandler != nullptr;
}

gboolean
UnityWindow::FocusDesktopTimeout(gpointer data)
{
  UnityWindow *self = reinterpret_cast<UnityWindow*>(data);

  self->focusdesktop_handle_ = 0;

  for (CompWindow *w : screen->clientList ())
  {
    if (!(w->type() & NO_FOCUS_MASK) && w->focus ())
      return FALSE;
  }
  self->window->moveInputFocusTo();

  return FALSE;
}

/* Called whenever a window is mapped, unmapped, minimized etc */
void UnityWindow::windowNotify(CompWindowNotify n)
{
  PluginAdapter::Default()->Notify(window, n);

  switch (n)
  {
    case CompWindowNotifyMap:
      if (window->type() == CompWindowTypeDesktopMask) {
        if (!focusdesktop_handle_)
           focusdesktop_handle_ = g_timeout_add (1000, &UnityWindow::FocusDesktopTimeout, this);
      }
    /* Fall through an re-evaluate wraps on map and unmap too */
    case CompWindowNotifyUnmap:
      if (UnityScreen::get (screen)->optionGetShowMinimizedWindows () &&
          window->mapNum ())
      {
        bool wasMinimized = window->minimized ();
        if (wasMinimized)
          window->unminimize ();
        window->focusSetEnabled (this, true);
        window->minimizeSetEnabled (this, true);
        window->unminimizeSetEnabled (this, true);
        window->minimizedSetEnabled (this, true);

        if (wasMinimized)
          window->minimize ();
      }
      else
      {
        window->focusSetEnabled (this, false);
        window->minimizeSetEnabled (this, false);
        window->unminimizeSetEnabled (this, false);
        window->minimizedSetEnabled (this, false);
      }
        break;
      default:
        break;
  }


  window->windowNotify(n);

  if (mMinimizeHandler)
  {
    /* The minimize handler will short circuit the frame
     * region update func and ensure that the frame
     * does not have a region */
    mMinimizeHandler->windowNotify (n);
  }
  else if (mShowdesktopHandler)
  {
    mShowdesktopHandler->windowNotify (n);
  }

  // We do this after the notify to ensure input focus has actually been moved.
  if (n == CompWindowNotifyFocusChange)
  {
    UnityScreen* us = UnityScreen::get(screen);
    CompWindow *lw;

    if (us->dash_is_open_)
    {
      lw = screen->findWindow(us->launcher_controller_->LauncherWindowId(0));
      lw->moveInputFocusTo();
    }
  }
}

void UnityWindow::stateChangeNotify(unsigned int lastState)
{
  if (window->state () & CompWindowStateFullscreenMask &&
      !(lastState & CompWindowStateFullscreenMask))
    UnityScreen::get (screen)->fullscreen_windows_.push_back(window);
  else if (lastState & CompWindowStateFullscreenMask &&
	   !(window->state () & CompWindowStateFullscreenMask))
    UnityScreen::get (screen)->fullscreen_windows_.remove(window);

  PluginAdapter::Default()->NotifyStateChange(window, window->state(), lastState);
  window->stateChangeNotify(lastState);
}

void UnityWindow::updateFrameRegion(CompRegion &region)
{
  /* The minimize handler will short circuit the frame
   * region update func and ensure that the frame
   * does not have a region */

  if (mMinimizeHandler)
    mMinimizeHandler->updateFrameRegion (region);
  else if (mShowdesktopHandler)
    mShowdesktopHandler->updateFrameRegion (region);
  else
    window->updateFrameRegion (region);
}

void UnityWindow::moveNotify(int x, int y, bool immediate)
{
  PluginAdapter::Default()->NotifyMoved(window, x, y);
  window->moveNotify(x, y, immediate);
}

void UnityWindow::resizeNotify(int x, int y, int w, int h)
{
  PluginAdapter::Default()->NotifyResized(window, x, y, w, h);
  window->resizeNotify(x, y, w, h);
}

CompPoint UnityWindow::tryNotIntersectUI(CompPoint& pos)
{
  UnityScreen* us = UnityScreen::get(screen);
  auto window_geo = window->borderRect ();
  nux::Geometry target_monitor;
  nux::Point result(pos.x(), pos.y());

  // seriously why does compiz not track monitors XRandR style???
  auto monitors = UScreen::GetDefault()->GetMonitors();
  for (auto monitor : monitors)
  {
    if (monitor.IsInside(result))
    {
      target_monitor = monitor;
      break;
    }
  }

  auto launchers = us->launcher_controller_->launchers();
  for (auto launcher : launchers)
  {
    nux::Geometry geo = launcher->GetAbsoluteGeometry();
    
    if (launcher->Hidden() || launcher->options()->hide_mode == LAUNCHER_HIDE_NEVER || launcher->options()->hide_mode == LAUNCHER_HIDE_AUTOHIDE)
      continue;
    
    if (geo.IsInside(result))
    {
      if (geo.x + geo.width + 1 + window_geo.width() < target_monitor.x + target_monitor.width)
      {
        result.x = geo.x + geo.width + 1;
      }
    }
  }

  for (nux::Geometry &geo : us->panel_controller_->GetGeometries ())
  {
    if (geo.IsInside(result))
    {
      if (geo.y + geo.height + window_geo.height() < target_monitor.y + target_monitor.height)
      {
        result.y = geo.y + geo.height;
      }
    }
  }

  pos.setX(result.x);
  pos.setY(result.y);
  return pos;
}

bool UnityWindow::place(CompPoint& pos)
{
  bool was_maximized = PluginAdapter::Default ()->MaximizeIfBigEnough(window);

  if (!was_maximized)
  {
    bool result = window->place(pos);

    if (window->type() & NO_FOCUS_MASK)
      return result;

    pos = tryNotIntersectUI(pos);
    return result;
  }

  return true;
}


/* Start up nux after OpenGL is initialized */
void UnityScreen::initUnity(nux::NThread* thread, void* InitData)
{
  Timer timer;
  UnityScreen* self = reinterpret_cast<UnityScreen*>(InitData);
  self->initLauncher();

  nux::ColorLayer background(nux::color::Transparent);
  static_cast<nux::WindowThread*>(thread)->SetWindowBackgroundPaintLayer(&background);
  LOG_INFO(logger) << "UnityScreen::initUnity: " << timer.ElapsedSeconds() << "s";
}

gboolean UnityScreen::OnRedrawTimeout(gpointer data)
{
  UnityScreen *self = reinterpret_cast<UnityScreen*>(data);

  self->_redraw_handle = 0;
  self->onRedrawRequested();

  return FALSE;
}

void UnityScreen::onRedrawRequested()
{
  // disable blur updates so we dont waste perf. This can stall the blur during animations
  // but ensures a smooth animation.
  if (_in_paint)
  {
    if (!_redraw_handle)
      _redraw_handle = g_idle_add_full (G_PRIORITY_DEFAULT, &UnityScreen::OnRedrawTimeout, this, NULL);
  }
  else
  {
    damageNuxRegions();
  }
}

/* Handle option changes and plug that into nux windows */
void UnityScreen::optionChanged(CompOption* opt, UnityshellOptions::Options num)
{
  // Note: perhaps we should put the options here into the controller.
  unity::launcher::Options::Ptr launcher_options = launcher_controller_->options();
  switch (num)
  {
    case UnityshellOptions::BackgroundColor:
    {
      nux::Color override_color (optionGetBackgroundColorRed() / 65535.0f, 
                                 optionGetBackgroundColorGreen() / 65535.0f, 
                                 optionGetBackgroundColorBlue() / 65535.0f, 
                                 optionGetBackgroundColorAlpha() / 65535.0f);

      override_color.red = override_color.red / override_color.alpha;
      override_color.green = override_color.green / override_color.alpha;
      override_color.blue = override_color.blue / override_color.alpha;
      _bghash.OverrideColor(override_color);
      break;
    }
    case UnityshellOptions::LauncherHideMode:
      launcher_options->hide_mode = (unity::launcher::LauncherHideMode) optionGetLauncherHideMode();
      break;
    case UnityshellOptions::BacklightMode:
      launcher_options->backlight_mode = (unity::launcher::BacklightMode) optionGetBacklightMode();
      break;
    case UnityshellOptions::LaunchAnimation:
      launcher_options->launch_animation = (unity::launcher::LaunchAnimation) optionGetLaunchAnimation();
      break;
    case UnityshellOptions::UrgentAnimation:
      launcher_options->urgent_animation = (unity::launcher::UrgentAnimation) optionGetUrgentAnimation();
      break;
    case UnityshellOptions::PanelOpacity:
      panel_controller_->SetOpacity(optionGetPanelOpacity());
      break;
    case UnityshellOptions::PanelOpacityMaximizedToggle:
      panel_controller_->SetOpacityMaximizedToggle(optionGetPanelOpacityMaximizedToggle());
      break;
    case UnityshellOptions::MenusFadein:
    case UnityshellOptions::MenusFadeout:
    case UnityshellOptions::MenusDiscoveryFadein:
    case UnityshellOptions::MenusDiscoveryFadeout:
    case UnityshellOptions::MenusDiscoveryDuration:
      panel_controller_->SetMenuShowTimings(optionGetMenusFadein(),
                                            optionGetMenusFadeout(),
                                            optionGetMenusDiscoveryDuration(),
                                            optionGetMenusDiscoveryFadein(),
                                            optionGetMenusDiscoveryFadeout());
      break;
    case UnityshellOptions::LauncherOpacity:
      launcher_options->background_alpha = optionGetLauncherOpacity();
      break;
    case UnityshellOptions::IconSize:
    {
      CompPlugin         *p = CompPlugin::find ("expo");

      launcher_options->icon_size = optionGetIconSize();
      launcher_options->tile_size = optionGetIconSize() + 6;

      dash_controller_->launcher_width = optionGetIconSize() + 18;

      if (p)
      {
        CompOption::Vector &opts = p->vTable->getOptions ();

        for (CompOption &o : opts)
        {
          if (o.name () == std::string ("x_offset"))
          {
            CompOption::Value v;
            v.set (static_cast <int> (optionGetIconSize() + 18));

            screen->setOptionForPlugin (p->vTable->name ().c_str (), o.name ().c_str (), v);
            break;
          }
        }
      }
      break;
    }
    case UnityshellOptions::AutohideAnimation:
      launcher_options->auto_hide_animation = (unity::launcher::AutoHideAnimation) optionGetAutohideAnimation();
      break;
    case UnityshellOptions::DashBlurExperimental:
      BackgroundEffectHelper::blur_type = (unity::BlurType)optionGetDashBlurExperimental();
      break;
    case UnityshellOptions::AutomaximizeValue:
      PluginAdapter::Default()->SetCoverageAreaBeforeAutomaximize(optionGetAutomaximizeValue() / 100.0f);
      break;
    case UnityshellOptions::DevicesOption:
      unity::DevicesSettings::GetDefault().SetDevicesOption((unity::DevicesSettings::DevicesOption) optionGetDevicesOption());
      break;
    case UnityshellOptions::AltTabTimeout:
      switcher_controller_->detail_on_timeout = optionGetAltTabTimeout();
    case UnityshellOptions::AltTabBiasViewport:
      PluginAdapter::Default()->bias_active_to_viewport = optionGetAltTabBiasViewport();
      break;
    case UnityshellOptions::ShowMinimizedWindows:
      compiz::CompizMinimizedWindowHandler<UnityScreen, UnityWindow>::setFunctions (optionGetShowMinimizedWindows ());
      screen->enterShowDesktopModeSetEnabled (this, optionGetShowMinimizedWindows ());
      screen->leaveShowDesktopModeSetEnabled (this, optionGetShowMinimizedWindows ());
      break;
    case UnityshellOptions::ShortcutOverlay:
      enable_shortcut_overlay_ = optionGetShortcutOverlay();
      shortcut_controller_->SetEnabled(enable_shortcut_overlay_);
      break;
    case UnityshellOptions::ShowDesktopIcon:
      launcher_controller_->SetShowDesktopIcon(optionGetShowDesktopIcon());
      break;
    case UnityshellOptions::DecayRate:
      launcher_options->edge_decay_rate = optionGetDecayRate() * 100;
      break;
    case UnityshellOptions::OvercomePressure:
      launcher_options->edge_overcome_pressure = optionGetOvercomePressure() * 100;
      break;
    case UnityshellOptions::StopVelocity:
      launcher_options->edge_stop_velocity = optionGetStopVelocity() * 100;
      break;
    case UnityshellOptions::RevealPressure:
      launcher_options->edge_reveal_pressure = optionGetRevealPressure() * 100;
    case UnityshellOptions::EdgeResponsiveness:
      launcher_options->edge_responsiveness = optionGetEdgeResponsiveness();
      break;
    default:
      break;
  }
}

void UnityScreen::NeedsRelayout()
{
  needsRelayout = true;
}

void UnityScreen::ScheduleRelayout(guint timeout)
{
  if (relayoutSourceId == 0)
    relayoutSourceId = g_timeout_add(timeout, &UnityScreen::RelayoutTimeout, this);
}

void UnityScreen::Relayout()
{
  nux::Geometry geometry (0, 0, screen->width (), screen->height ());

  if (!needsRelayout)
    return;

#ifndef USE_GLES
  if (GL::fbo)
  {
    uScreen->_fbo = ScreenEffectFramebufferObject::Ptr (new ScreenEffectFramebufferObject (glXGetProcAddressP, geometry));
    uScreen->_fbo->onScreenSizeChanged (geometry);
  }
#endif

  UScreen *uscreen = UScreen::GetDefault();
  int primary_monitor = uscreen->GetPrimaryMonitor();
  auto geo = uscreen->GetMonitorGeometry(primary_monitor);

  primary_monitor_ = nux::Geometry(geo.x, geo.y, geo.width, geo.height);
  wt->SetWindowSize(geo.width, geo.height);

  LOG_DEBUG(logger) << "Setting to primary screen rect:"
                    << " x=" << primary_monitor_().x
                    << " y=" << primary_monitor_().y
                    << " w=" << primary_monitor_().width
                    << " h=" << primary_monitor_().height;

  needsRelayout = false;
}

gboolean UnityScreen::RelayoutTimeout(gpointer data)
{
  UnityScreen* uScr = reinterpret_cast<UnityScreen*>(data);

  uScr->NeedsRelayout ();
  uScr->Relayout();
  uScr->relayoutSourceId = 0;

  uScr->cScreen->damageScreen();

  return FALSE;
}

/* Handle changes in the number of workspaces by showing the switcher
 * or not showing the switcher */
bool UnityScreen::setOptionForPlugin(const char* plugin, const char* name,
                                     CompOption::Value& v)
{
  bool status = screen->setOptionForPlugin(plugin, name, v);
  if (status)
  {
    if (strcmp(plugin, "core") == 0 && strcmp(name, "hsize") == 0)
    {
      launcher_controller_->UpdateNumWorkspaces(screen->vpSize().width() * screen->vpSize().height());
    }
  }
  return status;
}

void UnityScreen::outputChangeNotify()
{
  screen->outputChangeNotify ();

  ScheduleRelayout(500);
}

void UnityScreen::OnDashRealized ()
{
  /* stack any windows named "onboard" above us */
  for (CompWindow *w : screen->windows ())
  {
    if (w->resName() == "onboard")
    {
      Window xid = dash_controller_->window()->GetInputWindowId();
      XSetTransientForHint (screen->dpy(), w->id(), xid);
      w->raise ();
    }
  }
}

/* Start up the launcher */
void UnityScreen::initLauncher()
{
  Timer timer;
  launcher_controller_.reset(new launcher::Controller(screen->dpy()));
  AddChild(launcher_controller_.get());

  switcher_controller_.reset(new switcher::Controller());
  AddChild(switcher_controller_.get());

  LOG_INFO(logger) << "initLauncher-Launcher " << timer.ElapsedSeconds() << "s";

  /* Setup panel */
  timer.Reset();
  panel_controller_.reset(new panel::Controller());
  AddChild(panel_controller_.get());
  panel_controller_->SetMenuShowTimings(optionGetMenusFadein(),
                                        optionGetMenusFadeout(),
                                        optionGetMenusDiscoveryDuration(),
                                        optionGetMenusDiscoveryFadein(),
                                        optionGetMenusDiscoveryFadeout());
  LOG_INFO(logger) << "initLauncher-Panel " << timer.ElapsedSeconds() << "s";

  /* Setup Places */
  dash_controller_.reset(new dash::Controller());
  dash_controller_->on_realize.connect(sigc::mem_fun(this, &UnityScreen::OnDashRealized));
  
  // Setup Shortcut Hint
  InitHints();
  shortcut_controller_.reset(new shortcut::Controller(hints_));

  AddChild(dash_controller_.get());

  ScheduleRelayout(0);
}

void UnityScreen::InitHints()
{
  // TODO move category text into a vector...
  
  // Launcher...
  std::string const launcher = _("Launcher");
  
  hints_.push_back(new shortcut::Hint(launcher, "", _(" (Press)"), _("Open Launcher, displays shortcuts."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher" ));
  hints_.push_back(new shortcut::Hint(launcher, "", "", _("Open Launcher keyboard navigation mode."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "keyboard_focus"));
  hints_.push_back(new shortcut::Hint(launcher, "", "", _("Switch applications via Launcher."), shortcut::HARDCODED_OPTION, "Super + Tab"));
  hints_.push_back(new shortcut::Hint(launcher, "", _(" + 1 to 9"), _("Same as clicking on a Launcher icon."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher"));
  hints_.push_back(new shortcut::Hint(launcher, "", _(" + Shift + 1 to 9"), _("Open new window of the app."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher"));
  hints_.push_back(new shortcut::Hint(launcher, "", " + T", _("Open the Trash."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher")); 

  // Dash...
  std::string const dash = _("Dash");

  hints_.push_back(new shortcut::Hint(dash, "", _(" (Tap)"), _("Open the Dash Home."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher"));
  hints_.push_back(new shortcut::Hint(dash, "", " + A", _("Open the Dash App Lens."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher"));
  hints_.push_back(new shortcut::Hint(dash, "", " + F", _("Open the Dash Files Lens."), shortcut::COMPIZ_KEY_OPTION,"unityshell", "show_launcher"));
  hints_.push_back(new shortcut::Hint(dash, "", " + M", _("Open the Dash Music Lens."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher"));
  hints_.push_back(new shortcut::Hint(dash, "", "", _("Switches between Lenses."), shortcut::HARDCODED_OPTION, "Ctrl + Tab"));
  hints_.push_back(new shortcut::Hint(dash, "", "", _("Moves the focus."), shortcut::HARDCODED_OPTION, _("Cursor Keys")));
  hints_.push_back(new shortcut::Hint(dash, "", "", _("Open currently focused item."), shortcut::HARDCODED_OPTION, _("Enter & Return")));
  hints_.push_back(new shortcut::Hint(dash, "", "", _("'Run Command' mode."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "execute_command"));
 
  // Top Bar
  std::string const topbar = _("Top Bar");
  
  hints_.push_back(new shortcut::Hint(topbar, "", "", _("Reveals application menu."), shortcut::HARDCODED_OPTION, "Alt"));
  hints_.push_back(new shortcut::Hint(topbar, "", "", _("Opens the indicator menu."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "panel_first_menu"));
  hints_.push_back(new shortcut::Hint(topbar, "", "", _("Moves focus between indicators."), shortcut::HARDCODED_OPTION, _("Cursor Left or Right")));

  // Switching
  std::string const switching = _("Switching");
  
  hints_.push_back(new shortcut::Hint(switching, "", "", _("Switch between applications."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "alt_tab_forward"));
  hints_.push_back(new shortcut::Hint(switching, "", "", _("Switch windows of current application."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "alt_tab_next_window"));
  hints_.push_back(new shortcut::Hint(switching, "", "", _("Moves the focus."), shortcut::HARDCODED_OPTION, _("Cursor Left or Right")));

  // Workspaces
  std::string const workspaces = _("Workspaces");
  hints_.push_back(new shortcut::Hint(workspaces, "", "", _("Spread workspaces."), shortcut::COMPIZ_KEY_OPTION, "expo", "expo_key"));
  hints_.push_back(new shortcut::Hint(workspaces, "", "", _("Switch workspaces."), shortcut::HARDCODED_OPTION, _("Super + Cursor Keys")));
  hints_.push_back(new shortcut::Hint(workspaces, "", " or Right", _("Move focused window to different workspace."), shortcut::HARDCODED_OPTION, _("Super + Alt + Cursor Keys")));

  // Windows
  std::string const windows = _("Windows");
  hints_.push_back(new shortcut::Hint(windows, "", "", _("Spreads all windows in the current workspace."), shortcut::COMPIZ_KEY_OPTION, "scale", "initiate_all_key"));
  hints_.push_back(new shortcut::Hint(windows, "", "", _("Minimises all windows."), shortcut::COMPIZ_KEY_OPTION, "core", "show_desktop_key"));
  hints_.push_back(new shortcut::Hint(windows, "", "", _("Maximises the current window."), shortcut::COMPIZ_KEY_OPTION, "core", "maximize_window_key"));
  hints_.push_back(new shortcut::Hint(windows, "", "", _("Restores or minimises current window."), shortcut::COMPIZ_KEY_OPTION, "core", "unmaximize_window_key"));
  hints_.push_back(new shortcut::Hint(windows, "", " or Right", _("Semi-maximises current window."), shortcut::COMPIZ_KEY_OPTION, "grid", "put_left_key"));
  hints_.push_back(new shortcut::Hint(windows, "", "", _("Closes current window."), shortcut::COMPIZ_KEY_OPTION, "core", "close_window_key"));
  hints_.push_back(new shortcut::Hint(windows, "", "", _("Opens window accessibility menu."), shortcut::HARDCODED_OPTION, "Alt + Space"));
  hints_.push_back(new shortcut::Hint(windows, "", "", _("Places window in corresponding positions."), shortcut::HARDCODED_OPTION, "Ctrl + Alt + Num"));
  hints_.push_back(new shortcut::Hint(windows, "", " Drag", _("Move window."), shortcut::COMPIZ_MOUSE_OPTION, "move", "initiate_button"));
  hints_.push_back(new shortcut::Hint(windows, "", " Drag", _("Resize window."), shortcut::COMPIZ_MOUSE_OPTION, "resize", "initiate_button"));
}

/* Window init */
UnityWindow::UnityWindow(CompWindow* window)
  : BaseSwitchWindow (dynamic_cast<BaseSwitchScreen *> (UnityScreen::get (screen)), window)
  , PluginClassHandler<UnityWindow, CompWindow>(window)
  , window(window)
  , gWindow(GLWindow::get(window))
  , mMinimizeHandler(nullptr)
  , mShowdesktopHandler(nullptr)
  , focusdesktop_handle_(0)
{
  WindowInterface::setHandler(window);
  GLWindowInterface::setHandler(gWindow);

  if (UnityScreen::get (screen)->optionGetShowMinimizedWindows () &&
      window->mapNum ())
  {
    bool wasMinimized = window->minimized ();
    if (wasMinimized)
      window->unminimize ();
    window->minimizeSetEnabled (this, true);
    window->unminimizeSetEnabled (this, true);
    window->minimizedSetEnabled (this, true);

    if (wasMinimized)
      window->minimize ();
  }
  else
  {
    window->minimizeSetEnabled (this, false);
    window->unminimizeSetEnabled (this, false);
    window->minimizedSetEnabled (this, false);
  }

  if (window->state () & CompWindowStateFullscreenMask)
    UnityScreen::get (screen)->fullscreen_windows_.push_back(window);

  /* We might be starting up so make sure that
   * we don't deref the dashcontroller that doesnt
   * exist */
  dash::Controller::Ptr dp = UnityScreen::get(screen)->dash_controller_;

  if (dp)
  {
    nux::BaseWindow* w = dp->window ();

    if (w)
    {
      if (window->resName() == "onboard")
      {
        Window xid = dp->window()->GetInputWindowId();
        XSetTransientForHint (screen->dpy(), window->id(), xid);
      }
    }
  }
}

UnityWindow::~UnityWindow()
{
  UnityScreen* us = UnityScreen::get(screen);
  if (us->newFocusedWindow && (UnityWindow::get(us->newFocusedWindow) == this))
    us->newFocusedWindow = NULL;

  UnityShowdesktopHandler::animating_windows.remove (window);

  if (mMinimizeHandler)
  {
    unminimize ();
    window->focusSetEnabled (this, false);
    window->minimizeSetEnabled (this, false);
    window->unminimizeSetEnabled (this, false);
    window->minimizedSetEnabled (this, false);
    window->minimize ();

    delete mMinimizeHandler;
    mMinimizeHandler = nullptr;
  }

  if (mShowdesktopHandler)
    delete mShowdesktopHandler;
    
  if (focusdesktop_handle_)
    g_source_remove(focusdesktop_handle_);

  if (window->state () & CompWindowStateFullscreenMask)
    UnityScreen::get (screen)->fullscreen_windows_.remove(window);

  PluginAdapter::Default ()->OnWindowClosed (window);
}

/* vtable init */
bool UnityPluginVTable::init()
{
  if (!CompPlugin::checkPluginABI("core", CORE_ABIVERSION))
    return false;
  if (!CompPlugin::checkPluginABI("composite", COMPIZ_COMPOSITE_ABI))
    return false;
  if (!CompPlugin::checkPluginABI("opengl", COMPIZ_OPENGL_ABI))
    return false;

  return true;
}


namespace
{

void configure_logging()
{
  // The default behaviour of the logging infrastructure is to send all output
  // to std::cout for warning or above.

  // TODO: write a file output handler that keeps track of backups.
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));
  g_log_set_default_handler(capture_g_log_calls, NULL);
}

/* Checks whether an extension is supported by the GLX or OpenGL implementation
 * given the extension name and the list of supported extensions. */
gboolean is_extension_supported(const gchar* extensions, const gchar* extension)
{
  if (extensions != NULL && extension != NULL)
  {
    const gsize len = strlen(extension);
    gchar* p = (gchar*) extensions;
    gchar* end = p + strlen(p);

    while (p < end)
    {
      const gsize size = strcspn(p, " ");
      if (len == size && strncmp(extension, p, size) == 0)
        return TRUE;
      p += size + 1;
    }
  }

  return FALSE;
}

/* Gets the OpenGL version as a floating-point number given the string. */
gfloat get_opengl_version_f32(const gchar* version_string)
{
  gfloat version = 0.0f;
  gint32 i;

  for (i = 0; isdigit(version_string[i]); i++)
    version = version * 10.0f + (version_string[i] - 48);

  if ((version_string[i] == '.' || version_string[i] == ',') &&
      isdigit(version_string[i + 1]))
  {
    version = version * 10.0f + (version_string[i + 1] - 48);
    return (version + 0.1f) * 0.1f;
  }
  else
    return 0.0f;
}

nux::logging::Level glog_level_to_nux(GLogLevelFlags log_level)
{
  // For some weird reason, ERROR is more critical than CRITICAL in gnome.
  if (log_level & G_LOG_LEVEL_ERROR)
    return nux::logging::Critical;
  if (log_level & G_LOG_LEVEL_CRITICAL)
    return nux::logging::Error;
  if (log_level & G_LOG_LEVEL_WARNING)
    return nux::logging::Warning;
  if (log_level & G_LOG_LEVEL_MESSAGE ||
      log_level & G_LOG_LEVEL_INFO)
    return nux::logging::Info;
  // default to debug.
  return nux::logging::Debug;
}

void capture_g_log_calls(const gchar* log_domain,
                         GLogLevelFlags log_level,
                         const gchar* message,
                         gpointer user_data)
{
  // If nothing else, all log messages from unity should be identified as such
  std::string module("unity");
  if (log_domain)
  {
    module += std::string(".") + log_domain;
  }
  nux::logging::Logger logger(module);
  nux::logging::Level level = glog_level_to_nux(log_level);
  if (logger.GetEffectiveLogLevel() >= level)
  {
    nux::logging::LogStream(level, logger.module(), "<unknown>", 0).stream()
        << message;
  }
}

} // anonymous namespace
} // namespace unity
