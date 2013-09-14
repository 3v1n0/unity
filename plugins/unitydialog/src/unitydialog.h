/*
 * Copyright (C) 2010 Canonical Ltd
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

#include <gtk/gtk.h>
#include <core/core.h>
#include <core/atoms.h>
#include <composite/composite.h>
#include <opengl/opengl.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>

#include <cairo-xlib-xrender.h>

#include "unitydialog_options.h"

namespace unity
{
  typedef std::vector <GLTexture::MatrixList> MatrixListVector;
  typedef std::vector <CompRegion> CompRegionVector;
  typedef std::vector <int> IntVector;

  class GeometryCollection
  {
    public:
      GeometryCollection ();
      bool status ();

      bool addGeometryForWindow (CompWindow *, const CompRegion &paintRegion);
      void addGeometry (const GLTexture::MatrixList &ml,
			const CompRegion            &r,
			int                         min,
			int                         max);

    private:

      MatrixListVector collectedMatrixLists;
      CompRegionVector collectedRegions;
      IntVector        collectedMinVertices;
      IntVector        collectedMaxVertices;
  };

  class TexGeometryCollection
  {
    public:
      TexGeometryCollection ();
      void addGeometry (const GLTexture::MatrixList &ml,
			const CompRegion            &r,
			int                         min,
			int                         max);
      void setTexture (GLTexture *);

      void addGeometriesAndDrawTextureForWindow (CompWindow     *w,
						 const GLMatrix &transform,
						 unsigned int    mask);

    private:
      GLTexture*         mTexture;
      GeometryCollection mGeometries;
  };

  class PaintInfoCollector
  {
    public:

      PaintInfoCollector (CompWindow *w);

      void collect ();
      void drawGeometriesForWindow (CompWindow     *w,
				    const GLMatrix &transform,
				    unsigned int    pm);

      void processGeometry (const GLTexture::MatrixList &ml,
			    const CompRegion            &r,
			    int                         min,
			    int                         max);

      void processTexture (GLTexture *tex);

      static PaintInfoCollector * Active ();

      static PaintInfoCollector *active_collector;

    private:

      /* Collected regions, textures */
      std::vector <TexGeometryCollection> mCollection;
      CompWindow                          *mWindow;
  };
}

class UnityDialogShadeTexture
{
public:

  static UnityDialogShadeTexture*
  create();

  static void
  onThemeChanged(GObject*   obj,
                 GParamSpec*  pspec,
                 gpointer  data);

  void
  render(float alpha);

  void
  render();

  const GLTexture::List&
  texture();

  UnityDialogShadeTexture();
  ~UnityDialogShadeTexture();

private:

  Pixmap    mPixmap;
  GLTexture::List mTexture;
  cairo_surface_t* mSurface;
  cairo_t*   mCairo;
  Damage    mDamage;

  GtkWidget* mOffscreenContainer;
  char*    mThemeName;
  GtkStyle*  mStyle;

  float   mAlpha;

  void
  clear();

  void
  destroy();

  void
  context();
};

class UnityDialogWindow;

class UnityDialogScreen :
  public PluginClassHandler <UnityDialogScreen, CompScreen>,
  public ScreenInterface,
  public GLScreenInterface,
  public CompositeScreenInterface,
  public UnitydialogOptions
{
public:

  UnityDialogScreen(CompScreen*);
  ~UnityDialogScreen();

  void
  handleEvent(XEvent*);

  void
  handleCompizEvent(const char*, const char*, CompOption::Vector&);

  void
  matchExpHandlerChanged();

  CompMatch::Expression*
  matchInitExp(const CompString& value);

  void
  preparePaint(int);

  bool
  glPaintOutput(const GLScreenPaintAttrib& attrib,
                const GLMatrix&    transform,
                const CompRegion&    region,
                CompOutput*    output,
                unsigned int    mask);

  void
  donePaint();

  void
  optionChanged(CompOption*, Options num);

  void
  trackParent(CompWindow* w);

  void
  untrackParent(CompWindow* w);

public:

  CompositeScreen* cScreen;
  GLScreen*  gScreen;

  bool              switchingVp()
  {
    return mSwitchingVp;
  }
  const GLTexture::List& tex()
  {
    return mTex->texture();
  }

private:

  bool    mSwitchingVp;
  UnityDialogShadeTexture* mTex;

  Atom    mCompizResizeAtom;
  Atom    mUnityIsParentAtom;

  CompWindowList  mParentWindows;
};

#define UNITY_DIALOG_SCREEN(s)                   \
    UnityDialogScreen *uds = UnityDialogScreen::get (s);

class UnityDialogWindow :
  public PluginClassHandler <UnityDialogWindow, CompWindow>,
  public WindowInterface,
  public GLWindowInterface
{
public:

  UnityDialogWindow(CompWindow* w);
  ~UnityDialogWindow();

public:

  CompWindow* window;
  CompositeWindow* cWindow;
  GLWindow*   gWindow;

public:

  bool
  glDraw(const GLMatrix&,
         const GLWindowPaintAttrib&,
         const CompRegion&, unsigned int);

  bool
  glPaint(const GLWindowPaintAttrib&, const GLMatrix&,
          const CompRegion&, unsigned int);

  void
  glAddGeometry(const GLTexture::MatrixList& matrices,
                const CompRegion& region,
                const CompRegion& clipRegion,
                unsigned int min,
                unsigned int max);

  void
  glDrawTexture(GLTexture* texture,
                const GLMatrix& transform,
                const GLWindowPaintAttrib& attrib,
                unsigned int mask);


  void windowNotify(CompWindowNotify n);
  void grabNotify(int x, int y,
                  unsigned int state,
                  unsigned int mask);
  void ungrabNotify();
  void moveNotify(int x, int y, bool immediate);
  void resizeNotify(int, int, int, int);

  bool place(CompPoint&);

  void getAllowedActions(unsigned int& setActions, unsigned int& clearActions);

  /* True if on adding or removing transient, window becomes
   * or ceases to become a parent window */
  bool addTransient(CompWindow* transient);
  bool removeTransient(CompWindow* transient);

  bool hasParent()
  {
    return mParent != NULL;
  }
  bool hasTransients()
  {
    return !mTransients.empty();
  }
  bool hasTransient(CompWindow* w)
  {
    return std::find(mTransients.begin(),
                     mTransients.end(),
                     w) == mTransients.end();
  }

  CompWindow*
  findTopParentWindow();

  CompPoint getChildCenteredPositionForRect(CompRect rect);
  CompPoint getParentCenteredPositionForRect(CompRect rect);

  void    moveTransientsToRect(CompWindow* skip, CompRect rect, bool);
  void    moveParentToRect(CompWindow* requestor, CompRect rect, bool);

  void      moveToRect(CompRect currentRect);

  void    grabTransients(CompWindow* skip, int x, int y,
                         unsigned int state, unsigned int mask, bool);
  void    grabParent(CompWindow* requestor, int x, int y,
                     unsigned int state, unsigned int mask, bool);

  void      animateTransients(CompWindow* skip, CompPoint& orig, CompPoint& dest, bool cont);
  void    animateParent(CompWindow* requestor, CompPoint& orig, CompPoint& dest);

  void      setMaxConstrainingAreas();

  CompWindow* transientParent() const;

  void adjustIPW();

  bool animate(int ms, float fadeTime);
  CompRegion getDamageRegion();

  void setIsAnimated(bool animated)
  {
    mIsAnimated = animated;
  }
  bool isAnimated()
  {
    return mIsAnimated;
  }

private:

  bool         mSkipNotify;
  CompWindowList mTransients;
  CompWindow*     mParent;
  CompSize       mOldHintsSize;
  int        mGrabMask;
  int        mShadeProgress;
  CompPoint      mTargetPos;
  CompPoint      mCurrentPos;
  CompPoint      mOffset;
  XWindowChanges mDiffXWC;
  Window         mIpw;
  bool           mIsAnimated;

  void collectDrawInfo();
};

#define VIG_WINDOW(w)                  \
    UnityDialogWindow *vw = UnityDialogWindow::get (w);

class UnityDialogPluginVTable :
  public CompPlugin::VTableForScreenAndWindow <UnityDialogScreen, UnityDialogWindow>
{
public:

  bool init();
};
