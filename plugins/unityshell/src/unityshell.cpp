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

#include <UnityCore/ScopeProxyInterface.h>
#include <UnityCore/GnomeSessionManager.h>
#include <UnityCore/Variant.h>

#include "BaseWindowRaiserImp.h"
#include "IconRenderer.h"
#include "Launcher.h"
#include "LauncherIcon.h"
#include "LauncherController.h"
#include "SwitcherController.h"
#include "SwitcherView.h"
#include "PanelView.h"
#include "PluginAdapter.h"
#include "QuicklistManager.h"
#include "StartupNotifyService.h"
#include "Timer.h"
#include "XKeyboardUtil.h"
#include "glow_texture.h"
#include "unityshell.h"
#include "BackgroundEffectHelper.h"
#include "UnityGestureBroker.h"
#include "launcher/EdgeBarrierController.h"
#include "launcher/XdndCollectionWindowImp.h"
#include "launcher/XdndManagerImp.h"
#include "launcher/XdndStartStopNotifierImp.h"
#include "CompizShortcutModeller.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <libnotify/notify.h>
#include <cairo-xlib-xrender.h>

#include <text/text.h>

#include <sstream>
#include <memory>

#include <core/atoms.h>

#include "unitya11y.h"

#include "UBusMessages.h"
#include "UBusWrapper.h"
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
using ui::LayoutWindow;
using util::Timer;

DECLARE_LOGGER(logger, "unity.shell.compiz");
namespace
{
UnityScreen* uScreen = 0;

void reset_glib_logging();
void configure_logging();
void capture_g_log_calls(const gchar* log_domain,
                         GLogLevelFlags log_level,
                         const gchar* message,
                         gpointer user_data);
gboolean is_extension_supported(const gchar* extensions, const gchar* extension);
gfloat get_opengl_version_f32(const gchar* version_string);

namespace local
{
// Tap duration in milliseconds.
const int ALT_TAP_DURATION = 250;
const unsigned int SCROLL_DOWN_BUTTON = 6;
const unsigned int SCROLL_UP_BUTTON = 7;

const std::string RELAYOUT_TIMEOUT = "relayout-timeout";
} // namespace local

namespace win
{
namespace decoration
{
const unsigned CLOSE_SIZE = 19;
const unsigned ITEMS_PADDING = 5;
const unsigned RADIUS = 8;
const unsigned GLOW = 5;
const nux::Color GLOW_COLOR(221, 72, 20);
} // decoration namespace
} // win namespace

} // anon namespace

UnityScreen::UnityScreen(CompScreen* screen)
  : BaseSwitchScreen(screen)
  , PluginClassHandler <UnityScreen, CompScreen>(screen)
  , screen(screen)
  , cScreen(CompositeScreen::get(screen))
  , gScreen(GLScreen::get(screen))
  , debugger_(this)
  , needsRelayout(false)
  , super_keypressed_(false)
  , newFocusedWindow(nullptr)
  , doShellRepaint(false)
  , didShellRepaint(false)
  , allowWindowPaint(false)
  , _key_nav_mode_requested(false)
  , _last_output(nullptr)
  , grab_index_ (0)
  , painting_tray_ (false)
  , last_scroll_event_(0)
  , hud_keypress_time_(0)
  , first_menu_keypress_time_(0)
  , paint_panel_under_dash_(false)
  , scale_just_activated_(false)
  , big_tick_(0)
  , screen_introspection_(screen)
  , is_desktop_active_(false)
{
  Timer timer;
#ifndef USE_GLES
  gfloat version;
  gchar* extensions;
#endif
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

    //In case of software rendering then enable lowgfx mode.
  std::string renderer = ANSI_TO_TCHAR(NUX_REINTERPRET_CAST(const char *, glGetString(GL_RENDERER)));

  if (renderer.find("Software Rasterizer") != std::string::npos ||
      renderer.find("Mesa X11") != std::string::npos ||
      renderer.find("LLVM") != std::string::npos ||
      renderer.find("on softpipe") != std::string::npos ||
      (getenv("UNITY_LOW_GFX_MODE") != NULL && atoi(getenv("UNITY_LOW_GFX_MODE")) == 1))
    {
      Settings::Instance().SetLowGfxMode(true);
    }
#endif

  if (!failed)
  {
     notify_init("unityshell");

     unity_a11y_preset_environment();

     XSetErrorHandler(old_handler);

     /* Wrap compiz interfaces */
     ScreenInterface::setHandler(screen);
     CompositeScreenInterface::setHandler(cScreen);
     GLScreenInterface::setHandler(gScreen);

     PluginAdapter::Initialize(screen);
     AddChild(&WindowManager::Default());

     StartupNotifyService::Default()->SetSnDisplay(screen->snDisplay(), screen->screenNum());

     nux::NuxInitialize(0);
#ifndef USE_GLES
     wt.reset(nux::CreateFromForeignWindow(cScreen->output(),
                                           glXGetCurrentContext(),
                                           &UnityScreen::initUnity,
                                           this));
#else
     wt.reset(nux::CreateFromForeignWindow(cScreen->output(),
                                           eglGetCurrentContext(),
                                           &UnityScreen::initUnity,
                                           this));
#endif

    tick_source_.reset(new na::TickSource);
    animation_controller_.reset(new na::AnimationController(*tick_source_));

     wt->RedrawRequested.connect(sigc::mem_fun(this, &UnityScreen::onRedrawRequested));

    // _bghash is a pointer. We don't want it to be created before Nux system has had a chance
    // to start. BGHash relies on animations. Nux animation system starts after the WindowThread
    // has been created.
    _bghash.reset(new BGHash());

     unity_a11y_init(wt.get());

     /* i18n init */
     bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
     bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

     wt->Run(NULL);
     uScreen = this;

     optionSetShowHudInitiate(boost::bind(&UnityScreen::ShowHudInitiate, this, _1, _2, _3));
     optionSetShowHudTerminate(boost::bind(&UnityScreen::ShowHudTerminate, this, _1, _2, _3));
     optionSetBackgroundColorNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetLauncherHideModeNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetBacklightModeNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetRevealTriggerNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
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
     optionSetShortcutOverlayNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetShowLauncherInitiate(boost::bind(&UnityScreen::showLauncherKeyInitiate, this, _1, _2, _3));
     optionSetShowLauncherTerminate(boost::bind(&UnityScreen::showLauncherKeyTerminate, this, _1, _2, _3));
     optionSetKeyboardFocusInitiate(boost::bind(&UnityScreen::setKeyboardFocusKeyInitiate, this, _1, _2, _3));
     //optionSetKeyboardFocusTerminate (boost::bind (&UnityScreen::setKeyboardFocusKeyTerminate, this, _1, _2, _3));
     optionSetExecuteCommandInitiate(boost::bind(&UnityScreen::executeCommand, this, _1, _2, _3));
     optionSetShowDesktopKeyInitiate(boost::bind(&UnityScreen::showDesktopKeyInitiate, this, _1, _2, _3));
     optionSetPanelFirstMenuInitiate(boost::bind(&UnityScreen::showPanelFirstMenuKeyInitiate, this, _1, _2, _3));
     optionSetPanelFirstMenuTerminate(boost::bind(&UnityScreen::showPanelFirstMenuKeyTerminate, this, _1, _2, _3));
     optionSetAutomaximizeValueNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetDashTapDurationNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetAltTabTimeoutNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetAltTabBiasViewportNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetDisableShowDesktopNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetDisableMouseNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));

     optionSetAltTabForwardAllInitiate(boost::bind(&UnityScreen::altTabForwardAllInitiate, this, _1, _2, _3));
     optionSetAltTabForwardInitiate(boost::bind(&UnityScreen::altTabForwardInitiate, this, _1, _2, _3));
     optionSetAltTabForwardTerminate(boost::bind(&UnityScreen::altTabTerminateCommon, this, _1, _2, _3));
     optionSetAltTabForwardAllTerminate(boost::bind(&UnityScreen::altTabTerminateCommon, this, _1, _2, _3));
     optionSetAltTabPrevAllInitiate(boost::bind(&UnityScreen::altTabPrevAllInitiate, this, _1, _2, _3));
     optionSetAltTabPrevInitiate(boost::bind(&UnityScreen::altTabPrevInitiate, this, _1, _2, _3));

     optionSetAltTabDetailStartInitiate(boost::bind(&UnityScreen::altTabDetailStart, this, _1, _2, _3));
     optionSetAltTabDetailStopInitiate(boost::bind(&UnityScreen::altTabDetailStop, this, _1, _2, _3));

     optionSetAltTabNextWindowInitiate(boost::bind(&UnityScreen::altTabNextWindowInitiate, this, _1, _2, _3));
     optionSetAltTabNextWindowTerminate(boost::bind(&UnityScreen::altTabTerminateCommon, this, _1, _2, _3));

     optionSetAltTabPrevWindowInitiate(boost::bind(&UnityScreen::altTabPrevWindowInitiate, this, _1, _2, _3));

     optionSetAltTabLeftInitiate(boost::bind (&UnityScreen::altTabPrevInitiate, this, _1, _2, _3));
     optionSetAltTabRightInitiate([&](CompAction* action, CompAction::State state, CompOption::Vector& options) -> bool
     {
      if (switcher_controller_->Visible())
      {
        switcher_controller_->Next();
        return true;
      }
      return false;
     });

     optionSetLauncherSwitcherForwardInitiate(boost::bind(&UnityScreen::launcherSwitcherForwardInitiate, this, _1, _2, _3));
     optionSetLauncherSwitcherPrevInitiate(boost::bind(&UnityScreen::launcherSwitcherPrevInitiate, this, _1, _2, _3));
     optionSetLauncherSwitcherForwardTerminate(boost::bind(&UnityScreen::launcherSwitcherTerminate, this, _1, _2, _3));

     optionSetStopVelocityNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetRevealPressureNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetEdgeResponsivenessNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetOvercomePressureNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetDecayRateNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetShowMinimizedWindowsNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetEdgePassedDisabledMsNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));

     optionSetNumLaunchersNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetLauncherCaptureMouseNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));

     ubus_manager_.RegisterInterest(UBUS_LAUNCHER_START_KEY_NAV,
                   sigc::mem_fun(this, &UnityScreen::OnLauncherStartKeyNav));

     ubus_manager_.RegisterInterest(UBUS_LAUNCHER_START_KEY_SWITCHER,
                   sigc::mem_fun(this, &UnityScreen::OnLauncherStartKeyNav));

     ubus_manager_.RegisterInterest(UBUS_LAUNCHER_END_KEY_NAV,
                   sigc::mem_fun(this, &UnityScreen::OnLauncherEndKeyNav));

     ubus_manager_.RegisterInterest(UBUS_LAUNCHER_END_KEY_SWITCHER,
                   sigc::mem_fun(this, &UnityScreen::OnLauncherEndKeyNav));

     ubus_manager_.RegisterInterest(UBUS_SWITCHER_START,
                   sigc::mem_fun(this, &UnityScreen::OnSwitcherStart));

     ubus_manager_.RegisterInterest(UBUS_SWITCHER_END,
                   sigc::mem_fun(this, &UnityScreen::OnSwitcherEnd));

     auto init_plugins_cb = sigc::mem_fun(this, &UnityScreen::initPluginActions);
     sources_.Add(std::make_shared<glib::Idle>(init_plugins_cb, glib::Source::Priority::DEFAULT));

     InitGesturesSupport();

     CompString name(PKGDATADIR"/panel-shadow.png");
     CompString pname("unityshell");
     CompSize size(1, 20);
     _shadow_texture = GLTexture::readImageToTexture(name, pname, size);

     BackgroundEffectHelper::updates_enabled = true;

     ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN, [&](GVariant * data)
     {
       unity::glib::String overlay_identity;
       gboolean can_maximise = FALSE;
       gint32 overlay_monitor = 0;
       int width, height;
       g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING,
                    &overlay_identity, &can_maximise, &overlay_monitor, &width, &height);

       overlay_monitor_ = overlay_monitor;

       RaiseInputWindows();
     });

    Display* display = gdk_x11_display_get_xdisplay(gdk_display_get_default());;
    XSelectInput(display, GDK_ROOT_WINDOW(), PropertyChangeMask);
    LOG_INFO(logger) << "UnityScreen constructed: " << timer.ElapsedSeconds() << "s";

    panel::Style::Instance().changed.connect(sigc::mem_fun(this, &UnityScreen::OnPanelStyleChanged));

    minimize_speed_controller_.DurationChanged.connect(
        sigc::mem_fun(this, &UnityScreen::OnMinimizeDurationChanged)
    );

    WindowManager& wm = WindowManager::Default();
    wm.initiate_spread.connect(sigc::mem_fun(this, &UnityScreen::OnInitiateSpread));
    wm.terminate_spread.connect(sigc::mem_fun(this, &UnityScreen::OnTerminateSpread));

    AddChild(&screen_introspection_);
  }
}

UnityScreen::~UnityScreen()
{
  notify_uninit();

  unity_a11y_finalize();
  QuicklistManager::Destroy();

  reset_glib_logging();
}

void UnityScreen::initAltTabNextWindow()
{
  Display* display = screen->dpy();
  KeySym tab_keysym = XStringToKeysym("Tab");
  KeySym above_tab_keysym = keyboard::get_key_above_key_symbol(display, tab_keysym);

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
    LOG_WARN(logger) << "Could not find key above tab!";
  }

}

void UnityScreen::OnInitiateSpread()
{
  UnityWindow::SetupSharedTextures();

  for (auto const& swin : ScaleScreen::get(screen)->getWindows())
    UnityWindow::get(swin->window)->OnInitiateSpread();
}

void UnityScreen::OnTerminateSpread()
{
  for (auto const& swin : ScaleScreen::get(screen)->getWindows())
    UnityWindow::get(swin->window)->OnTerminateSpread();

  UnityWindow::CleanupSharedTextures();
}

void UnityScreen::EnsureSuperKeybindings()
{
  for (auto action : _shortcut_actions)
    screen->removeAction(action.get());

  _shortcut_actions.clear ();

  for (auto shortcut : launcher_controller_->GetAllShortcuts())
  {
    CreateSuperNewAction(shortcut, impl::ActionModifiers::NONE);
    CreateSuperNewAction(shortcut, impl::ActionModifiers::USE_NUMPAD);
    CreateSuperNewAction(shortcut, impl::ActionModifiers::USE_SHIFT);
  }

  for (auto shortcut : dash_controller_->GetAllShortcuts())
    CreateSuperNewAction(shortcut, impl::ActionModifiers::NONE);
}

void UnityScreen::CreateSuperNewAction(char shortcut, impl::ActionModifiers flag)
{
    CompActionPtr action(new CompAction());
    const std::string key(optionGetShowLauncher().keyToString());

    CompAction::KeyBinding binding;
    binding.fromString(impl::CreateActionString(key, shortcut, flag));

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

  glPopAttrib();

  glDepthRange(0, 1);
#else
  glDepthRangef(0, 1);
  gScreen->resetRasterPos();
#endif

  glDisable(GL_SCISSOR_TEST);
}

void UnityScreen::setPanelShadowMatrix(GLMatrix const& matrix)
{
  panel_shadow_matrix_ = matrix;
}

void UnityScreen::paintPanelShadow(CompRegion const& clip)
{
  // You have no shadow texture. But how?
  if (_shadow_texture.empty() || !_shadow_texture[0])
    return;

  if (panel_controller_->opacity() == 0.0f)
    return;

  if (sources_.GetSource(local::RELAYOUT_TIMEOUT))
    return;

  if (WindowManager::Default().IsExpoActive())
    return;

  CompOutput* output = _last_output;

  if (fullscreenRegion.contains(*output))
    return;

  if (launcher_controller_->IsOverlayOpen())
  {
    auto const& uscreen = UScreen::GetDefault();
    if (uscreen->GetMonitorAtPosition(output->x(), output->y()) == overlay_monitor_)
      return;
  }

  int shadowX = output->x();
  int shadowY = output->y() + panel_style_.panel_height;
  int shadowWidth = output->width();
  int shadowHeight = _shadow_texture[0]->height();
  CompRect shadowRect(shadowX, shadowY, shadowWidth, shadowHeight);

  CompRegion redraw(clip);
  redraw &= shadowRect;
  redraw -= panelShadowPainted;

  if (redraw.isEmpty())
    return;

  panelShadowPainted |= redraw;

  for (auto const& r : redraw.rects())
  {
    for (GLTexture* tex : _shadow_texture)
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

      // Sub-rectangle of the shadow needing redrawing:
      float x1 = r.x1();
      float y1 = r.y1();
      float x2 = r.x2();
      float y2 = r.y2();

      // Texture coordinates of the above rectangle:
      float tx1 = (x1 - shadowX) / shadowWidth;
      float ty1 = (y1 - shadowY) / shadowHeight;
      float tx2 = (x2 - shadowX) / shadowWidth;
      float ty2 = (y2 - shadowY) / shadowHeight;

      vertexData = {
        x1, y1, 0,
        x1, y2, 0,
        x2, y1, 0,
        x2, y2, 0,
      };

      textureData = {
        tx1, ty1,
        tx1, ty2,
        tx2, ty1,
        tx2, ty2,
      };

      streamingBuffer->begin(GL_TRIANGLE_STRIP);

      streamingBuffer->addColors(1, &colorData[0]);
      streamingBuffer->addVertices(4, &vertexData[0]);
      streamingBuffer->addTexCoords(0, 4, &textureData[0]);

      streamingBuffer->end();
      streamingBuffer->render(panel_shadow_matrix_);

      tex->disable();
      if (!wasBlend)
        glDisable(GL_BLEND);
    }
  }
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

void UnityScreen::OnPanelStyleChanged()
{
  // Reload the windows themed textures
  UnityWindow::CleanupSharedTextures();

  if (!fake_decorated_windows_.empty())
  {
    UnityWindow::SetupSharedTextures();

    for (UnityWindow* uwin : fake_decorated_windows_)
      uwin->CleanupCachedTextures();
  }
}

void UnityScreen::paintDisplay()
{
  CompOutput *output = _last_output;

  DrawPanelUnderDash();

  if (BackgroundEffectHelper::HasDirtyHelpers())
  {
    auto gpu_device = nux::GetGraphicsDisplay()->GetGpuDevice();
    auto graphics_engine = nux::GetGraphicsDisplay()->GetGraphicsEngine();

    nux::ObjectPtr<nux::IOpenGLTexture2D> bg_texture =
      graphics_engine->CreateTextureFromBackBuffer(0, 0,
                                                   screen->width(),
                                                   screen->height());
    gpu_device->backup_texture0_ = bg_texture;
  }

  nux::Geometry outputGeo(output->x (), output->y (), output->width (), output->height ());
  BackgroundEffectHelper::monitor_rect_.Set(0, 0, screen->width(), screen->height());

  GLint fboID;
  // Nux renders to the referenceFramebuffer when it's embedded.
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fboID);
  wt->GetWindowCompositor().SetReferenceFramebuffer(fboID, outputGeo);

  nuxPrologue();
  wt->RenderInterfaceFromForeignCmd (&outputGeo);
  nuxEpilogue();

  for (Window tray_xid : panel_controller_->GetTrayXids())
  {
    if (tray_xid && !allowWindowPaint)
    {
      CompWindow *tray = screen->findWindow (tray_xid);

      if (tray)
      {
        GLMatrix oTransform;
        UnityWindow  *uTrayWindow = UnityWindow::get (tray);
        GLWindowPaintAttrib attrib (uTrayWindow->gWindow->lastPaintAttrib());
        unsigned int oldGlAddGeometryIndex = uTrayWindow->gWindow->glAddGeometryGetCurrentIndex ();
        unsigned int oldGlDrawIndex = uTrayWindow->gWindow->glDrawGetCurrentIndex ();

        attrib.opacity = OPAQUE;
        attrib.brightness = BRIGHT;
        attrib.saturation = COLOR;

        oTransform.toScreenSpace (output, -DEFAULT_Z_CAMERA);

        painting_tray_ = true;

        /* force the use of the core functions */
        uTrayWindow->gWindow->glDrawSetCurrentIndex (MAXSHORT);
        uTrayWindow->gWindow->glAddGeometrySetCurrentIndex ( MAXSHORT);
        uTrayWindow->gWindow->glDraw (oTransform, attrib, infiniteRegion,
               PAINT_WINDOW_TRANSFORMED_MASK |
               PAINT_WINDOW_BLEND_MASK |
               PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK);
        uTrayWindow->gWindow->glAddGeometrySetCurrentIndex (oldGlAddGeometryIndex);
        uTrayWindow->gWindow->glDrawSetCurrentIndex (oldGlDrawIndex);
        painting_tray_ = false;
      }
    }
  }

  if (switcher_controller_->Opacity() > 0.0f)
  {
    LayoutWindow::Vector const& targets = switcher_controller_->ExternalRenderTargets();

    for (LayoutWindow::Ptr const& target : targets)
    {
      if (CompWindow* window = screen->findWindow(target->xid))
      {
        UnityWindow *unity_window = UnityWindow::get(window);
        double scale = target->result.width / static_cast<double>(target->geo.width);
        double parent_alpha = switcher_controller_->Opacity();
        unity_window->paintThumbnail(target->result, target->alpha, parent_alpha, scale,
                                     target->decoration_height, target->selected);
      }
    }
  }

  doShellRepaint = false;
  didShellRepaint = true;
}

void UnityScreen::DrawPanelUnderDash()
{
  if (!paint_panel_under_dash_ || !launcher_controller_->IsOverlayOpen())
    return;

  if (_last_output->id() != screen->currentOutputDev().id())
    return;

  auto graphics_engine = nux::GetGraphicsDisplay()->GetGraphicsEngine();

  if (!graphics_engine->UsingGLSLCodePath())
    return;

  graphics_engine->ResetModelViewMatrixStack();
  graphics_engine->Push2DTranslationModelViewMatrix(0.0f, 0.0f, 0.0f);
  graphics_engine->ResetProjectionMatrix();
  graphics_engine->SetOrthographicProjectionMatrix(screen->width(), screen->height());

  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_CLAMP);
  int panel_height = panel_style_.panel_height;
  auto const& texture = panel_style_.GetBackground()->GetDeviceTexture();
  graphics_engine->QRP_GLSL_1Tex(0, 0, screen->width(), panel_height, texture, texxform, nux::color::White);
}

bool UnityScreen::forcePaintOnTop ()
{
    return !allowWindowPaint ||
      ((switcher_controller_->Visible() ||
        WindowManager::Default().IsExpoActive())
       && !fullscreen_windows_.empty () && (!(screen->grabbed () && !screen->otherGrabExist (NULL))));
}

void UnityScreen::EnableCancelAction(CancelActionTarget target, bool enabled, int modifiers)
{
  if (enabled)
  {
    /* Create a new keybinding for the Escape key and the current modifiers,
     * compiz will take of the ref-counting of the repeated actions */
    KeyCode escape = XKeysymToKeycode(screen->dpy(), XK_Escape);
    CompAction::KeyBinding binding(escape, modifiers);

    CompActionPtr &escape_action = _escape_actions[target];
    escape_action = CompActionPtr(new CompAction());
    escape_action->setKey(binding);

    screen->addAction(escape_action.get());
  }
  else if (!enabled && _escape_actions[target].get())
  {
    screen->removeAction(_escape_actions[target].get());
    _escape_actions.erase(target);
  }
}

void UnityScreen::enterShowDesktopMode ()
{
  for (CompWindow *w : screen->windows ())
  {
    CompPoint const& viewport = w->defaultViewport();
    UnityWindow *uw = UnityWindow::get (w);

    if (viewport == uScreen->screen->vp() &&
        ShowdesktopHandler::ShouldHide (static_cast <ShowdesktopHandlerWindowInterface *> (uw)))
    {
      UnityWindow::get (w)->enterShowDesktop ();
      // the animation plugin does strange things here ...
      // if this notification is sent
      // w->windowNotify (CompWindowNotifyEnterShowDesktopMode);
    }
    if (w->type() & CompWindowTypeDesktopMask)
      w->moveInputFocusTo();
  }

  PluginAdapter::Default().OnShowDesktop();

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
  if (!ShowdesktopHandler::InhibitingXid ())
  {
    for (CompWindow *cw : screen->windows ())
    {
      CompPoint const& viewport = cw->defaultViewport();

      if (viewport == uScreen->screen->vp() &&
          cw->inShowDesktopMode ())
      {
        UnityWindow::get (cw)->leaveShowDesktop ();
        // the animation plugin does strange things here ...
        // if this notification is sent
        //cw->windowNotify (CompWindowNotifyLeaveShowDesktopMode);
      }
    }

    PluginAdapter::Default().OnLeaveDesktop();

    if (w)
    {
      CompPoint const& viewport = w->defaultViewport();

      if (viewport == uScreen->screen->vp())
        screen->leaveShowDesktopMode (w);
    }
    else
    {
      screen->focusDefaultWindow();
    }
  }
  else
  {
    CompWindow *cw = screen->findWindow (ShowdesktopHandler::InhibitingXid ());
    if (cw)
    {
      if (cw->inShowDesktopMode ())
      {
        UnityWindow::get (cw)->leaveShowDesktop ();
      }
    }
  }
}

bool UnityScreen::DoesPointIntersectUnityGeos(nux::Point const& pt)
{
  auto launchers = launcher_controller_->launchers();
  for (auto launcher : launchers)
  {
    nux::Geometry hud_geo = launcher->GetAbsoluteGeometry();

    if (launcher->Hidden())
      continue;

    if (hud_geo.IsInside(pt))
    {
      return true;
    }
  }

  for (nux::Geometry &panel_geo : panel_controller_->GetGeometries ())
  {
    if (panel_geo.IsInside(pt))
    {
      return true;
    }
  }

  return false;
}

LayoutWindow::Ptr UnityScreen::GetSwitcherDetailLayoutWindow(Window window) const
{
  LayoutWindow::Vector const& targets = switcher_controller_->ExternalRenderTargets();

  for (LayoutWindow::Ptr const& target : targets)
  {
    if (target->xid == window)
      return target;
  }

  return nullptr;
}

void UnityWindow::enterShowDesktop ()
{
  if (!mShowdesktopHandler)
    mShowdesktopHandler.reset(new ShowdesktopHandler(static_cast <ShowdesktopHandlerWindowInterface *>(this),
                                                     static_cast <compiz::WindowInputRemoverLockAcquireInterface *>(this)));

  window->setShowDesktopMode (true);
  mShowdesktopHandler->FadeOut ();
}

void UnityWindow::leaveShowDesktop ()
{
  if (mShowdesktopHandler)
  {
    mShowdesktopHandler->FadeIn ();
    window->setShowDesktopMode (false);
  }
}

void UnityWindow::activate ()
{
  ShowdesktopHandler::InhibitLeaveShowdesktopMode (window->id ());
  window->activate ();
  ShowdesktopHandler::AllowLeaveShowdesktopMode (window->id ());

  PluginAdapter::Default().OnLeaveDesktop();
}

void UnityWindow::DoEnableFocus ()
{
  window->focusSetEnabled (this, true);
}

void UnityWindow::DoDisableFocus ()
{
  window->focusSetEnabled (this, false);
}

bool UnityWindow::IsOverrideRedirect ()
{
  return window->overrideRedirect ();
}

bool UnityWindow::IsManaged ()
{
  return window->managed ();
}

bool UnityWindow::IsGrabbed ()
{
  return window->grabbed ();
}

bool UnityWindow::IsDesktopOrDock ()
{
  return (window->type () & (CompWindowTypeDesktopMask | CompWindowTypeDockMask));
}

bool UnityWindow::IsSkipTaskbarOrPager ()
{
  return (window->state () & (CompWindowStateSkipTaskbarMask | CompWindowStateSkipPagerMask));
}

bool UnityWindow::IsInShowdesktopMode ()
{
  return window->inShowDesktopMode ();
}

bool UnityWindow::IsHidden ()
{
  return window->state () & CompWindowStateHiddenMask;
}

bool UnityWindow::IsShaded ()
{
  return window->shaded ();
}

bool UnityWindow::IsMinimized ()
{
  return window->minimized ();
}

void UnityWindow::DoOverrideFrameRegion (CompRegion &region)
{
  unsigned int oldUpdateFrameRegionIndex = window->updateFrameRegionGetCurrentIndex ();

  window->updateFrameRegionSetCurrentIndex (MAXSHORT);
  window->updateFrameRegion (region);
  window->updateFrameRegionSetCurrentIndex (oldUpdateFrameRegionIndex);
}

void UnityWindow::DoHide ()
{
  window->changeState (window->state () | CompWindowStateHiddenMask);
}

void UnityWindow::DoNotifyHidden ()
{
  window->windowNotify (CompWindowNotifyHide);
}

void UnityWindow::DoShow ()
{
  window->changeState (window->state () & ~(CompWindowStateHiddenMask));
}

void UnityWindow::DoNotifyShown ()
{
  window->windowNotify (CompWindowNotifyShow);
}

void UnityWindow::DoMoveFocusAway ()
{
  window->moveInputFocusToOtherWindow ();
}

ShowdesktopHandlerWindowInterface::PostPaintAction UnityWindow::DoHandleAnimations (unsigned int ms)
{
  ShowdesktopHandlerWindowInterface::PostPaintAction action = ShowdesktopHandlerWindowInterface::PostPaintAction::Wait;

  if (mShowdesktopHandler)
    action = mShowdesktopHandler->Animate (ms);

  return action;
}

void UnityWindow::DoAddDamage()
{
  cWindow->addDamage();
}

void UnityWindow::DoDeleteHandler ()
{
  mShowdesktopHandler.reset();

  window->updateFrameRegion ();
}

compiz::WindowInputRemoverLock::Ptr
UnityWindow::GetInputRemover ()
{
  if (!input_remover_.expired ())
    return input_remover_.lock ();

  compiz::WindowInputRemoverLock::Ptr
      ret (new compiz::WindowInputRemoverLock (
             new compiz::WindowInputRemover (screen->dpy (),
                                             window->id (),
                                             window->id ())));
  input_remover_ = ret;
  return ret;
}

unsigned int
UnityWindow::GetNoCoreInstanceMask ()
{
  return PAINT_WINDOW_NO_CORE_INSTANCE_MASK;
}

bool UnityWindow::handleEvent(XEvent *event)
{
  bool handled = false;

  switch(event->type)
  {
    case MotionNotify:
      if (close_icon_state_ != panel::WindowState::PRESSED)
      {
        panel::WindowState old_state = close_icon_state_;

        if (close_button_geo_.IsPointInside(event->xmotion.x_root, event->xmotion.y_root))
        {
          close_icon_state_ = panel::WindowState::PRELIGHT;
        }
        else
        {
          close_icon_state_ = panel::WindowState::NORMAL;
        }

        if (old_state != close_icon_state_)
        {
          auto const& g = close_button_geo_;
          cWindow->addDamageRect(CompRect(g.x, g.y, g.width, g.height));
        }
      }
      break;

    case ButtonPress:
      if (event->xbutton.button == Button1 &&
          close_button_geo_.IsPointInside(event->xbutton.x_root, event->xbutton.y_root))
      {
        close_icon_state_ = panel::WindowState::PRESSED;
        auto const& g = close_button_geo_;
        cWindow->addDamageRect(CompRect(g.x, g.y, g.width, g.height));
        handled = true;
      }
      else if (event->xbutton.button == Button2 &&
              (GetScaledGeometry().IsPointInside(event->xbutton.x_root, event->xbutton.y_root) ||
               GetLayoutWindowGeometry().IsPointInside(event->xbutton.x_root, event->xbutton.y_root)))
      {
        middle_clicked_ = true;
        handled = true;
      }
      break;

    case ButtonRelease:
      {
        bool was_pressed = (close_icon_state_ == panel::WindowState::PRESSED);

        if (close_icon_state_ != panel::WindowState::NORMAL)
        {
          close_icon_state_ = panel::WindowState::NORMAL;
          auto const& g = close_button_geo_;
          cWindow->addDamageRect(CompRect(g.x, g.y, g.width, g.height));
        }

        if (was_pressed)
        {
          if (close_button_geo_.IsPointInside(event->xbutton.x_root, event->xbutton.y_root))
          {
            window->close(0);
            handled = true;
          }
        }

        if (middle_clicked_)
        {
          if (event->xbutton.button == Button2 &&
             (GetScaledGeometry().IsPointInside(event->xbutton.x_root, event->xbutton.y_root) ||
              GetLayoutWindowGeometry().IsPointInside(event->xbutton.x_root, event->xbutton.y_root)))
          {
            window->close(0);
            handled = true;
          }

          middle_clicked_ = false;
        }
      }
      break;

    default:
      if (!event->xany.send_event && screen->XShape() &&
          event->type == screen->shapeEvent() + ShapeNotify)
      {
        if (mShowdesktopHandler)
        {
          mShowdesktopHandler->HandleShapeEvent();
          handled = true;
        }
      }
  }

  return handled;
}

/* called whenever we need to repaint parts of the screen */
bool UnityScreen::glPaintOutput(const GLScreenPaintAttrib& attrib,
                                const GLMatrix& transform,
                                const CompRegion& region,
                                CompOutput* output,
                                unsigned int mask)
{
  bool ret;

  /*
   * Very important!
   * Don't waste GPU and CPU rendering the shell on every frame if you don't
   * need to. Doing so on every frame causes Nux to hog the GPU and slow down
   * ALL rendering. (LP: #988079)
   */
  bool force = forcePaintOnTop();
  doShellRepaint = force ||
                   ( !region.isEmpty() &&
                     ( !wt->GetDrawList().empty() ||
                       (mask & PAINT_SCREEN_FULL_MASK)
                     )
                   );

  allowWindowPaint = true;
  _last_output = output;
  paint_panel_under_dash_ = false;

  // CompRegion has no clear() method. So this is the fastest alternative.
  fullscreenRegion = CompRegion();
  nuxRegion = CompRegion();
  windows_for_monitor_.clear();

  /* glPaintOutput is part of the opengl plugin, so we need the GLScreen base class. */
  ret = gScreen->glPaintOutput(attrib, transform, region, output, mask);

  if (doShellRepaint && !force && fullscreenRegion.contains(*output))
    doShellRepaint = false;

  if (doShellRepaint)
    paintDisplay();

  return ret;
}

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
  paintPanelShadow(region);
}

void UnityScreen::preparePaint(int ms)
{
  cScreen->preparePaint(ms);

  big_tick_ += ms*1000;
  tick_source_->tick(big_tick_);

  for (ShowdesktopHandlerWindowInterface *wi : ShowdesktopHandler::animating_windows)
    wi->HandleAnimations (ms);

  didShellRepaint = false;
  panelShadowPainted = CompRegion();
  firstWindowAboveShell = NULL;

  compizDamageNux(cScreen->currentDamage());
}

void UnityScreen::donePaint()
{
  /*
   * It's only safe to clear the draw list if drawing actually occurred
   * (i.e. the shell was not obscured behind a fullscreen window).
   * If you clear the draw list and drawing has not occured then you'd be
   * left with all your views thinking they're queued for drawing still and
   * would refuse to redraw when you return from fullscreen.
   * I think this is a Nux bug. ClearDrawList should ideally also mark all
   * the queued views as draw_cmd_queued_=false.
   */
  if (didShellRepaint)
    wt->ClearDrawList();

  if (animation_controller_->HasRunningAnimations())
    nuxDamageCompiz();

  std::list <ShowdesktopHandlerWindowInterface *> remove_windows;

  for (ShowdesktopHandlerWindowInterface *wi : ShowdesktopHandler::animating_windows)
  {
    ShowdesktopHandlerWindowInterface::PostPaintAction action = wi->HandleAnimations (0);
    if (action == ShowdesktopHandlerWindowInterface::PostPaintAction::Remove)
      remove_windows.push_back(wi);
    else if (action == ShowdesktopHandlerWindowInterface::PostPaintAction::Damage)
      wi->AddDamage ();
  }

  for (ShowdesktopHandlerWindowInterface *wi : remove_windows)
  {
    wi->DeleteHandler ();
    ShowdesktopHandler::animating_windows.remove (wi);
  }

  cScreen->donePaint ();
}

void UnityScreen::compizDamageNux(CompRegion const& damage)
{
  if (!launcher_controller_)
    return;

  /*
   * Prioritise user interaction over active blur updates. So the general
   * slowness of the active blur doesn't affect the UI interaction performance.
   *
   * Also, BackgroundEffectHelper::ProcessDamage() is causing a feedback loop
   * while the dash is open. Calling it results in the NEXT frame (and the
   * current one?) to get some damage. This GetDrawList().empty() check avoids
   * that feedback loop and allows us to idle correctly.
   */
  if (wt->GetDrawList().empty() && BackgroundEffectHelper::HasDamageableHelpers())
  {
    CompRect::vector const& rects(damage.rects());
    for (CompRect const& r : rects)
    {
      nux::Geometry geo(r.x(), r.y(), r.width(), r.height());
      BackgroundEffectHelper::ProcessDamage(geo);
    }
  }

  auto const& launchers = launcher_controller_->launchers();
  for (auto const& launcher : launchers)
  {
    if (!launcher->Hidden())
    {
      nux::Geometry const& geo = launcher->GetAbsoluteGeometry();
      CompRegion launcher_region(geo.x, geo.y, geo.width, geo.height);

      if (damage.intersects(launcher_region))
        launcher->QueueDraw();

      nux::ObjectPtr<nux::View> const& tooltip = launcher->GetActiveTooltip();

      if (tooltip)
      {
        nux::Geometry const& g = tooltip->GetAbsoluteGeometry();
        CompRegion tip_region(g.x, g.y, g.width, g.height);

        if (damage.intersects(tip_region))
          tooltip->QueueDraw();
      }

      nux::ObjectPtr<LauncherDragWindow> const& dragged_icon = launcher->GetDraggedIcon();

      if (dragged_icon)
      {
        nux::Geometry const& g = dragged_icon->GetAbsoluteGeometry();
        CompRegion icon_region(g.x, g.y, g.width, g.height);

        if (damage.intersects(icon_region))
          dragged_icon->QueueDraw();
      }
    }
  }

  for (auto const& panel : panel_controller_->panels())
  {
    nux::Geometry const& geo = panel->GetAbsoluteGeometry();

    CompRegion panel_region(geo.x, geo.y, geo.width, geo.height);

    if (damage.intersects(panel_region))
      panel->QueueDraw();
  }

  QuicklistManager* qm = QuicklistManager::Default();
  if (qm)
  {
    auto const& view = qm->Current();

    if (view)
    {
      nux::Geometry const& geo = view->GetAbsoluteGeometry();
      CompRegion quicklist_region(geo.x, geo.y, geo.width, geo.height);

      if (damage.intersects(quicklist_region))
        view->QueueDraw();
    }
  }

  if (switcher_controller_ && switcher_controller_->Visible())
  {
    auto const& view = switcher_controller_->GetView();

    if (G_LIKELY(view))
    {
      nux::Geometry const& geo = view->GetAbsoluteGeometry();
      CompRegion switcher_region(geo.x, geo.y, geo.width, geo.height);

      if (damage.intersects(switcher_region))
        view->QueueDraw();
    }
  }
}

/* Grab changed nux regions and add damage rects for them */
void UnityScreen::nuxDamageCompiz()
{
  /*
   * If Nux is going to redraw anything then we have to tell Compiz to
   * redraw everything. This is because Nux has a bad habit (bug??) of drawing
   * more than just the regions of its DrawList. (LP: #1036519)
   *
   * Forunately, this does not happen on most frames. Only when the Unity
   * Shell needs to redraw something.
   *
   * TODO: Try to figure out why redrawing the panel makes the launcher also
   *       redraw even though the launcher's geometry is not in DrawList, and
   *       stop it. Then maybe we can revert back to the old code below #else.
   */
  if (!wt->GetDrawList().empty() || animation_controller_->HasRunningAnimations())
  {
    cScreen->damageRegionSetEnabled(this, false);
    cScreen->damageScreen();
    cScreen->damageRegionSetEnabled(this, true);
  }
}

/* handle X Events */
void UnityScreen::handleEvent(XEvent* event)
{
  bool skip_other_plugins = false;
  PluginAdapter& wm = PluginAdapter::Default();

  switch (event->type)
  {
    case FocusIn:
    case FocusOut:
      if (event->xfocus.mode == NotifyGrab)
        wm.OnScreenGrabbed();
      else if (event->xfocus.mode == NotifyUngrab)
        wm.OnScreenUngrabbed();

      if (_key_nav_mode_requested)
      {
        // Close any overlay that is open.
        if (launcher_controller_->IsOverlayOpen())
        {
          dash_controller_->HideDash();
          hud_controller_->HideHud();
        }
        launcher_controller_->KeyNavGrab();
      }
      _key_nav_mode_requested = false;
      break;
    case MotionNotify:
      if (wm.IsScaleActive())
      {
        ScaleScreen* ss = ScaleScreen::get(screen);
        if (CompWindow *w = screen->findWindow(ss->getSelectedWindow()))
          skip_other_plugins = UnityWindow::get(w)->handleEvent(event);
      }
      else if (switcher_controller_->IsDetailViewShown())
      {
        Window win = switcher_controller_->GetCurrentSelection().window_;
        CompWindow* w = screen->findWindow(win);

        if (w)
          skip_other_plugins = UnityWindow::get(w)->handleEvent(event);
      }
      break;
    case ButtonPress:
      if (shortcut_controller_->Visible())
      {
        shortcut_controller_->Hide();
      }

      if (super_keypressed_)
      {
        launcher_controller_->KeyNavTerminate(false);
        EnableCancelAction(CancelActionTarget::LAUNCHER_SWITCHER, false);
      }
      if (wm.IsScaleActive())
      {
        ScaleScreen* ss = ScaleScreen::get(screen);
        if (CompWindow *w = screen->findWindow(ss->getSelectedWindow()))
          skip_other_plugins = UnityWindow::get(w)->handleEvent(event);
      }
      else if (switcher_controller_->IsDetailViewShown())
      {
        Window win = switcher_controller_->GetCurrentSelection().window_;
        CompWindow* w = screen->findWindow(win);

        if (w)
          skip_other_plugins = UnityWindow::get(w)->handleEvent(event);
      }

      if (dash_controller_->IsVisible())
      {
        nux::Point pt(event->xbutton.x_root, event->xbutton.y_root);
        nux::Geometry const& dash_geo = dash_controller_->GetInputWindowGeometry();

        Window dash_xid = dash_controller_->window()->GetInputWindowId();
        Window top_xid = wm.GetTopWindowAbove(dash_xid);
        nux::Geometry const& always_on_top_geo = wm.GetWindowGeometry(top_xid);

        bool indicator_clicked = panel_controller_->IsMouseInsideIndicator(pt);
        bool outside_dash = !dash_geo.IsInside(pt) && !DoesPointIntersectUnityGeos(pt);

        if ((outside_dash || indicator_clicked) && !always_on_top_geo.IsInside(pt))
        {
          if (indicator_clicked)
          {
            // We must skip the Dash hide animation, otherwise the indicators
            // wont recive the mouse click event
            dash_controller_->QuicklyHideDash();
          }
          else
          {
            dash_controller_->HideDash(false);
          }
        }
      }
      else if (hud_controller_->IsVisible())
      {
        nux::Point pt(event->xbutton.x_root, event->xbutton.y_root);
        nux::Geometry const& hud_geo = hud_controller_->GetInputWindowGeometry();

        Window hud_xid = hud_controller_->window()->GetInputWindowId();
        Window top_xid = wm.GetTopWindowAbove(hud_xid);
        nux::Geometry const& on_top_geo = wm.GetWindowGeometry(top_xid);

        if (!hud_geo.IsInside(pt) && !DoesPointIntersectUnityGeos(pt) && !on_top_geo.IsInside(pt))
        {
          hud_controller_->HideHud(false);
        }
      }
      else if (switcher_controller_->Visible())
      {
        nux::Point pt(event->xbutton.x_root, event->xbutton.y_root);
        nux::Geometry const& switcher_geo = switcher_controller_->GetInputWindowGeometry();

        if (!switcher_geo.IsInside(pt))
        {
          switcher_controller_->Hide(false);
        }
      }
      break;
    case ButtonRelease:

      if (switcher_controller_->IsDetailViewShown())
      {
        Window win = switcher_controller_->GetCurrentSelection().window_;
        CompWindow* w = screen->findWindow(win);

        if (w)
          skip_other_plugins = UnityWindow::get(w)->handleEvent(event);
      }
      else if (wm.IsScaleActive())
      {
        ScaleScreen* ss = ScaleScreen::get(screen);
        if (CompWindow *w = screen->findWindow(ss->getSelectedWindow()))
          skip_other_plugins = UnityWindow::get(w)->handleEvent(event);
      }
      break;
    case KeyPress:
    {
      if (shortcut_controller_->Visible())
      {
        shortcut_controller_->Hide();
      }

      if (super_keypressed_)
      {
        /* We need an idle to postpone this action, after the current event
         * has been processed */
        sources_.AddIdle([&] {
          shortcut_controller_->SetEnabled(false);
          shortcut_controller_->Hide();
          LOG_DEBUG(logger) << "Hiding shortcut controller due to keypress event.";
          EnableCancelAction(CancelActionTarget::SHORTCUT_HINT, false);

          return false;
        });
      }

      KeySym key_sym;
      char key_string[2];
      int result = XLookupString(&(event->xkey), key_string, 2, &key_sym, 0);

      if (launcher_controller_->KeyNavIsActive())
      {
        if (key_sym == XK_Up)
        {
          launcher_controller_->KeyNavPrevious();
          break;
        }
        else if (key_sym == XK_Down)
        {
          launcher_controller_->KeyNavNext();
          break;
        }
      }
      else if (switcher_controller_->Visible())
      {
        auto const& close_key = wm.close_window_key();

        if (key_sym == close_key.second && XModifiersToNux(event->xkey.state) == close_key.first)
        {
          switcher_controller_->Hide(false);
          skip_other_plugins = true;
        }
      }

      if (result > 0)
      {
        // NOTE: does this have the potential to do an invalid write?  Perhaps
        // we should just say "key_string[1] = 0" because that is the only
        // thing that could possibly make sense here.
        key_string[result] = 0;

        if (super_keypressed_)
        {
          skip_other_plugins = launcher_controller_->HandleLauncherKeyEvent(screen->dpy(), key_sym, event->xkey.keycode, event->xkey.state, key_string, event->xkey.time);
          if (!skip_other_plugins)
            skip_other_plugins = dash_controller_->CheckShortcutActivation(key_string);

          if (skip_other_plugins && launcher_controller_->KeyNavIsActive())
          {
            launcher_controller_->KeyNavTerminate(false);
            EnableCancelAction(CancelActionTarget::LAUNCHER_SWITCHER, false);
          }
        }
      }
      break;
    }
    case MapRequest:
      ShowdesktopHandler::InhibitLeaveShowdesktopMode (event->xmaprequest.window);
      break;
    case PropertyNotify:
      if (event->xproperty.window == GDK_ROOT_WINDOW() &&
          event->xproperty.atom == gdk_x11_get_xatom_by_name("_GNOME_BACKGROUND_REPRESENTATIVE_COLORS"))
      {
        if (_bghash)
          _bghash->RefreshColor();
      }
      break;
    default:
        if (screen->shapeEvent () + ShapeNotify == event->type)
        {
          Window xid = event->xany.window;
          CompWindow *w = screen->findWindow(xid);

          if (w)
          {
            UnityWindow *uw = UnityWindow::get (w);

            uw->handleEvent(event);
          }
        }
      break;
  }

  compiz::CompizMinimizedWindowHandler<UnityScreen, UnityWindow>::handleEvent (event);

  // avoid further propagation (key conflict for instance)
  if (!skip_other_plugins)
    screen->handleEvent(event);

  switch (event->type)
  {
    case PropertyNotify:
      if (event->xproperty.atom == Atoms::mwmHints)
      {
        PluginAdapter::Default().NotifyNewDecorationState(event->xproperty.window);
      }
      break;
    case MapRequest:
      ShowdesktopHandler::AllowLeaveShowdesktopMode(event->xmaprequest.window);
      break;
  }

  if (!skip_other_plugins && !switcher_controller_->IsMouseDisabled() &&
      screen->otherGrabExist("deco", "move", "switcher", "resize", nullptr))
  {
    wt->ProcessForeignEvent(event, nullptr);
  }
}

void UnityScreen::damageRegion(const CompRegion &region)
{
  compizDamageNux(region);
  cScreen->damageRegion(region);
}

void UnityScreen::handleCompizEvent(const char* plugin,
                                    const char* event,
                                    CompOption::Vector& option)
{
  PluginAdapter& adapter = PluginAdapter::Default();
  adapter.NotifyCompizEvent(plugin, event, option);
  compiz::CompizMinimizedWindowHandler<UnityScreen, UnityWindow>::handleCompizEvent (plugin, event, option);

  if (launcher_controller_->IsOverlayOpen() && g_strcmp0(event, "start_viewport_switch") == 0)
  {
    ubus_manager_.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);
  }

  if (adapter.IsScaleActive() && g_strcmp0(plugin, "scale") == 0 &&
      super_keypressed_)
  {
    scale_just_activated_ = true;
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
  int when = options[7].value().i();  // XEvent time in millisec
  launcher_controller_->HandleLauncherKeyPress(when);
  EnsureSuperKeybindings ();

  if (!shortcut_controller_->Visible() && shortcut_controller_->IsEnabled())
  {
    if (shortcut_controller_->Show())
    {
      LOG_DEBUG(logger) << "Showing shortcut hint.";
      EnableCancelAction(CancelActionTarget::SHORTCUT_HINT, true, action->key().modifiers());
    }
  }

  return true;
}

bool UnityScreen::showLauncherKeyTerminate(CompAction* action,
                                           CompAction::State state,
                                           CompOption::Vector& options)
{
  // Remember StateCancel and StateCommit will be broadcast to all actions
  // so we need to verify that we are actually being toggled...
  if (!(state & CompAction::StateTermKey))
    return false;

  if (state & CompAction::StateCancel)
    return false;

  bool was_tap = state & CompAction::StateTermTapped;
  LOG_DEBUG(logger) << "Super released: " << (was_tap ? "tapped" : "released");
  int when = options[7].value().i();  // XEvent time in millisec

  // hack...if the scale just wasn't activated AND the 'when' time is within time to start the
  // dash then assume was_tap is also true, since the ScalePlugin doesn't accept that state...
  PluginAdapter& adapter = PluginAdapter::Default();
  if (adapter.IsScaleActive() && !scale_just_activated_ &&
      launcher_controller_->AboutToShowDash(true, when))
  {
    adapter.TerminateScale();
    was_tap = true;
  }
  else if (scale_just_activated_)
  {
    scale_just_activated_ = false;
  }

  if (launcher_controller_->AboutToShowDash(was_tap, when))
  {
    if (hud_controller_->IsVisible())
    {
      hud_controller_->HideHud();
    }

    if (QuicklistManager::Default()->Current())
    {
      QuicklistManager::Default()->Current()->Hide();
    }
  }

  super_keypressed_ = false;
  launcher_controller_->KeyNavTerminate(true);
  launcher_controller_->HandleLauncherKeyRelease(was_tap, when);
  EnableCancelAction(CancelActionTarget::LAUNCHER_SWITCHER, false);

  shortcut_controller_->SetEnabled(optionGetShortcutOverlay());
  shortcut_controller_->Hide();
  LOG_DEBUG(logger) << "Hiding shortcut controller";
  EnableCancelAction(CancelActionTarget::SHORTCUT_HINT, false);

  action->setState (action->state() & (unsigned)~(CompAction::StateTermKey));
  return true;
}

bool UnityScreen::showPanelFirstMenuKeyInitiate(CompAction* action,
                                                CompAction::State state,
                                                CompOption::Vector& options)
{
  /* In order to avoid too many events when keeping the keybinding pressed,
   * that would make the unity-panel-service to go crazy (see bug #948522)
   * we need to filter them, just considering an event every 750 ms */
  int event_time = options[7].value().i();  // XEvent time in millisec

  if (event_time - first_menu_keypress_time_ < 750)
  {
    first_menu_keypress_time_ = event_time;
    return false;
  }

  first_menu_keypress_time_ = event_time;

  /* Even if we do nothing on key terminate, we must enable it, not to to hide
   * the menus entries after that a menu has been shown and hidden via the
   * keyboard and the Alt key is still pressed */
  action->setState(action->state() | CompAction::StateTermKey);
  panel_controller_->FirstMenuShow();

  return true;
}

bool UnityScreen::showPanelFirstMenuKeyTerminate(CompAction* action,
                                                 CompAction::State state,
                                                 CompOption::Vector& options)
{
  action->setState (action->state() & (unsigned)~(CompAction::StateTermKey));
  return true;
}

void UnityScreen::SendExecuteCommand()
{
  if (hud_controller_->IsVisible())
  {
    hud_controller_->HideHud();
  }

  PluginAdapter& adapter = PluginAdapter::Default();
  if (adapter.IsScaleActive())
  {
    adapter.TerminateScale();
  }

  if (dash_controller_->IsCommandLensOpen())
  {
    ubus_manager_.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);
  }
  else
  {
    ubus_manager_.SendMessage(UBUS_DASH_ABOUT_TO_SHOW, NULL, glib::Source::Priority::HIGH);

    ubus_manager_.SendMessage(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                              g_variant_new("(sus)", "commands.scope", dash::ScopeHandledType::GOTO_DASH_URI, ""),
                              glib::Source::Priority::LOW);
  }
}

bool UnityScreen::executeCommand(CompAction* action,
                                 CompAction::State state,
                                 CompOption::Vector& options)
{
  SendExecuteCommand();
  return true;
}

bool UnityScreen::showDesktopKeyInitiate(CompAction* action,
                                         CompAction::State state,
                                         CompOption::Vector& options)
{
  WindowManager::Default().ShowDesktop();
  return true;
}

bool UnityScreen::setKeyboardFocusKeyInitiate(CompAction* action,
                                              CompAction::State state,
                                              CompOption::Vector& options)
{
  _key_nav_mode_requested = true;
  return true;
}

bool UnityScreen::altTabInitiateCommon(CompAction* action, switcher::ShowMode show_mode)
{
  if (!grab_index_)
  {
    if (switcher_controller_->IsMouseDisabled())
    {
      grab_index_ = screen->pushGrab (screen->invisibleCursor(), "unity-switcher");
    }
    else
    {
      grab_index_ = screen->pushGrab (screen->normalCursor(), "unity-switcher");
    }
  }

  screen->addAction(&optionGetAltTabRight());
  screen->addAction(&optionGetAltTabDetailStart());
  screen->addAction(&optionGetAltTabDetailStop());
  screen->addAction(&optionGetAltTabLeft());

  /* Create a new keybinding for scroll buttons and current modifiers */
  CompAction scroll_up;
  CompAction scroll_down;
  scroll_up.setButton(CompAction::ButtonBinding(local::SCROLL_UP_BUTTON, action->key().modifiers()));
  scroll_down.setButton(CompAction::ButtonBinding(local::SCROLL_DOWN_BUTTON, action->key().modifiers()));
  screen->addAction(&scroll_up);
  screen->addAction(&scroll_down);

  if (!optionGetAltTabBiasViewport())
  {
    if (show_mode == switcher::ShowMode::CURRENT_VIEWPORT)
      show_mode = switcher::ShowMode::ALL;
    else
      show_mode = switcher::ShowMode::CURRENT_VIEWPORT;
  }

  UnityWindow::SetupSharedTextures();
  SetUpAndShowSwitcher(show_mode);

  return true;
}

void UnityScreen::SetUpAndShowSwitcher(switcher::ShowMode show_mode)
{
  RaiseInputWindows();

  auto results = launcher_controller_->GetAltTabIcons(show_mode == switcher::ShowMode::CURRENT_VIEWPORT,
                                                      switcher_controller_->IsShowDesktopDisabled());

  if (switcher_controller_->CanShowSwitcher(results))
    switcher_controller_->Show(show_mode, switcher::SortMode::FOCUS_ORDER, results);
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
  }

  screen->removeAction(&optionGetAltTabRight ());
  screen->removeAction(&optionGetAltTabDetailStart ());
  screen->removeAction(&optionGetAltTabDetailStop ());
  screen->removeAction(&optionGetAltTabLeft ());

  /* Removing the scroll actions */
  CompAction scroll_up;
  CompAction scroll_down;
  scroll_up.setButton(CompAction::ButtonBinding(local::SCROLL_UP_BUTTON, action->key().modifiers()));
  scroll_down.setButton(CompAction::ButtonBinding(local::SCROLL_DOWN_BUTTON, action->key().modifiers()));
  screen->removeAction(&scroll_up);
  screen->removeAction(&scroll_down);

  bool accept_state = (state & CompAction::StateCancel) == 0;
  switcher_controller_->Hide(accept_state);

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
    altTabInitiateCommon(action, switcher::ShowMode::CURRENT_VIEWPORT);

  action->setState(action->state() | CompAction::StateTermKey);
  return true;
}

bool UnityScreen::altTabForwardAllInitiate(CompAction* action,
                                        CompAction::State state,
                                        CompOption::Vector& options)
{
  if (WindowManager::Default().IsWallActive())
    return false;
  else if (switcher_controller_->Visible())
    switcher_controller_->Next();
  else
    altTabInitiateCommon(action, switcher::ShowMode::ALL);

  action->setState(action->state() | CompAction::StateTermKey);
  return true;
}

bool UnityScreen::altTabPrevAllInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  if (switcher_controller_->Visible())
  {
    switcher_controller_->Prev();
    return true;
  }

  return false;
}

bool UnityScreen::altTabPrevInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  if (switcher_controller_->Visible())
  {
    switcher_controller_->Prev();
    return true;
  }

  return false;
}

bool UnityScreen::altTabDetailStart(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  return switcher_controller_->StartDetailMode();
}

bool UnityScreen::altTabDetailStop(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  return switcher_controller_->StopDetailMode();
}

bool UnityScreen::altTabNextWindowInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  if (!switcher_controller_->Visible())
  {
    altTabInitiateCommon(action, switcher::ShowMode::CURRENT_VIEWPORT);
    switcher_controller_->Select((switcher_controller_->StartIndex())); // always select the current application
    switcher_controller_->InitiateDetail();
  }
  else if (switcher_controller_->IsDetailViewShown())
  {
    switcher_controller_->NextDetail();
  }
  else
  {
    switcher_controller_->SetDetail(true);
  }

  action->setState(action->state() | CompAction::StateTermKey);
  return true;
}

bool UnityScreen::altTabPrevWindowInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  if (switcher_controller_->Visible())
  {
    switcher_controller_->PrevDetail();
    return true;
  }

  return false;
}

bool UnityScreen::launcherSwitcherForwardInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  if (!launcher_controller_->KeyNavIsActive())
  {
    int modifiers = action->key().modifiers();

    launcher_controller_->KeyNavActivate();

    EnableCancelAction(CancelActionTarget::LAUNCHER_SWITCHER, true, modifiers);

    KeyCode down_key = XKeysymToKeycode(screen->dpy(), XStringToKeysym("Down"));
    KeyCode up_key = XKeysymToKeycode(screen->dpy(), XStringToKeysym("Up"));

    CompAction down_action;
    down_action.setKey(CompAction::KeyBinding(down_key, modifiers));
    screen->addAction(&down_action);

    CompAction up_action;
    up_action.setKey(CompAction::KeyBinding(up_key, modifiers));
    screen->addAction(&up_action);
  }
  else
  {
    launcher_controller_->KeyNavNext();
  }

  action->setState(action->state() | CompAction::StateTermKey);
  return true;
}
bool UnityScreen::launcherSwitcherPrevInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  launcher_controller_->KeyNavPrevious();

  return true;
}
bool UnityScreen::launcherSwitcherTerminate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  bool accept_state = (state & CompAction::StateCancel) == 0;
  launcher_controller_->KeyNavTerminate(accept_state);

  EnableCancelAction(CancelActionTarget::LAUNCHER_SWITCHER, false);

  KeyCode down_key = XKeysymToKeycode(screen->dpy(), XStringToKeysym("Down"));
  KeyCode up_key = XKeysymToKeycode(screen->dpy(), XStringToKeysym("Up"));

  CompAction down_action;
  down_action.setKey(CompAction::KeyBinding(down_key, action->key().modifiers()));
  screen->removeAction(&down_action);

  CompAction up_action;
  up_action.setKey(CompAction::KeyBinding(up_key, action->key().modifiers()));
  screen->removeAction(&up_action);

  action->setState (action->state() & (unsigned)~(CompAction::StateTermKey));
  return true;
}

void UnityScreen::OnLauncherStartKeyNav(GVariant* data)
{
  // Put the launcher BaseWindow at the top of the BaseWindow stack. The
  // input focus coming from the XinputWindow will be processed by the
  // launcher BaseWindow only. Then the Launcher BaseWindow will decide
  // which View will get the input focus.
  if (SaveInputThenFocus(launcher_controller_->KeyNavLauncherInputWindowId()))
    launcher_controller_->PushToFront();
}

void UnityScreen::OnLauncherEndKeyNav(GVariant* data)
{
  // Return input-focus to previously focused window (before key-nav-mode was
  // entered)
  if (data && g_variant_get_boolean(data))
    PluginAdapter::Default().RestoreInputFocus();
}

void UnityScreen::OnSwitcherStart(GVariant* data)
{
  if (switcher_controller_->Visible())
  {
    UnityWindow::SetupSharedTextures();
  }
}

void UnityScreen::OnSwitcherEnd(GVariant* data)
{
  UnityWindow::CleanupSharedTextures();

  for (UnityWindow* uwin : fake_decorated_windows_)
    uwin->CleanupCachedTextures();
}

bool UnityScreen::SaveInputThenFocus(const guint xid)
{
  // get CompWindow*
  newFocusedWindow = screen->findWindow(xid);

  // check if currently focused window isn't it self
  if (xid != screen->activeWindow())
    PluginAdapter::Default().SaveInputFocus();

  // set input-focus on window
  if (newFocusedWindow)
  {
    newFocusedWindow->moveInputFocusTo();
    return true;
  }
  return false;
}

bool UnityScreen::ShowHud()
{
  if (switcher_controller_->Visible())
  {
    LOG_ERROR(logger) << "this should never happen";
    return false; // early exit if the switcher is open
  }

  if (hud_controller_->IsVisible())
  {
    ubus_manager_.SendMessage(UBUS_HUD_CLOSE_REQUEST);
  }
  else
  {
    // Handles closing KeyNav (Alt+F1) if the hud is about to show
    if (launcher_controller_->KeyNavIsActive())
      launcher_controller_->KeyNavTerminate(false);

    // If an overlay is open, it must be the dash! Close it!
    if (launcher_controller_->IsOverlayOpen())
      dash_controller_->HideDash();

    if (QuicklistManager::Default()->Current())
      QuicklistManager::Default()->Current()->Hide();

    hud_controller_->ShowHud();
  }

  // Consume the event.
  return true;
}

bool UnityScreen::ShowHudInitiate(CompAction* action,
                                  CompAction::State state,
                                  CompOption::Vector& options)
{
  // Look to see if there is a keycode.  If there is, then this isn't a
  // modifier only keybinding.
  if (options[6].type() != CompOption::TypeUnset)
  {
    int key_code = options[6].value().i();
    LOG_DEBUG(logger) << "HUD initiate key code: " << key_code;
    // show it now, no timings or terminate needed.
    return ShowHud();
  }
  else
  {
    LOG_DEBUG(logger) << "HUD initiate key code option not set, modifier only keypress.";
  }


  // to receive the Terminate event
  if (state & CompAction::StateInitKey)
    action->setState(action->state() | CompAction::StateTermKey);
  hud_keypress_time_ = options[7].value().i();  // XEvent time in millisec

  // pass key through
  return false;
}

bool UnityScreen::ShowHudTerminate(CompAction* action,
                                   CompAction::State state,
                                   CompOption::Vector& options)
{
  // Remember StateCancel and StateCommit will be broadcast to all actions
  // so we need to verify that we are actually being toggled...
  if (!(state & CompAction::StateTermKey))
    return false;

  action->setState(action->state() & ~CompAction::StateTermKey);

  // If we have a modifier only keypress, check for tap and timing.
  if (!(state & CompAction::StateTermTapped))
    return false;

  int release_time = options[7].value().i();  // XEvent time in millisec
  int tap_duration = release_time - hud_keypress_time_;
  if (tap_duration > local::ALT_TAP_DURATION)
  {
    LOG_DEBUG(logger) << "Tap too long";
    return false;
  }

  return ShowHud();
}

unsigned UnityScreen::CompizModifiersToNux(unsigned input) const
{
  unsigned modifiers = 0;

  if (input & CompAltMask)
  {
    input &= ~CompAltMask;
    input |= Mod1Mask;
  }

  if (modifiers & CompSuperMask)
  {
    input &= ~CompSuperMask;
    input |= Mod4Mask;
  }

  return XModifiersToNux(input);
}

unsigned UnityScreen::XModifiersToNux(unsigned input) const
{
  unsigned modifiers = 0;

  if (input & Mod1Mask)
    modifiers |= nux::KEY_MODIFIER_ALT;

  if (input & ShiftMask)
    modifiers |= nux::KEY_MODIFIER_SHIFT;

  if (input & ControlMask)
    modifiers |= nux::KEY_MODIFIER_CTRL;

  if (input & Mod4Mask)
    modifiers |= nux::KEY_MODIFIER_SUPER;

  return modifiers;
}

void UnityScreen::UpdateCloseWindowKey(CompAction::KeyBinding const& keybind)
{
  KeySym keysym = XkbKeycodeToKeysym(screen->dpy(), keybind.keycode(), 0, 0);
  unsigned modifiers = CompizModifiersToNux(keybind.modifiers());

  WindowManager::Default().close_window_key = std::make_pair(modifiers, keysym);
}

bool UnityScreen::initPluginActions()
{
  PluginAdapter& adapter = PluginAdapter::Default();

  CompPlugin* p = CompPlugin::find("core");

  if (p)
  {
    for (CompOption& option : p->vTable->getOptions())
    {
      if (option.name() == "close_window_key")
      {
        UpdateCloseWindowKey(option.value().action().key());
        break;
      }
    }
  }

  p = CompPlugin::find("expo");

  if (p)
  {
    MultiActionList expoActions;

    for (CompOption& option : p->vTable->getOptions())
    {
      std::string const& option_name = option.name();

      if (!expoActions.HasPrimary() &&
          (option_name == "expo_key" ||
           option_name == "expo_button" ||
           option_name == "expo_edge"))
      {
        CompAction* action = &option.value().action();
        expoActions.AddNewAction(option_name, action, true);
      }
      else if (option_name == "exit_button")
      {
        CompAction* action = &option.value().action();
        expoActions.AddNewAction(option_name, action, false);
      }
    }

    adapter.SetExpoAction(expoActions);
  }

  p = CompPlugin::find("scale");

  if (p)
  {
    MultiActionList scaleActions;

    for (CompOption& option : p->vTable->getOptions())
    {
      std::string const& option_name = option.name();

      if (option_name == "initiate_all_key" ||
          option_name == "initiate_all_edge" ||
          option_name == "initiate_key" ||
          option_name == "initiate_button" ||
          option_name == "initiate_edge" ||
          option_name == "initiate_group_key" ||
          option_name == "initiate_group_button" ||
          option_name == "initiate_group_edge" ||
          option_name == "initiate_output_key" ||
          option_name == "initiate_output_button" ||
          option_name == "initiate_output_edge")
      {
        CompAction* action = &option.value().action();
        scaleActions.AddNewAction(option_name, action, false);
      }
      else if (option_name == "initiate_all_button")
      {
        CompAction* action = &option.value().action();
        scaleActions.AddNewAction(option_name, action, true);
      }
    }

    adapter.SetScaleAction(scaleActions);
  }

  p = CompPlugin::find("unitymtgrabhandles");

  if (p)
  {
    foreach(CompOption & option, p->vTable->getOptions())
    {
      if (option.name() == "show_handles_key")
        adapter.SetShowHandlesAction(&option.value().action());
      else if (option.name() == "hide_handles_key")
        adapter.SetHideHandlesAction(&option.value().action());
      else if (option.name() == "toggle_handles_key")
        adapter.SetToggleHandlesAction(&option.value().action());
    }
  }

  return false;
}

/* Set up expo and scale actions on the launcher */
bool UnityScreen::initPluginForScreen(CompPlugin* p)
{
  if (p->vTable->name() == "expo" ||
      p->vTable->name() == "scale")
  {
    initPluginActions();
  }

  bool result = screen->initPluginForScreen(p);
  if (p->vTable->name() == "unityshell")
    initAltTabNextWindow();

  return result;
}

void UnityScreen::AddProperties(GVariantBuilder* builder)
{}

std::string UnityScreen::GetName() const
{
  return "Unity";
}

bool isNuxWindow(CompWindow* value)
{
  std::vector<Window> const& xwns = nux::XInputWindow::NativeHandleList();
  auto id = value->id();

  // iterate loop by hand rather than use std::find as this is considerably faster
  // we care about performance here because of the high frequency in which this function is
  // called (nearly every frame)
  unsigned int size = xwns.size();
  for (unsigned int i = 0; i < size; ++i)
  {
    if (xwns[i] == id)
      return true;
  }
  return false;
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
  /*
   * The occlusion pass tests windows from TOP to BOTTOM. That's opposite to
   * the actual painting loop.
   *
   * Detect uScreen->fullscreenRegion here. That represents the region which
   * fully covers the shell on its output. It does not include regular windows
   * stacked above the shell like DnD icons or Onboard etc.
   */
  if (G_UNLIKELY(is_nux_window_))
  {
    if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
    {
      uScreen->nuxRegion += window->geometry();
      uScreen->nuxRegion -= uScreen->fullscreenRegion;
    }

    if (window->id() == screen->activeWindow() &&
        !(mask & PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK))
    {
      if (!mask)
        uScreen->panelShadowPainted = CompRect();

      uScreen->paintPanelShadow(region);
    }

    return false;  // Ensure nux windows are never painted by compiz
  }
  else if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
  {
    static const unsigned int nonOcclusionBits =
                              PAINT_WINDOW_TRANSLUCENT_MASK |
                              PAINT_WINDOW_TRANSFORMED_MASK |
                              PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

    if (window->isMapped() &&
        window->defaultViewport() == uScreen->screen->vp())
    {
      int monitor = window->outputDevice();

      auto it = uScreen->windows_for_monitor_.find(monitor);

      if (it != end(uScreen->windows_for_monitor_))
        ++(it->second);
      else
        uScreen->windows_for_monitor_[monitor] = 1;

      if (!(mask & nonOcclusionBits) &&
          (window->state() & CompWindowStateFullscreenMask && !window->minimized()) &&
          uScreen->windows_for_monitor_[monitor] == 1)
          // And I've been advised to test other things, but they don't work:
          // && (attrib.opacity == OPAQUE)) <-- Doesn't work; Only set in glDraw
          // && !window->alpha() <-- Doesn't work; Opaque windows often have alpha
      {
        uScreen->fullscreenRegion += window->geometry();
      }

      if (uScreen->nuxRegion.isEmpty())
        uScreen->firstWindowAboveShell = window;
    }
  }

  GLWindowPaintAttrib wAttrib = attrib;

  if (mMinimizeHandler)
  {
    mask |= mMinimizeHandler->getPaintMask ();
  }
  else if (mShowdesktopHandler)
  {
    mShowdesktopHandler->PaintOpacity (wAttrib.opacity);
    mask |= mShowdesktopHandler->GetPaintMask ();
  }

  std::vector<Window> const& tray_xids = uScreen->panel_controller_->GetTrayXids();
  if (std::find(tray_xids.begin(), tray_xids.end(), window->id()) != tray_xids.end() &&
      !uScreen->allowWindowPaint)
  {
    if (!uScreen->painting_tray_)
    {
      uScreen->tray_paint_mask_ = mask;
      mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;
    }
  }

  if (WindowManager::Default().IsScaleActive() &&
      ScaleScreen::get(screen)->getSelectedWindow() == window->id())
  {
    nux::Geometry const& scaled_geo = GetScaledGeometry();
    paintInnerGlow(scaled_geo, matrix, attrib, mask);
  }

  if (uScreen->session_controller_ && uScreen->session_controller_->Visible())
  {
    // Let's darken the other windows if the session dialog is visible
    wAttrib.brightness *= 0.75f;
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
#ifndef USE_MODERN_COMPIZ_GL
                         GLFragment::Attrib& attrib,
#else
                         const GLWindowPaintAttrib& attrib,
#endif
                         const CompRegion& region,
                         unsigned int mask)
{
  if (uScreen->doShellRepaint && !uScreen->paint_panel_under_dash_ && window->type() == CompWindowTypeNormalMask)
  {
    if ((window->state() & MAXIMIZE_STATE) && window->onCurrentDesktop() && !window->overrideRedirect() && window->managed())
    {
      CompPoint const& viewport = window->defaultViewport();
      unsigned output = window->outputDevice();

      if (viewport == uScreen->screen->vp() && output == uScreen->screen->currentOutputDev().id())
      {
        uScreen->paint_panel_under_dash_ = true;
      }
    }
  }

  if (uScreen->doShellRepaint &&
      !uScreen->forcePaintOnTop () &&
      window == uScreen->firstWindowAboveShell &&
      !uScreen->fullscreenRegion.contains(window->geometry()))
  {
    uScreen->paintDisplay();
  }

  enum class DrawPanelShadow
  {
    NO,
    BELOW_WINDOW,
    OVER_WINDOW,
  };

  auto draw_panel_shadow = DrawPanelShadow::NO;

  if (!(mask & PAINT_WINDOW_ON_TRANSFORMED_SCREEN_MASK))
  {
    Window active_window = screen->activeWindow();

    if (G_UNLIKELY(window->type() == CompWindowTypeDesktopMask))
    {
      uScreen->setPanelShadowMatrix(matrix);

      if (active_window == 0 || active_window == window->id())
      {
        if (PluginAdapter::Default().IsWindowOnTop(window->id()))
        {
          draw_panel_shadow = DrawPanelShadow::OVER_WINDOW;
        }
        uScreen->is_desktop_active_ = true;
      } 
    }
    else
    {
      if (window->id() == active_window)
      {
        draw_panel_shadow = DrawPanelShadow::BELOW_WINDOW;
        uScreen->is_desktop_active_ = false;

        if (!(window->state() & CompWindowStateMaximizedVertMask) &&
            !(window->state() & CompWindowStateFullscreenMask) &&
            !(window->type() & CompWindowTypeFullscreenMask))
        {
          auto const& output = uScreen->screen->currentOutputDev();

          if (window->y() - window->border().top < output.y() + uScreen->panel_style_.panel_height)
          {
            draw_panel_shadow = DrawPanelShadow::OVER_WINDOW;
          }
        }
      }
      else
      {
        if (uScreen->is_desktop_active_)
        {
          if (PluginAdapter::Default().IsWindowOnTop(window->id()))
          {
            draw_panel_shadow = DrawPanelShadow::OVER_WINDOW;
            uScreen->panelShadowPainted = CompRegion();
          }
        }
      }
    }
  }

  if (draw_panel_shadow == DrawPanelShadow::BELOW_WINDOW)
    uScreen->paintPanelShadow(region);

  bool ret = gWindow->glDraw(matrix, attrib, region, mask);

  if (draw_panel_shadow == DrawPanelShadow::OVER_WINDOW)
    uScreen->paintPanelShadow(region);

  return ret;
}

void
UnityScreen::OnMinimizeDurationChanged ()
{
  /* Update the compiz plugin setting with the new computed speed so that it
   * will be used in the following minimizations */
  CompPlugin *p = CompPlugin::find("animation");
  if (p)
  {
    CompOption::Vector &opts = p->vTable->getOptions();

    for (CompOption &o : opts)
    {
      if (o.name () == std::string ("minimize_durations"))
      {
        /* minimize_durations is a list value, but minimize applies only to
         * normal windows, so there's always one value */
        CompOption::Value& value = o.value();
        CompOption::Value::Vector& list = value.list();
        CompOption::Value::Vector::iterator i = list.begin();
        if (i != list.end())
          i->set(minimize_speed_controller_.getDuration());

        value.set(list);
        screen->setOptionForPlugin(p->vTable->name().c_str(),
                                   o.name().c_str(), value);
        break;
      }
    }
  }
  else {
    LOG_WARN(logger) << "Animation plugin not found. Can't set minimize speed.";
  }
}

void
UnityWindow::minimize ()
{
  if (!window->managed ())
    return;

  if (!mMinimizeHandler)
  {
    mMinimizeHandler.reset (new UnityMinimizedHandler (window, this));
    mMinimizeHandler->minimize ();
  }
}

void
UnityWindow::unminimize ()
{
  if (mMinimizeHandler)
  {
    mMinimizeHandler->unminimize ();
    mMinimizeHandler.reset ();
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

  if (window->geometry ().x () + window->geometry ().width ()  <= 0 ||
      window->geometry ().y () + window->geometry ().height () <= 0 ||
      window->geometry ().x () >= (int) screen->width ()||
      window->geometry ().y () >= (int) screen->height ())
    return false;

  return true;
}

bool
UnityWindow::minimized () const
{
  return mMinimizeHandler.get () != nullptr;
}

/* Called whenever a window is mapped, unmapped, minimized etc */
void UnityWindow::windowNotify(CompWindowNotify n)
{
  PluginAdapter::Default().Notify(window, n);

  switch (n)
  {
    case CompWindowNotifyMap:
      if (window->type() == CompWindowTypeDesktopMask) {
        if (!focus_desktop_timeout_)
        {
          focus_desktop_timeout_.reset(new glib::Timeout(1000, [&] {
            for (CompWindow *w : screen->clientList())
            {
              if (!(w->type() & NO_FOCUS_MASK) && w->focus ())
              {
                focus_desktop_timeout_ = nullptr;
                return false;
              }
            }

            window->moveInputFocusTo();

            focus_desktop_timeout_ = nullptr;
            return false;
          }));
        }
      }
    /* Fall through an re-evaluate wraps on map and unmap too */
    case CompWindowNotifyUnmap:
      if (UnityScreen::get (screen)->optionGetShowMinimizedWindows () &&
          window->mapNum () &&
          !window->pendingUnmaps())
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

      PluginAdapter::Default().UpdateShowDesktopState();
        break;
      case CompWindowNotifyBeforeDestroy:
        being_destroyed.emit();
        break;
      case CompWindowNotifyMinimize:
        /* Updating the count in dconf will trigger a "changed" signal to which
         * the method setting the new animation speed is attached */
        UnityScreen::get(screen)->minimize_speed_controller_.UpdateCount();
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
    if (n == CompWindowNotifyFocusChange)
      mShowdesktopHandler->WindowFocusChangeNotify ();
  }

  // We do this after the notify to ensure input focus has actually been moved.
  if (n == CompWindowNotifyFocusChange)
  {
    UnityScreen* us = UnityScreen::get(screen);

    // If focus gets moved to an external program while the Dash/Hud is open, refocus key input.
    if (us)
    {
      if (us->dash_controller_->IsVisible())
      {
        us->dash_controller_->ReFocusKeyInput();
      }
      else if (us->hud_controller_->IsVisible())
      {
        us->hud_controller_->ReFocusKeyInput();
      }
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

  PluginAdapter::Default().NotifyStateChange(window, window->state(), lastState);
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
    mShowdesktopHandler->UpdateFrameRegion (region);
  else
    window->updateFrameRegion (region);
}

void UnityWindow::moveNotify(int x, int y, bool immediate)
{
  PluginAdapter::Default().NotifyMoved(window, x, y);
  window->moveNotify(x, y, immediate);
}

void UnityWindow::resizeNotify(int x, int y, int w, int h)
{
  PluginAdapter::Default().NotifyResized(window, x, y, w, h);
  window->resizeNotify(x, y, w, h);
}

CompPoint UnityWindow::tryNotIntersectUI(CompPoint& pos)
{
  UnityScreen* us = UnityScreen::get(screen);
  auto window_geo = window->borderRect ();
  nux::Geometry target_monitor;
  nux::Point result(pos.x(), pos.y());

  // seriously why does compiz not track monitors XRandR style???
  auto const& monitors = UScreen::GetDefault()->GetMonitors();
  for (auto const& monitor : monitors)
  {
    if (monitor.IsInside(result))
    {
      target_monitor = monitor;
      break;
    }
  }

  auto const& launchers = us->launcher_controller_->launchers();

  for (auto const& launcher : launchers)
  {
    nux::Geometry geo = launcher->GetAbsoluteGeometry();

    if (launcher->options()->hide_mode == LAUNCHER_HIDE_AUTOHIDE && launcher->Hidden())
      continue;

    if (geo.IsInside(result))
    {
      if (geo.x + geo.width + 1 + window_geo.width() < target_monitor.x + target_monitor.width)
      {
        result.x = geo.x + geo.width + 1;
      }
    }
  }

  for (nux::Geometry const& geo : us->panel_controller_->GetGeometries())
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
  bool was_maximized = PluginAdapter::Default().MaximizeIfBigEnough(window);

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

void UnityScreen::onRedrawRequested()
{
  nuxDamageCompiz();
}

/* Handle option changes and plug that into nux windows */
void UnityScreen::optionChanged(CompOption* opt, UnityshellOptions::Options num)
{
  // Note: perhaps we should put the options here into the controller.
  unity::launcher::Options::Ptr launcher_options = launcher_controller_->options();
  switch (num)
  {
    case UnityshellOptions::NumLaunchers:
      launcher_controller_->multiple_launchers = optionGetNumLaunchers() == 0;
      dash_controller_->use_primary = !launcher_controller_->multiple_launchers();
      hud_controller_->multiple_launchers = launcher_controller_->multiple_launchers();
      break;
    case UnityshellOptions::LauncherCaptureMouse:
      launcher_options->edge_resist = optionGetLauncherCaptureMouse();
      break;
    case UnityshellOptions::BackgroundColor:
    {
      nux::Color override_color (optionGetBackgroundColorRed() / 65535.0f,
                                 optionGetBackgroundColorGreen() / 65535.0f,
                                 optionGetBackgroundColorBlue() / 65535.0f,
                                 optionGetBackgroundColorAlpha() / 65535.0f);

      override_color.red = override_color.red / override_color.alpha;
      override_color.green = override_color.green / override_color.alpha;
      override_color.blue = override_color.blue / override_color.alpha;
      if (_bghash)
        _bghash->OverrideColor(override_color);
      break;
    }
    case UnityshellOptions::LauncherHideMode:
    {
      launcher_options->hide_mode = (unity::launcher::LauncherHideMode) optionGetLauncherHideMode();
      hud_controller_->launcher_locked_out = (launcher_options->hide_mode == unity::launcher::LauncherHideMode::LAUNCHER_HIDE_NEVER);
      break;
    }
    case UnityshellOptions::BacklightMode:
      launcher_options->backlight_mode = (unity::launcher::BacklightMode) optionGetBacklightMode();
      break;
    case UnityshellOptions::RevealTrigger:
      launcher_options->reveal_trigger = (unity::launcher::RevealTrigger) optionGetRevealTrigger();
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

      hud_controller_->icon_size = launcher_options->icon_size();
      hud_controller_->tile_size = launcher_options->tile_size();

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
      PluginAdapter::Default().SetCoverageAreaBeforeAutomaximize(optionGetAutomaximizeValue() / 100.0f);
      break;
    case UnityshellOptions::DashTapDuration:
      launcher_options->super_tap_duration = optionGetDashTapDuration();
      break;
    case UnityshellOptions::AltTabTimeout:
      switcher_controller_->SetDetailOnTimeout(optionGetAltTabTimeout());
    case UnityshellOptions::AltTabBiasViewport:
      PluginAdapter::Default().bias_active_to_viewport = optionGetAltTabBiasViewport();
      break;
    case UnityshellOptions::DisableShowDesktop:
      switcher_controller_->SetShowDesktopDisabled(optionGetDisableShowDesktop());
      break;
    case UnityshellOptions::DisableMouse:
      switcher_controller_->SetMouseDisabled(optionGetDisableMouse());
      break;
    case UnityshellOptions::ShowMinimizedWindows:
      compiz::CompizMinimizedWindowHandler<UnityScreen, UnityWindow>::setFunctions (optionGetShowMinimizedWindows ());
      screen->enterShowDesktopModeSetEnabled (this, optionGetShowMinimizedWindows ());
      screen->leaveShowDesktopModeSetEnabled (this, optionGetShowMinimizedWindows ());
      break;
    case UnityshellOptions::ShortcutOverlay:
      shortcut_controller_->SetEnabled(optionGetShortcutOverlay());
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
      break;
    case UnityshellOptions::EdgeResponsiveness:
      launcher_options->edge_responsiveness = optionGetEdgeResponsiveness();
      break;
    case UnityshellOptions::EdgePassedDisabledMs:
      launcher_options->edge_passed_disabled_ms = optionGetEdgePassedDisabledMs();
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
  if (!sources_.GetSource(local::RELAYOUT_TIMEOUT))
  {
    sources_.AddTimeout(timeout, [&] {
      NeedsRelayout();
      Relayout();

      cScreen->damageScreen();

      return false;
    }, local::RELAYOUT_TIMEOUT);
  }
}

void UnityScreen::Relayout()
{
  nux::Geometry geometry (0, 0, screen->width (), screen->height ());

  if (!needsRelayout)
    return;

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

/* Handle changes in the number of workspaces by showing the switcher
 * or not showing the switcher */
bool UnityScreen::setOptionForPlugin(const char* plugin, const char* name,
                                     CompOption::Value& v)
{
  bool status = screen->setOptionForPlugin(plugin, name, v);

  if (status)
  {
    if (strcmp(plugin, "core") == 0)
    {
      if (strcmp(name, "hsize") == 0 || strcmp(name, "vsize") == 0)
      {
        WindowManager& wm = WindowManager::Default();
        wm.viewport_layout_changed.emit(screen->vpSize().width(), screen->vpSize().height());
      }
      else if (strcmp(name, "close_window_key") == 0)
      {
        UpdateCloseWindowKey(v.action().key());
      }
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

  auto xdnd_collection_window = std::make_shared<XdndCollectionWindowImp>();
  auto xdnd_start_stop_notifier = std::make_shared<XdndStartStopNotifierImp>();
  auto xdnd_manager = std::make_shared<XdndManagerImp>(xdnd_start_stop_notifier, xdnd_collection_window);
  auto edge_barriers = std::make_shared<ui::EdgeBarrierController>();

  launcher_controller_ = std::make_shared<launcher::Controller>(xdnd_manager, edge_barriers);
  AddChild(launcher_controller_.get());

  switcher_controller_ = std::make_shared<switcher::Controller>();
  AddChild(switcher_controller_.get());

  LOG_INFO(logger) << "initLauncher-Launcher " << timer.ElapsedSeconds() << "s";

  /* Setup panel */
  timer.Reset();
  panel_controller_ = std::make_shared<panel::Controller>(edge_barriers);
  AddChild(panel_controller_.get());
  panel_controller_->SetMenuShowTimings(optionGetMenusFadein(),
                                        optionGetMenusFadeout(),
                                        optionGetMenusDiscoveryDuration(),
                                        optionGetMenusDiscoveryFadein(),
                                        optionGetMenusDiscoveryFadeout());
  LOG_INFO(logger) << "initLauncher-Panel " << timer.ElapsedSeconds() << "s";

  /* Setup Places */
  dash_controller_ = std::make_shared<dash::Controller>();
  dash_controller_->on_realize.connect(sigc::mem_fun(this, &UnityScreen::OnDashRealized));
  AddChild(dash_controller_.get());

  /* Setup Hud */
  hud_controller_ = std::make_shared<hud::Controller>();
  auto hide_mode = (unity::launcher::LauncherHideMode) optionGetLauncherHideMode();
  hud_controller_->launcher_locked_out = (hide_mode == unity::launcher::LauncherHideMode::LAUNCHER_HIDE_NEVER);
  hud_controller_->multiple_launchers = (optionGetNumLaunchers() == 0);
  hud_controller_->icon_size = launcher_controller_->options()->icon_size();
  hud_controller_->tile_size = launcher_controller_->options()->tile_size();
  AddChild(hud_controller_.get());
  LOG_INFO(logger) << "initLauncher-hud " << timer.ElapsedSeconds() << "s";

  // Setup Shortcut Hint
  auto base_window_raiser = std::make_shared<shortcut::BaseWindowRaiserImp>();
  auto shortcuts_modeller = std::make_shared<shortcut::CompizModeller>();
  shortcut_controller_ = std::make_shared<shortcut::Controller>(base_window_raiser, shortcuts_modeller);
  AddChild(shortcut_controller_.get());

  // Setup Session Controller
  auto manager = std::make_shared<session::GnomeManager>();
  session_controller_ = std::make_shared<session::Controller>(manager);
  AddChild(session_controller_.get());

  launcher_controller_->launcher().size_changed.connect([this] (nux::Area*, int w, int h) {
    /* The launcher geometry includes 1px used to draw the right margin
     * that must not be considered when drawing an overlay */
    int launcher_width = w - 1;
    hud_controller_->launcher_width = launcher_width;
    dash_controller_->launcher_width = launcher_width;
    panel_controller_->launcher_width = launcher_width;
    shortcut_controller_->SetAdjustment(launcher_width, panel_style_.panel_height);
  });

  ScheduleRelayout(0);
}

switcher::Controller::Ptr UnityScreen::switcher_controller()
{
  return switcher_controller_;
}

launcher::Controller::Ptr UnityScreen::launcher_controller()
{
  return launcher_controller_;
}

void UnityScreen::InitGesturesSupport()
{
  std::unique_ptr<nux::GestureBroker> gesture_broker(new UnityGestureBroker);
  wt->GetWindowCompositor().SetGestureBroker(std::move(gesture_broker));

  gestures_sub_launcher_.reset(new nux::GesturesSubscription);
  gestures_sub_launcher_->SetGestureClasses(nux::DRAG_GESTURE);
  gestures_sub_launcher_->SetNumTouches(4);
  gestures_sub_launcher_->SetWindowId(GDK_ROOT_WINDOW());
  gestures_sub_launcher_->Activate();

  gestures_sub_dash_.reset(new nux::GesturesSubscription);
  gestures_sub_dash_->SetGestureClasses(nux::TAP_GESTURE);
  gestures_sub_dash_->SetNumTouches(4);
  gestures_sub_dash_->SetWindowId(GDK_ROOT_WINDOW());
  gestures_sub_dash_->Activate();

  gestures_sub_windows_.reset(new nux::GesturesSubscription);
  gestures_sub_windows_->SetGestureClasses(nux::TOUCH_GESTURE
                                         | nux::DRAG_GESTURE
                                         | nux::PINCH_GESTURE);
  gestures_sub_windows_->SetNumTouches(3);
  gestures_sub_windows_->SetWindowId(GDK_ROOT_WINDOW());
  gestures_sub_windows_->Activate();
}

/* Window init */
GLTexture::List UnityWindow::close_normal_tex_;
GLTexture::List UnityWindow::close_prelight_tex_;
GLTexture::List UnityWindow::close_pressed_tex_;
GLTexture::List UnityWindow::glow_texture_;

struct UnityWindow::PixmapTexture
{
  PixmapTexture(unsigned width, unsigned height)
    : w_(width)
    , h_(height)
    , pixmap_(XCreatePixmap(screen->dpy(), screen->root(), w_, h_, 32))
    , texture_(GLTexture::bindPixmapToTexture(pixmap_, w_, h_, 32))
  {}

  ~PixmapTexture()
  {
    texture_.clear();

    if (pixmap_)
      XFreePixmap(screen->dpy(), pixmap_);
  }

  unsigned w_;
  unsigned h_;
  Pixmap pixmap_;
  GLTexture::List texture_;
};

struct UnityWindow::CairoContext
{
  CairoContext(unsigned width, unsigned height)
    : w_(width)
    , h_(height)
    , pixmap_texture_(std::make_shared<PixmapTexture>(w_, h_))
    , surface_(nullptr)
    , cr_(nullptr)
  {
    Screen *xscreen = ScreenOfDisplay(screen->dpy(), screen->screenNum());
    XRenderPictFormat* format = XRenderFindStandardFormat(screen->dpy(), PictStandardARGB32);
    surface_ = cairo_xlib_surface_create_with_xrender_format(screen->dpy(),
                                                             pixmap_texture_->pixmap_,
                                                             xscreen, format,
                                                             w_, h_);
    cr_ = cairo_create(surface_);

    // clear
    cairo_save(cr_);
    cairo_set_operator(cr_, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr_);
    cairo_restore(cr_);
  }

  ~CairoContext()
  {
    if (cr_)
      cairo_destroy(cr_);

    if (surface_)
      cairo_surface_destroy(surface_);
  }

  unsigned w_;
  unsigned h_;
  PixmapTexturePtr pixmap_texture_;
  cairo_surface_t* surface_;
  cairo_t *cr_;
};

namespace
{
bool WindowHasInconsistentShapeRects (Display *d,
                                      Window  w)
{
  int n;
  Atom *atoms = XListProperties(d, w, &n);
  Atom unity_shape_rects_atom = XInternAtom (d, "_UNITY_SAVED_WINDOW_SHAPE", FALSE);
  bool has_inconsistent_shape = false;

  for (int i = 0; i < n; ++i)
    if (atoms[i] == unity_shape_rects_atom)
      has_inconsistent_shape = true;

  XFree (atoms);
  return has_inconsistent_shape;
}
}

UnityWindow::UnityWindow(CompWindow* window)
  : BaseSwitchWindow (dynamic_cast<BaseSwitchScreen *> (UnityScreen::get (screen)), window)
  , PluginClassHandler<UnityWindow, CompWindow>(window)
  , window(window)
  , cWindow(CompositeWindow::get(window))
  , gWindow(GLWindow::get(window))
  , is_nux_window_(isNuxWindow(window))
{
  WindowInterface::setHandler(window);
  GLWindowInterface::setHandler(gWindow);
  ScaleWindowInterface::setHandler (ScaleWindow::get (window));

  PluginAdapter::Default().OnLeaveDesktop();

  /* This needs to happen before we set our wrapable functions, since we
   * need to ask core (and not ourselves) whether or not the window is
   * minimized */
  if (UnityScreen::get (screen)->optionGetShowMinimizedWindows () &&
      window->mapNum () &&
      WindowHasInconsistentShapeRects (screen->dpy (),
                                       window->id ()))
  {
    /* Query the core function */
    window->minimizedSetEnabled (this, false);

    bool wasMinimized = window->minimized ();
    if (wasMinimized)
      window->unminimize ();
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

  /* Keep this after the optionGetShowMinimizedWindows branch */

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


void UnityWindow::AddProperties(GVariantBuilder* builder)
{
  Window xid = window->id();
  auto const& swins = ScaleScreen::get(screen)->getWindows();
  bool scaled = std::find(swins.begin(), swins.end(), ScaleWindow::get(window)) != swins.end();
  WindowManager& wm = WindowManager::Default();

  variant::BuilderWrapper(builder)
    .add(scaled ? GetScaledGeometry() : wm.GetWindowGeometry(xid))
    .add("xid", (uint64_t)xid)
    .add("title", wm.GetWindowName(xid))
    .add("fake_decorated", uScreen->fake_decorated_windows_.find(this) != uScreen->fake_decorated_windows_.end())
    .add("scaled", scaled)
    .add("scaled_close_x", close_button_geo_.x)
    .add("scaled_close_y", close_button_geo_.y)
    .add("scaled_close_width", close_button_geo_.width)
    .add("scaled_close_height", close_button_geo_.height);
}

std::string UnityWindow::GetName() const
{
  return "Window";
}


void UnityWindow::DrawTexture(GLTexture::List const& textures,
                              GLWindowPaintAttrib const& attrib,
                              GLMatrix const& transform,
                              unsigned int mask,
                              int x, int y,
                              double scale)
{
  for (auto const& texture : textures)
  {
    if (!texture)
      continue;

    gWindow->vertexBuffer()->begin();

    if (texture->width() && texture->height())
    {
      GLTexture::MatrixList ml(1);
      ml[0] = texture->matrix();
      CompRegion texture_region(0, 0, texture->width(), texture->height());
      gWindow->glAddGeometry(ml, texture_region, texture_region);
    }

    if (gWindow->vertexBuffer()->end())
    {
      GLMatrix wTransform(transform);
      wTransform.translate(x, y, 0.0f);
      wTransform.scale(scale, scale, 1.0f);

      gWindow->glDrawTexture(texture, wTransform, attrib, mask);
    }
  }
}

void UnityWindow::RenderDecoration(CairoContext const& context, double aspect)
{
  cairo_save(context.cr_);

  // Draw window decoration based on gtk style
  cairo_push_group(context.cr_);
  auto& style = panel::Style::Instance();
  gtk_render_background(style.GetStyleContext(), context.cr_, 0, 0, context.w_, context.h_);
  gtk_render_frame(style.GetStyleContext(), context.cr_, 0, 0, context.w_, context.h_);
  cairo_pop_group_to_source(context.cr_);

  // Round window decoration top border
  const double radius = win::decoration::RADIUS * aspect;

  cairo_new_sub_path(context.cr_);
  cairo_line_to(context.cr_, 0, context.h_);
  cairo_arc(context.cr_, radius, radius, radius, M_PI, -M_PI * 0.5f);
  cairo_line_to(context.cr_, context.w_ - radius, 0);
  cairo_arc(context.cr_, context.w_ - radius, radius, radius, M_PI * 0.5f, 0);
  cairo_line_to(context.cr_, context.w_, context.h_);
  cairo_close_path(context.cr_);

  cairo_fill(context.cr_);

  cairo_restore(context.cr_);
}

void UnityWindow::RenderText(CairoContext const& context, int x, int y, int width, int height)
{
  panel::Style& style = panel::Style::Instance();
  std::string const& font_desc = style.GetFontDescription(panel::PanelItem::TITLE);

  glib::Object<PangoLayout> layout(pango_cairo_create_layout(context.cr_));
  std::shared_ptr<PangoFontDescription> font(pango_font_description_from_string(font_desc.c_str()),
                                             pango_font_description_free);

  pango_layout_set_font_description(layout, font.get());

  GdkScreen* gdk_screen = gdk_screen_get_default();
  PangoContext* pango_ctx = pango_layout_get_context(layout);
  int dpi = style.GetTextDPI();

  pango_cairo_context_set_font_options(pango_ctx, gdk_screen_get_font_options(gdk_screen));
  pango_cairo_context_set_resolution(pango_ctx, dpi / static_cast<float>(PANGO_SCALE));
  pango_layout_context_changed(layout);

  decoration_title_ = WindowManager::Default().GetWindowName(window->id());

  pango_layout_set_height(layout, height);
  pango_layout_set_width(layout, -1); //avoid wrap lines
  pango_layout_set_auto_dir(layout, false);
  pango_layout_set_text(layout, decoration_title_.c_str(), -1);

  /* update the size of the pango layout */
  pango_cairo_update_layout(context.cr_, layout);

  GtkStyleContext* style_context = style.GetStyleContext();
  gtk_style_context_save(style_context);
  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUBAR);
  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUITEM);

  // alignment
  PangoRectangle lRect;
  pango_layout_get_extents(layout, nullptr, &lRect);
  int text_width = lRect.width / PANGO_SCALE;
  int text_height = lRect.height / PANGO_SCALE;
  int text_space = width - x;
  y += (height - text_height) / 2.0f;
  if (text_width > text_space)
  {
    // Cut the text with fade
    int out_pixels = text_width - text_space;
    const int fading_pixels = 35;
    int fading_width = (out_pixels < fading_pixels) ? out_pixels : fading_pixels;

    cairo_push_group(context.cr_);
    gtk_render_layout(style_context, context.cr_, x, y, layout);
    cairo_pop_group_to_source(context.cr_);

    std::shared_ptr<cairo_pattern_t> linpat(cairo_pattern_create_linear(width - fading_width, y, width, y),
                                            cairo_pattern_destroy);
    cairo_pattern_add_color_stop_rgba(linpat.get(), 0, 0, 0, 0, 1);
    cairo_pattern_add_color_stop_rgba(linpat.get(), 1, 0, 0, 0, 0);
    cairo_mask(context.cr_, linpat.get());
  }
  else
  {
    gtk_render_layout(style_context, context.cr_, x, y, layout);
  }

  gtk_style_context_restore(style_context);
}

void UnityWindow::BuildDecorationTexture()
{
  if (decoration_tex_)
    return;

  auto& wm = WindowManager::Default();
  auto const& deco_size = wm.GetWindowDecorationSize(window->id(), WindowManager::Edge::TOP);

  if (deco_size.height)
  {
    CairoContext context(deco_size.width, deco_size.height);
    RenderDecoration(context);
    decoration_tex_ = context.pixmap_texture_;
  }
}

void UnityWindow::LoadCloseIcon(panel::WindowState state, GLTexture::List& texture)
{
  if (!texture.empty())
    return;

  CompString plugin("unityshell");
  auto& style = panel::Style::Instance();
  auto const& files = style.GetWindowButtonFileNames(panel::WindowButtonType::CLOSE, state);

  for (std::string const& file : files)
  {
    CompString file_name = file;
    CompSize size(win::decoration::CLOSE_SIZE, win::decoration::CLOSE_SIZE);
    texture = GLTexture::readImageToTexture(file_name, plugin, size);
    if (!texture.empty())
      break;
  }

  if (texture.empty())
  {
    std::string suffix;
    if (state == panel::WindowState::PRELIGHT)
      suffix = "_prelight";
    else if (state == panel::WindowState::PRESSED)
      suffix = "_pressed";

    CompString file_name(PKGDATADIR"/close_dash" + suffix + ".png");
    CompSize size(win::decoration::CLOSE_SIZE, win::decoration::CLOSE_SIZE);
    texture = GLTexture::readImageToTexture(file_name, plugin, size);
  }
}

void UnityWindow::SetupSharedTextures()
{
  LoadCloseIcon(panel::WindowState::NORMAL, close_normal_tex_);
  LoadCloseIcon(panel::WindowState::PRELIGHT, close_prelight_tex_);
  LoadCloseIcon(panel::WindowState::PRESSED, close_pressed_tex_);

  if (glow_texture_.empty())
  {
    CompSize size(texture::GLOW_SIZE, texture::GLOW_SIZE);
    glow_texture_ = GLTexture::imageDataToTexture(texture::GLOW, size, GL_RGBA, GL_UNSIGNED_BYTE);
  }
}

void UnityWindow::CleanupSharedTextures()
{
  close_normal_tex_.clear();
  close_prelight_tex_.clear();
  close_pressed_tex_.clear();
  glow_texture_.clear();
}

void UnityWindow::CleanupCachedTextures()
{
  decoration_tex_.reset();
  decoration_selected_tex_.reset();
  decoration_title_.clear();
}

void UnityWindow::paintFakeDecoration(nux::Geometry const& geo, GLWindowPaintAttrib const& attrib, GLMatrix const& transform, unsigned int mask, bool highlighted, double scale)
{
  mask |= PAINT_WINDOW_BLEND_MASK;

  if (!highlighted)
  {
    BuildDecorationTexture();

    if (decoration_tex_)
      DrawTexture(decoration_tex_->texture_, attrib, transform, mask, geo.x, geo.y, scale);

    close_button_geo_.Set(0, 0, 0, 0);
  }
  else
  {
    Window xid = window->id();
    auto& wm = WindowManager::Default();
    auto const& deco_top = wm.GetWindowDecorationSize(xid, WindowManager::Edge::TOP);
    unsigned width = geo.width;
    unsigned height = deco_top.height;
    bool redraw_decoration = true;

    if (decoration_selected_tex_)
    {
      if (decoration_selected_tex_->w_ == width && decoration_selected_tex_->h_ == height)
      {
        if (decoration_title_ == wm.GetWindowName(xid))
          redraw_decoration = false;
      }
    }

    if (redraw_decoration)
    {
      if (width != 0 && height != 0)
      {
        CairoContext context(width, height);
        RenderDecoration(context, scale);

        // Draw window title
        int text_x = win::decoration::ITEMS_PADDING * 2 + win::decoration::CLOSE_SIZE;
        RenderText(context, text_x, 0.0, width - win::decoration::ITEMS_PADDING, height);
        decoration_selected_tex_ = context.pixmap_texture_;
        uScreen->damageRegion(CompRegion(geo.x, geo.y, width, height));
      }
      else
      {
        decoration_selected_tex_.reset();
        redraw_decoration = false;
      }
    }

    if (decoration_selected_tex_)
      DrawTexture(decoration_selected_tex_->texture_, attrib, transform, mask, geo.x, geo.y);

    int x = geo.x + win::decoration::ITEMS_PADDING;
    int y = geo.y + (height - win::decoration::CLOSE_SIZE) / 2.0f;

    switch (close_icon_state_)
    {
      case panel::WindowState::NORMAL:
      default:
        DrawTexture(close_normal_tex_, attrib, transform, mask, x, y);
        break;

      case panel::WindowState::PRELIGHT:
        DrawTexture(close_prelight_tex_, attrib, transform, mask, x, y);
        break;

      case panel::WindowState::PRESSED:
        DrawTexture(close_pressed_tex_, attrib, transform, mask, x, y);
        break;
    }

    close_button_geo_.Set(x, y, win::decoration::CLOSE_SIZE, win::decoration::CLOSE_SIZE);
  }

  uScreen->fake_decorated_windows_.insert(this);
}

void UnityWindow::scalePaintDecoration(GLWindowPaintAttrib const& attrib,
                                       GLMatrix const& transform,
                                       CompRegion const& region,
                                       unsigned int mask)
{
  ScaleWindow* scale_win = ScaleWindow::get(window);
  scale_win->scalePaintDecoration(attrib, transform, region, mask);

  if (!scale_win->hasSlot()) // animation not finished
    return;

  ScaleScreen* ss = ScaleScreen::get(screen);
  auto state = ss->getState();

  if (state != ScaleScreen::Wait && state != ScaleScreen::Out)
    return;

  nux::Geometry const& scale_geo = GetScaledGeometry();

  auto const& pos = scale_win->getCurrentPosition();

  bool highlighted = (ss->getSelectedWindow() == window->id());
  paintFakeDecoration(scale_geo, attrib, transform, mask, highlighted, pos.scale);
}

nux::Geometry UnityWindow::GetLayoutWindowGeometry()
{
  auto const& layout_window = UnityScreen::get(screen)->GetSwitcherDetailLayoutWindow(window->id());

  if (layout_window)
    return layout_window->result;

  return nux::Geometry();
}

nux::Geometry UnityWindow::GetScaledGeometry()
{
  WindowManager& wm = WindowManager::Default();

  if (!wm.IsScaleActive())
    return nux::Geometry();

  ScaleWindow* scale_win = ScaleWindow::get(window);

  ScalePosition const& pos = scale_win->getCurrentPosition();
  auto const& border_rect = window->borderRect();
  auto const& deco_ext = window->border();

  const unsigned width = std::round(border_rect.width() * pos.scale);
  const unsigned height = std::round(border_rect.height() * pos.scale);
  const int x = pos.x() + window->x() - std::round(deco_ext.left * pos.scale);
  const int y = pos.y() + window->y() - std::round(deco_ext.top * pos.scale);

  return nux::Geometry(x, y, width, height);
}

void UnityWindow::OnInitiateSpread()
{
  close_icon_state_ = panel::WindowState::NORMAL;
  middle_clicked_ = false;

  WindowManager& wm = WindowManager::Default();
  Window xid = window->id();

  if (wm.HasWindowDecorations(xid))
    wm.Decorate(xid);
}

void UnityWindow::OnTerminateSpread()
{
  WindowManager& wm = WindowManager::Default();
  Window xid = window->id();

  if (wm.IsWindowMaximized(xid))
    wm.Undecorate(xid);

  CleanupCachedTextures();
}

void UnityWindow::paintInnerGlow(nux::Geometry glow_geo, GLMatrix const& matrix, GLWindowPaintAttrib const& attrib, unsigned mask)
{
  unsigned glow_size = win::decoration::GLOW;

  if (!glow_size)
    return;

  if (win::decoration::RADIUS > 0)
  {
    // We paint the glow below the window edges to correctly
    // render the rounded corners
    glow_size += win::decoration::RADIUS;
    int inside_glow = win::decoration::RADIUS / 4;
    glow_geo.Expand(-inside_glow, -inside_glow);
  }

  glow::Quads const& quads = computeGlowQuads(glow_geo, glow_texture_, glow_size);
  paintGlow(matrix, attrib, quads, glow_texture_, win::decoration::GLOW_COLOR, mask);
}

void UnityWindow::paintThumbnail(nux::Geometry const& geo, float alpha, float parent_alpha, float scale_ratio, unsigned deco_height, bool selected)
{
  GLMatrix matrix;
  matrix.toScreenSpace(UnityScreen::get(screen)->_last_output, -DEFAULT_Z_CAMERA);
  last_bound = geo;

  GLWindowPaintAttrib attrib = gWindow->lastPaintAttrib();
  attrib.opacity = (alpha * parent_alpha * G_MAXUSHORT);
  unsigned mask = gWindow->lastMask();
  nux::Geometry thumb_geo = geo;

  if (selected)
    paintInnerGlow(thumb_geo, matrix, attrib, mask);

  thumb_geo.y += std::round(deco_height * 0.5f * scale_ratio);
  nux::Geometry const& g = thumb_geo;

  paintThumb(attrib, matrix, mask, g.x, g.y, g.width, g.height, g.width, g.height);

  mask |= PAINT_WINDOW_BLEND_MASK;
  attrib.opacity = parent_alpha * G_MAXUSHORT;

  // The thumbnail is still animating, don't draw the decoration as selected
  if (selected && alpha < 1.0f)
    selected = false;

  paintFakeDecoration(geo, attrib, matrix, mask, selected, scale_ratio);
}

UnityWindow::~UnityWindow()
{
  if (uScreen->newFocusedWindow && UnityWindow::get(uScreen->newFocusedWindow) == this)
    uScreen->newFocusedWindow = NULL;

  if (!window->destroyed ())
  {
    bool wasMinimized = window->minimized ();
    if (wasMinimized)
      window->unminimize ();
    window->focusSetEnabled (this, false);
    window->minimizeSetEnabled (this, false);
    window->unminimizeSetEnabled (this, false);
    if (wasMinimized)
      window->minimize ();
  }

  ShowdesktopHandler::animating_windows.remove (static_cast <ShowdesktopHandlerWindowInterface *> (this));

  if (window->state () & CompWindowStateFullscreenMask)
    uScreen->fullscreen_windows_.remove(window);

  uScreen->fake_decorated_windows_.erase(this);
  PluginAdapter::Default().OnWindowClosed(window);
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

  /*
   * GTK needs to be initialized or else unity's gdk/gtk calls will crash.
   * This is already done in compiz' main() if using ubuntu packages, but not
   * if you're using the regular (upstream) compiz.
   * Admittedly this is the same as what the "gtkloader" plugin does. But it
   * is faster, more efficient (one less plugin in memory), and more reliable
   * to do the init here where its needed. And yes, init'ing multiple times is
   * safe, and does nothing after the first init.
   */
  if (!gtk_init_check(&programArgc, &programArgv))
  {
    compLogMessage("unityshell", CompLogLevelError, "GTK init failed\n");
    return false;
  }

  return true;
}

namespace debug
{

ScreenIntrospection::ScreenIntrospection(CompScreen* screen)
  : screen_(screen)
{}

std::string ScreenIntrospection::GetName() const
{
  return "Screen";
}

void ScreenIntrospection::AddProperties(GVariantBuilder* builder)
{}

Introspectable::IntrospectableList ScreenIntrospection::GetIntrospectableChildren()
{
  IntrospectableList children;

  for (auto const& win : screen_->windows())
    children.push_back(UnityWindow::get(win));

  return children;
}

} // debug namespace

namespace
{

void reset_glib_logging()
{
  // Reinstate the default glib logger.
  g_log_set_default_handler(g_log_default_handler, NULL);
}

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

  for (i = 0; isdigit(version_string[i]); ++i)
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
  // If the environment variable is set, we capture the backtrace.
  static bool glog_backtrace = ::getenv("UNITY_LOG_GLOG_BACKTRACE");
  // If nothing else, all log messages from unity should be identified as such
  std::string module("unity");
  if (log_domain)
  {
    module += std::string(".") + log_domain;
  }
  nux::logging::Logger logger(module);
  nux::logging::Level level = glog_level_to_nux(log_level);
  if (level >= logger.GetEffectiveLogLevel())
  {
    std::string backtrace;
    if (glog_backtrace && level >= nux::logging::Warning)
    {
      backtrace = "\n" + nux::logging::Backtrace();
    }
    nux::logging::LogStream(level, logger.module(), "<unknown>", 0).stream()
      << message << backtrace;
  }
}

} // anonymous namespace
} // namespace unity

