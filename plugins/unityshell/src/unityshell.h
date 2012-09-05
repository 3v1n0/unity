// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/* Compiz unity plugin
 * unity.h
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

#ifndef UNITYSHELL_H
#define UNITYSHELL_H

#include <NuxCore/AnimationController.h>
#include <Nux/GesturesSubscription.h>
#include <Nux/WindowThread.h>
#include <NuxCore/Property.h>
#include <sigc++/sigc++.h>
#include <boost/shared_ptr.hpp>

#include <scale/scale.h>
#include <core/core.h>
#include <core/pluginclasshandler.h>
#include <composite/composite.h>
#include <opengl/opengl.h>

#include "unityshell_options.h"

#include "Introspectable.h"
#include "DashController.h"
#include "UnitySettings.h"
#include "DashStyle.h"
#include "FavoriteStoreGSettings.h"
#include "FontSettings.h"
#include "ShortcutController.h"
#include "ShortcutHint.h"
#include "LauncherController.h"
#include "PanelController.h"
#include "PanelStyle.h"
#include "UScreen.h"
#include "DebugDBusInterface.h"
#include "SwitcherController.h"
#include "UBusWrapper.h"
#include "UnityshellPrivate.h"
#include "UnityShowdesktopHandler.h"
#include "ThumbnailGenerator.h"
#ifndef USE_MODERN_COMPIZ_GL
#include "ScreenEffectFramebufferObject.h"
#endif

#include "compizminimizedwindowhandler.h"
#include "BGHash.h"
#include <compiztoolbox/compiztoolbox.h>
#include <dlfcn.h>

#include "HudController.h"
#include "ThumbnailGenerator.h"
#include "WindowMinimizeSpeedController.h"

namespace unity
{

class WindowCairoContext;

/* base screen class */
class UnityScreen :
  public unity::debug::Introspectable,
  public sigc::trackable,
  public ScreenInterface,
  public CompositeScreenInterface,
  public GLScreenInterface,
  public BaseSwitchScreen,
  public PluginClassHandler <UnityScreen, CompScreen>,
  public UnityshellOptions
{
public:
  UnityScreen(CompScreen* s);
  ~UnityScreen();

  /* We store these  to avoid unecessary calls to ::get */
  CompScreen* screen;
  CompositeScreen* cScreen;
  GLScreen* gScreen;

  /* prepares nux for drawing */
  void nuxPrologue();
  /* pops nux draw stack */
  void nuxEpilogue();

  /* nux draw wrapper */
#ifdef USE_MODERN_COMPIZ_GL
  void paintDisplay();
#else
  void paintDisplay(const CompRegion& region, const GLMatrix& transform, unsigned int mask);
#endif
  void paintPanelShadow(const GLMatrix& matrix);
  void setPanelShadowMatrix(const GLMatrix& matrix);

  void preparePaint (int ms);
  void paintFboForOutput (CompOutput *output);
  void donePaint ();

  void RaiseInputWindows();

  void
  handleCompizEvent (const char         *pluginName,
                     const char         *eventName,
                     CompOption::Vector &o);

  void damageRegion(const CompRegion &region);

  bool shellCouldBeHidden(CompOutput const& output);

  /* paint on top of all windows if we could not find a window
   * to paint underneath */
  bool glPaintOutput(const GLScreenPaintAttrib&,
                     const GLMatrix&,
                     const CompRegion&,
                     CompOutput*,
                     unsigned int);

  /* paint in the special case that the output is transformed */
  void glPaintTransformedOutput(const GLScreenPaintAttrib&,
                                const GLMatrix&,
                                const CompRegion&,
                                CompOutput*,
                                unsigned int);

  /* handle X11 events */
  void handleEvent(XEvent*);

  /* handle showdesktop */
  void enterShowDesktopMode ();
  void leaveShowDesktopMode (CompWindow *w);

  bool showLauncherKeyInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool showLauncherKeyTerminate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool showPanelFirstMenuKeyInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool showPanelFirstMenuKeyTerminate(CompAction* action, CompAction::State state, CompOption::Vector& options);

  bool executeCommand(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool setKeyboardFocusKeyInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);

  bool altTabInitiateCommon(CompAction* action, switcher::ShowMode mode);
  bool altTabTerminateCommon(CompAction* action,
                             CompAction::State state,
                             CompOption::Vector& options);

  bool altTabForwardInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool altTabPrevInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool altTabForwardAllInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool altTabPrevAllInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool altTabDetailStartInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool altTabDetailStopInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool altTabNextWindowInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool altTabPrevWindowInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);

  bool ShowHud();
  /* handle hud key activations */
  bool ShowHudInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool ShowHudTerminate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool launcherSwitcherForwardInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool launcherSwitcherPrevInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool launcherSwitcherTerminate(CompAction* action, CompAction::State state, CompOption::Vector& options);

  /* handle option changes and change settings inside of the
   * panel and dock views */
  void optionChanged(CompOption*, Options num);

  /* Handle changes in the number of workspaces by showing the switcher
   * or not showing the switcher */
  bool setOptionForPlugin(const char* plugin, const char* name,
                          CompOption::Value& v);

  /* init plugin actions for screen */
  bool initPluginForScreen(CompPlugin* p);

  void outputChangeNotify();
  void NeedsRelayout();
  void ScheduleRelayout(guint timeout);

  bool forcePaintOnTop ();

  void SetUpAndShowSwitcher(switcher::ShowMode show_mode = switcher::ShowMode::CURRENT_VIEWPORT);

  void OnMinimizeDurationChanged();

  switcher::Controller::Ptr switcher_controller();
  launcher::Controller::Ptr launcher_controller();

protected:
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  enum CancelActionTarget
  {
    LAUNCHER_SWITCHER,
    SHORTCUT_HINT
  };

  void initAltTabNextWindow ();

  void SendExecuteCommand();

  void EnsureSuperKeybindings();
  void CreateSuperNewAction(char shortcut, impl::ActionModifiers flag);
  void EnableCancelAction(CancelActionTarget target, bool enabled, int modifiers = 0);

  bool initPluginActions();
  void initLauncher();

  void compizDamageNux(CompRegion const& region);
  void nuxDamageCompiz();

  void onRedrawRequested();
  void Relayout();

  static void initUnity(nux::NThread* thread, void* InitData);
  static void OnStartKeyNav(GVariant* data, void* value);
  static void OnExitKeyNav(GVariant* data, void* value);

  void restartLauncherKeyNav();

  void OnDashRealized ();

  void OnLauncherStartKeyNav(GVariant* data);
  void OnLauncherEndKeyNav(GVariant* data);

  void OnSwitcherStart(GVariant* data);
  void OnSwitcherEnd(GVariant* data);

  void RestoreWindow(GVariant* data);
  bool SaveInputThenFocus(const guint xid);

  void InitHints();

  void OnPanelStyleChanged();

  void InitGesturesSupport();

  nux::animation::TickSource tick_source_;
  nux::animation::AnimationController animation_controller_;

  Settings dash_settings_;
  dash::Style    dash_style_;
  panel::Style   panel_style_;
  FontSettings   font_settings_;
  internal::FavoriteStoreGSettings favorite_store_;
  ThumbnailGenerator thumbnail_generator_;

  /* The window thread should be the last thing removed, as c++ does it in reverse order */
  std::unique_ptr<nux::WindowThread> wt;

  /* These must stay below the window thread, please keep the order */
  launcher::Controller::Ptr launcher_controller_;
  dash::Controller::Ptr     dash_controller_;
  panel::Controller::Ptr    panel_controller_;
  switcher::Controller::Ptr switcher_controller_;
  hud::Controller::Ptr      hud_controller_;
  shortcut::Controller::Ptr shortcut_controller_;
  debug::DebugDBusInterface debugger_;

  std::list<shortcut::AbstractHint::Ptr> hints_;
  bool enable_shortcut_overlay_;

  /* Subscription for gestures that manipulate Unity launcher */
  std::unique_ptr<nux::GesturesSubscription> gestures_sub_launcher_;

  /* Subscription for gestures that manipulate Unity dash */
  std::unique_ptr<nux::GesturesSubscription> gestures_sub_dash_;

  /* Subscription for gestures that manipulate windows. */
  std::unique_ptr<nux::GesturesSubscription> gestures_sub_windows_;

  bool                                  needsRelayout;
  bool                                  _in_paint;
  bool                                  super_keypressed_;
  typedef std::shared_ptr<CompAction> CompActionPtr;
  typedef std::vector<CompActionPtr> ShortcutActions;
  ShortcutActions _shortcut_actions;
  std::map<CancelActionTarget, CompActionPtr> _escape_actions;

  /* keyboard-nav mode */
  CompWindow* newFocusedWindow;
  CompWindow* lastFocusedWindow;

  GLTexture::List _shadow_texture;

  /* handle paint order */
  bool    doShellRepaint;
  bool    didShellRepaint;
  bool    allowWindowPaint;
  bool    _key_nav_mode_requested;
  CompOutput* _last_output;

  CompRegion nuxRegion;
  CompRegion fullscreenRegion;
  CompWindow* firstWindowAboveShell;

  nux::Property<nux::Geometry> primary_monitor_;

  BGHash _bghash;

#ifdef USE_MODERN_COMPIZ_GL
  ::GLFramebufferObject *oldFbo;
#else
  ScreenEffectFramebufferObject::Ptr _fbo;
  GLuint                             _active_fbo;
#endif

  bool   queryForShader ();

  int dash_monitor_;
  CompScreen::GrabHandle grab_index_;
  CompWindowList         fullscreen_windows_;
  bool                   painting_tray_;
  unsigned int           tray_paint_mask_;
  unsigned int           last_scroll_event_;
  int                    hud_keypress_time_;
  int                    first_menu_keypress_time_;

  GLMatrix panel_shadow_matrix_;

  bool panel_texture_has_changed_;
  bool paint_panel_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> panel_texture_;

  bool scale_just_activated_;

#ifndef USE_MODERN_COMPIZ_GL
  ScreenEffectFramebufferObject::GLXGetProcAddressProc glXGetProcAddressP;
#endif

  UBusManager ubus_manager_;
  glib::SourceManager sources_;
  unity::ThumbnailGenerator thumb_generator;

  Window highlighted_window_;

  WindowMinimizeSpeedController* minimize_speed_controller;
  friend class UnityWindow;
};

class UnityWindow :
  public WindowInterface,
  public GLWindowInterface,
  public ShowdesktopHandlerWindowInterface,
  public compiz::WindowInputRemoverLockAcquireInterface,
  public WrapableHandler<ScaleWindowInterface, 4>,
  public BaseSwitchWindow,
  public PluginClassHandler <UnityWindow, CompWindow>
{
public:
  UnityWindow(CompWindow*);
  ~UnityWindow();

  CompWindow* window;
  GLWindow* gWindow;

  nux::Geometry last_bound;

  void minimize ();
  void unminimize ();
  bool minimized ();
  bool focus ();
  void activate ();

  void updateFrameRegion (CompRegion &region);

  /* occlusion detection
   * and window hiding */
  bool glPaint(const GLWindowPaintAttrib& attrib,
               const GLMatrix&            matrix,
               const CompRegion&          region,
               unsigned int              mask);

  /* basic window draw function */
  bool glDraw(const GLMatrix& matrix,
#ifndef USE_MODERN_COMPIZ_GL
              GLFragment::Attrib& attrib,
#else
              const GLWindowPaintAttrib& attrib,
#endif
              const CompRegion& region,
              unsigned intmask);

  void updateIconPos (int   &wx,
                      int   &wy,
                      int   x,
                      int   y,
                      float width,
                      float height);

  void windowNotify(CompWindowNotify n);
  void moveNotify(int x, int y, bool immediate);
  void resizeNotify(int x, int y, int w, int h);
  void stateChangeNotify(unsigned int lastState);

  bool place(CompPoint& pos);
  CompPoint tryNotIntersectUI(CompPoint& pos);

  void paintThumbnail (nux::Geometry const& bounding, float alpha);

  void enterShowDesktop ();
  void leaveShowDesktop ();
  bool HandleAnimations (unsigned int ms);

  void handleEvent (XEvent *event);

  CompRect CloseButtonArea();

  typedef compiz::CompizMinimizedWindowHandler<UnityScreen, UnityWindow>
          UnityMinimizedHandler;
  std::unique_ptr <UnityMinimizedHandler> mMinimizeHandler;
  std::unique_ptr <ShowdesktopHandler> mShowdesktopHandler;

  //! Emited when CompWindowNotifyBeforeDestroy is received
  sigc::signal<void> being_destroyed;

  void scaleSelectWindow();
  void scalePaintDecoration(const GLWindowPaintAttrib &,
                            const GLMatrix &,
                            const CompRegion &,
                            unsigned int);

private:
  void DoEnableFocus ();
  void DoDisableFocus ();

  bool IsOverrideRedirect ();
  bool IsManaged ();
  bool IsGrabbed ();
  bool IsDesktopOrDock ();
  bool IsSkipTaskbarOrPager ();
  bool IsHidden ();
  bool IsInShowdesktopMode ();
  bool IsShaded ();
  bool IsMinimized ();
  void DoOverrideFrameRegion (CompRegion &r);

  void DoHide ();
  void DoNotifyHidden ();
  void DoShow ();
  void DoNotifyShown ();

  void OnInitiateSpreed();
  void OnTerminateSpreed();

  void DoAddDamage ();
  ShowdesktopHandlerWindowInterface::PostPaintAction DoHandleAnimations (unsigned int ms);

  void DoMoveFocusAway ();

  void DoDeleteHandler ();

  unsigned int GetNoCoreInstanceMask ();

  compiz::WindowInputRemoverLock::Ptr GetInputRemover ();

  void DrawWindowDecoration(GLWindowPaintAttrib const& attrib,
                            GLMatrix const& transform,
                            unsigned int mask,
                            bool highlighted,
                            int x, int y, unsigned width, unsigned height);
  void DrawTexture(GLTexture *icon,
                   const GLWindowPaintAttrib& attrib,
                   const GLMatrix& transform,
                   unsigned int mask,
                   float x, float y,
                   int &maxWidth, int &maxHeight);
  void RenderText(std::shared_ptr<WindowCairoContext> const& context,
                  float x, float y,
                  float maxWidth, float maxHeight);

  void SetupScaleHeaderStyle();
  std::shared_ptr<WindowCairoContext> CreateCairoContext(float width, float height);

  static GLTexture::List close_icon_;
  compiz::WindowInputRemoverLock::Weak input_remover_;
  CompRect close_button_area_;
  glib::Source::UniquePtr focus_desktop_timeout_;
};


/**
 * Your vTable class is some basic info about the plugin that core uses.
 */
class UnityPluginVTable :
  public CompPlugin::VTableForScreenAndWindow<UnityScreen, UnityWindow>
{
public:
  bool init();
};

} // namespace unity

#endif // UNITYSHELL_H
