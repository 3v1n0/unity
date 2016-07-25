// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/* Compiz unity plugin
 * unityshell.cpp
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

#include <UnityCore/DBusIndicators.h>
#include <UnityCore/DesktopUtilities.h>
#include <UnityCore/GnomeSessionManager.h>
#include <UnityCore/ScopeProxyInterface.h>

#include "CompizUtils.h"
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
#include "ThemeSettings.h"
#include "Timer.h"
#include "XKeyboardUtil.h"
#include "unityshell.h"
#include "BackgroundEffectHelper.h"
#include "UnityGestureBroker.h"
#include "launcher/XdndCollectionWindowImp.h"
#include "launcher/XdndManagerImp.h"
#include "launcher/XdndStartStopNotifierImp.h"
#include "CompizShortcutModeller.h"
#include "GnomeKeyGrabber.h"
#include "RawPixel.h"

#include "decorations/DecorationsDataPool.h"
#include "decorations/DecorationsManager.h"

#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <libnotify/notify.h>
#include <cairo-xlib-xrender.h>

#include <text/text.h>

#include <sstream>
#include <memory>

#include <core/atoms.h>

#include "a11y/unitya11y.h"
#include "UBusMessages.h"
#include "UBusWrapper.h"
#include "UScreen.h"

#include "config.h"
#include "unity-shared/UnitySettings.h"

/* FIXME: once we get a better method to add the toplevel windows to
   the accessible root object, this include would not be required */
#include "a11y/unity-util-accessible.h"

/* Set up vtable symbols */
COMPIZ_PLUGIN_20090315(unityshell, unity::UnityPluginVTable);

static void save_state()
{
#ifndef USE_GLES
  glPushAttrib(GL_ALL_ATTRIB_BITS);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
#endif
}

static void restore_state()
{
#ifndef USE_GLES
  glMatrixMode(GL_TEXTURE);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  glPopAttrib();
#endif
}

namespace cgl = compiz::opengl;

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
UnityScreen* uScreen = nullptr;

void reset_glib_logging();
void configure_logging();
void capture_g_log_calls(const gchar* log_domain,
                         GLogLevelFlags log_level,
                         const gchar* message,
                         gpointer user_data);

#ifndef USE_GLES
gboolean is_extension_supported(const gchar* extensions, const gchar* extension);
gfloat get_opengl_version_f32(const gchar* version_string);
#endif

inline CompRegion CompRegionFromNuxGeo(nux::Geometry const& geo)
{
  return CompRegion(geo.x, geo.y, geo.width, geo.height);
}

inline CompRect CompRectFromNuxGeo(nux::Geometry const& geo)
{
  return CompRect(geo.x, geo.y, geo.width, geo.height);
}

inline nux::Geometry NuxGeometryFromCompRect(CompRect const& rect)
{
  return nux::Geometry(rect.x(), rect.y(), rect.width(), rect.height());
}

inline nux::Color NuxColorFromCompizColor(unsigned short* color)
{
  return nux::Color(color[0]/65535.0f, color[1]/65535.0f, color[2]/65535.0f, color[3]/65535.0f);
}

namespace local
{
// Tap duration in milliseconds.
const int ALT_TAP_DURATION = 250;
const unsigned int SCROLL_DOWN_BUTTON = 6;
const unsigned int SCROLL_UP_BUTTON = 7;
const int MAX_BUFFER_AGE = 11;
const int FRAMES_TO_REDRAW_ON_RESUME = 10;
const RawPixel SCALE_PADDING = 40_em;
const RawPixel SCALE_SPACING = 20_em;
const std::string RELAYOUT_TIMEOUT = "relayout-timeout";
const std::string HUD_UNGRAB_WAIT = "hud-ungrab-wait";
const std::string FIRST_RUN_STAMP = "first_run.stamp";
const std::string LOCKED_STAMP = "locked.stamp";
} // namespace local

namespace atom
{
Atom _UNITY_SHELL = 0;
Atom _UNITY_SAVED_WINDOW_SHAPE = 0;
}

} // anon namespace

UnityScreen::UnityScreen(CompScreen* screen)
  : BaseSwitchScreen(screen)
  , PluginClassHandler <UnityScreen, CompScreen>(screen)
  , screen(screen)
  , cScreen(CompositeScreen::get(screen))
  , gScreen(GLScreen::get(screen))
  , sScreen(ScaleScreen::get(screen))
  , WM(PluginAdapter::Initialize(screen))
  , menus_(std::make_shared<menu::Manager>(std::make_shared<indicator::DBusIndicators>(), std::make_shared<key::GnomeGrabber>()))
  , deco_manager_(std::make_shared<decoration::Manager>(menus_))
  , debugger_(this)
  , needsRelayout(false)
  , super_keypressed_(false)
  , newFocusedWindow(nullptr)
  , doShellRepaint(false)
  , didShellRepaint(false)
  , allowWindowPaint(false)
  , last_output_(nullptr)
  , force_draw_countdown_(0)
  , firstWindowAboveShell(nullptr)
  , onboard_(nullptr)
  , grab_index_(0)
  , painting_tray_ (false)
  , last_scroll_event_(0)
  , hud_keypress_time_(0)
  , first_menu_keypress_time_(0)
  , paint_panel_under_dash_(false)
  , scale_just_activated_(false)
  , screen_introspection_(screen)
  , ignore_redraw_request_(false)
  , dirty_helpers_on_this_frame_(false)
  , is_desktop_active_(false)
  , key_nav_mode_requested_(false)
  , big_tick_(0)
  , back_buffer_age_(0)
  , next_active_window_(0)
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
      renderer.find("llvmpipe") != std::string::npos ||
      renderer.find("softpipe") != std::string::npos ||
      (getenv("UNITY_LOW_GFX_MODE") != NULL && atoi(getenv("UNITY_LOW_GFX_MODE")) == 1) ||
       optionGetLowGraphicsMode())
    {
      unity_settings_.low_gfx = true;
    }

  if (getenv("UNITY_LOW_GFX_MODE") != NULL && atoi(getenv("UNITY_LOW_GFX_MODE")) == 0)
  {
    unity_settings_.low_gfx = false;
  }
#endif

  if (!failed)
  {
     notify_init("unityshell");

     XSetErrorHandler(old_handler);

     /* Wrap compiz interfaces */
     ScreenInterface::setHandler(screen);
     CompositeScreenInterface::setHandler(cScreen);
     GLScreenInterface::setHandler(gScreen);
     ScaleScreenInterface::setHandler(sScreen);

     atom::_UNITY_SHELL = XInternAtom(screen->dpy(), "_UNITY_SHELL", False);
     atom::_UNITY_SAVED_WINDOW_SHAPE = XInternAtom(screen->dpy(), "_UNITY_SAVED_WINDOW_SHAPE", False);
     screen->updateSupportedWmHints();

     nux::NuxInitialize(0);
#ifndef USE_GLES
     wt.reset(nux::CreateFromForeignWindow(cScreen->output(),
                                           glXGetCurrentContext(),
                                           &UnityScreen::InitNuxThread,
                                           this));
#else
     wt.reset(nux::CreateFromForeignWindow(cScreen->output(),
                                           eglGetCurrentContext(),
                                           &UnityScreen::InitNuxThread,
                                           this));
#endif

    tick_source_.reset(new na::TickSource);
    animation_controller_.reset(new na::AnimationController(*tick_source_));

     wt->RedrawRequested.connect(sigc::mem_fun(this, &UnityScreen::OnRedrawRequested));

     unity_a11y_init(wt.get());

     /* i18n init */
     bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
     bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

     wt->Run(NULL);
     uScreen = this;

     optionSetShowMenuBarInitiate(boost::bind(&UnityScreen::showMenuBarInitiate, this, _1, _2, _3));
     optionSetShowMenuBarTerminate(boost::bind(&UnityScreen::showMenuBarTerminate, this, _1, _2, _3));
     optionSetLockScreenInitiate(boost::bind(&UnityScreen::LockScreenInitiate, this, _1, _2, _3));
     optionSetOverrideDecorationThemeNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetShadowXOffsetNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetShadowYOffsetNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetActiveShadowRadiusNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetInactiveShadowRadiusNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetActiveShadowColorNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetInactiveShadowColorNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
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
     optionSetLowGraphicsModeNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetShowLauncherInitiate(boost::bind(&UnityScreen::showLauncherKeyInitiate, this, _1, _2, _3));
     optionSetShowLauncherTerminate(boost::bind(&UnityScreen::showLauncherKeyTerminate, this, _1, _2, _3));
     optionSetKeyboardFocusInitiate(boost::bind(&UnityScreen::setKeyboardFocusKeyInitiate, this, _1, _2, _3));
     //optionSetKeyboardFocusTerminate (boost::bind (&UnityScreen::setKeyboardFocusKeyTerminate, this, _1, _2, _3));
     optionSetExecuteCommandInitiate(boost::bind(&UnityScreen::executeCommand, this, _1, _2, _3));
     optionSetShowDesktopKeyInitiate(boost::bind(&UnityScreen::showDesktopKeyInitiate, this, _1, _2, _3));
     optionSetPanelFirstMenuInitiate(boost::bind(&UnityScreen::showPanelFirstMenuKeyInitiate, this, _1, _2, _3));
     optionSetPanelFirstMenuTerminate(boost::bind(&UnityScreen::showPanelFirstMenuKeyTerminate, this, _1, _2, _3));
     optionSetPanelFirstMenuNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetSpreadAppWindowsInitiate(boost::bind(&UnityScreen::spreadAppWindowsInitiate, this, _1, _2, _3));
     optionSetSpreadAppWindowsAnywhereInitiate(boost::bind(&UnityScreen::spreadAppWindowsAnywhereInitiate, this, _1, _2, _3));
     optionSetAutomaximizeValueNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetDashTapDurationNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetAltTabTimeoutNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetAltTabBiasViewportNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetSwitchStrictlyBetweenApplicationsNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetDisableShowDesktopNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetDisableMouseNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));

     optionSetAltTabForwardAllInitiate(boost::bind(&UnityScreen::altTabForwardAllInitiate, this, _1, _2, _3));
     optionSetAltTabForwardInitiate(boost::bind(&UnityScreen::altTabForwardInitiate, this, _1, _2, _3));
     optionSetAltTabForwardTerminate(boost::bind(&UnityScreen::altTabTerminateCommon, this, _1, _2, _3));
     optionSetAltTabForwardAllTerminate(boost::bind(&UnityScreen::altTabTerminateCommon, this, _1, _2, _3));
     optionSetAltTabPrevAllInitiate(boost::bind(&UnityScreen::altTabPrevAllInitiate, this, _1, _2, _3));
     optionSetAltTabPrevInitiate(boost::bind(&UnityScreen::altTabPrevInitiate, this, _1, _2, _3));
     optionSetAltTabNextWindowInitiate(boost::bind(&UnityScreen::altTabNextWindowInitiate, this, _1, _2, _3));
     optionSetAltTabNextWindowTerminate(boost::bind(&UnityScreen::altTabTerminateCommon, this, _1, _2, _3));
     optionSetAltTabPrevWindowInitiate(boost::bind(&UnityScreen::altTabPrevWindowInitiate, this, _1, _2, _3));

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

     optionSetScrollInactiveIconsNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetLauncherMinimizeWindowNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));

     ubus_manager_.RegisterInterest(UBUS_LAUNCHER_START_KEY_NAV,
                   sigc::mem_fun(this, &UnityScreen::OnLauncherStartKeyNav));

     ubus_manager_.RegisterInterest(UBUS_LAUNCHER_START_KEY_SWITCHER,
                   sigc::mem_fun(this, &UnityScreen::OnLauncherStartKeyNav));

     ubus_manager_.RegisterInterest(UBUS_LAUNCHER_END_KEY_NAV,
                   sigc::mem_fun(this, &UnityScreen::OnLauncherEndKeyNav));

     ubus_manager_.RegisterInterest(UBUS_LAUNCHER_END_KEY_SWITCHER,
                   sigc::mem_fun(this, &UnityScreen::OnLauncherEndKeyNav));

     auto init_plugins_cb = sigc::mem_fun(this, &UnityScreen::InitPluginActions);
     sources_.Add(std::make_shared<glib::Idle>(init_plugins_cb, glib::Source::Priority::DEFAULT));

     Settings::Instance().gestures_changed.connect(sigc::mem_fun(this, &UnityScreen::UpdateGesturesSupport));
     InitGesturesSupport();

     LoadPanelShadowTexture();
     theme::Settings::Get()->theme.changed.connect(sigc::hide(sigc::mem_fun(this, &UnityScreen::LoadPanelShadowTexture)));

     ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN, [this](GVariant * data)
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

    UScreen::GetDefault()->resuming.connect([this] {
      /* Force paint local::FRAMES_TO_REDRAW_ON_RESUME frames on resume */
      force_draw_countdown_ += local::FRAMES_TO_REDRAW_ON_RESUME;
    });

    Introspectable::AddChild(deco_manager_.get());
    auto const& deco_style = decoration::Style::Get();
    auto deco_style_cb = sigc::hide(sigc::mem_fun(this, &UnityScreen::OnDecorationStyleChanged));
    deco_style->theme.changed.connect(deco_style_cb);
    deco_style->title_font.changed.connect(deco_style_cb);

    minimize_speed_controller_.DurationChanged.connect(
      sigc::mem_fun(this, &UnityScreen::OnMinimizeDurationChanged)
    );

    WM.initiate_spread.connect(sigc::mem_fun(this, &UnityScreen::OnInitiateSpread));
    WM.terminate_spread.connect(sigc::mem_fun(this, &UnityScreen::OnTerminateSpread));
    WM.initiate_expo.connect(sigc::mem_fun(this, &UnityScreen::DamagePanelShadow));
    WM.terminate_expo.connect(sigc::mem_fun(this, &UnityScreen::DamagePanelShadow));

    Introspectable::AddChild(&WM);
    Introspectable::AddChild(&screen_introspection_);

    /* Create blur backup texture */
    auto gpu_device = nux::GetGraphicsDisplay()->GetGpuDevice();
    gpu_device->backup_texture0_ =
        gpu_device->CreateSystemCapableDeviceTexture(screen->width(), screen->height(),
                                                     1, nux::BITFMT_R8G8B8A8,
                                                     NUX_TRACKER_LOCATION);

    auto const& blur_update_cb = sigc::mem_fun(this, &UnityScreen::DamageBlurUpdateRegion);
    BackgroundEffectHelper::blur_region_needs_update_.connect(blur_update_cb);

    /* Track whole damage on the very first frame */
    cScreen->damageScreen();
  }
}

UnityScreen::~UnityScreen()
{
  notify_uninit();

  unity_a11y_finalize();
  QuicklistManager::Destroy();
  decoration::DataPool::Reset();
  SaveLockStamp(false);
  reset_glib_logging();

  screen->addSupportedAtomsSetEnabled(this, false);
  screen->updateSupportedWmHints();
}

void UnityScreen::InitAltTabNextWindow()
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
  scale_just_activated_ = super_keypressed_;
  spread_filter_ = std::make_shared<spread::Filter>();
  spread_filter_->text.changed.connect([this] (std::string const& filter) {
    if (filter.empty())
    {
      sScreen->relayoutSlots(CompMatch::emptyMatch);
    }
    else
    {
      CompMatch windows_match;
      auto const& filtered_windows = spread_filter_->FilteredWindows();

      for (auto const& swin : sScreen->getWindows())
      {
        if (filtered_windows.find(swin->window->id()) != filtered_windows.end())
          continue;

        auto* uwin = UnityWindow::get(swin->window);
        uwin->OnTerminateSpread();
        fake_decorated_windows_.erase(uwin);
      }

      for (auto xid : filtered_windows)
        windows_match |= "xid="+std::to_string(xid);

      auto match = sScreen->getCustomMatch();
      match &= windows_match;
      sScreen->relayoutSlots(match);
    }
  });

  for (auto const& swin : sScreen->getWindows())
  {
    auto* uwin = UnityWindow::get(swin->window);
    fake_decorated_windows_.insert(uwin);
    uwin->OnInitiateSpread();
  }
}

void UnityScreen::OnTerminateSpread()
{
  spread_filter_.reset();

  for (auto const& swin : sScreen->getWindows())
    UnityWindow::get(swin->window)->OnTerminateSpread();

  fake_decorated_windows_.clear();
}

void UnityScreen::DamagePanelShadow()
{
  CompRect panelShadow;

  for (CompOutput const& output : screen->outputDevs())
  {
    FillShadowRectForOutput(panelShadow, output);
    cScreen->damageRegion(CompRegion(panelShadow));
  }
}

void UnityScreen::OnViewHidden(nux::BaseWindow *bw)
{
  /* Count this as regular damage */
  auto const& geo = bw->GetAbsoluteGeometry();
  cScreen->damageRegion(CompRegionFromNuxGeo(geo));
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
    CreateSuperNewAction(shortcut, impl::ActionModifiers::USE_SHIFT_NUMPAD);
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
#endif

  save_state();
  glGetError();
}

void UnityScreen::nuxEpilogue()
{
#ifndef USE_GLES
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  /* In some unknown place inside nux drawing we change the viewport without
   * setting it back to the default one, so we need to restore it before allowing
   * compiz to take the scene */
  auto* o = last_output_;
  glViewport(o->x(), screen->height() - o->y2(), o->width(), o->height());

  glDepthRange(0, 1);
#else
  glDepthRangef(0, 1);
#endif

  restore_state();

  gScreen->resetRasterPos();
  glDisable(GL_SCISSOR_TEST);
}

void UnityScreen::LoadPanelShadowTexture()
{
  CompString name(theme::Settings::Get()->ThemedFilePath("panel_shadow", {PKGDATADIR}));
  CompString pname;
  CompSize size;
  _shadow_texture = GLTexture::readImageToTexture(name, pname, size);
}

void UnityScreen::setPanelShadowMatrix(GLMatrix const& matrix)
{
  panel_shadow_matrix_ = matrix;
}

void UnityScreen::FillShadowRectForOutput(CompRect& shadowRect, CompOutput const& output)
{
  if (_shadow_texture.empty())
    return;

  int monitor = WM.MonitorGeometryIn(NuxGeometryFromCompRect(output));
  float panel_h = panel_style_.PanelHeight(monitor);
  float shadowX = output.x();
  float shadowY = output.y() + panel_h;
  float shadowWidth = output.width();
  float shadowHeight = _shadow_texture[0]->height() * unity_settings_.em(monitor)->DPIScale();
  shadowRect.setGeometry(shadowX, shadowY, shadowWidth, shadowHeight);
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

  if (WM.IsExpoActive())
    return;

  CompOutput* output = last_output_;

  if (fullscreenRegion.contains(*output))
    return;

  if (launcher_controller_->IsOverlayOpen())
  {
    auto const& uscreen = UScreen::GetDefault();
    if (uscreen->GetMonitorAtPosition(output->x(), output->y()) == overlay_monitor_)
      return;
  }

  CompRect shadowRect;
  FillShadowRectForOutput(shadowRect, *output);

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
      float tx1 = (x1 - shadowRect.x()) / shadowRect.width();
      float ty1 = (y1 - shadowRect.y()) / shadowRect.height();
      float tx2 = (x2 - shadowRect.x()) / shadowRect.width();
      float ty2 = (y2 - shadowRect.y()) / shadowRect.height();

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
UnityWindow::updateIconPos(int &wx, int &wy, int x, int y, float width, float height)
{
  wx = x + (last_bound.width - width) / 2;
  wy = y + (last_bound.height - height) / 2;
}

void UnityScreen::OnDecorationStyleChanged()
{
  for (UnityWindow* uwin : fake_decorated_windows_)
    uwin->CleanupCachedTextures();

  auto const& style = decoration::Style::Get();
  deco_manager_->shadow_offset = style->ShadowOffset();
  deco_manager_->active_shadow_color = style->ActiveShadowColor();
  deco_manager_->active_shadow_radius = style->ActiveShadowRadius();
  deco_manager_->inactive_shadow_color = style->InactiveShadowColor();
  deco_manager_->inactive_shadow_radius = style->InactiveShadowRadius();
}

void UnityScreen::DamageBlurUpdateRegion(nux::Geometry const& blur_update)
{
  cScreen->damageRegion(CompRegionFromNuxGeo(blur_update));
}

void UnityScreen::paintDisplay()
{
  CompOutput *output = last_output_;

  DrawPanelUnderDash();

  /* Bind the currently bound draw framebuffer to the read framebuffer binding.
   * The reason being that we want to use the results of nux images being
   * drawn to this framebuffer in glCopyTexSubImage2D operations */
  GLint current_draw_binding = 0,
        old_read_binding = 0;
#ifndef USE_GLES
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &old_read_binding);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_EXT, &current_draw_binding);
  if (old_read_binding != current_draw_binding)
    (*GL::bindFramebuffer) (GL_READ_FRAMEBUFFER_BINDING_EXT, current_draw_binding);
#else
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_read_binding);
  current_draw_binding = old_read_binding;
#endif

  BackgroundEffectHelper::monitor_rect_.Set(0, 0, screen->width(), screen->height());

  // If we have dirty helpers re-copy the backbuffer into a texture
  if (dirty_helpers_on_this_frame_)
  {
    /* We are using a CompRegion here so that we can combine rectangles
     * where it might make sense to. Saves calls into OpenGL */
    CompRegion blur_region;

    for (auto const& blur_geometry : BackgroundEffectHelper::GetBlurGeometries())
    {
      auto blur_rect = CompRectFromNuxGeo(blur_geometry);
      blur_region += (blur_rect & *output);
    }

    /* Copy from the read buffer into the backup texture */
    auto gpu_device = nux::GetGraphicsDisplay()->GetGpuDevice();
    GLuint backup_texture_id = gpu_device->backup_texture0_->GetOpenGLID();
    GLuint surface_target = gpu_device->backup_texture0_->GetSurfaceLevel(0)->GetSurfaceTarget();

    CHECKGL(glEnable(surface_target));
    CHECKGL(glBindTexture(surface_target, backup_texture_id));

    for (CompRect const& rect : blur_region.rects())
    {
      int x = nux::Clamp<int>(rect.x(), 0, screen->width());
      int y = nux::Clamp<int>(screen->height() - rect.y2(), 0, screen->height());
      int width = std::min<int>(screen->width() - rect.x(), rect.width());
      int height = std::min<int>(screen->height() - y, rect.height());

      CHECKGL(glCopyTexSubImage2D(surface_target, 0, x, y, x, y, width, height));
    }

    CHECKGL(glDisable(surface_target));

    back_buffer_age_ = 0;
  }

  nux::Geometry const& outputGeo = NuxGeometryFromCompRect(*output);
  wt->GetWindowCompositor().SetReferenceFramebuffer(current_draw_binding, old_read_binding, outputGeo);

  nuxPrologue();
  wt->RenderInterfaceFromForeignCmd(outputGeo);
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

        attrib.opacity = COMPIZ_COMPOSITE_OPAQUE;
        attrib.brightness = COMPIZ_COMPOSITE_BRIGHT;
        attrib.saturation = COMPIZ_COMPOSITE_COLOR;

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

  if (switcher_controller_->detail())
  {
    auto const& targets = switcher_controller_->ExternalRenderTargets();

    for (LayoutWindow::Ptr const& target : targets)
    {
      if (CompWindow* window = screen->findWindow(target->xid))
      {
        UnityWindow *unity_window = UnityWindow::get(window);
        double parent_alpha = switcher_controller_->Opacity();
        unity_window->paintThumbnail(target->result, target->alpha, parent_alpha, target->scale,
                                     target->decoration_height, target->selected);
      }
    }
  }

  doShellRepaint = false;
  didShellRepaint = true;
}

void UnityScreen::DrawPanelUnderDash()
{
  if (!paint_panel_under_dash_ || (!dash_controller_->IsVisible() && !hud_controller_->IsVisible()))
    return;

  auto const& output_dev = screen->currentOutputDev();

  if (last_output_->id() != output_dev.id())
    return;

  auto graphics_engine = nux::GetGraphicsDisplay()->GetGraphicsEngine();

  if (!graphics_engine->UsingGLSLCodePath())
    return;

  graphics_engine->ResetModelViewMatrixStack();
  graphics_engine->Push2DTranslationModelViewMatrix(0.0f, 0.0f, 0.0f);
  graphics_engine->ResetProjectionMatrix();
  graphics_engine->SetOrthographicProjectionMatrix(output_dev.width(), output_dev.height());

  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_CLAMP);

  int monitor = WM.MonitorGeometryIn(NuxGeometryFromCompRect(output_dev));
  auto const& texture = panel_style_.GetBackground(monitor)->GetDeviceTexture();
  graphics_engine->QRP_GLSL_1Tex(0, 0, output_dev.width(), texture->GetHeight(), texture, texxform, nux::color::White);
}

bool UnityScreen::forcePaintOnTop()
{
  if (!allowWindowPaint ||
      lockscreen_controller_->IsLocked() ||
      (dash_controller_->IsVisible() && !nux::GetGraphicsDisplay()->PointerIsGrabbed()) ||
      hud_controller_->IsVisible() ||
      session_controller_->Visible())
  {
    return true;
  }

  if (!fullscreen_windows_.empty())
  {
    if (menus_->menu_open())
      return true;

    if (switcher_controller_->Visible() || WM.IsExpoActive())
    {
      if (!screen->grabbed() || screen->otherGrabExist(nullptr))
        return true;
    }
  }

  return false;
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

  if (dash_controller_->IsVisible())
    dash_controller_->HideDash();
    
  if (hud_controller_->IsVisible())
    hud_controller_->HideHud();

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

  for (nux::Geometry const& panel_geo : panel_controller_->GetGeometries ())
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
  uScreen->SetNextActiveWindow(window->id());

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

bool UnityWindow::CanBypassLockScreen() const
{
  if (window->type() == CompWindowTypePopupMenuMask &&
      uScreen->lockscreen_controller_->HasOpenMenu())
  {
    return true;
  }

  if (window == uScreen->onboard_)
    return true;

  return false;
}

void UnityWindow::DoOverrideFrameRegion(CompRegion &region)
{
  unsigned int oldUpdateFrameRegionIndex = window->updateFrameRegionGetCurrentIndex();

  window->updateFrameRegionSetCurrentIndex(MAXSHORT);
  window->updateFrameRegion(region);
  window->updateFrameRegionSetCurrentIndex(oldUpdateFrameRegionIndex);
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

  window->updateFrameRegion();
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
      if (close_icon_state_ != decoration::WidgetState::PRESSED)
      {
        auto old_state = close_icon_state_;

        if (close_button_geo_.IsPointInside(event->xmotion.x_root, event->xmotion.y_root))
        {
          close_icon_state_ = decoration::WidgetState::PRELIGHT;
        }
        else
        {
          close_icon_state_ = decoration::WidgetState::NORMAL;
        }

        if (old_state != close_icon_state_)
        {
          cWindow->addDamageRect(CompRectFromNuxGeo(close_button_geo_));
        }
      }
      break;

    case ButtonPress:
      if (event->xbutton.button == Button1 &&
          close_button_geo_.IsPointInside(event->xbutton.x_root, event->xbutton.y_root))
      {
        close_icon_state_ = decoration::WidgetState::PRESSED;
        cWindow->addDamageRect(CompRectFromNuxGeo(close_button_geo_));
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
        bool was_pressed = (close_icon_state_ == decoration::WidgetState::PRESSED);

        if (close_icon_state_ != decoration::WidgetState::NORMAL)
        {
          close_icon_state_ = decoration::WidgetState::NORMAL;
          cWindow->addDamageRect(CompRectFromNuxGeo(close_button_geo_));
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
                       !wt->GetPresentationListGeometries().empty() ||
                       (mask & PAINT_SCREEN_FULL_MASK)
                     )
                   );

  allowWindowPaint = true;
  last_output_ = output;
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

  /* PAINT_SCREEN_FULL_MASK means that we are ignoring the damage
   * region and redrawing the whole screen, so we should make all
   * nux windows be added to the presentation list that intersect
   * this output.
   *
   * However, damaging nux has a side effect of notifying compiz
   * through OnRedrawRequested that we need to queue another frame.
   * In most cases that would be desirable, and in the case where
   * we did that in damageCutoff, it would not be a problem as compiz
   * does not queue up new frames for damage that can be processed
   * on the current frame. However, we're now past damage cutoff, but
   * a change in circumstances has required that we redraw all the nux
   * windows on this frame. As such, we need to ensure that damagePending
   * is not called as a result of queuing windows for redraw, as that
   * would effectively result in a damage feedback loop in plugins that
   * require screen transformations (eg, new frame -> plugin redraws full
   * screen -> we reach this point and request another redraw implicitly)
   */
  if (mask & PAINT_SCREEN_FULL_MASK)
  {
    ignore_redraw_request_ = true;
    compizDamageNux(CompRegionRef(output->region()));
    ignore_redraw_request_ = false;
  }

  gScreen->glPaintTransformedOutput(attrib, transform, region, output, mask);
  paintPanelShadow(region);
}

void UnityScreen::updateBlurDamage()
{
  /* If there are enabled helpers, we want to apply damage
   * based on how old our tracking fbo is since we may need
   * to redraw some of the blur regions if there has been
   * damage since we last bound it
   *
   * XXX: Unfortunately there's a nasty feedback loop here, and not
   * a whole lot we can do about it. If part of the damage from any frame
   * intersects a nux window, we have to mark the entire region that the
   * nux window covers as damaged, because nux does not have any concept
   * of geometry clipping. That damage will feed back to us on the next frame.
   */
  if (BackgroundEffectHelper::HasEnabledHelpers())
  {
    cScreen->applyDamageForFrameAge(back_buffer_age_);

    /*
     * Prioritise user interaction over active blur updates. So the general
     * slowness of the active blur doesn't affect the UI interaction performance.
     *
     * Also, BackgroundEffectHelper::ProcessDamage() is causing a feedback loop
     * while the dash is open. Calling it results in the NEXT frame (and the
     * current one?) to get some damage. This GetDrawList().empty() check avoids
     * that feedback loop and allows us to idle correctly.
     *
     * We are doing damage processing for the blurs here, as this represents
     * the most up to date compiz damage under the nux windows.
     */
    if (wt->GetDrawList().empty())
    {
      CompRect::vector const& rects(buffered_compiz_damage_this_frame_.rects());
      for (CompRect const& r : rects)
      {
        BackgroundEffectHelper::ProcessDamage(NuxGeometryFromCompRect(r));
      }
    }
  }
}

void UnityScreen::damageCutoff()
{
  if (force_draw_countdown_ > 0)
  {
    typedef nux::WindowCompositor::WeakBaseWindowPtr WeakBaseWindowPtr;

    /* We have to force-redraw the whole scene because
     * of a bug in the nvidia driver that causes framebuffers
     * to be trashed on resume for a few swaps */
    wt->GetWindowCompositor().ForEachBaseWindow([] (WeakBaseWindowPtr const& w) {
      w->QueueDraw();
    });

    --force_draw_countdown_;
  }

  /* At this point we want to take all of the compiz damage
   * for this frame and use it to determine which blur regions
   * need to be redrawn. We don't want to do this any later because
   * the nux damage is logically on top of the blurs and doesn't
   * affect them */
  updateBlurDamage();

  /* Determine nux region damage last */
  cScreen->damageCutoff();

  CompRegion damage_buffer, last_damage_buffer;

  do
  {
    last_damage_buffer = damage_buffer;

    /* First apply any damage accumulated to nux to see
     * what windows need to be redrawn there */
    compizDamageNux(buffered_compiz_damage_this_frame_);

    /* Apply the redraw regions to compiz so that we can
     * draw this frame with that region included */
    determineNuxDamage(damage_buffer);

    /* We want to track the nux damage here as we will use it to
     * determine if we need to present other nux windows too */
    cScreen->damageRegion(damage_buffer);

    /* If we had to put more damage into the damage buffer then
     * damage compiz with it and keep going */
  } while (last_damage_buffer != damage_buffer);

  /* Clear damage buffer */
  buffered_compiz_damage_last_frame_ = buffered_compiz_damage_this_frame_;
  buffered_compiz_damage_this_frame_ = CompRegion();

  /* Tell nux that any damaged windows should be redrawn on the next
   * frame and not this one */
  wt->ForeignFrameCutoff();

  /* We need to track this per-frame to figure out whether or not
   * to bind the contents fbo on each monitor pass */
  dirty_helpers_on_this_frame_ = BackgroundEffectHelper::HasDirtyHelpers();
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

  /* To prevent any potential overflow problems, we are assuming here
   * that compiz caps the maximum number of frames tracked at 10, so
   * don't increment the age any more than local::MAX_BUFFER_AGE */
  if (back_buffer_age_ < local::MAX_BUFFER_AGE)
    ++back_buffer_age_;

  if (didShellRepaint)
    wt->ClearDrawList();

  /* Tell nux that a new frame is now beginning and any damaged windows should
   * now be painted on this frame */
  wt->ForeignFrameEnded();

  if (animation_controller_->HasRunningAnimations())
    OnRedrawRequested();

  for (auto it = ShowdesktopHandler::animating_windows.begin(); it != ShowdesktopHandler::animating_windows.end();)
  {
    auto const& wi = *it;
    auto action = wi->HandleAnimations(0);

    if (action == ShowdesktopHandlerWindowInterface::PostPaintAction::Remove)
    {
      wi->DeleteHandler();
      it = ShowdesktopHandler::animating_windows.erase(it);
      continue;
    }
    else if (action == ShowdesktopHandlerWindowInterface::PostPaintAction::Damage)
    {
      wi->AddDamage();
    }

    ++it;
  }

  cScreen->donePaint();
}

void redraw_view_if_damaged(nux::ObjectPtr<CairoBaseWindow> const& view, CompRegion const& damage)
{
  if (!view || view->IsRedrawNeeded())
    return;

  auto const& geo = view->GetAbsoluteGeometry();

  if (damage.intersects(CompRectFromNuxGeo(geo)))
    view->RedrawBlur();
}

void UnityScreen::compizDamageNux(CompRegion const& damage)
{
  /* Ask nux to present anything in our damage region
   *
   * Note: This is using a new nux API, to "present" windows
   * to the screen, as opposed to drawing them. The difference is
   * important. The former will just draw the window backing texture
   * directly to the screen, the latter will re-draw the entire window.
   *
   * The former is a lot faster, do not use QueueDraw unless the contents
   * of the window need to be re-drawn.
   */
  auto const& rects = damage.rects();

  for (CompRect const& r : rects)
  {
    auto const& geo = NuxGeometryFromCompRect(r);
    wt->PresentWindowsIntersectingGeometryOnThisFrame(geo);
  }
  
  auto const& launchers = launcher_controller_->launchers();

  for (auto const& launcher : launchers)
  {
    if (!launcher->Hidden())
    {
      redraw_view_if_damaged(launcher->GetActiveTooltip(), damage);
    }
  }

  if (QuicklistManager* qm = QuicklistManager::Default())
  {
    redraw_view_if_damaged(qm->Current(), damage);
  }
}

/* Grab changed nux regions and add damage rects for them */
void UnityScreen::determineNuxDamage(CompRegion& nux_damage)
{
  /* Fetch all the dirty geometry from nux and aggregate it */
  auto const& dirty = wt->GetPresentationListGeometries();
  auto const& panels_geometries = panel_controller_->GetGeometries();

  for (auto const& dirty_geo : dirty)
  {
    nux_damage += CompRegionFromNuxGeo(dirty_geo);

    /* Special case, we need to redraw the panel shadow on panel updates */
    for (auto const& panel_geo : panels_geometries)
    {
      if (!dirty_geo.IsIntersecting(panel_geo))
        continue;

      for (CompOutput const& o : screen->outputDevs())
      {
        CompRect shadowRect;
        FillShadowRectForOutput(shadowRect, o);
        nux_damage += shadowRect;
      }
    }
  }
}

void UnityScreen::addSupportedAtoms(std::vector<Atom>& atoms)
{
  screen->addSupportedAtoms(atoms);
  atoms.push_back(atom::_UNITY_SHELL);
  atoms.push_back(atom::_UNITY_SAVED_WINDOW_SHAPE);
  deco_manager_->AddSupportedAtoms(atoms);
}

/* handle X Events */
void UnityScreen::handleEvent(XEvent* event)
{
  bool skip_other_plugins = false;
  PluginAdapter& wm = PluginAdapter::Default();

  if (deco_manager_->HandleEventBefore(event))
    return;

  switch (event->type)
  {
    case FocusIn:
    case FocusOut:
      if (event->xfocus.mode == NotifyGrab)
        wm.OnScreenGrabbed();
      else if (event->xfocus.mode == NotifyUngrab)
        wm.OnScreenUngrabbed();
      else if (!screen->grabbed() && event->xfocus.mode == NotifyWhileGrabbed)
        wm.OnScreenGrabbed();

      if (key_nav_mode_requested_)
      {
        // Close any overlay that is open.
        if (launcher_controller_->IsOverlayOpen())
        {
          dash_controller_->HideDash();
          hud_controller_->HideHud();
        }
        key_nav_mode_requested_ = false;
        launcher_controller_->KeyNavGrab();
      }
      break;
    case MotionNotify:
      if (wm.IsScaleActive())
      {
        if (CompWindow *w = screen->findWindow(sScreen->getSelectedWindow()))
          skip_other_plugins = UnityWindow::get(w)->handleEvent(event);
      }
      else if (switcher_controller_->detail())
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
        if (spread_filter_ && spread_filter_->Visible())
          skip_other_plugins = spread_filter_->GetAbsoluteGeometry().IsPointInside(event->xbutton.x_root, event->xbutton.y_root);

        if (!skip_other_plugins)
        {
          if (CompWindow *w = screen->findWindow(sScreen->getSelectedWindow()))
            skip_other_plugins = UnityWindow::get(w)->handleEvent(event);
        }
      }
      else if (switcher_controller_->detail())
      {
        Window win = switcher_controller_->GetCurrentSelection().window_;
        CompWindow* w = screen->findWindow(win);

        if (w)
          skip_other_plugins = UnityWindow::get(w)->handleEvent(event);
      }

      if (dash_controller_->IsVisible())
      {
        int panel_height = panel_style_.PanelHeight(dash_controller_->Monitor());
        nux::Point pt(event->xbutton.x_root, event->xbutton.y_root);
        nux::Geometry const& dash = dash_controller_->GetInputWindowGeometry();
        nux::Geometry const& dash_geo = nux::Geometry(dash.x, dash.y, dash.width,
                                                      dash.height + panel_height);

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
            dash_controller_->HideDash();
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
          hud_controller_->HideHud();
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

      if (switcher_controller_->detail())
      {
        Window win = switcher_controller_->GetCurrentSelection().window_;
        CompWindow* w = screen->findWindow(win);

        if (w)
          skip_other_plugins = UnityWindow::get(w)->handleEvent(event);
      }
      else if (wm.IsScaleActive())
      {
        if (spread_filter_ && spread_filter_->Visible())
          skip_other_plugins = spread_filter_->GetAbsoluteGeometry().IsPointInside(event->xbutton.x_root, event->xbutton.y_root);

        if (!skip_other_plugins)
        {
          if (CompWindow *w = screen->findWindow(sScreen->getSelectedWindow()))
            skip_other_plugins = skip_other_plugins || UnityWindow::get(w)->handleEvent(event);
        }
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
        sources_.AddIdle([this] {
          shortcut_controller_->SetEnabled(false);
          shortcut_controller_->Hide();
          LOG_DEBUG(logger) << "Hiding shortcut controller due to keypress event.";
          EnableCancelAction(CancelActionTarget::SHORTCUT_HINT, false);

          return false;
        });
      }

      KeySym key_sym = XkbKeycodeToKeysym(event->xany.display, event->xkey.keycode, 0, 0);

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
          break;
        }
      }

      if (super_keypressed_)
      {
        if (IsKeypadKey(key_sym))
        {
          key_sym = XkbKeycodeToKeysym(event->xany.display, event->xkey.keycode, 0, 1);
          key_sym = key_sym - XK_KP_0 + XK_0;
        }

        skip_other_plugins = launcher_controller_->HandleLauncherKeyEvent(XModifiersToNux(event->xkey.state), key_sym, event->xkey.time);
        if (!skip_other_plugins)
          skip_other_plugins = dash_controller_->CheckShortcutActivation(XKeysymToString(key_sym));

        if (skip_other_plugins && launcher_controller_->KeyNavIsActive())
        {
          launcher_controller_->KeyNavTerminate(false);
          EnableCancelAction(CancelActionTarget::LAUNCHER_SWITCHER, false);
        }
      }

      if (spread_filter_ && spread_filter_->Visible())
      {
        if (key_sym == XK_Escape)
        {
          skip_other_plugins = true;
          spread_filter_->text = "";
        }
      }

      break;
    }
    case MapRequest:
      ShowdesktopHandler::InhibitLeaveShowdesktopMode (event->xmaprequest.window);
      break;
    case PropertyNotify:
      if (bghash_ && event->xproperty.window == GDK_ROOT_WINDOW() &&
          event->xproperty.atom == bghash_->ColorAtomId())
      {
        bghash_->RefreshColor();
      }
      break;
    default:
        if (screen->shapeEvent() + ShapeNotify == event->type)
        {
          Window xid = event->xany.window;
          CompWindow *w = screen->findWindow(xid);

          if (w)
          {
            UnityWindow *uw = UnityWindow::get(w);
            uw->handleEvent(event);
          }
        }
      break;
  }

  compiz::CompizMinimizedWindowHandler<UnityScreen, UnityWindow>::handleEvent(event);

  // avoid further propagation (key conflict for instance)
  if (!skip_other_plugins)
    screen->handleEvent(event);

  if (deco_manager_->HandleEventAfter(event))
    return;

  if (event->type == MapRequest)
    ShowdesktopHandler::AllowLeaveShowdesktopMode(event->xmaprequest.window);

  if (switcher_controller_->Visible() && switcher_controller_->mouse_disabled() &&
      (event->type == MotionNotify || event->type == ButtonPress || event->type == ButtonRelease))
  {
    skip_other_plugins = true;
  }

  if (spread_filter_ && spread_filter_->Visible())
    skip_other_plugins = false;

  if (!skip_other_plugins &&
      screen->otherGrabExist("deco", "move", "switcher", "resize", nullptr))
  {
    wt->ProcessForeignEvent(event, nullptr);
  }
}

void UnityScreen::damageRegion(const CompRegion &region)
{
  buffered_compiz_damage_this_frame_ += region;
  cScreen->damageRegion(region);
}

void UnityScreen::handleCompizEvent(const char* plugin,
                                    const char* event,
                                    CompOption::Vector& option)
{
  PluginAdapter& adapter = PluginAdapter::Default();
  adapter.NotifyCompizEvent(plugin, event, option);
  compiz::CompizMinimizedWindowHandler<UnityScreen, UnityWindow>::handleCompizEvent(plugin, event, option);
  screen->handleCompizEvent(plugin, event, option);
}

bool UnityScreen::showMenuBarInitiate(CompAction* action,
                                      CompAction::State state,
                                      CompOption::Vector& options)
{
  if (state & CompAction::StateInitKey)
  {
    action->setState(action->state() | CompAction::StateTermKey);
    menus_->show_menus = true;
  }

  return false;
}

bool UnityScreen::showMenuBarTerminate(CompAction* action,
                                       CompAction::State state,
                                       CompOption::Vector& options)
{
  if (state & CompAction::StateTermKey)
  {
    action->setState(action->state() & ~CompAction::StateTermKey);
    menus_->show_menus = false;
  }

  return false;
}

bool UnityScreen::showLauncherKeyInitiate(CompAction* action,
                                          CompAction::State state,
                                          CompOption::Vector& options)
{
  // to receive the Terminate event
  if (state & CompAction::StateInitKey)
    action->setState(action->state() | CompAction::StateTermKey);

  super_keypressed_ = true;
  int when = CompOption::getIntOptionNamed(options, "time");
  launcher_controller_->HandleLauncherKeyPress(when);
  EnsureSuperKeybindings ();

  if (!shortcut_controller_->Visible() &&
      shortcut_controller_->IsEnabled())
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
  bool tap_handled = false;
  LOG_DEBUG(logger) << "Super released: " << (was_tap ? "tapped" : "released");
  int when = CompOption::getIntOptionNamed(options, "time");

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

    if (!dash_controller_->IsVisible())
    {
      if (dash_controller_->ShowDash())
      {
        tap_handled = true;
        ubus_manager_.SendMessage(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                                  g_variant_new("(sus)", "home.scope", dash::GOTO_DASH_URI, ""));
      }
    }
    else if (dash_controller_->IsCommandLensOpen())
    {
      tap_handled = true;
      ubus_manager_.SendMessage(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                                g_variant_new("(sus)", "home.scope", dash::GOTO_DASH_URI, ""));
    }
    else
    {
      dash_controller_->HideDash();
      tap_handled = true;
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
  return (was_tap && tap_handled) || !was_tap;
}

bool UnityScreen::showPanelFirstMenuKeyInitiate(CompAction* action,
                                                CompAction::State state,
                                                CompOption::Vector& options)
{
  /* In order to avoid too many events when keeping the keybinding pressed,
   * that would make the unity-panel-service to go crazy (see bug #948522)
   * we need to filter them, just considering an event every 750 ms */
  int event_time = CompOption::getIntOptionNamed(options, "time");

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
  menus_->open_first.emit();

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
  WM.ShowDesktop();
  return true;
}

void UnityScreen::SpreadAppWindows(bool anywhere)
{
  if (ApplicationPtr const& active_app = ApplicationManager::Default().GetActiveApplication())
  {
    std::vector<Window> windows;

    for (auto& window : active_app->GetWindows())
    {
      if (anywhere || WM.IsWindowOnCurrentDesktop(window->window_id()))
        windows.push_back(window->window_id());
    }

    WM.ScaleWindowGroup(windows, 0, true);
  }
}

bool UnityScreen::spreadAppWindowsInitiate(CompAction* action,
                                           CompAction::State state,
                                           CompOption::Vector& options)
{
  SpreadAppWindows(false);
  return true;
}

bool UnityScreen::spreadAppWindowsAnywhereInitiate(CompAction* action,
                                                   CompAction::State state,
                                                   CompOption::Vector& options)
{
  SpreadAppWindows(true);
  return true;
}

bool UnityScreen::setKeyboardFocusKeyInitiate(CompAction* action,
                                              CompAction::State state,
                                              CompOption::Vector& options)
{
  if (WM.IsScaleActive())
    WM.TerminateScale();
  else if (WM.IsExpoActive())
    WM.TerminateExpo();

  key_nav_mode_requested_ = true;
  return true;
}

bool UnityScreen::altTabInitiateCommon(CompAction* action, switcher::ShowMode show_mode)
{
  if (!grab_index_)
  {
    auto cursor = switcher_controller_->mouse_disabled() ? screen->invisibleCursor() : screen->normalCursor();
    grab_index_ = screen->pushGrab(cursor, "unity-switcher");
  }

  if (WM.IsScaleActive())
    WM.TerminateScale();

  launcher_controller_->ClearTooltips();

  /* Create a new keybinding for scroll buttons and current modifiers */
  CompAction scroll_up;
  CompAction scroll_down;
  scroll_up.setButton(CompAction::ButtonBinding(local::SCROLL_UP_BUTTON, action->key().modifiers()));
  scroll_down.setButton(CompAction::ButtonBinding(local::SCROLL_DOWN_BUTTON, action->key().modifiers()));
  screen->addAction(&scroll_up);
  screen->addAction(&scroll_down);

  menus_->show_menus = false;
  SetUpAndShowSwitcher(show_mode);

  return true;
}

void UnityScreen::SetUpAndShowSwitcher(switcher::ShowMode show_mode)
{
  RaiseInputWindows();

  if (!optionGetAltTabBiasViewport())
  {
    if (show_mode == switcher::ShowMode::CURRENT_VIEWPORT)
      show_mode = switcher::ShowMode::ALL;
    else
      show_mode = switcher::ShowMode::CURRENT_VIEWPORT;
  }

  auto results = launcher_controller_->GetAltTabIcons(show_mode == switcher::ShowMode::CURRENT_VIEWPORT,
                                                      switcher_controller_->show_desktop_disabled());

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
  if (WM.IsWallActive())
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

bool UnityScreen::altTabNextWindowInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  if (!switcher_controller_->Visible())
  {
    altTabInitiateCommon(action, switcher::ShowMode::CURRENT_VIEWPORT);
    switcher_controller_->Select((switcher_controller_->StartIndex())); // always select the current application
    switcher_controller_->InitiateDetail();
  }
  else if (switcher_controller_->detail())
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

void UnityScreen::OnSwitcherDetailChanged(bool detail)
{
  if (detail)
  {
    for (LayoutWindow::Ptr const& target : switcher_controller_->ExternalRenderTargets())
    {
      if (CompWindow* window = screen->findWindow(target->xid))
      {
        auto* uwin = UnityWindow::get(window);
        uwin->close_icon_state_ = decoration::WidgetState::NORMAL;
        uwin->middle_clicked_ = false;
        fake_decorated_windows_.insert(uwin);
      }
    }
  }
  else
  {
    for (UnityWindow* uwin : fake_decorated_windows_)
      uwin->CleanupCachedTextures();

    fake_decorated_windows_.clear();
  }
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
    LOG_ERROR(logger) << "Switcher is visible when showing HUD: this should never happen";
    return false; // early exit if the switcher is open
  }

  if (hud_controller_->IsVisible())
  {
    hud_controller_->HideHud();
  }
  else
  {
    // Handles closing KeyNav (Alt+F1) if the hud is about to show
    if (launcher_controller_->KeyNavIsActive())
      launcher_controller_->KeyNavTerminate(false);

    if (dash_controller_->IsVisible())
      dash_controller_->HideDash();

    if (QuicklistManager::Default()->Current())
      QuicklistManager::Default()->Current()->Hide();

    if (WM.IsScreenGrabbed())
    {
      hud_ungrab_slot_ = WM.screen_ungrabbed.connect([this] { ShowHud(); });

      // Let's wait ungrab event for maximum a couple of seconds...
      sources_.AddTimeoutSeconds(2, [this] {
        hud_ungrab_slot_->disconnect();
        return false;
      }, local::HUD_UNGRAB_WAIT);

      return false;
    }

    hud_ungrab_slot_->disconnect();
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
  hud_keypress_time_ = CompOption::getIntOptionNamed(options, "time");

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

  int release_time = CompOption::getIntOptionNamed(options, "time");
  int tap_duration = release_time - hud_keypress_time_;
  if (tap_duration > local::ALT_TAP_DURATION)
  {
    LOG_DEBUG(logger) << "Tap too long";
    return false;
  }

  return ShowHud();
}

bool UnityScreen::LockScreenInitiate(CompAction* action,
                                     CompAction::State state,
                                     CompOption::Vector& options)
{
  sources_.AddIdle([this] {
    session_controller_->LockScreen();
    return false;
  });

  return true;
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

  WM.close_window_key = std::make_pair(modifiers, keysym);
}

void UnityScreen::UpdateActivateIndicatorsKey()
{
  CompAction::KeyBinding const& keybind = optionGetPanelFirstMenu().key();
  KeySym keysym = XkbKeycodeToKeysym(screen->dpy(), keybind.keycode(), 0, 0);
  unsigned modifiers = CompizModifiersToNux(keybind.modifiers());

  WM.activate_indicators_key = std::make_pair(modifiers, keysym);
}

bool UnityScreen::InitPluginActions()
{
  PluginAdapter& adapter = PluginAdapter::Default();

  if (CompPlugin* p = CompPlugin::find("core"))
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

  if (CompPlugin* p = CompPlugin::find("expo"))
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

  if (CompPlugin* p = CompPlugin::find("scale"))
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

  if (CompPlugin* p = CompPlugin::find("unitymtgrabhandles"))
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

  if (CompPlugin* p = CompPlugin::find("decor"))
  {
    LOG_ERROR(logger) << "Decoration plugin is active, disabling it...";
    screen->finiPluginForScreen(p);
    p->vTable->finiScreen(screen);
    CompPlugin::getPlugins().remove(p);
    CompPlugin::unload(p);
  }

  return false;
}

/* Set up expo and scale actions on the launcher */
bool UnityScreen::initPluginForScreen(CompPlugin* p)
{
  if (p->vTable->name() == "expo" ||
      p->vTable->name() == "scale")
  {
    InitPluginActions();
  }

  bool result = screen->initPluginForScreen(p);
  if (p->vTable->name() == "unityshell")
    InitAltTabNextWindow();

  return result;
}

void UnityScreen::AddProperties(debug::IntrospectionData& introspection)
{}

std::string UnityScreen::GetName() const
{
  return "Unity";
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
          (window->state() & CompWindowStateFullscreenMask && !window->minimized() && !window->inShowDesktopMode()) &&
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

  if (uScreen->lockscreen_controller_->IsLocked() && uScreen->lockscreen_controller_->opacity() == 1.0)
  {
    if (!window->minimized() && !CanBypassLockScreen())
    {
      // For some reasons PAINT_WINDOW_NO_CORE_INSTANCE_MASK doesn't work here
      // (well, it works too much, as it applies to menus too), so we need
      // to paint the windows at the proper opacity, overriding any other
      // paint plugin (animation, fade?) that might interfere with us.
      wAttrib.opacity = 0.0;
      int old_index = gWindow->glPaintGetCurrentIndex();
      gWindow->glPaintSetCurrentIndex(MAXSHORT);
      deco_win_->Paint(matrix, wAttrib, region, mask);
      bool ret = gWindow->glPaint(wAttrib, matrix, region, mask);
      gWindow->glPaintSetCurrentIndex(old_index);
      return ret;
    }
  }

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

  if (uScreen->WM.IsScaleActive() &&
      uScreen->sScreen->getSelectedWindow() == window->id())
  {
    nux::Geometry const& scaled_geo = GetScaledGeometry();
    paintInnerGlow(scaled_geo, matrix, attrib, mask);
  }

  if (uScreen->session_controller_->Visible())
  {
    // Let's darken the other windows if the session dialog is visible
    wAttrib.brightness *= 0.75f;
  }

  deco_win_->Paint(matrix, wAttrib, region, mask);
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
  auto window_state = window->state();
  auto window_type = window->type();
  bool locked = uScreen->lockscreen_controller_->IsLocked();

  if (uScreen->doShellRepaint && !uScreen->paint_panel_under_dash_ && window_type == CompWindowTypeNormalMask)
  {
    if ((window_state & MAXIMIZE_STATE) && window->onCurrentDesktop() && !window->overrideRedirect() && window->managed())
    {
      CompPoint const& viewport = window->defaultViewport();
      unsigned output = window->outputDevice();

      if (viewport == uScreen->screen->vp() && output == uScreen->screen->currentOutputDev().id())
      {
        uScreen->paint_panel_under_dash_ = true;
      }
    }
  }

  if (uScreen->doShellRepaint && window == uScreen->onboard_)
  {
    uScreen->paintDisplay();
  }
  else if (uScreen->doShellRepaint &&
           window == uScreen->firstWindowAboveShell &&
           !uScreen->forcePaintOnTop() &&
           !uScreen->fullscreenRegion.contains(window->geometry()))
  {
    uScreen->paintDisplay();
  }
  else if (locked && CanBypassLockScreen())
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

    if (G_UNLIKELY(window_type == CompWindowTypeDesktopMask))
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

        if (!(window_state & CompWindowStateMaximizedVertMask) &&
            !(window_state & CompWindowStateFullscreenMask) &&
            !(window_type & CompWindowTypeFullscreenMask))
        {
          auto const& output = uScreen->screen->currentOutputDev();
          int monitor = uScreen->WM.MonitorGeometryIn(NuxGeometryFromCompRect(output));

          if (window->y() - window->border().top < output.y() + uScreen->panel_style_.PanelHeight(monitor))
          {
            draw_panel_shadow = DrawPanelShadow::OVER_WINDOW;
          }
        }
      }
      else if (uScreen->menus_->integrated_menus())
      {
        draw_panel_shadow = DrawPanelShadow::BELOW_WINDOW;
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

  if (locked)
    draw_panel_shadow = DrawPanelShadow::NO;

  if (draw_panel_shadow == DrawPanelShadow::BELOW_WINDOW)
    uScreen->paintPanelShadow(region);

  deco_win_->Draw(matrix, attrib, region, mask);
  bool ret = gWindow->glDraw(matrix, attrib, region, mask);

  if (draw_panel_shadow == DrawPanelShadow::OVER_WINDOW)
    uScreen->paintPanelShadow(region);

  return ret;
}

bool UnityWindow::damageRect(bool initial, CompRect const& rect)
{
  if (initial)
    deco_win_->Update();

  return cWindow->damageRect(initial, rect);
}

void
UnityScreen::OnMinimizeDurationChanged ()
{
  /* Update the compiz plugin setting with the new computed speed so that it
   * will be used in the following minimizations */
  if (CompPlugin* p = CompPlugin::find("animation"))
  {
    CompOption::Vector &opts = p->vTable->getOptions();

    for (CompOption &o : opts)
    {
      if (o.name() == "minimize_durations")
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
  else
  {
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
      deco_win_->Update();
      deco_win_->UpdateDecorationPosition();

      if (window->type() == CompWindowTypeDesktopMask)
      {
        if (!focus_desktop_timeout_)
        {
          focus_desktop_timeout_.reset(new glib::Timeout(1000, [this] {
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
      else if (WindowManager::Default().IsOnscreenKeyboard(window->id()))
      {
        uScreen->onboard_ = window;
        uScreen->RaiseOSK();
      }
    /* Fall through an re-evaluate wraps on map and unmap too */
    case CompWindowNotifyUnmap:
      if (uScreen->optionGetShowMinimizedWindows() && window->mapNum() &&
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

      deco_win_->Update();
      deco_win_->UpdateDecorationPosition();

      PluginAdapter::Default().UpdateShowDesktopState();
      break;
    case CompWindowNotifyBeforeDestroy:
      being_destroyed.emit();
      break;
    case CompWindowNotifyMinimize:
      /* Updating the count in dconf will trigger a "changed" signal to which
       * the method setting the new animation speed is attached */
      uScreen->minimize_speed_controller_.UpdateCount();
      break;
    case CompWindowNotifyUnreparent:
      deco_win_->Undecorate();
      break;
    case CompWindowNotifyReparent:
      deco_win_->Update();
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
    if (uScreen->dash_controller_->IsVisible())
    {
      uScreen->dash_controller_->ReFocusKeyInput();
    }
    else if (uScreen->hud_controller_->IsVisible())
    {
      uScreen->hud_controller_->ReFocusKeyInput();
    }
  }
}

void UnityWindow::stateChangeNotify(unsigned int lastState)
{
  if (window->state() & CompWindowStateFullscreenMask &&
      !(lastState & CompWindowStateFullscreenMask))
  {
    uScreen->fullscreen_windows_.push_back(window);
  }
  else if (lastState & CompWindowStateFullscreenMask &&
           !(window->state() & CompWindowStateFullscreenMask))
  {
    uScreen->fullscreen_windows_.remove(window);
  }

  deco_win_->Update();
  PluginAdapter::Default().NotifyStateChange(window, window->state(), lastState);
  window->stateChangeNotify(lastState);
}

void UnityWindow::updateFrameRegion(CompRegion &region)
{
  /* The minimize handler will short circuit the frame
   * region update func and ensure that the frame
   * does not have a region */

  if (mMinimizeHandler)
  {
    mMinimizeHandler->updateFrameRegion(region);
  }
  else if (mShowdesktopHandler)
  {
    mShowdesktopHandler->UpdateFrameRegion(region);
  }
  else
  {
    window->updateFrameRegion(region);
    deco_win_->UpdateFrameRegion(region);
  }
}

void UnityWindow::getOutputExtents(CompWindowExtents& output)
{
  window->getOutputExtents(output);
  deco_win_->UpdateOutputExtents(output);
}

void UnityWindow::moveNotify(int x, int y, bool immediate)
{
  deco_win_->UpdateDecorationPositionDelayed();
  PluginAdapter::Default().NotifyMoved(window, x, y);
  window->moveNotify(x, y, immediate);
}

void UnityWindow::resizeNotify(int x, int y, int w, int h)
{
  deco_win_->UpdateDecorationPositionDelayed();
  PluginAdapter::Default().NotifyResized(window, x, y, w, h);
  window->resizeNotify(x, y, w, h);
}

CompPoint UnityWindow::tryNotIntersectUI(CompPoint& pos)
{
  auto const& window_geo = window->borderRect();
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

  auto const& launchers = uScreen->launcher_controller_->launchers();

  for (auto const& launcher : launchers)
  {
    if (launcher->options()->hide_mode == LAUNCHER_HIDE_AUTOHIDE && launcher->Hidden())
      continue;

    auto const& geo = launcher->GetAbsoluteGeometry();

    if (geo.IsInside(result))
    {
      if (geo.x + geo.width + 1 + window_geo.width() < target_monitor.x + target_monitor.width)
      {
        result.x = geo.x + geo.width + 1;
      }
    }
  }

  for (nux::Geometry const& geo : uScreen->panel_controller_->GetGeometries())
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
    deco_win_->Update();
    bool result = window->place(pos);

    if (window->type() & NO_FOCUS_MASK)
      return result;

    pos = tryNotIntersectUI(pos);
    return result;
  }

  return true;
}


/* Start up nux after OpenGL is initialized */
void UnityScreen::InitNuxThread(nux::NThread* thread, void* data)
{
  Timer timer;
  static_cast<UnityScreen*>(data)->InitUnityComponents();

  nux::ColorLayer background(nux::color::Transparent);
  static_cast<nux::WindowThread*>(thread)->SetWindowBackgroundPaintLayer(&background);
  LOG_INFO(logger) << "UnityScreen::InitNuxThread: " << timer.ElapsedSeconds() << "s";
}

void UnityScreen::OnRedrawRequested()
{
  if (!ignore_redraw_request_)
    cScreen->damagePending();
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
    case UnityshellOptions::ScrollInactiveIcons:
      launcher_options->scroll_inactive_icons = optionGetScrollInactiveIcons();
      break;
    case UnityshellOptions::LauncherMinimizeWindow:
      launcher_options->minimize_window_on_click = optionGetLauncherMinimizeWindow();
      break;
    case UnityshellOptions::BackgroundColor:
    {
      auto override_color = NuxColorFromCompizColor(optionGetBackgroundColor());
      override_color.red = override_color.red / override_color.alpha;
      override_color.green = override_color.green / override_color.alpha;
      override_color.blue = override_color.blue / override_color.alpha;
      bghash_->OverrideColor(override_color);
      break;
    }
    case UnityshellOptions::OverrideDecorationTheme:
      if (optionGetOverrideDecorationTheme())
      {
        deco_manager_->active_shadow_color = NuxColorFromCompizColor(optionGetActiveShadowColor());
        deco_manager_->inactive_shadow_color = NuxColorFromCompizColor(optionGetInactiveShadowColor());
        deco_manager_->active_shadow_radius = optionGetActiveShadowRadius();
        deco_manager_->inactive_shadow_radius = optionGetInactiveShadowRadius();
        deco_manager_->shadow_offset = nux::Point(optionGetShadowXOffset(), optionGetShadowYOffset());
      }
      else
      {
        OnDecorationStyleChanged();
      }
      break;
    case UnityshellOptions::ActiveShadowColor:
      if (optionGetOverrideDecorationTheme())
        deco_manager_->active_shadow_color = NuxColorFromCompizColor(optionGetActiveShadowColor());
      break;
    case UnityshellOptions::InactiveShadowColor:
      if (optionGetOverrideDecorationTheme())
        deco_manager_->inactive_shadow_color = NuxColorFromCompizColor(optionGetInactiveShadowColor());
      break;
    case UnityshellOptions::ActiveShadowRadius:
      if (optionGetOverrideDecorationTheme())
        deco_manager_->active_shadow_radius = optionGetActiveShadowRadius();
      break;
    case UnityshellOptions::InactiveShadowRadius:
      if (optionGetOverrideDecorationTheme())
        deco_manager_->inactive_shadow_radius = optionGetInactiveShadowRadius();
      break;
    case UnityshellOptions::ShadowXOffset:
      if (optionGetOverrideDecorationTheme())
        deco_manager_->shadow_offset = nux::Point(optionGetShadowXOffset(), optionGetShadowYOffset());
      break;
    case UnityshellOptions::ShadowYOffset:
      if (optionGetOverrideDecorationTheme())
        deco_manager_->shadow_offset = nux::Point(optionGetShadowXOffset(), optionGetShadowYOffset());
      break;
    case UnityshellOptions::LauncherHideMode:
    {
      launcher_options->hide_mode = (launcher::LauncherHideMode) optionGetLauncherHideMode();
      hud_controller_->launcher_locked_out = (launcher_options->hide_mode == LAUNCHER_HIDE_NEVER);

      int scale_offset = (launcher_options->hide_mode == LAUNCHER_HIDE_NEVER) ? 0 : launcher_controller_->launcher().GetWidth();
      CompOption::Value v(scale_offset);
      CompOption::Value bv(0);
      if (Settings::Instance().launcher_position() == LauncherPosition::LEFT)
      {
        screen->setOptionForPlugin("scale", "x_offset", v);
        screen->setOptionForPlugin("scale", "y_bottom_offset", bv);
      }
      else
      {
        screen->setOptionForPlugin("scale", "x_offset", bv);
        screen->setOptionForPlugin("scale", "y_bottom_offset", v);
      }
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
      menus_->fadein = optionGetMenusFadein();
      break;
    case UnityshellOptions::MenusFadeout:
      menus_->fadeout = optionGetMenusFadeout();
      break;
    case UnityshellOptions::MenusDiscoveryFadein:
      menus_->discovery_fadein = optionGetMenusDiscoveryFadein();
      break;
    case UnityshellOptions::MenusDiscoveryFadeout:
      menus_->discovery_fadeout = optionGetMenusDiscoveryFadeout();
      break;
    case UnityshellOptions::MenusDiscoveryDuration:
      menus_->discovery = optionGetMenusDiscoveryDuration();
      break;
    case UnityshellOptions::LauncherOpacity:
      launcher_options->background_alpha = optionGetLauncherOpacity();
      break;
    case UnityshellOptions::IconSize:
    {
      launcher_options->icon_size = optionGetIconSize();
      launcher_options->tile_size = optionGetIconSize() + 6;

      hud_controller_->icon_size = launcher_options->icon_size();
      hud_controller_->tile_size = launcher_options->tile_size();
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
      switcher_controller_->detail_on_timeout = optionGetAltTabTimeout();
    case UnityshellOptions::AltTabBiasViewport:
      PluginAdapter::Default().bias_active_to_viewport = optionGetAltTabBiasViewport();
      break;
    case UnityshellOptions::SwitchStrictlyBetweenApplications:
      switcher_controller_->first_selection_mode = optionGetSwitchStrictlyBetweenApplications() ?
                                                   switcher::FirstSelectionMode::LAST_ACTIVE_APP :
                                                   switcher::FirstSelectionMode::LAST_ACTIVE_VIEW;
      break;
    case UnityshellOptions::DisableShowDesktop:
      switcher_controller_->show_desktop_disabled = optionGetDisableShowDesktop();
      break;
    case UnityshellOptions::DisableMouse:
      switcher_controller_->mouse_disabled = optionGetDisableMouse();
      break;
    case UnityshellOptions::ShowMinimizedWindows:
      compiz::CompizMinimizedWindowHandler<UnityScreen, UnityWindow>::setFunctions (optionGetShowMinimizedWindows ());
      screen->enterShowDesktopModeSetEnabled (this, optionGetShowMinimizedWindows ());
      screen->leaveShowDesktopModeSetEnabled (this, optionGetShowMinimizedWindows ());
      break;
    case UnityshellOptions::ShortcutOverlay:
      shortcut_controller_->SetEnabled(optionGetShortcutOverlay());
      break;
    case UnityshellOptions::LowGraphicsMode:
      if (optionGetLowGraphicsMode())
          BackgroundEffectHelper::blur_type = BLUR_NONE;
      else
          BackgroundEffectHelper::blur_type = (unity::BlurType)optionGetDashBlurExperimental();

      unity::Settings::Instance().low_gfx = optionGetLowGraphicsMode();
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
    case UnityshellOptions::PanelFirstMenu:
      UpdateActivateIndicatorsKey();
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
    sources_.AddTimeout(timeout, [this] {
      NeedsRelayout();
      Relayout();

      cScreen->damageScreen();

      return false;
    }, local::RELAYOUT_TIMEOUT);
  }
}

void UnityScreen::Relayout()
{
  if (!needsRelayout)
    return;

  UScreen *uscreen = UScreen::GetDefault();
  int primary_monitor = uscreen->GetPrimaryMonitor();
  auto const& geo = uscreen->GetMonitorGeometry(primary_monitor);
  wt->SetWindowSize(geo.width, geo.height);

  LOG_DEBUG(logger) << "Setting to primary screen rect; " << geo;
  needsRelayout = false;

  DamagePanelShadow();
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
        WM.viewport_layout_changed.emit(screen->vpSize().width(), screen->vpSize().height());
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

  auto gpu_device = nux::GetGraphicsDisplay()->GetGpuDevice();
  gpu_device->backup_texture0_ =
      gpu_device->CreateSystemCapableDeviceTexture(screen->width(), screen->height(),
                                                   1, nux::BITFMT_R8G8B8A8,
                                                   NUX_TRACKER_LOCATION);

  ScheduleRelayout(500);
}

bool UnityScreen::layoutSlotsAndAssignWindows()
{
  auto const& scaled_windows = sScreen->getWindows();

  for (auto const& output : screen->outputDevs())
  {
    ui::LayoutWindow::Vector layout_windows;
    int monitor = UScreen::GetDefault()->GetMonitorAtPosition(output.centerX(), output.centerY());
    double monitor_scale = unity_settings_.em(monitor)->DPIScale();

    for (ScaleWindow *sw : scaled_windows)
    {
      if (sw->window->outputDevice() == static_cast<int>(output.id()))
      {
        UnityWindow::get(sw->window)->deco_win_->scaled = true;
        layout_windows.emplace_back(std::make_shared<LayoutWindow>(sw->window->id()));
      }
    }

    auto max_bounds = NuxGeometryFromCompRect(output.workArea());
    if (launcher_controller_->options()->hide_mode != LAUNCHER_HIDE_NEVER)
    {
      if (Settings::Instance().launcher_position() == LauncherPosition::LEFT)
      {
        int monitor_width = unity_settings_.LauncherSize(monitor);
        max_bounds.x += monitor_width;
        max_bounds.width -= monitor_width;
      }
      else if (Settings::Instance().launcher_position() == LauncherPosition::BOTTOM)
      {
        int launcher_size = unity_settings_.LauncherSize(monitor);
        max_bounds.height -= launcher_size;
      }
    }

    nux::Geometry final_bounds;
    ui::LayoutSystem layout;
    layout.max_row_height = max_bounds.height;
    layout.spacing = local::SCALE_SPACING.CP(monitor_scale);
    int padding = local::SCALE_PADDING.CP(monitor_scale);
    max_bounds.Expand(-padding, -padding);
    layout.LayoutWindowsNearest(layout_windows, max_bounds, final_bounds);

    for (auto const& lw : layout_windows)
    {
      auto sw_it = std::find_if(scaled_windows.begin(), scaled_windows.end(), [&lw] (ScaleWindow* sw) {
        return sw->window->id() == lw->xid;
      });

      if (sw_it == scaled_windows.end())
        continue;

      ScaleWindow* sw = *sw_it;
      ScaleSlot slot(CompRectFromNuxGeo(lw->result));
      slot.scale = lw->scale;

      float sx = lw->geo.width * slot.scale;
      float sy = lw->geo.height * slot.scale;
      float cx = (slot.x1() + slot.x2()) / 2;
      float cy = (slot.y1() + slot.y2()) / 2;

      CompWindow *w = sw->window;
      cx += w->input().left * slot.scale;
      cy += w->input().top * slot.scale;

      slot.setGeometry(cx - sx / 2, cy - sy / 2, sx, sy);
      slot.filled = true;

      sw->setSlot(slot);
    }
  }

  return true;
}

void UnityScreen::OnDashRealized()
{
  RaiseOSK();
}

void UnityScreen::OnLockScreenRequested()
{
  if (switcher_controller_->Visible())
    switcher_controller_->Hide(false);

  if (dash_controller_->IsVisible())
    dash_controller_->HideDash();

  if (hud_controller_->IsVisible())
    hud_controller_->HideHud();

  if (session_controller_->Visible())
    session_controller_->Hide();

  menus_->Indicators()->CloseActiveEntry();
  launcher_controller_->ClearTooltips();

  if (launcher_controller_->KeyNavIsActive())
    launcher_controller_->KeyNavTerminate(false);

  if (QuicklistManager::Default()->Current())
    QuicklistManager::Default()->Current()->Hide();

  if (WM.IsScaleActive())
    WM.TerminateScale();

  if (WM.IsExpoActive())
    WM.TerminateExpo();

  RaiseOSK();
}

void UnityScreen::OnScreenLocked()
{
  SaveLockStamp(true);

  for (auto& option : getOptions())
  {
    if (option.isAction())
    {
      auto& value = option.value();

      if (value != mOptions[UnityshellOptions::PanelFirstMenu].value())
        screen->removeAction(&value.action());
    }
  }

  for (auto& action : getActions())
    screen->removeAction(&action);

  // We notify that super/alt have been released, to avoid to leave unity in inconsistent state
  CompOption::Vector options(1);
  options.back().setName("time", CompOption::TypeInt);
  options.back().value().set<int>(screen->getCurrentTime());

  showLauncherKeyTerminate(&optionGetShowLauncher(), CompAction::StateTermKey, options);
  showMenuBarTerminate(&optionGetShowMenuBar(), CompAction::StateTermKey, options);

  // We disable the edge barriers, to avoid blocking the mouse pointer during lockscreen
  edge_barriers_->force_disable = true;
}

void UnityScreen::OnScreenUnlocked()
{
  SaveLockStamp(false);

  for (auto& option : getOptions())
  {
    if (option.isAction())
      screen->addAction(&option.value().action());
  }

  for (auto& action : getActions())
    screen->addAction(&action);

  edge_barriers_->force_disable = false;
}

void UnityScreen::SaveLockStamp(bool save)
{
  auto const& cache_dir = DesktopUtilities::GetUserRuntimeDirectory();

  if (cache_dir.empty())
    return;

  if (save)
  {
    glib::Error error;
    g_file_set_contents((cache_dir+local::LOCKED_STAMP).c_str(), "", 0, &error);

    if (error)
    {
      LOG_ERROR(logger) << "Impossible to save the unity locked stamp file: " << error;
    }
  }
  else
  {
    if (g_unlink((cache_dir+local::LOCKED_STAMP).c_str()) < 0)
    {
      LOG_ERROR(logger) << "Impossible to delete the unity locked stamp file";
    }
  }
}

void UnityScreen::RaiseOSK()
{
  /* stack the onboard window above us */
  if (onboard_)
  {
    if (nux::BaseWindow* dash = dash_controller_->window())
    {
      Window xid = dash->GetInputWindowId();
      XSetTransientForHint(screen->dpy(), onboard_->id(), xid);
      onboard_->raise();
    }
  }
}

/* Start up the unity components */
void UnityScreen::InitUnityComponents()
{
  Timer timer;
  nux::GetWindowCompositor().sigHiddenViewWindow.connect(sigc::mem_fun(this, &UnityScreen::OnViewHidden));

  bghash_.reset(new BGHash());
  LOG_INFO(logger) << "InitUnityComponents-BGHash " << timer.ElapsedSeconds() << "s";

  auto xdnd_collection_window = std::make_shared<XdndCollectionWindowImp>();
  auto xdnd_start_stop_notifier = std::make_shared<XdndStartStopNotifierImp>();
  auto xdnd_manager = std::make_shared<XdndManagerImp>(xdnd_start_stop_notifier, xdnd_collection_window);
  edge_barriers_ = std::make_shared<ui::EdgeBarrierController>();

  launcher_controller_ = std::make_shared<launcher::Controller>(xdnd_manager, edge_barriers_);
  Introspectable::AddChild(launcher_controller_.get());
  LOG_INFO(logger) << "InitUnityComponents-Launcher " << timer.ElapsedSeconds() << "s";

  switcher_controller_ = std::make_shared<switcher::Controller>();
  switcher_controller_->detail.changed.connect(sigc::mem_fun(this, &UnityScreen::OnSwitcherDetailChanged));
  Introspectable::AddChild(switcher_controller_.get());
  launcher_controller_->icon_added.connect(sigc::mem_fun(switcher_controller_.get(), &switcher::Controller::AddIcon));
  launcher_controller_->icon_removed.connect(sigc::mem_fun(switcher_controller_.get(), &switcher::Controller::RemoveIcon));
  LOG_INFO(logger) << "InitUnityComponents-Switcher " << timer.ElapsedSeconds() << "s";

  /* Setup panel */
  timer.Reset();
  panel_controller_ = std::make_shared<panel::Controller>(menus_, edge_barriers_);
  Introspectable::AddChild(panel_controller_.get());
  LOG_INFO(logger) << "InitUnityComponents-Panel " << timer.ElapsedSeconds() << "s";

  /* Setup Places */
  dash_controller_ = std::make_shared<dash::Controller>();
  dash_controller_->on_realize.connect(sigc::mem_fun(this, &UnityScreen::OnDashRealized));
  Introspectable::AddChild(dash_controller_.get());

  /* Setup Hud */
  hud_controller_ = std::make_shared<hud::Controller>();
  auto hide_mode = (unity::launcher::LauncherHideMode) optionGetLauncherHideMode();
  hud_controller_->launcher_locked_out = (hide_mode == unity::launcher::LauncherHideMode::LAUNCHER_HIDE_NEVER);
  hud_controller_->multiple_launchers = (optionGetNumLaunchers() == 0);
  hud_controller_->icon_size = launcher_controller_->options()->icon_size();
  hud_controller_->tile_size = launcher_controller_->options()->tile_size();
  Introspectable::AddChild(hud_controller_.get());
  LOG_INFO(logger) << "InitUnityComponents-Hud " << timer.ElapsedSeconds() << "s";

  // Setup Shortcut Hint
  auto base_window_raiser = std::make_shared<shortcut::BaseWindowRaiserImp>();
  auto shortcuts_modeller = std::make_shared<shortcut::CompizModeller>();
  shortcut_controller_ = std::make_shared<shortcut::Controller>(base_window_raiser, shortcuts_modeller);
  Introspectable::AddChild(shortcut_controller_.get());
  LOG_INFO(logger) << "InitUnityComponents-ShortcutHints " << timer.ElapsedSeconds() << "s";
  ShowFirstRunHints();

  // Setup Session Controller
  auto session = std::make_shared<session::GnomeManager>();
  session->lock_requested.connect(sigc::mem_fun(this, &UnityScreen::OnLockScreenRequested));
  session->prompt_lock_requested.connect(sigc::mem_fun(this, &UnityScreen::OnLockScreenRequested));
  session->locked.connect(sigc::mem_fun(this, &UnityScreen::OnScreenLocked));
  session->unlocked.connect(sigc::mem_fun(this, &UnityScreen::OnScreenUnlocked));
  session_dbus_manager_ = std::make_shared<session::DBusManager>(session);
  session_controller_ = std::make_shared<session::Controller>(session);
  LOG_INFO(logger) << "InitUnityComponents-Session " << timer.ElapsedSeconds() << "s";
  Introspectable::AddChild(session_controller_.get());

  // Setup Lockscreen Controller
  screensaver_dbus_manager_ = std::make_shared<lockscreen::DBusManager>(session);
  lockscreen_controller_ = std::make_shared<lockscreen::Controller>(screensaver_dbus_manager_, session, menus_->KeyGrabber());
  UpdateActivateIndicatorsKey();
  LOG_INFO(logger) << "InitUnityComponents-Lockscreen " << timer.ElapsedSeconds() << "s";

  if (g_file_test((DesktopUtilities::GetUserRuntimeDirectory()+local::LOCKED_STAMP).c_str(), G_FILE_TEST_EXISTS))
    session->PromptLockScreen();

  auto on_launcher_size_changed = [this] (nux::Area* area, int w, int h) {
    /* The launcher geometry includes 1px used to draw the right/top margin
     * that must not be considered when drawing an overlay */

    auto* launcher = static_cast<Launcher*>(area);
    auto launcher_position = Settings::Instance().launcher_position();

    int size = 0;
    if (launcher_position == LauncherPosition::LEFT)
      size = w;
    else
      size = h;
    int launcher_size = size - (1_em).CP(unity_settings_.em(launcher->monitor)->DPIScale());

    unity::Settings::Instance().SetLauncherSize(launcher_size, launcher->monitor);
    int adjustment_x = 0;
    if (launcher_position == LauncherPosition::LEFT)
      adjustment_x = launcher_size;
    shortcut_controller_->SetAdjustment(adjustment_x, panel_style_.PanelHeight(launcher->monitor));

    CompOption::Value v(launcher_size);
    if (launcher_position == LauncherPosition::LEFT)
    {
      screen->setOptionForPlugin("expo", "x_offset", v);

      if (launcher_controller_->options()->hide_mode == LAUNCHER_HIDE_NEVER)
        v.set(0);

      screen->setOptionForPlugin("scale", "x_offset", v);

      v.set(0);
      screen->setOptionForPlugin("expo", "y_bottom_offset", v);
      screen->setOptionForPlugin("scale", "y_bottom_offset", v);
    }
    else
    {
      screen->setOptionForPlugin("expo", "y_bottom_offset", v);

      if (launcher_controller_->options()->hide_mode == LAUNCHER_HIDE_NEVER)
        v.set(0);

      screen->setOptionForPlugin("scale", "y_bottom_offset", v);

      v.set(0);
      screen->setOptionForPlugin("expo", "x_offset", v);
      screen->setOptionForPlugin("scale", "x_offset", v);
    }
  };

  auto check_launchers_size = [this, on_launcher_size_changed] {
    launcher_size_connections_.Clear();

    for (auto const& launcher : launcher_controller_->launchers())
    {
      launcher_size_connections_.Add(launcher->size_changed.connect(on_launcher_size_changed));
      on_launcher_size_changed(launcher.GetPointer(), launcher->GetWidth(), launcher->GetHeight());
    }
  };

  UScreen::GetDefault()->changed.connect([this, check_launchers_size] (int, std::vector<nux::Geometry> const&) {
    check_launchers_size();
  });

  Settings::Instance().launcher_position.changed.connect([this, check_launchers_size] (LauncherPosition const&) {
    check_launchers_size();
  });

  check_launchers_size();

  launcher_controller_->options()->scroll_inactive_icons = optionGetScrollInactiveIcons();
  launcher_controller_->options()->minimize_window_on_click = optionGetLauncherMinimizeWindow();

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

lockscreen::Controller::Ptr UnityScreen::lockscreen_controller()
{
  return lockscreen_controller_;
}

void UnityScreen::UpdateGesturesSupport()
{
  Settings::Instance().gestures_launcher_drag() ? gestures_sub_launcher_->Activate() : gestures_sub_launcher_->Deactivate();
  Settings::Instance().gestures_dash_tap() ? gestures_sub_dash_->Activate() : gestures_sub_dash_->Deactivate();
  Settings::Instance().gestures_windows_drag_pinch() ? gestures_sub_windows_->Activate() : gestures_sub_windows_->Deactivate();
}

void UnityScreen::InitGesturesSupport()
{
  std::unique_ptr<nux::GestureBroker> gesture_broker(new UnityGestureBroker);
  wt->GetWindowCompositor().SetGestureBroker(std::move(gesture_broker));
  gestures_sub_launcher_.reset(new nux::GesturesSubscription);
  gestures_sub_launcher_->SetGestureClasses(nux::DRAG_GESTURE);
  gestures_sub_launcher_->SetNumTouches(4);
  gestures_sub_launcher_->SetWindowId(GDK_ROOT_WINDOW());

  gestures_sub_dash_.reset(new nux::GesturesSubscription);
  gestures_sub_dash_->SetGestureClasses(nux::TAP_GESTURE);
  gestures_sub_dash_->SetNumTouches(4);
  gestures_sub_dash_->SetWindowId(GDK_ROOT_WINDOW());

  gestures_sub_windows_.reset(new nux::GesturesSubscription);
  gestures_sub_windows_->SetGestureClasses(nux::TOUCH_GESTURE
                                         | nux::DRAG_GESTURE
                                         | nux::PINCH_GESTURE);
  gestures_sub_windows_->SetNumTouches(3);
  gestures_sub_windows_->SetWindowId(GDK_ROOT_WINDOW());

  // Apply the user's settings
  UpdateGesturesSupport();
}

CompAction::Vector& UnityScreen::getActions()
{
  return menus_->KeyGrabber()->GetActions();
}

void UnityScreen::ShowFirstRunHints()
{
  sources_.AddTimeoutSeconds(2, [this] {
    auto const& config_dir = DesktopUtilities::GetUserConfigDirectory();
    if (!config_dir.empty() && !g_file_test((config_dir+local::FIRST_RUN_STAMP).c_str(), G_FILE_TEST_EXISTS))
    {
      // We focus the panel, so the shortcut hint will be hidden at first user input
      auto const& panels = panel_controller_->panels();
      if (!panels.empty())
      {
        auto panel_win = static_cast<nux::BaseWindow*>(panels.front()->GetTopLevelViewWindow());
        SaveInputThenFocus(panel_win->GetInputWindowId());
      }
      shortcut_controller_->first_run = true;
      shortcut_controller_->Show();

      glib::Error error;
      g_file_set_contents((config_dir+local::FIRST_RUN_STAMP).c_str(), "", 0, &error);

      if (error)
      {
        LOG_ERROR(logger) << "Impossible to save the unity stamp file: " << error;
      }
    }
    return false;
  });
}

Window UnityScreen::GetNextActiveWindow() const
{
  return next_active_window_;
}

void UnityScreen::SetNextActiveWindow(Window next_active_window)
{
  next_active_window_ = next_active_window;
}

/* Window init */

namespace
{
bool WindowHasInconsistentShapeRects(Display *d, Window  w)
{
  int n;
  Atom *atoms = XListProperties(d, w, &n);
  bool has_inconsistent_shape = false;

  for (int i = 0; i < n; ++i)
  {
    if (atoms[i] == atom::_UNITY_SAVED_WINDOW_SHAPE)
    {
      has_inconsistent_shape = true;
      break;
    }
  }

  XFree(atoms);
  return has_inconsistent_shape;
}
}

UnityWindow::UnityWindow(CompWindow* window)
  : BaseSwitchWindow(static_cast<BaseSwitchScreen*>(uScreen), window)
  , PluginClassHandler<UnityWindow, CompWindow>(window)
  , window(window)
  , cWindow(CompositeWindow::get(window))
  , gWindow(GLWindow::get(window))
  , close_icon_state_(decoration::WidgetState::NORMAL)
  , deco_win_(uScreen->deco_manager_->HandleWindow(window))
  , need_fake_deco_redraw_(false)
  , is_nux_window_(PluginAdapter::IsNuxWindow(window))
{
  WindowInterface::setHandler(window);
  GLWindowInterface::setHandler(gWindow);
  CompositeWindowInterface::setHandler(cWindow);
  ScaleWindowInterface::setHandler(ScaleWindow::get(window));

  PluginAdapter::Default().OnLeaveDesktop();

  /* This needs to happen before we set our wrapable functions, since we
   * need to ask core (and not ourselves) whether or not the window is
   * minimized */
  if (uScreen->optionGetShowMinimizedWindows() && window->mapNum() &&
      WindowHasInconsistentShapeRects(screen->dpy (), window->id()))
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

  /* Keep this after the optionGetShowMIntrospectable.hinimizedWindows branch */

  if (window->state() & CompWindowStateFullscreenMask)
    uScreen->fullscreen_windows_.push_back(window);

  if (WindowManager::Default().IsOnscreenKeyboard(window->id()) && window->isViewable())
  {
    uScreen->onboard_ = window;
    uScreen->RaiseOSK();
  }
}


void UnityWindow::AddProperties(debug::IntrospectionData& introspection)
{
  Window xid = window->id();
  auto const& swins = uScreen->sScreen->getWindows();
  WindowManager& wm = uScreen->WM;
  bool scaled = std::find(swins.begin(), swins.end(), ScaleWindow::get(window)) != swins.end();

  introspection
    .add(scaled ? GetScaledGeometry() : wm.GetWindowGeometry(xid))
    .add("xid", xid)
    .add("title", wm.GetWindowName(xid))
    .add("fake_decorated", uScreen->fake_decorated_windows_.find(this) != uScreen->fake_decorated_windows_.end())
    .add("maximized", wm.IsWindowMaximized(xid))
    .add("horizontally_maximized", wm.IsWindowHorizontallyMaximized(xid))
    .add("vertically_maximized", wm.IsWindowVerticallyMaximized(xid))
    .add("minimized", wm.IsWindowMinimized(xid))
    .add("scaled", scaled)
    .add("scaled_close_geo", close_button_geo_)
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
      GLTexture::MatrixList ml({texture->matrix()});
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

void UnityWindow::RenderDecoration(compiz_utils::CairoContext const& ctx, double aspect)
{
  if (aspect <= 0)
    return;

  using namespace decoration;

  aspect *= deco_win_->dpi_scale();
  double w = ctx.width() / aspect;
  double h = ctx.height() / aspect;
  Style::Get()->DrawSide(Side::TOP, WidgetState::NORMAL, ctx, w, h);
}

void UnityWindow::RenderTitle(compiz_utils::CairoContext const& ctx, int x, int y, int width, int height, double aspect)
{
  using namespace decoration;
  auto const& style = Style::Get();
  auto const& title = deco_win_->title();
  auto text_size = style->TitleNaturalSize(title);
  x += style->TitleIndent();
  y += (height - text_size.height)/2;

  cairo_save(ctx);
  cairo_scale(ctx, 1.0f/aspect, 1.0f/aspect);
  cairo_translate(ctx, x, y);
  style->DrawTitle(title, WidgetState::NORMAL, ctx, width - x, height);
  cairo_restore(ctx);
}

void UnityWindow::BuildDecorationTexture()
{
  auto const& border = decoration::Style::Get()->Border();

  if (border.top)
  {
    double dpi_scale = deco_win_->dpi_scale();
    compiz_utils::CairoContext context(window->borderRect().width(), border.top * dpi_scale, dpi_scale);
    RenderDecoration(context);
    decoration_tex_ = context;
  }
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

  if (!decoration_tex_ && compiz_utils::IsWindowFullyDecorable(window))
    BuildDecorationTexture();

  if (!highlighted)
  {
    if (decoration_tex_)
      DrawTexture(*decoration_tex_, attrib, transform, mask, geo.x, geo.y, scale);

    close_button_geo_.Set(0, 0, 0, 0);
  }
  else
  {
    auto const& style = decoration::Style::Get();
    double dpi_scale = deco_win_->dpi_scale();
    int width = geo.width;
    int height = style->Border().top * dpi_scale;
    auto const& padding = style->Padding(decoration::Side::TOP);
    bool redraw_decoration = true;
    compiz_utils::SimpleTexture::Ptr close_texture;

    if (decoration_selected_tex_)
    {
      auto* texture = decoration_selected_tex_->texture();
      if (texture && texture->width() == width && texture->height() == height)
      {
        if (decoration_title_ == deco_win_->title())
          redraw_decoration = false;
      }
    }

    if (window->actions() & CompWindowActionCloseMask)
    {
      using namespace decoration;
      close_texture = DataPool::Get()->ButtonTexture(dpi_scale, WindowButtonType::CLOSE, close_icon_state_);
    }

    if (redraw_decoration)
    {
      if (width != 0 && height != 0)
      {
        compiz_utils::CairoContext context(width, height, scale * dpi_scale);
        RenderDecoration(context, scale);

        // Draw window title
        int text_x = padding.left + (close_texture ? close_texture->width() : 0) / dpi_scale;
        RenderTitle(context, text_x, padding.top, (width - padding.right) / dpi_scale, height / dpi_scale, scale);
        decoration_selected_tex_ = context;
        decoration_title_ = deco_win_->title();
        uScreen->damageRegion(CompRegionFromNuxGeo(geo));
        need_fake_deco_redraw_ = true;

        if (decoration_tex_)
          DrawTexture(*decoration_tex_, attrib, transform, mask, geo.x, geo.y, scale);

        return; // Let's draw this at next repaint cycle
      }
      else
      {
        decoration_selected_tex_.reset();
        redraw_decoration = false;
      }
    }
    else
    {
      need_fake_deco_redraw_ = false;
    }

    if (decoration_selected_tex_)
      DrawTexture(*decoration_selected_tex_, attrib, transform, mask, geo.x, geo.y);

    if (close_texture)
    {
      int w = close_texture->width();
      int h = close_texture->height();
      int x = geo.x + padding.left * dpi_scale;
      int y = geo.y + padding.top * dpi_scale + (height - w) / 2.0f;

      close_button_geo_.Set(x, y, w, h);
      DrawTexture(*close_texture, attrib, transform, mask, x, y);
    }
    else
    {
      close_button_geo_.Set(0, 0, 0, 0);
    }
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

  auto state = uScreen->sScreen->getState();

  if (state != ScaleScreen::Wait && state != ScaleScreen::Out && !need_fake_deco_redraw_)
    return;

  nux::Geometry const& scale_geo = GetScaledGeometry();
  auto const& pos = scale_win->getCurrentPosition();
  auto deco_attrib = attrib;
  deco_attrib.opacity = COMPIZ_COMPOSITE_OPAQUE;

  bool highlighted = (uScreen->sScreen->getSelectedWindow() == window->id());
  paintFakeDecoration(scale_geo, deco_attrib, transform, mask, highlighted, pos.scale);
}

nux::Geometry UnityWindow::GetLayoutWindowGeometry()
{
  auto const& layout_window = uScreen->GetSwitcherDetailLayoutWindow(window->id());

  if (layout_window)
    return layout_window->result;

  return nux::Geometry();
}

nux::Geometry UnityWindow::GetScaledGeometry()
{
  if (!uScreen->WM.IsScaleActive())
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
  close_icon_state_ = decoration::WidgetState::NORMAL;
  middle_clicked_ = false;
  deco_win_->scaled = true;

  if (IsInShowdesktopMode())
  {
    if (mShowdesktopHandler)
    {
      mShowdesktopHandler->FadeIn();
    }
  }
}

void UnityWindow::OnTerminateSpread()
{
  CleanupCachedTextures();
  deco_win_->scaled = false;

  if (IsInShowdesktopMode())
  {
    if (uScreen->GetNextActiveWindow() != window->id())
    {
      if (!mShowdesktopHandler)
        mShowdesktopHandler.reset(new ShowdesktopHandler(static_cast <ShowdesktopHandlerWindowInterface *>(this),
                                                         static_cast <compiz::WindowInputRemoverLockAcquireInterface *>(this)));
      mShowdesktopHandler->FadeOut();
    }
    else
    {
      window->setShowDesktopMode(false);
    }
  }
}

void UnityWindow::paintInnerGlow(nux::Geometry glow_geo, GLMatrix const& matrix, GLWindowPaintAttrib const& attrib, unsigned mask)
{
  using namespace decoration;
  auto const& style = Style::Get();
  double dpi_scale = deco_win_->dpi_scale();
  unsigned glow_size = std::round(style->GlowSize() * dpi_scale);
  auto const& glow_texture = DataPool::Get()->GlowTexture();

  if (!glow_size || !glow_texture)
    return;

  auto const& radius = style->CornerRadius();
  int decoration_radius = std::max({radius.top, radius.left, radius.right, radius.bottom});

  if (decoration_radius > 0)
  {
    // We paint the glow below the window edges to correctly
    // render the rounded corners
    int inside_glow = decoration_radius * dpi_scale / 4;
    glow_size += inside_glow;
    glow_geo.Expand(-inside_glow, -inside_glow);
  }

  glow::Quads const& quads = computeGlowQuads(glow_geo, *glow_texture, glow_size);
  paintGlow(matrix, attrib, quads, *glow_texture, style->GlowColor(), mask);
}

void UnityWindow::paintThumbnail(nux::Geometry const& geo, float alpha, float parent_alpha, float scale_ratio, unsigned deco_height, bool selected)
{
  GLMatrix matrix;
  matrix.toScreenSpace(uScreen->last_output_, -DEFAULT_Z_CAMERA);
  last_bound = geo;

  GLWindowPaintAttrib attrib = gWindow->lastPaintAttrib();
  attrib.opacity = (alpha * parent_alpha * COMPIZ_COMPOSITE_OPAQUE);
  unsigned mask = gWindow->lastMask();
  nux::Geometry thumb_geo = geo;

  if (selected)
    paintInnerGlow(thumb_geo, matrix, attrib, mask);

  thumb_geo.y += std::round(deco_height * 0.5f * scale_ratio);
  nux::Geometry const& g = thumb_geo;

  paintThumb(attrib, matrix, mask, g.x, g.y, g.width, g.height, g.width, g.height);

  mask |= PAINT_WINDOW_BLEND_MASK;
  attrib.opacity = parent_alpha * COMPIZ_COMPOSITE_OPAQUE;

  // The thumbnail is still animating, don't draw the decoration as selected
  if (selected && alpha < 1.0f)
    selected = false;

  paintFakeDecoration(geo, attrib, matrix, mask, selected, scale_ratio);
}

UnityWindow::~UnityWindow()
{
  if (uScreen->newFocusedWindow && UnityWindow::get(uScreen->newFocusedWindow) == this)
    uScreen->newFocusedWindow = NULL;

  uScreen->deco_manager_->UnHandleWindow(window);

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

  if (window == uScreen->onboard_)
    uScreen->onboard_ = nullptr;

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

  unity_a11y_preset_environment();

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

void ScreenIntrospection::AddProperties(debug::IntrospectionData& introspection)
{}

Introspectable::IntrospectableList ScreenIntrospection::GetIntrospectableChildren()
{
  IntrospectableList children({uScreen->spread_filter_.get()});

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

#ifndef USE_GLES
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
#endif

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
    module += '.' + log_domain;
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

