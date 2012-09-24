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
#include <Nux/NuxTimerTickSource.h>

#include <UnityCore/Variant.h>

#include "BaseWindowRaiserImp.h"
#include "IconRenderer.h"
#include "Launcher.h"
#include "LauncherIcon.h"
#include "LauncherController.h"
#include "PluginAdapter.h"
#include "QuicklistManager.h"
#include "StartupNotifyService.h"
#include "Timer.h"
#include "KeyboardUtil.h"
#include "unityshell.h"
#include "BackgroundEffectHelper.h"
#include "UnityGestureBroker.h"

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

const unsigned int SCALE_CLOSE_ICON_SIZE = 19;
const unsigned int SCALE_ITEMS_PADDING = 5;

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
} // anon namespace

UnityScreen::UnityScreen(CompScreen* screen)
  : BaseSwitchScreen (screen)
  , PluginClassHandler <UnityScreen, CompScreen> (screen)
  , screen(screen)
  , cScreen(CompositeScreen::get(screen))
  , gScreen(GLScreen::get(screen))
  , debugger_(this)
  , enable_shortcut_overlay_(true)
  , needsRelayout(false)
  , _in_paint(false)
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
  , panel_texture_has_changed_(true)
  , paint_panel_(false)
  , scale_just_activated_(false)
  , screen_introspection_(screen)
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
     WindowManager::SetDefault(PluginAdapter::Default());
     AddChild(PluginAdapter::Default());

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

    tick_source_.reset(new nux::NuxTimerTickSource);
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
     _in_paint = false;

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
     optionSetPanelFirstMenuInitiate(boost::bind(&UnityScreen::showPanelFirstMenuKeyInitiate, this, _1, _2, _3));
     optionSetPanelFirstMenuTerminate(boost::bind(&UnityScreen::showPanelFirstMenuKeyTerminate, this, _1, _2, _3));
     optionSetAutomaximizeValueNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetAltTabTimeoutNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetAltTabBiasViewportNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));
     optionSetDisableShowDesktopNotify(boost::bind(&UnityScreen::optionChanged, this, _1, _2));

     optionSetAltTabForwardAllInitiate(boost::bind(&UnityScreen::altTabForwardAllInitiate, this, _1, _2, _3));
     optionSetAltTabForwardInitiate(boost::bind(&UnityScreen::altTabForwardInitiate, this, _1, _2, _3));
     optionSetAltTabForwardTerminate(boost::bind(&UnityScreen::altTabTerminateCommon, this, _1, _2, _3));
     optionSetAltTabForwardAllTerminate(boost::bind(&UnityScreen::altTabTerminateCommon, this, _1, _2, _3));
     optionSetAltTabPrevAllInitiate(boost::bind(&UnityScreen::altTabPrevAllInitiate, this, _1, _2, _3));
     optionSetAltTabPrevInitiate(boost::bind(&UnityScreen::altTabPrevInitiate, this, _1, _2, _3));

     optionSetAltTabDetailStartInitiate(boost::bind(&UnityScreen::altTabDetailStartInitiate, this, _1, _2, _3));
     optionSetAltTabDetailStopInitiate(boost::bind(&UnityScreen::altTabDetailStopInitiate, this, _1, _2, _3));

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
       g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING,
                    &overlay_identity, &can_maximise, &overlay_monitor);

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

    AddChild(&screen_introspection_);
  }
}

UnityScreen::~UnityScreen()
{
  notify_uninit();

  unity_a11y_finalize();
  ::unity::ui::IconRenderer::DestroyTextures();
  QuicklistManager::Destroy();

  reset_glib_logging();
}

void UnityScreen::initAltTabNextWindow()
{
  KeyboardUtil key_util(screen->dpy());
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

  glDrawBuffer(GL_BACK);
  glReadBuffer(GL_BACK);

  glPopAttrib();

  glDepthRange(0, 1);
#else
  glDepthRangef(0, 1);
  gScreen->resetRasterPos();
#endif

  glDisable(GL_SCISSOR_TEST);
}

void UnityScreen::setPanelShadowMatrix(const GLMatrix& matrix)
{
  panel_shadow_matrix_ = matrix;
}

void UnityScreen::paintPanelShadow(const CompRegion& clip)
{
  if (sources_.GetSource(local::RELAYOUT_TIMEOUT))
    return;

  if (PluginAdapter::Default()->IsExpoActive())
    return;

  CompOutput* output = _last_output;
  float panel_h = static_cast<float>(panel_style_.panel_height);

  // You have no shadow texture. But how?
  if (_shadow_texture.empty() || !_shadow_texture[0])
    return;

  float shadowX = output->x();
  float shadowY = output->y() + panel_h;
  float shadowWidth = output->width();
  float shadowHeight = _shadow_texture[0]->height();
  CompRect shadowRect(shadowX, shadowY, shadowWidth, shadowHeight);

  CompRegion redraw(clip);
  redraw &= shadowRect;

  if (redraw.isEmpty())
    return;

  const CompRect& bounds(redraw.boundingRect());

  // Sub-rectangle of the shadow needing redrawing:
  float x1 = bounds.x1();
  float y1 = bounds.y1();
  float x2 = bounds.x2();
  float y2 = bounds.y2();

  // Texture coordinates of the above rectangle:
  float tx1 = (x1 - shadowX) / shadowWidth;
  float ty1 = (y1 - shadowY) / shadowHeight;
  float tx2 = (x2 - shadowX) / shadowWidth;
  float ty2 = (y2 - shadowY) / shadowHeight;

  nuxPrologue();

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

  if (!(launcher_controller_->IsOverlayOpen() && current_monitor == overlay_monitor_)
      && panel_controller_->opacity() > 0.0f)
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
  nuxEpilogue();
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
  panel_texture_has_changed_ = true;

  // Reload the windows themed textures
  UnityWindow::CleanupSharedTextures();

  if (WindowManager::Default()->IsScaleActive())
    UnityWindow::SetupSharedTextures();
}

void UnityScreen::paintDisplay()
{
  CompOutput *output = _last_output;

  DrawTopPanelBackground();

  auto gpu_device = nux::GetGraphicsDisplay()->GetGpuDevice();

  nux::ObjectPtr<nux::IOpenGLTexture2D> device_texture =
    gpu_device->CreateTexture2DFromID(gScreen->fbo ()->tex ()->name (),
                                      screen->width(), screen->height(), 1, nux::BITFMT_R8G8B8A8);

  gpu_device->backup_texture0_ = device_texture;

  nux::Geometry geo(0, 0, screen->width (), screen->height ());
  nux::Geometry outputGeo(output->x (), output->y (), output->width (), output->height ());
  BackgroundEffectHelper::monitor_rect_ = geo;

  GLint fboID;
  // Nux renders to the referenceFramebuffer when it's embedded.
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fboID);
  wt->GetWindowCompositor().SetReferenceFramebuffer(fboID, outputGeo);

  nuxPrologue();
  _in_paint = true;
  wt->RenderInterfaceFromForeignCmd (&outputGeo);
  _in_paint = false;
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
  didShellRepaint = true;
}

void UnityScreen::DrawTopPanelBackground()
{
  auto graphics_engine = nux::GetGraphicsDisplay()->GetGraphicsEngine();

  if (!graphics_engine->UsingGLSLCodePath() || !launcher_controller_->IsOverlayOpen() || !paint_panel_)
   return;

  if (TopPanelBackgroundTextureNeedsUpdate())
    UpdateTopPanelBackgroundTexture();

  if (panel_texture_.IsValid())
  {
    graphics_engine->ResetModelViewMatrixStack();
    graphics_engine->Push2DTranslationModelViewMatrix(0.0f, 0.0f, 0.0f);
    graphics_engine->ResetProjectionMatrix();
    graphics_engine->SetOrthographicProjectionMatrix(screen->width (), screen->height());

    nux::TexCoordXForm texxform;
    int panel_height = panel_style_.panel_height;
    graphics_engine->QRP_GLSL_1Tex(0, 0, screen->width (), panel_height, panel_texture_, texxform, nux::color::White);
  }
}

bool UnityScreen::TopPanelBackgroundTextureNeedsUpdate() const
{
  return panel_texture_has_changed_ || !panel_texture_.IsValid();
}

void UnityScreen::UpdateTopPanelBackgroundTexture()
{
  auto gpu_device = nux::GetGraphicsDisplay()->GetGpuDevice();
  auto &panel_style = panel::Style::Instance();

  panel_texture_.Release();

  std::unique_ptr<nux::NBitmapData> bitmap(panel_style.GetBackground(screen->width(), screen->height(), 1.0f));
  nux::ObjectPtr<nux::BaseTexture> texture2D(gpu_device->CreateSystemCapableTexture());

  if (bitmap && texture2D)
  {
    texture2D->Update(bitmap.get());
    panel_texture_ = texture2D->GetDeviceTexture();
  }

  panel_texture_has_changed_ = false;
}

bool UnityScreen::forcePaintOnTop ()
{
    return !allowWindowPaint ||
      ((switcher_controller_->Visible() ||
        PluginAdapter::Default()->IsExpoActive())
       && !fullscreen_windows_.empty () && (!(screen->grabbed () && !screen->otherGrabExist (NULL))));
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

void UnityScreen::EnableCancelAction(CancelActionTarget target, bool enabled, int modifiers)
{
  if (enabled)
  {
    /* Create a new keybinding for the Escape key and the current modifiers,
     * compiz will take of the ref-counting of the repeated actions */
    KeyCode escape = XKeysymToKeycode(screen->dpy(), XStringToKeysym("Escape"));
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
    UnityWindow *uw = UnityWindow::get (w);

    if (ShowdesktopHandler::ShouldHide (static_cast <ShowdesktopHandlerWindowInterface *> (uw)))
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
  if (!ShowdesktopHandler::InhibitingXid ())
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

  compiz::WindowInputRemoverLock::Ptr ret (new compiz::WindowInputRemoverLock (new compiz::WindowInputRemover (screen->dpy (), window->id ())));
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
          DoAddDamage();
      }
      break;

    case ButtonPress:
      if (event->xbutton.button == Button1 &&
          close_button_geo_.IsPointInside(event->xbutton.x_root, event->xbutton.y_root))
      {
        close_icon_state_ = panel::WindowState::PRESSED;
        DoAddDamage();
        handled = true;
      }
      else if (event->xbutton.button == Button2 &&
               GetScaledGeometry().IsPointInside(event->xbutton.x_root, event->xbutton.y_root))
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
          DoAddDamage();
        }

        if (was_pressed)
        {
          if (close_button_geo_.IsPointInside(event->xbutton.x_root, event->xbutton.y_root))
            window->close(0);

          handled = true;
        }

        if (middle_clicked_)
        {
          if (event->xbutton.button == Button2 &&
              GetScaledGeometry().IsPointInside(event->xbutton.x_root, event->xbutton.y_root))
          {
            window->close(0);
          }

          middle_clicked_ = false;
          handled = true;
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

bool UnityScreen::shellCouldBeHidden(CompOutput const& output)
{
  std::vector<Window> const& nuxwins(nux::XInputWindow::NativeHandleList());

  // Loop through windows from front to back
  CompWindowList const& wins = screen->windows();
  for ( CompWindowList::const_reverse_iterator r = wins.rbegin()
      ; r != wins.rend()
      ; ++r
      )
  {
    CompWindow* w = *r;

    /*
     * The shell is hidden if there exists any window that fully covers
     * the output and is in front of all Nux windows on that output.
     */
    if (w->isMapped() &&
        !(w->state () & CompWindowStateHiddenMask) &&
        w->geometry().contains(output))
    {
      return true;
    }
    else
    {
      for (Window n : nuxwins)
      {
        if (w->id() == n && output.intersects(w->geometry()))
          return false;
      }
    }
  }

  return false;
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
  paint_panel_ = false;

  // CompRegion has no clear() method. So this is the fastest alternative.
  fullscreenRegion = CompRegion();
  nuxRegion = CompRegion();

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

}

void UnityScreen::preparePaint(int ms)
{
  cScreen->preparePaint(ms);

  for (ShowdesktopHandlerWindowInterface *wi : ShowdesktopHandler::animating_windows)
    wi->HandleAnimations (ms);

  didShellRepaint = false;
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
  if (didShellRepaint)
    wt->ClearDrawList();

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
  if (wt->GetDrawList().empty())
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

  std::vector<nux::View*> const& panels(panel_controller_->GetPanelViews());
  for (nux::View* view : panels)
  {
    nux::Geometry const& geo = view->GetAbsoluteGeometry();

    CompRegion panel_region(geo.x, geo.y, geo.width, geo.height);

    if (damage.intersects(panel_region))
      view->QueueDraw();
  }

  QuicklistManager* qm = QuicklistManager::Default();
  if (qm)
  {
    QuicklistView* view = qm->Current();
    if (view)
    {
      nux::Geometry const& geo = view->GetAbsoluteGeometry();
      CompRegion quicklist_region(geo.x, geo.y, geo.width, geo.height);

      if (damage.intersects(quicklist_region))
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
  std::vector<nux::Geometry> const& dirty = wt->GetDrawList();
  if (!dirty.empty())
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
  auto wm = PluginAdapter::Default();

  switch (event->type)
  {
    case FocusIn:
    case FocusOut:
      if (event->xfocus.mode == NotifyGrab)
        wm->OnScreenGrabbed();
      else if (event->xfocus.mode == NotifyUngrab)
        wm->OnScreenUngrabbed();

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
      if (wm->IsScaleActive())
      {
        ScaleScreen* ss = ScaleScreen::get(screen);
        if (CompWindow *w = screen->findWindow(ss->getSelectedWindow()))
          skip_other_plugins = UnityWindow::get(w)->handleEvent(event);
      }
      break;
    case ButtonPress:
      if (super_keypressed_)
      {
        launcher_controller_->KeyNavTerminate(false);
        EnableCancelAction(CancelActionTarget::LAUNCHER_SWITCHER, false);
      }
      if (wm->IsScaleActive())
      {
        ScaleScreen* ss = ScaleScreen::get(screen);
        if (CompWindow *w = screen->findWindow(ss->getSelectedWindow()))
          skip_other_plugins = UnityWindow::get(w)->handleEvent(event);
      }


      if (dash_controller_->IsVisible())
      {
        nux::Point pt(event->xbutton.x_root, event->xbutton.y_root);
        nux::Geometry dash_geo = dash_controller_->GetInputWindowGeometry();

        if (!dash_geo.IsInside(pt) && !DoesPointIntersectUnityGeos(pt))
        {
          dash_controller_->HideDash(false);
        }
      }

      if (hud_controller_->IsVisible())
      {
        nux::Point pt(event->xbutton.x_root, event->xbutton.y_root);
        nux::Geometry hud_geo = hud_controller_->GetInputWindowGeometry();

        if (!hud_geo.IsInside(pt) && !DoesPointIntersectUnityGeos(pt))
        {
          hud_controller_->HideHud(false);
        }
      }
      break;
    case ButtonRelease:
      if (switcher_controller_ && switcher_controller_->Visible())
      {
        XButtonEvent *bev = reinterpret_cast<XButtonEvent*>(event);
        if (bev->time - last_scroll_event_ > 150)
        {
          if (bev->button == Button4 || bev->button == local::SCROLL_UP_BUTTON)
          {
            switcher_controller_->Prev();
            last_scroll_event_ = bev->time;
          }
          else if (bev->button == Button5 || bev->button == local::SCROLL_DOWN_BUTTON)
          {
            switcher_controller_->Next();
            last_scroll_event_ = bev->time;
          }
        }
      }
      else if (wm->IsScaleActive())
      {
        ScaleScreen* ss = ScaleScreen::get(screen);
        if (CompWindow *w = screen->findWindow(ss->getSelectedWindow()))
          skip_other_plugins = UnityWindow::get(w)->handleEvent(event);
      }
      break;
    case KeyPress:
    {
      if (super_keypressed_)
      {
        /* We need an idle to postpone this action, after the current event
         * has been processed */
        sources_.AddIdle([&] {
          shortcut_controller_->SetEnabled(false);
          shortcut_controller_->Hide();
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

      if (result > 0)
      {
        // NOTE: does this have the potential to do an invalid write?  Perhaps
        // we should just say "key_string[1] = 0" because that is the only
        // thing that could possibly make sense here.
        key_string[result] = 0;

        if (super_keypressed_)
        {
          skip_other_plugins = launcher_controller_->HandleLauncherKeyEvent(screen->dpy(), key_sym, event->xkey.keycode, event->xkey.state, key_string);
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
        PluginAdapter::Default ()->NotifyNewDecorationState(event->xproperty.window);
      }
      break;
    case MapRequest:
      ShowdesktopHandler::AllowLeaveShowdesktopMode (event->xmaprequest.window);
      break;
  }

  if (!skip_other_plugins &&
      screen->otherGrabExist("deco", "move", "switcher", "resize", NULL) &&
      !switcher_controller_->Visible())
  {
    wt->ProcessForeignEvent(event, NULL);
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
  PluginAdapter::Default()->NotifyCompizEvent(plugin, event, option);
  compiz::CompizMinimizedWindowHandler<UnityScreen, UnityWindow>::handleCompizEvent (plugin, event, option);

  if (launcher_controller_->IsOverlayOpen() && g_strcmp0(event, "start_viewport_switch") == 0)
  {
    ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
  }

  if (PluginAdapter::Default()->IsScaleActive() && g_strcmp0(plugin, "scale") == 0 &&
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
    int launcher_width = optionGetIconSize() + 18;
    int panel_height = panel_style_.panel_height;

    if (shortcut_controller_->Show())
    {
      shortcut_controller_->SetAdjustment(launcher_width, panel_height);
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
  if (PluginAdapter::Default()->IsScaleActive() && !scale_just_activated_ && launcher_controller_->AboutToShowDash(true, when))
  {
    PluginAdapter::Default()->TerminateScale();
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

  shortcut_controller_->SetEnabled(enable_shortcut_overlay_);
  shortcut_controller_->Hide();
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

  if (PluginAdapter::Default()->IsScaleActive())
  {
    PluginAdapter::Default()->TerminateScale();
  }

  ubus_manager_.SendMessage(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                            g_variant_new("(sus)", "commands.lens", 0, ""));
}

bool UnityScreen::executeCommand(CompAction* action,
                                 CompAction::State state,
                                 CompOption::Vector& options)
{
  SendExecuteCommand();
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
    grab_index_ = screen->pushGrab (screen->invisibleCursor(), "unity-switcher");
  if (!grab_index_)
    return false;

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

  SetUpAndShowSwitcher(show_mode);

  return true;
}

void UnityScreen::SetUpAndShowSwitcher(switcher::ShowMode show_mode)
{
  // maybe check launcher position/hide state?

  auto uscreen = UScreen::GetDefault();
  int monitor = uscreen->GetMonitorWithMouse();
  auto monitor_geo = uscreen->GetMonitorGeometry(monitor);

  monitor_geo.x += 100;
  monitor_geo.y += 100;
  monitor_geo.width -= 200;
  monitor_geo.height -= 200;
  switcher_controller_->SetWorkspace(monitor_geo, monitor);

  RaiseInputWindows();

  auto results = launcher_controller_->GetAltTabIcons(show_mode == switcher::ShowMode::CURRENT_VIEWPORT,
                                                      switcher_controller_->IsShowDesktopDisabled());

  if (switcher_controller_->CanShowSwitcher(results))
    switcher_controller_->Show(show_mode, switcher::SortMode::FOCUS_ORDER, false, results);
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
    altTabInitiateCommon(action, switcher::ShowMode::CURRENT_VIEWPORT);

  action->setState(action->state() | CompAction::StateTermKey);
  return true;
}

bool UnityScreen::altTabForwardAllInitiate(CompAction* action,
                                        CompAction::State state,
                                        CompOption::Vector& options)
{
  if (WindowManager::Default()->IsWallActive())
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

bool UnityScreen::altTabDetailStartInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  if (switcher_controller_->Visible())
  {
    switcher_controller_->SetDetail(true);
    return true;
  }

  return false;
}

bool UnityScreen::altTabDetailStopInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options)
{
  if (switcher_controller_->Visible())
  {
    switcher_controller_->SetDetail(false);
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
  }

  switcher_controller_->NextDetail();

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
  RestoreWindow(data);
}

void UnityScreen::OnSwitcherStart(GVariant* data)
{
  SaveInputThenFocus(switcher_controller_->GetSwitcherInputWindowId());
}

void UnityScreen::OnSwitcherEnd(GVariant* data)
{
  RestoreWindow(data);
}

void UnityScreen::RestoreWindow(GVariant* data)
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

bool UnityScreen::SaveInputThenFocus(const guint xid)
{
  // get CompWindow*
  newFocusedWindow = screen->findWindow(xid);

  // check if currently focused window isn't it self
  if (xid != screen->activeWindow())
    PluginAdapter::Default()->saveInputFocus();

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

bool UnityScreen::initPluginActions()
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

bool isNuxWindow (CompWindow* value)
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
  if (isNuxWindow(window))
  {
    if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
    {
      uScreen->nuxRegion += window->geometry();
      uScreen->nuxRegion -= uScreen->fullscreenRegion;
    }
    return false;  // Ensure nux windows are never painted by compiz
  }
  else if (mask & PAINT_WINDOW_OCCLUSION_DETECTION_MASK)
  {
    static const unsigned int nonOcclusionBits =
                              PAINT_WINDOW_TRANSLUCENT_MASK |
                              PAINT_WINDOW_TRANSFORMED_MASK |
                              PAINT_WINDOW_NO_CORE_INSTANCE_MASK;
    if (!(mask & nonOcclusionBits) &&
        (window->state() & CompWindowStateFullscreenMask))
        // And I've been advised to test other things, but they don't work:
        // && (attrib.opacity == OPAQUE)) <-- Doesn't work; Only set in glDraw
        // && !window->alpha() <-- Doesn't work; Opaque windows often have alpha
    {
      uScreen->fullscreenRegion += window->geometry();
      uScreen->fullscreenRegion -= uScreen->nuxRegion;
    }
    if (uScreen->nuxRegion.isEmpty())
      uScreen->firstWindowAboveShell = window;
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
  if (uScreen->doShellRepaint && !uScreen->paint_panel_ && window->type() == CompWindowTypeNormalMask)
  {
    guint32 id = window->id();
    bool maximized = WindowManager::Default()->IsWindowMaximized(id);
    bool on_current = window->onCurrentDesktop();
    bool override_redirect = window->overrideRedirect();
    bool managed = window->managed();
    CompPoint viewport = window->defaultViewport();
    int output = window->outputDevice();

    if (maximized && on_current && !override_redirect && managed && viewport == uScreen->screen->vp() && output == (int)uScreen->screen->currentOutputDev().id())
    {
      uScreen->paint_panel_ = true;
    }
  }

  if (uScreen->doShellRepaint &&
      !uScreen->forcePaintOnTop () &&
      window == uScreen->firstWindowAboveShell &&
      !uScreen->fullscreenRegion.contains(window->geometry())
     )
  {
    uScreen->paintDisplay();
  }

  if (window->type() == CompWindowTypeDesktopMask)
    uScreen->setPanelShadowMatrix(matrix);

  Window active_window = screen->activeWindow();
  if (window->id() == active_window && window->type() != CompWindowTypeDesktopMask)
  {
    uScreen->paintPanelShadow(region);
  }

  bool ret = gWindow->glDraw(matrix, attrib, region, mask);

  if ((active_window == 0 || active_window == window->id()) &&
      (window->type() == CompWindowTypeDesktopMask))
  {
    uScreen->paintPanelShadow(region);
  }


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
UnityWindow::minimized ()
{
  return mMinimizeHandler.get () != nullptr;
}

/* Called whenever a window is mapped, unmapped, minimized etc */
void UnityWindow::windowNotify(CompWindowNotify n)
{
  PluginAdapter::Default()->Notify(window, n);

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
    CompWindow *lw;

    // can't rely on launcher->IsOverlayVisible on focus change (because ubus is async close on focus change.)
    if (us && (us->dash_controller_->IsVisible() || us->hud_controller_->IsVisible()))
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
    mShowdesktopHandler->UpdateFrameRegion (region);
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

      /* The launcher geometry includes 1px used to draw the right margin
       * that must not be considered when drawing an overlay */
      hud_controller_->launcher_width = launcher_controller_->launcher().GetAbsoluteWidth() - 1;
      dash_controller_->launcher_width = launcher_controller_->launcher().GetAbsoluteWidth() - 1;
      panel_controller_->launcher_width = launcher_controller_->launcher().GetAbsoluteWidth() - 1;

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
    case UnityshellOptions::AltTabTimeout:
      switcher_controller_->detail_on_timeout = optionGetAltTabTimeout();
    case UnityshellOptions::AltTabBiasViewport:
      PluginAdapter::Default()->bias_active_to_viewport = optionGetAltTabBiasViewport();
      break;
    case UnityshellOptions::DisableShowDesktop:
      switcher_controller_->SetShowDesktopDisabled(optionGetDisableShowDesktop());
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
  launcher_controller_ = std::make_shared<launcher::Controller>();
  AddChild(launcher_controller_.get());

  switcher_controller_ = std::make_shared<switcher::Controller>();
  AddChild(switcher_controller_.get());

  LOG_INFO(logger) << "initLauncher-Launcher " << timer.ElapsedSeconds() << "s";

  /* Setup panel */
  timer.Reset();
  panel_controller_ = std::make_shared<panel::Controller>();
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
  InitHints();
  auto base_window_raiser_ = std::make_shared<shortcut::BaseWindowRaiserImp>();
  shortcut_controller_ = std::make_shared<shortcut::Controller>(hints_, base_window_raiser_);
  AddChild(shortcut_controller_.get());

  AddChild(dash_controller_.get());

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

void UnityScreen::InitHints()
{
  // TODO move category text into a vector...

  // Compiz' plug-in names
  static const std::string COMPIZ_CORE_PLUGIN_NAME = "core";
  static const std::string COMPIZ_EXPO_PLUGIN_NAME = "expo";
  static const std::string COMPIZ_GRID_PLUGIN_NAME = "grid";
  static const std::string COMPIZ_MOVE_PLUGIN_NAME = "move";
  static const std::string COMPIZ_RESIZE_PLUGIN_NAME = "resize";
  static const std::string COMPIZ_SCALE_PLUGIN_NAME = "scale";
  static const std::string COMPIZ_UNITYSHELL_PLUGIN_NAME = "unityshell";
  static const std::string COMPIZ_WALL_PLUGIN_NAME = "wall";

  // Compiz Core Options
  static const std::string COMPIZ_CORE_OPTION_SHOW_DESKTOP_KEY = "show_desktop_key";
  static const std::string COMPIZ_CORE_OPTION_MAXIMIZE_WINDOW_KEY = "maximize_window_key";
  static const std::string COMPIZ_CORE_OPTION_UNMAXIMIZE_WINDOW_KEY = "unmaximize_window_key";
  static const std::string COMPIZ_CORE_OPTION_CLOSE_WINDOW_KEY = "close_window_key";
  static const std::string COMPIZ_CORE_OPTION_WINDOW_MENU_KEY = "window_menu_key";

  // Compiz Expo Options
  static const std::string COMPIZ_EXPO_OPTION_EXPO_KEY = "expo_key";

  // Compiz Grid Options
  static const std::string COMPIZ_GRID_OPTION_PUT_LEFT_KEY = "put_left_key";

  // Compiz Move Options
  static const std::string COMPIZ_MOVE_OPTION_INITIATE_BUTTON = "initiate_button";

  // Compiz Resize Options
  static const std::string COMPIZ_RESIZE_OPTION_INITIATE_BUTTON = "initiate_button";

  // Compiz Scale Options
  static const std::string COMPIZ_SCALE_OPTION_INITIATE_ALL_KEY = "initiate_all_key";

  // Compiz Unityshell Options
  static const std::string COMPIZ_UNITYSHELL_OPTION_SHOW_LAUNCHER = "show_launcher";
  static const std::string COMPIZ_UNITYSHELL_OPTION_KEYBOARD_FOCUS = "keyboard_focus";
  static const std::string COMPIZ_UNITYSHELL_OPTION_LAUNCHER_SWITCHER_FORWARD = "launcher_switcher_forward";
  static const std::string COMPIZ_UNITYSHELL_OPTION_SHOW_HUD = "show_hud";
  static const std::string COMPIZ_UNITYSHELL_OPTION_PANEL_FIRST_MENU = "panel_first_menu";
  static const std::string COMPIZ_UNITYSHELL_OPTION_ALT_TAB_FORWARD = "alt_tab_forward";
  static const std::string COMPIZ_UNITYSHELL_OPTION_ALT_TAB_NEXT_WINDOW = "alt_tab_next_window";

  // Compiz Wall Options
  static const std::string COMPIZ_WALL_OPTION_LEFT_KEY = "left_key";
  static const std::string COMPIZ_WALL_OPTION_LEFT_WINDOW_KEY = "left_window_key";


  // Launcher...
  static const std::string launcher(_("Launcher"));

  hints_.push_back(std::make_shared<shortcut::Hint>(launcher, "", _(" (Hold)"),
                                                   _("Opens the Launcher, displays shortcuts."),
                                                   shortcut::COMPIZ_KEY_OPTION,
                                                   COMPIZ_UNITYSHELL_PLUGIN_NAME,
                                                   COMPIZ_UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints_.push_back(std::make_shared<shortcut::Hint>(launcher, "", "",
                                                    _("Opens Launcher keyboard navigation mode."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_UNITYSHELL_PLUGIN_NAME,
                                                    COMPIZ_UNITYSHELL_OPTION_KEYBOARD_FOCUS));

  hints_.push_back(std::make_shared<shortcut::Hint>(launcher, "", "",
                                                    _("Switches applications via the Launcher."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_UNITYSHELL_PLUGIN_NAME,
                                                    COMPIZ_UNITYSHELL_OPTION_LAUNCHER_SWITCHER_FORWARD));

  hints_.push_back(std::make_shared<shortcut::Hint>(launcher, "", _(" + 1 to 9"),
                                                    _("Same as clicking on a Launcher icon."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_UNITYSHELL_PLUGIN_NAME,
                                                    COMPIZ_UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints_.push_back(std::make_shared<shortcut::Hint>(launcher, "", _(" + Shift + 1 to 9"),
                                                    _("Opens a new window in the app."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_UNITYSHELL_PLUGIN_NAME,
                                                    COMPIZ_UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints_.push_back(std::make_shared<shortcut::Hint>(launcher, "", " + T",
                                                    _("Opens the Trash."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_UNITYSHELL_PLUGIN_NAME,
                                                    COMPIZ_UNITYSHELL_OPTION_SHOW_LAUNCHER));


  // Dash...
  static const std::string dash( _("Dash"));

  hints_.push_back(std::make_shared<shortcut::Hint>(dash, "", _(" (Tap)"),
                                                    _("Opens the Dash Home."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_UNITYSHELL_PLUGIN_NAME,
                                                    COMPIZ_UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints_.push_back(std::make_shared<shortcut::Hint>(dash, "", " + A",
                                                    _("Opens the Dash App Lens."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_UNITYSHELL_PLUGIN_NAME,
                                                    COMPIZ_UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints_.push_back(std::make_shared<shortcut::Hint>(dash, "", " + F",
                                                    _("Opens the Dash Files Lens."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_UNITYSHELL_PLUGIN_NAME,
                                                    COMPIZ_UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints_.push_back(std::make_shared<shortcut::Hint>(dash, "", " + M",
                                                    _("Opens the Dash Music Lens."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_UNITYSHELL_PLUGIN_NAME,
                                                    COMPIZ_UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints_.push_back(std::make_shared<shortcut::Hint>(dash, "", " + V",
                                                    _("Opens the Dash Video Lens."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_UNITYSHELL_PLUGIN_NAME,
                                                    COMPIZ_UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints_.push_back(std::make_shared<shortcut::Hint>(dash, "", "",
                                                    _("Switches between Lenses."),
                                                    shortcut::HARDCODED_OPTION,
                                                    _("Ctrl + Tab")));

  hints_.push_back(std::make_shared<shortcut::Hint>(dash, "", "",
                                                    _("Moves the focus."),
                                                    shortcut::HARDCODED_OPTION,
                                                    _("Arrow Keys")));

  hints_.push_back(std::make_shared<shortcut::Hint>(dash, "", "",
                                                    _("Opens the currently focused item."),
                                                    shortcut::HARDCODED_OPTION,
                                                    _("Enter")));

  // Menu Bar
  static const std::string menubar(_("HUD & Menu Bar"));

  hints_.push_back(std::make_shared<shortcut::Hint>(menubar, "", _(" (Tap)"),
                                                    _("Opens the HUD."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_UNITYSHELL_PLUGIN_NAME,
                                                    COMPIZ_UNITYSHELL_OPTION_SHOW_HUD));

  hints_.push_back(std::make_shared<shortcut::Hint>(menubar, "", _(" (Hold)"),
                                                    _("Reveals the application menu."),
                                                    shortcut::HARDCODED_OPTION,
                                                    "Alt"));

  hints_.push_back(std::make_shared<shortcut::Hint>(menubar, "", "",
                                                    _("Opens the indicator menu."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_UNITYSHELL_PLUGIN_NAME,
                                                    COMPIZ_UNITYSHELL_OPTION_PANEL_FIRST_MENU));

  hints_.push_back(std::make_shared<shortcut::Hint>(menubar, "", "",
                                                    _("Moves focus between indicators."),
                                                    shortcut::HARDCODED_OPTION,
                                                    _("Cursor Left or Right")));

  // Switching
  static const std::string switching(_("Switching"));

  hints_.push_back(std::make_shared<shortcut::Hint>(switching, "", "",
                                                    _("Switches between applications."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_UNITYSHELL_PLUGIN_NAME,
                                                    COMPIZ_UNITYSHELL_OPTION_ALT_TAB_FORWARD));

  hints_.push_back(std::make_shared<shortcut::Hint>(switching, "", "",
                                                    _("Switches windows of current applications."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_UNITYSHELL_PLUGIN_NAME,
                                                    COMPIZ_UNITYSHELL_OPTION_ALT_TAB_NEXT_WINDOW));

  hints_.push_back(std::make_shared<shortcut::Hint>(switching, "", "",
                                                    _("Moves the focus."),
                                                    shortcut::HARDCODED_OPTION,
                                                    _("Cursor Left or Right")));

  // Workspaces
  static const std::string workspaces(_("Workspaces"));

  hints_.push_back(std::make_shared<shortcut::Hint>(workspaces, "", "",
                                                    _("Switches between workspaces."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_EXPO_PLUGIN_NAME,
                                                    COMPIZ_EXPO_OPTION_EXPO_KEY));

  hints_.push_back(std::make_shared<shortcut::Hint>(workspaces, "", _(" + Arrow Keys"),
                                                    _("Switches workspaces."),
                                                    shortcut::COMPIZ_METAKEY_OPTION,
                                                    COMPIZ_WALL_PLUGIN_NAME,
                                                    COMPIZ_WALL_OPTION_LEFT_KEY));

  hints_.push_back(std::make_shared<shortcut::Hint>(workspaces, "", _(" + Arrow Keys"),
                                                    _("Moves focused window to another workspace."),
                                                    shortcut::COMPIZ_METAKEY_OPTION,
                                                    COMPIZ_WALL_PLUGIN_NAME,
                                                    COMPIZ_WALL_OPTION_LEFT_WINDOW_KEY));


  // Windows
  static const std::string windows(_("Windows"));

  hints_.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                    _("Spreads all windows in the current workspace."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_SCALE_PLUGIN_NAME,
                                                    COMPIZ_SCALE_OPTION_INITIATE_ALL_KEY));

  hints_.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                    _("Minimises all windows."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_CORE_PLUGIN_NAME,
                                                    COMPIZ_CORE_OPTION_SHOW_DESKTOP_KEY));

  hints_.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                    _("Maximises the current window."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_CORE_PLUGIN_NAME,
                                                    COMPIZ_CORE_OPTION_MAXIMIZE_WINDOW_KEY));

  hints_.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                    _("Restores or minimises the current window."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_CORE_PLUGIN_NAME,
                                                    COMPIZ_CORE_OPTION_UNMAXIMIZE_WINDOW_KEY));

  hints_.push_back(std::make_shared<shortcut::Hint>(windows, "", _(" or Right"),
                                                    _("Semi-maximise the current window."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_GRID_PLUGIN_NAME,
                                                    COMPIZ_GRID_OPTION_PUT_LEFT_KEY));

  hints_.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                    _("Closes the current window."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_CORE_PLUGIN_NAME,
                                                    COMPIZ_CORE_OPTION_CLOSE_WINDOW_KEY));

  hints_.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                    _("Opens the window accessibility menu."),
                                                    shortcut::COMPIZ_KEY_OPTION,
                                                    COMPIZ_CORE_PLUGIN_NAME,
                                                    COMPIZ_CORE_OPTION_WINDOW_MENU_KEY));

  hints_.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                    _("Places the window in corresponding position."),
                                                    shortcut::HARDCODED_OPTION,
                                                    _("Ctrl + Alt + Num")));

  hints_.push_back(std::make_shared<shortcut::Hint>(windows, "", _(" Drag"),
                                                    _("Moves the window."),
                                                    shortcut::COMPIZ_MOUSE_OPTION,
                                                    COMPIZ_MOVE_PLUGIN_NAME,
                                                    COMPIZ_MOVE_OPTION_INITIATE_BUTTON));

  hints_.push_back(std::make_shared<shortcut::Hint>(windows, "", _(" Drag"),
                                                    _("Resizes the window."),
                                                    shortcut::COMPIZ_MOUSE_OPTION,
                                                    COMPIZ_RESIZE_PLUGIN_NAME,
                                                    COMPIZ_RESIZE_OPTION_INITIATE_BUTTON));
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

struct UnityWindow::CairoContext
{
  CairoContext(int width, int height)
    : pixmap_(XCreatePixmap(screen->dpy(), screen->root(), width, height, 32))
    , texture_(GLTexture::bindPixmapToTexture(pixmap_, width, height, 32))
    , surface_(nullptr)
    , cr_(nullptr)
  {
      Screen *xscreen = ScreenOfDisplay(screen->dpy(), screen->screenNum());
      XRenderPictFormat* format = XRenderFindStandardFormat(screen->dpy(), PictStandardARGB32);

      if (texture_.empty())
        return;

      surface_ = cairo_xlib_surface_create_with_xrender_format(screen->dpy(), pixmap_,
                                                               xscreen, format,
                                                               width, height);
      cr_ = cairo_create(surface_);

      // clear
      cairo_save(cr_);
      cairo_set_operator(cr_, CAIRO_OPERATOR_CLEAR);
      cairo_paint(cr_);
      cairo_restore(cr_);
  }

  ~CairoContext ()
  {
    if (cr_)
      cairo_destroy(cr_);

    if (surface_)
      cairo_surface_destroy(surface_);

    texture_.clear();

    if (pixmap_)
      XFreePixmap(screen->dpy (), pixmap_);
  }

  Pixmap pixmap_;
  GLTexture::List texture_;
  cairo_surface_t* surface_;
  cairo_t *cr_;
};

UnityWindow::UnityWindow(CompWindow* window)
  : BaseSwitchWindow (dynamic_cast<BaseSwitchScreen *> (UnityScreen::get (screen)), window)
  , PluginClassHandler<UnityWindow, CompWindow>(window)
  , window(window)
  , gWindow(GLWindow::get(window))
{
  WindowInterface::setHandler(window);
  GLWindowInterface::setHandler(gWindow);
  ScaleWindowInterface::setHandler (ScaleWindow::get (window));

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

  WindowManager::Default()->initiate_spread.connect(sigc::mem_fun(this, &UnityWindow::OnInitiateSpreed));
  WindowManager::Default()->terminate_spread.connect(sigc::mem_fun(this, &UnityWindow::OnTerminateSpreed));
}

void UnityWindow::AddProperties(GVariantBuilder* builder)
{
  Window xid = window->id();
  auto const& swins = ScaleScreen::get(screen)->getWindows();
  bool scaled = std::find(swins.begin(), swins.end(), ScaleWindow::get(window)) != swins.end();
  auto wm = WindowManager::Default();

  variant::BuilderWrapper(builder)
    .add(scaled ? GetScaledGeometry() : wm->GetWindowGeometry(xid))
    .add("xid", xid)
    .add("title", wm->GetWindowName(xid))
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

void UnityWindow::DrawTexture(GLTexture::List const& textures, GLWindowPaintAttrib const& attrib,
                              GLMatrix const& transform, unsigned int mask, int x, int y)
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

      gWindow->glDrawTexture(texture, wTransform, attrib, mask);
    }
  }
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

  std::string const& win_title = WindowManager::Default()->GetWindowName(window->id());
  pango_layout_set_height(layout, height);
  pango_layout_set_width(layout, -1); //avoid wrap lines
  pango_layout_set_auto_dir(layout, false);
  pango_layout_set_text(layout, win_title.c_str(), -1);

  /* update the size of the pango layout */
  pango_cairo_update_layout(context.cr_, layout);

  GtkStyleContext* style_context = style.GetStyleContext();
  gtk_style_context_save(style_context);

  std::shared_ptr<GtkWidgetPath> widget_path(gtk_widget_path_new(), gtk_widget_path_free);
  gtk_widget_path_append_type(widget_path.get(), GTK_TYPE_MENU_BAR);
  gtk_widget_path_append_type(widget_path.get(), GTK_TYPE_MENU_ITEM);
  gtk_widget_path_iter_set_name(widget_path.get(), -1 , "UnityPanelWidget");

  gtk_style_context_set_path(style_context, widget_path.get());
  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUBAR);
  gtk_style_context_add_class(style_context, GTK_STYLE_CLASS_MENUITEM);

  // alignment
  PangoRectangle lRect;
  pango_layout_get_extents(layout, nullptr, &lRect);
  int text_width = lRect.width / PANGO_SCALE;
  int text_height = lRect.height / PANGO_SCALE;
  y += (height - text_height) / 2.0f;
  int text_space = width - x;

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

void UnityWindow::DrawWindowDecoration(GLWindowPaintAttrib const& attrib,
                                       GLMatrix const& transform,
                                       unsigned int mask,
                                       bool highlighted,
                                       int x, int y, unsigned width, unsigned height)
{
  // Paint a fake window decoration
  CairoContext context(width, height);

  cairo_save(context.cr_);

  // Draw window decoration based on gtk style
  cairo_push_group(context.cr_);
  auto& style = panel::Style::Instance();
  gtk_render_background(style.GetStyleContext(), context.cr_, 0, 0, width, height);
  gtk_render_frame(style.GetStyleContext(), context.cr_, 0, 0, width, height);
  cairo_pop_group_to_source(context.cr_);

  // Round window decoration top border
  const double aspect = ScaleWindow::get(window)->getCurrentPosition().scale;
  const double radius = 8.0 * aspect;

  cairo_new_sub_path(context.cr_);
  cairo_line_to(context.cr_, 0, height);
  cairo_arc(context.cr_, radius, radius, radius, M_PI, -M_PI * 0.5f);
  cairo_line_to(context.cr_, width - radius, 0);
  cairo_arc(context.cr_, width - radius, radius, radius, M_PI * 0.5f, 0);
  cairo_line_to(context.cr_, width, height);
  cairo_close_path(context.cr_);

  cairo_fill(context.cr_);

  cairo_restore(context.cr_);

  if (highlighted)
  {
    // Draw windows title
    const float xText = SCALE_ITEMS_PADDING * 2 + SCALE_CLOSE_ICON_SIZE;
    RenderText(context, xText, 0.0, width - SCALE_ITEMS_PADDING, height);
  }

  mask |= PAINT_WINDOW_BLEND_MASK;
  DrawTexture(context.texture_, attrib, transform, mask, x, y);
}

void UnityWindow::LoadCloseIcon(panel::WindowState state, GLTexture::List& texture)
{
  if (!texture.empty())
    return;

  auto& style = panel::Style::Instance();
  auto const& files = style.GetWindowButtonFileNames(panel::WindowButtonType::CLOSE, state);

  CompString plugin("unityshell");
  for (std::string const& file : files)
  {
    CompString file_name = file;
    CompSize size(SCALE_CLOSE_ICON_SIZE, SCALE_CLOSE_ICON_SIZE);
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
    CompSize size(SCALE_CLOSE_ICON_SIZE, SCALE_CLOSE_ICON_SIZE);
    texture = GLTexture::readImageToTexture(file_name, plugin, size);
  }
}

void UnityWindow::SetupSharedTextures()
{
  LoadCloseIcon(panel::WindowState::NORMAL, close_normal_tex_);
  LoadCloseIcon(panel::WindowState::PRELIGHT, close_prelight_tex_);
  LoadCloseIcon(panel::WindowState::PRESSED, close_pressed_tex_);
}

void UnityWindow::CleanupSharedTextures()
{
  close_normal_tex_.clear();
  close_prelight_tex_.clear();
  close_pressed_tex_.clear();
}

void UnityWindow::scalePaintDecoration(GLWindowPaintAttrib const& attrib,
                                       GLMatrix const& transform,
                                       CompRegion const& region,
                                       unsigned int mask)
{
  ScaleWindow *scale_win = ScaleWindow::get(window);
  scale_win->scalePaintDecoration(attrib, transform, region, mask);

  if (!scale_win->hasSlot()) // animation not finished
    return;

  ScaleScreen* ss = ScaleScreen::get(screen);
  auto state = ss->getState();

  if (state != ScaleScreen::Wait && state != ScaleScreen::Out)
    return;

  auto const& scaled_geo = GetScaledGeometry();
  auto const& decoration_extents = window->border();
  auto const& pos = scale_win->getCurrentPosition();

  const bool highlighted = (ss->getSelectedWindow() == window->id());
  int width = scaled_geo.width;
  int height = decoration_extents.top;
  int x = scaled_geo.x;
  int y = scaled_geo.y;


  // If window is not highlighted, we draw the decoration at scaled size
  if (!highlighted)
    height *= pos.scale;

  DrawWindowDecoration(attrib, transform, mask, highlighted, x, y, width, height);

  if (highlighted)
  {
    x += SCALE_ITEMS_PADDING;
    y += (height - SCALE_CLOSE_ICON_SIZE) / 2.0f;
    mask |= PAINT_WINDOW_BLEND_MASK;

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

    close_button_geo_.Set(x, y, SCALE_CLOSE_ICON_SIZE, SCALE_CLOSE_ICON_SIZE);
  }
  else if (!close_button_geo_.IsNull())
  {
    close_button_geo_.Set(0, 0, 0, 0);
  }
}

nux::Geometry UnityWindow::GetScaledGeometry()
{
  ScaleWindow *scale_win = ScaleWindow::get(window);

  ScalePosition const& pos = scale_win->getCurrentPosition();
  auto const& border_rect = window->borderRect();
  auto const& deco_ext = window->border();

  const unsigned width = std::floor(border_rect.width() * pos.scale);
  const unsigned height = std::floor(border_rect.height() * pos.scale);
  const int x = pos.x() + window->x() - std::floor(deco_ext.left * pos.scale);
  const int y = pos.y() + window->y() - std::floor(deco_ext.top * pos.scale);

  return nux::Geometry(x, y, width, height);
}

void UnityWindow::OnInitiateSpreed()
{
  close_icon_state_ = panel::WindowState::NORMAL;
  middle_clicked_ = false;
  SetupSharedTextures();

  WindowManager *wm = WindowManager::Default();
  Window xid = window->id();

  if (wm->IsWindowDecorated(xid))
    wm->Decorate(xid);
}

void UnityWindow::OnTerminateSpreed()
{
  WindowManager *wm = WindowManager::Default();
  Window xid = window->id();

  if (wm->IsWindowDecorated(xid) && wm->IsWindowMaximized(xid))
    wm->Undecorate(xid);
}

UnityWindow::~UnityWindow()
{
  UnityScreen* us = UnityScreen::get(screen);
  if (us->newFocusedWindow && (UnityWindow::get(us->newFocusedWindow) == this))
    us->newFocusedWindow = NULL;

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
    UnityScreen::get (screen)->fullscreen_windows_.remove(window);

  PluginAdapter::Default ()->OnWindowClosed(window);
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

