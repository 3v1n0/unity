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

#include <Nux/WindowThread.h>
#include <sigc++/sigc++.h>
#include <boost/shared_ptr.hpp>

#include <core/core.h>
#include <core/pluginclasshandler.h>
#include <composite/composite.h>
#include <opengl/opengl.h>

#include "unityshell_options.h"

#include "Introspectable.h"
#include "DashController.h"
#include "DashSettings.h"
#include "DashStyle.h"
#include "DesktopLauncherIcon.h"
#include "FontSettings.h"
#include "Launcher.h"
#include "LauncherController.h"
#include "PanelController.h"
#include "PanelStyle.h"
#include "UScreen.h"
#include "GestureEngine.h"
#include "DebugDBusInterface.h"
#include "SwitcherController.h"
#include "UBusWrapper.h"

#include "compizminimizedwindowhandler.h"
#include "BGHash.h"
#include <compiztoolbox/compiztoolbox.h>

namespace unity
{

class UnityFBO
{
public:

  typedef boost::shared_ptr <UnityFBO> Ptr;

  UnityFBO (CompOutput *o);
  ~UnityFBO ();

public:

  void bind ();
  void unbind ();

  bool status ();
  bool bound ();
  void paint ();

  GLuint texture () { return mFBTexture; }

private:

  /* compiz fbo handle that goes through to nux */
  GLuint   mFboHandle; // actual handle to the framebuffer_ext
  bool    mFboStatus; // did the framebuffer texture bind succeed
  GLuint   mFBTexture;
  CompOutput *output;
  unsigned int mBoundCnt;
};

class UnityShowdesktopHandler
{
public:

  UnityShowdesktopHandler (CompWindow *w);
  ~UnityShowdesktopHandler ();

  typedef enum {
    Visible = 0,
    FadeOut = 1,
    FadeIn = 2,
    Invisible = 3
  } State;

public:

  void fadeOut ();
  void fadeIn ();
  bool animate (unsigned int ms);
  void paintAttrib (GLWindowPaintAttrib &attrib);
  unsigned int getPaintMask ();
  void handleEvent (XEvent *);
  void windowNotify (CompWindowNotify n);
  void updateFrameRegion (CompRegion &r);

  UnityShowdesktopHandler::State state ();

  static const unsigned int fade_time;
  static CompWindowList     animating_windows;
  static bool shouldHide (CompWindow *);

private:

  CompWindow                     *mWindow;
  compiz::WindowInputRemover     *mRemover;
  UnityShowdesktopHandler::State mState;
  float                          mProgress;
  bool                           mWasHidden;
};


/* base screen class */
class UnityScreen :
  public unity::Introspectable,
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
  void paintDisplay(const CompRegion& region, const GLMatrix& transform, unsigned int mask);
  void paintPanelShadow(const GLMatrix& matrix);

  void preparePaint (int ms);
  void paintFboForOutput (CompOutput *output);

  void
  handleCompizEvent (const char         *pluginName,
                     const char         *eventName,
                     CompOption::Vector &o);


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

  /* Pop our InputOutput windows from the paint list */
  const CompWindowList& getWindowPaintList();

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
  bool launcherRevealEdgeInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);

  bool altTabInitiateCommon(CompAction* action,
                            CompAction::State state,
                            CompOption::Vector& options);
  bool altTabTerminateCommon(CompAction* action,
                             CompAction::State state,
                             CompOption::Vector& options);

  bool altTabForwardInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool altTabPrevInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool altTabDetailStartInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool altTabDetailStopInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool altTabNextWindowInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);
  bool altTabPrevWindowInitiate(CompAction* action, CompAction::State state, CompOption::Vector& options);

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

  void setActiveFbo (GLuint fbo) { mActiveFbo = fbo; }

  bool forcePaintOnTop ();

protected:
  const gchar* GetName();
  void AddProperties(GVariantBuilder* builder);

private:
  void initAltTabNextWindow ();

  void SendExecuteCommand();

  void EnsureSuperKeybindings ();
  void CreateSuperNewAction(char shortcut, bool use_shift=false, bool use_numpad=false);

  static gboolean initPluginActions(gpointer data);
  static void initLauncher(nux::NThread* thread, void* InitData);
  void damageNuxRegions();
  void onRedrawRequested();
  void Relayout();

  static gboolean RelayoutTimeout(gpointer data);
  static void launcherWindowConfigureCallback(int WindowWidth, int WindowHeight,
                                              nux::Geometry& geo, void* user_data);
  static void initUnity(nux::NThread* thread, void* InitData);
  static void OnStartKeyNav(GVariant* data, void* value);
  static void OnExitKeyNav(GVariant* data, void* value);
  static gboolean OnEdgeTriggerTimeout(gpointer data);
  static gboolean OnRedrawTimeout(gpointer data);

  void startLauncherKeyNav();
  void restartLauncherKeyNav();
  void OnLauncherHiddenChanged();

  void OnDashRealized ();

  static void OnQuicklistEndKeyNav(GVariant* data, void* value);
  static void OnLauncherStartKeyNav(GVariant* data, void* value);
  static void OnLauncherEndKeyNav(GVariant* data, void* value);

  dash::Settings          dash_settings_;
  dash::Style             dash_style_;
  panel::Style            panel_style_;
  FontSettings            font_settings_;
  Launcher*               launcher;
  launcher::Controller::Ptr launcher_controller_;
  dash::DashController::Ptr     dashController;
  panel::Controller::Ptr  panelController;
  switcher::SwitcherController*     switcherController;
  GestureEngine*          gestureEngine;
  nux::WindowThread*      wt;
  nux::BaseWindow*        launcherWindow;
  nux::BaseWindow*        panelWindow;
  nux::Geometry           lastTooltipArea;
  DebugDBusInterface*     debugger;
  bool                    needsRelayout;
  bool                    _in_paint;
  guint32                 relayoutSourceId;
  guint                   _edge_timeout;
  guint                   _edge_trigger_handle;
  guint32                 _redraw_handle;
  gint                    _edge_pointerY;
  guint                   _ubus_handles[3];

  typedef std::shared_ptr<CompAction> CompActionPtr;
  typedef std::vector<CompActionPtr> ShortcutActions;
  ShortcutActions _shortcut_actions;
  bool            super_keypressed_;

  /* keyboard-nav mode */
  CompWindow* newFocusedWindow;
  CompWindow* lastFocusedWindow;

  GLTexture::List _shadow_texture;

  /* handle paint order */
  bool    doShellRepaint;
  bool    allowWindowPaint;
  bool    damaged;
  bool    _key_nav_mode_requested;
  CompOutput* _last_output;
  CompWindowList _withRemovedNuxWindows;

  DesktopLauncherIcon* switcher_desktop_icon;

  GdkRectangle _primary_monitor;

  unity::BGHash _bghash;

  std::map <CompOutput *, UnityFBO::Ptr> mFbos;
  GLuint                                 mActiveFbo;

  bool   queryForShader ();

  UBusManager ubus_manager_;
  bool dash_is_open_;
  CompScreen::GrabHandle grab_index_;
  CompWindowList         fullscreen_windows_;
  bool                   painting_tray_;
  unsigned int           tray_paint_mask_;

  friend class UnityWindow;
};

class UnityWindow :
  public WindowInterface,
  public GLWindowInterface,
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

  void updateFrameRegion (CompRegion &region);

  /* occlusion detection
   * and window hiding */
  bool glPaint(const GLWindowPaintAttrib& attrib,
               const GLMatrix&            matrix,
               const CompRegion&          region,
               unsigned int              mask);

  /* basic window draw function */
  bool glDraw(const GLMatrix& matrix,
              GLFragment::Attrib& attrib,
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
  bool handleAnimations (unsigned int ms);

  compiz::MinimizedWindowHandler::Ptr mMinimizeHandler;
  UnityShowdesktopHandler             *mShowdesktopHandler;
  
private:

  guint  focusdesktop_handle_;
  static gboolean FocusDesktopTimeout(gpointer data);
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
