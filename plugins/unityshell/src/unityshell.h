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
#include "ScreenIntrospection.h"
#include "SwitcherController.h"
#include "UBusWrapper.h"
#include "UnityshellPrivate.h"
#include "UnityShowdesktopHandler.h"
#include "ThumbnailGenerator.h"

#include "compizminimizedwindowhandler.h"
#include "BGHash.h"
#include <compiztoolbox/compiztoolbox.h>
#include <dlfcn.h>

#include "HudController.h"
#include "WindowMinimizeSpeedController.h"

#include "unityshell_glow.h"

namespace unity
{

/* base screen class */
class UnityScreen :
  public debug::Introspectable,
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
  void paintDisplay();
  void paintPanelShadow(const CompRegion& clip);
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

  bool DoesPointIntersectUnityGeos(nux::Point const& pt);

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

  void DrawTopPanelBackground();
  bool TopPanelBackgroundTextureNeedsUpdate() const;
  void UpdateTopPanelBackgroundTexture();

  std::unique_ptr<na::TickSource> tick_source_;
  std::unique_ptr<na::AnimationController> animation_controller_;

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

  std::unique_ptr<BGHash> _bghash;

  ::GLFramebufferObject *oldFbo;

  bool   queryForShader ();

  int overlay_monitor_;
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
  WindowMinimizeSpeedController minimize_speed_controller_;

  long long big_tick_;

  debug::ScreenIntrospection screen_introspection_;

  UBusManager ubus_manager_;
  glib::SourceManager sources_;

  friend class UnityWindow;
};

class UnityWindow :
  public WindowInterface,
  public GLWindowInterface,
  public ShowdesktopHandlerWindowInterface,
  public compiz::WindowInputRemoverLockAcquireInterface,
  public WrapableHandler<ScaleWindowInterface, 4>,
  public BaseSwitchWindow,
  public PluginClassHandler <UnityWindow, CompWindow>,
  public debug::Introspectable,
  public sigc::trackable
{
public:
  UnityWindow(CompWindow*);
  ~UnityWindow();

  CompWindow* window;
  GLWindow* gWindow;

  nux::Geometry last_bound;

  void minimize();
  void unminimize();
  bool minimized();
  bool focus();
  void activate();

  void updateFrameRegion(CompRegion &region);

  /* occlusion detection
   * and window hiding */
  bool glPaint(GLWindowPaintAttrib const&, GLMatrix const&, CompRegion const&, unsigned mask);

  /* basic window draw function */
  bool glDraw(GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&, unsigned mask);

  void updateIconPos (int &wx, int &wy, int x, int y, float width, float height);
  void windowNotify(CompWindowNotify n);
  void moveNotify(int x, int y, bool immediate);
  void resizeNotify(int x, int y, int w, int h);
  void stateChangeNotify(unsigned int lastState);

  bool place(CompPoint& pos);
  CompPoint tryNotIntersectUI(CompPoint& pos);
  nux::Geometry GetScaledGeometry();

  void paintThumbnail(nux::Geometry const& bounding, float alpha);

  void enterShowDesktop();
  void leaveShowDesktop();
  bool HandleAnimations(unsigned int ms);

  bool handleEvent(XEvent *event);
  void scalePaintDecoration(GLWindowPaintAttrib const&, GLMatrix const&, CompRegion const&, unsigned mask);

  //! Emited when CompWindowNotifyBeforeDestroy is received
  sigc::signal<void> being_destroyed;


protected:
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  typedef compiz::CompizMinimizedWindowHandler<UnityScreen, UnityWindow>
          UnityMinimizedHandler;
  struct PixmapTexture;
  typedef std::shared_ptr<PixmapTexture> PixmapTexturePtr;
  struct CairoContext;

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

  void OnInitiateSpread();
  void OnTerminateSpread();

  void DoAddDamage ();
  ShowdesktopHandlerWindowInterface::PostPaintAction DoHandleAnimations (unsigned int ms);

  void DoMoveFocusAway ();

  void DoDeleteHandler ();

  unsigned int GetNoCoreInstanceMask ();

  compiz::WindowInputRemoverLock::Ptr GetInputRemover ();

  void RenderDecoration(CairoContext const&, double aspect = 1.0f);
  void RenderText(CairoContext const&, int x, int y, int width, int height);
  void DrawTexture(GLTexture::List const& textures, GLWindowPaintAttrib const&,
                   GLMatrix const&, unsigned mask, int x, int y, double aspect = 1.0f);

  glow::Quads computeGlowQuads(nux::Geometry const& geo, GLTexture::List const& texture, int glow_size);
  void paintGlow(GLMatrix const&, GLWindowPaintAttrib const&, CompRegion const&,
                 glow::Quads const&, GLTexture::List const&, nux::Color const&, unsigned mask);

  void BuildDecorationTexture();
  void CleanupCachedTextures();
  static void SetupSharedTextures();
  static void CleanupSharedTextures();
  static void LoadCloseIcon(panel::WindowState state, GLTexture::List& texture);

public:
  std::unique_ptr <UnityMinimizedHandler> mMinimizeHandler;

private:
  std::unique_ptr <ShowdesktopHandler> mShowdesktopHandler;
  static GLTexture::List close_normal_tex_;
  static GLTexture::List close_prelight_tex_;
  static GLTexture::List close_pressed_tex_;
  static GLTexture::List glow_texture_;
  PixmapTexturePtr decoration_tex_;
  PixmapTexturePtr decoration_selected_tex_;
  std::string decoration_title_;
  compiz::WindowInputRemoverLock::Weak input_remover_;
  panel::WindowState close_icon_state_;
  nux::Geometry close_button_geo_;
  bool middle_clicked_;
  glib::Source::UniquePtr focus_desktop_timeout_;

  friend class UnityScreen;
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
