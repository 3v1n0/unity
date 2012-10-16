/*
 * Copyright (C) 2011 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include <Nux/Nux.h>
#include <glib.h>
#include <core/core.h>
#include <composite/composite.h>
#include <opengl/opengl.h>
#include <boost/noncopyable.hpp>
#include <memory>

#include <core/atoms.h>

#include "unity-mt-texture.h"
#include "unity-mt-grab-handle.h"
#include "unity-mt-grab-handle-group.h"
#include "unity-mt-grab-handle-window.h"
#include "unity-mt-grab-handle-impl-factory.h"
#include "unity-mt-grab-handle-layout.h"

#include "unitymtgrabhandles_options.h"

namespace unity
{
namespace MT
{

class DummyDamager
{
public:

    void damage (const nux::Geometry &g)
    {
      std::cout << "Damage rects: " << std::endl;
      std::cout << "x: " << g.x << " y: " << g.y << " width: " << g.width << " height: " << g.height << std::endl;
    }
};

class X11TextureFactory :
  public Texture::Factory
{
  public:

    void setActiveWrap (const GLTexture::List &);

    Texture::Ptr create ();

  private:

    GLTexture::List mWrap;
};

class X11Texture :
  public Texture
{
  public:

    typedef std::shared_ptr <X11Texture> Ptr;

    X11Texture (const GLTexture::List &t);

    const GLTexture::List & get ();

  private:

    GLTexture::List mTexture;
};

class X11ImplFactory :
  public GrabHandle::ImplFactory
{
  public:

    X11ImplFactory (Display *dpy);

    GrabHandle::Impl * create (const GrabHandle::Ptr &h);
  private:
    Display *mDpy;
};

class X11GrabHandleImpl :
  public GrabHandle::Impl
{
public:

  X11GrabHandleImpl (Display *dpy, const GrabHandle::Ptr &h);
  ~X11GrabHandleImpl ();

public:

  void show ();
  void hide ();

  void buttonPress (int x,
                    int y,
                    unsigned int button) const;

  void lockPosition (int x,
                     int y,
                     unsigned int flags);

  void damage (const nux::Geometry &g)
  {
    CompRegion r (g.x, g.y, g.width, g.height);
    CompositeScreen::get (screen)->damageRegion (r);
  }

private:

  std::weak_ptr <unity::MT::GrabHandle>  mGrabHandle;
  Window                                   mIpw;
  Display                                  *mDpy;
};

};
};

class UnityMTGrabHandlesScreen :
  public PluginClassHandler <UnityMTGrabHandlesScreen, CompScreen>,
  public ScreenInterface,
  public CompositeScreenInterface,
  public GLScreenInterface,
  public UnitymtgrabhandlesOptions
{
public:

  UnityMTGrabHandlesScreen(CompScreen*);
  ~UnityMTGrabHandlesScreen();

  CompositeScreen* cScreen;
  GLScreen*  gScreen;

public:

  void handleEvent(XEvent*);

  bool toggleHandles(CompAction*         action,
                     CompAction::State  state,
                     CompOption::Vector& options);

  bool showHandles(CompAction*         action,
                   CompAction::State  state,
                   CompOption::Vector& options);

  bool hideHandles(CompAction*         action,
                   CompAction::State  state,
                   CompOption::Vector& options);

  void optionChanged (CompOption *option,
                      UnitymtgrabhandlesOptions::Options num);

  void addHandles(const unity::MT::GrabHandleGroup::Ptr & handles);
  void removeHandles(const unity::MT::GrabHandleGroup::Ptr & handles);

  void addHandleWindow(const unity::MT::GrabHandle::Ptr &, Window);
  void removeHandleWindow(Window);

  void preparePaint(int);
  void donePaint();

  void raiseHandle (const std::shared_ptr <const unity::MT::GrabHandle> &,
                    Window                      owner);

  std::vector <unity::MT::TextureSize>  & textures()
  {
    return mHandleTextures;
  }

private:

  std::list <unity::MT::GrabHandleGroup::Ptr> mGrabHandles;
  std::vector <unity::MT::TextureSize> mHandleTextures;

  std::map <Window, const std::weak_ptr <unity::MT::GrabHandle> > mInputHandles;
  CompWindowVector         		    mLastClientListStacking;
  Atom             			    mCompResizeWindowAtom;

  bool          mMoreAnimate;
};

#define UMTGH_SCREEN(screen)                  \
    UnityMTGrabHandlesScreen *us = UnityMTGrabHandlesScreen::get (screen)

class UnityMTGrabHandlesWindow :
  public PluginClassHandler <UnityMTGrabHandlesWindow, CompWindow>,
  public WindowInterface,
  public CompositeWindowInterface,
  public GLWindowInterface,
  public unity::MT::GrabHandleWindow
{
public:

  UnityMTGrabHandlesWindow(CompWindow*);
  ~UnityMTGrabHandlesWindow();

  CompWindow*  window;
  CompositeWindow* cWindow;
  GLWindow*  gWindow;

public:

  bool allowHandles();
  bool handleTimerActive();

  void grabNotify(int, int, unsigned int, unsigned int);
  void ungrabNotify();

  void relayout(const CompRect&, bool);

  void getOutputExtents(CompWindowExtents& output);

  void moveNotify(int dx, int dy, bool immediate);

  bool glDraw(const GLMatrix&,
              const GLWindowPaintAttrib&,
              const CompRegion&,
              unsigned int);

  bool    handlesVisible();
  void    hideHandles();
  void    showHandles(bool use_timer);
  void    restackHandles();

  void resetTimer();
  void disableTimer();

protected:

  void raiseGrabHandle (const std::shared_ptr <const unity::MT::GrabHandle> &h);
  void requestMovement (int x,
                        int y,
                        unsigned int direction,
			unsigned int button);

private:

  bool onHideTimeout();

  unity::MT::GrabHandleGroup::Ptr mHandles;
  CompTimer                       mTimer;
};

#define UMTGH_WINDOW(window)                  \
    UnityMTGrabHandlesWindow *uw = UnityMTGrabHandlesWindow::get (window)

class UnityMTGrabHandlesPluginVTable :
  public CompPlugin::VTableForScreenAndWindow < UnityMTGrabHandlesScreen,
  UnityMTGrabHandlesWindow >
{
public:

  bool init();
};
