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

#include "unitydialog.h"
#include <cmath>
#include <boost/lexical_cast.hpp>

COMPIZ_PLUGIN_20090315(unitydialog, UnityDialogPluginVTable);

class UnityDialogExp : public CompMatch::Expression
{
public:
  UnityDialogExp(const CompString& str);

  bool evaluate(CompWindow const* w) const;

  bool value;
};


UnityDialogExp::UnityDialogExp(const CompString& str) :
  value(boost::lexical_cast<int> (str) != 0)
{
}

bool
UnityDialogExp::evaluate(CompWindow const* w) const
{
  UnityDialogWindow const* udw = UnityDialogWindow::get(w);

  return ((value && udw->transientParent()) || (!value && !udw->transientParent()));
}

CompMatch::Expression*
UnityDialogScreen::matchInitExp(const CompString& str)
{
  CompString matchString("transient-dialog=");
  /* Create a new match object */

  if (str.find(matchString) == 0)
    return new UnityDialogExp(str.substr(matchString.size()));

  return screen->matchInitExp(str);
}

void
UnityDialogScreen::matchExpHandlerChanged()
{
  screen->matchExpHandlerChanged();

  for (CompWindow* w : screen->windows())
  {
    if (UnityDialogWindow::get(w)->hasParent() ||
        UnityDialogWindow::get(w)->hasTransients())
      screen->matchPropertyChanged(w);
  }
}

void
UnityDialogWindow::moveToRect(CompRect currentRect)
{
  CompPoint pos = getChildCenteredPositionForRect(currentRect);

  mSkipNotify = true;
  window->move(pos.x() - window->borderRect().x(),
               pos.y() - window->borderRect().y(), true);

  setMaxConstrainingAreas();
  mSkipNotify = false;
}

CompWindow*
UnityDialogWindow::findTopParentWindow()
{
  CompWindow*        parent = window, *nextParent = NULL;

  /* Go to the bottom most window in the stack */
  while ((nextParent = UnityDialogWindow::get(parent)->mParent))
    parent = nextParent;

  /* parent now refers to the top most window */
  return parent;
}

UnityDialogShadeTexture::UnityDialogShadeTexture() :
  mPixmap(None),
  mTexture(0),
  mSurface(NULL),
  mCairo(NULL),
  mDamage(None)
{
  mOffscreenContainer = gtk_offscreen_window_new();
  gtk_widget_set_name(mOffscreenContainer, "UnityPanelWidget");
  gtk_style_context_add_class(gtk_widget_get_style_context(mOffscreenContainer),
                              "menubar");
  gtk_widget_set_size_request(mOffscreenContainer, 100, 24);
  gtk_widget_show_all(mOffscreenContainer);

  g_signal_connect(gtk_settings_get_default(), "notify::gtk-theme-name",
                   G_CALLBACK(UnityDialogShadeTexture::onThemeChanged), this);

  g_object_get(gtk_settings_get_default(), "gtk-theme-name", &mThemeName, NULL);
  mStyle = gtk_widget_get_style(mOffscreenContainer);

  context();
}

UnityDialogShadeTexture::~UnityDialogShadeTexture()
{
  gtk_widget_destroy(mOffscreenContainer);
  g_free(mThemeName);

  destroy();
}

void
UnityDialogShadeTexture::destroy()
{
  if (mPixmap)
  {
    XFreePixmap(screen->dpy(), mPixmap);
    mPixmap = None;
  }
  if (mSurface)
  {
    cairo_surface_destroy(mSurface);
    mSurface = NULL;
  }
  if (mCairo)
  {
    cairo_destroy(mCairo);
    mCairo = NULL;
  }
}

void
UnityDialogShadeTexture::clear()
{
  if (mCairo)
  {
    cairo_save(mCairo);
    cairo_set_operator(mCairo, CAIRO_OPERATOR_CLEAR);
    cairo_paint(mCairo);
    cairo_restore(mCairo);
  }
}

void
UnityDialogShadeTexture::context()
{
  if (!mCairo)
  {
    Screen*      xScreen;
    XRenderPictFormat* format;
    int     w, h;

    xScreen = ScreenOfDisplay(screen->dpy(), screen->screenNum());

    format = XRenderFindStandardFormat(screen->dpy(),
                                       PictStandardARGB32);

    w = 1;
    h = 1;

    mPixmap = XCreatePixmap(screen->dpy(), screen->root(), w, h, 32);

    mTexture = GLTexture::bindPixmapToTexture(mPixmap, w, h, 32);

    if (mTexture.empty())
    {
      compLogMessage("unitydialog", CompLogLevelError,
                     "Couldn't bind pixmap 0x%x to texture",
                     (int) mPixmap);

      XFreePixmap(screen->dpy(), mPixmap);

      return;
    }

    mDamage = XDamageCreate(screen->dpy(), mPixmap,
                            XDamageReportRawRectangles);

    mSurface =
      cairo_xlib_surface_create_with_xrender_format(screen->dpy(),
                                                    mPixmap, xScreen,
                                                    format, w, h);

    if (!mSurface)
    {
      compLogMessage("unitydialog", CompLogLevelError, "Couldn't create surface");

      XFreePixmap(screen->dpy(), mPixmap);
      XDamageDestroy(screen->dpy(), mDamage);
      mTexture.clear();

      return;
    }

    mCairo = cairo_create(mSurface);

    if (!mCairo)
    {
      compLogMessage("unitydialog", CompLogLevelError, "Couldn't create cairo context");

      cairo_surface_destroy(mSurface);
      XFreePixmap(screen->dpy(), mPixmap);
      XDamageDestroy(screen->dpy(), mDamage);
      mTexture.clear();
    }
  }
}

void
UnityDialogShadeTexture::render(float alpha)
{
  float divisorMask = 0xffff;
  mAlpha = alpha;

  clear();

  cairo_set_line_width(mCairo, 2);
  cairo_set_source_rgba(mCairo,
                        (mStyle->bg[1].red / divisorMask),
                        (mStyle->bg[1].green / divisorMask),
                        (mStyle->bg[1].blue / divisorMask),
                        (alpha));

  cairo_move_to(mCairo, 0, 0);
  cairo_rectangle(mCairo, 0, 0, 1, 1);

  cairo_fill(mCairo);
}

void
UnityDialogShadeTexture::render()
{
  render(mAlpha);
}

void
UnityDialogShadeTexture::onThemeChanged(GObject*    obj,
                                        GParamSpec* pspec,
                                        gpointer   data)
{
  UnityDialogShadeTexture* self = static_cast<UnityDialogShadeTexture*>(data);

  g_object_get(gtk_settings_get_default(), "gtk-theme-name", &self->mThemeName, NULL);
  self->mStyle = gtk_widget_get_style(self->mOffscreenContainer);

  self->render();
}

const GLTexture::List&
UnityDialogShadeTexture::texture()
{
  return mTexture;
}

bool
UnityDialogWindow::animate(int   ms,
                           float fadeTime)
{
  if (mTransients.size() && mShadeProgress < OPAQUE)
  {
    mShadeProgress += OPAQUE * (ms / fadeTime);

    if (mShadeProgress >= OPAQUE)
      mShadeProgress = OPAQUE;

    return true;
  }
  else if (!mTransients.size() && mShadeProgress > 0)
  {
    mShadeProgress -=  OPAQUE * (ms / fadeTime);

    if (mShadeProgress <= 0)
      mShadeProgress = 0;

    return true;
  }

  return false;
}

CompRegion
UnityDialogWindow::getDamageRegion()
{
  CompRect damageBounds;
  float progress = mShadeProgress / (float) OPAQUE;

  /* Current rect in animation expanded by output + 5 */
  damageBounds.setX(mCurrentPos.x() +
                    ((mTargetPos.x() - mCurrentPos.x()) * progress));
  damageBounds.setY(mCurrentPos.y() +
                    ((mTargetPos.y() - mCurrentPos.y()) * progress));
  damageBounds.setWidth(window->serverOutputRect().width() + 5);
  damageBounds.setHeight(window->serverOutputRect().height() + 5);

  return CompRegion(damageBounds);
}

void
UnityDialogScreen::preparePaint(int ms)
{
  cScreen->preparePaint(ms);

  for (CompWindow* w : mParentWindows)
    UnityDialogWindow::get(w)->animate(ms, optionGetFadeTime());

}

bool
UnityDialogScreen::glPaintOutput(const GLScreenPaintAttrib& attrib,
                                 const GLMatrix&      transform,
                                 const CompRegion&      region,
                                 CompOutput*        output,
                                 unsigned int        mask)
{
  mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

  return gScreen->glPaintOutput(attrib, transform, region, output, mask);
}

void
UnityDialogScreen::donePaint()
{
  CompWindowList::iterator it = mParentWindows.begin();

  cScreen->donePaint();

  while (it != mParentWindows.end())
  {
    CompWindow* w = (*it);
    UnityDialogWindow* udw = UnityDialogWindow::get(w);

    if (udw->animate(0, optionGetFadeTime()))
    {
      CompRegion damage = udw->getDamageRegion();
      cScreen->damageRegion(damage);
    }
    else if (!udw->hasTransients())
    {
      untrackParent(w);
      udw->gWindow->glDrawSetEnabled(udw, false);
      udw->gWindow->glPaintSetEnabled(udw, false);

      it = mParentWindows.begin();
      continue;
    }

    ++it;
  }
}

/* Paint the window transformed */

bool
UnityDialogWindow::glPaint(const GLWindowPaintAttrib& attrib,
                           const GLMatrix&        transform,
                           const CompRegion&        region,
                           unsigned int        mask)
{
  GLMatrix wTransform(transform);

  if (mParent)
    mask |= PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

  if (mTargetPos != mCurrentPos)
  {
    float progress = mShadeProgress / (float) OPAQUE;
    mask |= PAINT_WINDOW_TRANSFORMED_MASK;

    int dx = ((mTargetPos.x() - mCurrentPos.x()) * progress);
    int dy = ((mTargetPos.y() - mCurrentPos.y()) * progress);

    dx += mCurrentPos.x() - window->serverBorderRect().x();
    dy += mCurrentPos.y() - window->serverBorderRect().y();

    wTransform.translate(dx, dy, 0);

    if (mShadeProgress == OPAQUE)
      mTargetPos = mCurrentPos;
  }

  return gWindow->glPaint(attrib, wTransform, region, mask);
}

/* Collect regions and matrices */
void
UnityDialogWindow::glAddGeometry(const GLTexture::MatrixList& matrices,
                                 const CompRegion&            region,
                                 const CompRegion&            clipRegion,
                                 unsigned int                min,
                                 unsigned int                max)
{
  unity::PaintInfoCollector::Active ()->processGeometry (matrices, region, min, max);

  gWindow->glAddGeometry(matrices, region, clipRegion, min, max);
}

/* Collect textures */
void
UnityDialogWindow::glDrawTexture(GLTexture*          texture,
                                 const GLMatrix            &transform,
                                 const GLWindowPaintAttrib &attrib,
                                 unsigned int       mask)
{
  unity::PaintInfoCollector::Active ()->processTexture (texture);
}

unity::GeometryCollection::GeometryCollection() :
  collectedMatrixLists (1),
  collectedRegions (1),
  collectedMinVertices (1),
  collectedMaxVertices (1)
{
}

bool
unity::GeometryCollection::status ()
{
  return (collectedMatrixLists.size () == collectedRegions.size () &&
	  collectedRegions.size () == collectedMaxVertices.size () &&
	  collectedMaxVertices.size () == collectedMinVertices.size () &&
	  collectedMinVertices.size () == collectedMatrixLists.size ());
}

bool
unity::GeometryCollection::addGeometryForWindow (CompWindow *w, const CompRegion &paintRegion)
{
  /* We can reset the window geometry since it will be
   * re-added later */
  GLWindow::get (w)->vertexBuffer()->begin();

  for (unsigned int i = 0; i < collectedMatrixLists.size (); i++)
  {
    GLTexture::MatrixList matl = collectedMatrixLists[i];
    CompRegion            reg  = collectedRegions[i];
    int                   min = collectedMinVertices[i];
    int                   max = collectedMaxVertices[i];

    /* Now allow plugins to mess with the geometry of our
     * dim (so we get a nice render for things like
     * wobbly etc etc */
    GLWindow::get (w)->glAddGeometry(matl, reg, paintRegion, min, max);
  }

  return GLWindow::get (w)->vertexBuffer()->end();
}

void
unity::GeometryCollection::addGeometry(const GLTexture::MatrixList &ml,
				       const CompRegion            &r,
				       int                         min,
				       int                         max)
{
  collectedMatrixLists.push_back (ml);
  collectedRegions.push_back (r);
  collectedMaxVertices.push_back (max);
  collectedMinVertices.push_back (min);
}

unity::TexGeometryCollection::TexGeometryCollection() :
  mTexture (NULL)
{
}

void
unity::TexGeometryCollection::addGeometry(const GLTexture::MatrixList &ml,
					  const CompRegion            &r,
					  int                         max,
					  int                         min)
{
  mGeometries.addGeometry(ml, r, max, min);
}

void
unity::TexGeometryCollection::setTexture (GLTexture *tex)
{
  mTexture = tex;
}

void
unity::TexGeometryCollection::addGeometriesAndDrawTextureForWindow(CompWindow *w,
                                                                   const GLMatrix &transform,
                                                                   unsigned int mask)
{
  if (mTexture && mGeometries.status ())
  {
    CompRegion paintRegion = w->region ();
    GLWindow   *gWindow = GLWindow::get (w);

    if (mask & PAINT_WINDOW_TRANSFORMED_MASK)
      paintRegion = infiniteRegion;

    if (mGeometries.addGeometryForWindow (w, paintRegion))
    {
      UnityDialogScreen *uds = UnityDialogScreen::get (screen);
      GLWindowPaintAttrib attrib (gWindow->lastPaintAttrib());
      unsigned int glDrawTextureIndex = gWindow->glDrawTextureGetCurrentIndex();
      /* Texture rendering set-up */
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      /* Draw the dim texture with all of it's modified
       * geometry glory */
      gWindow->glDrawTextureSetCurrentIndex(MAXSHORT);
      gWindow->glDrawTexture(mTexture, transform, attrib, mask
                                                | PAINT_WINDOW_BLEND_MASK
                                                | PAINT_WINDOW_TRANSLUCENT_MASK
                                                | PAINT_WINDOW_TRANSFORMED_MASK);
      gWindow->glDrawTextureSetCurrentIndex(glDrawTextureIndex);
      /* Texture rendering tear-down */
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      uds->gScreen->setTexEnvMode(GL_REPLACE);
    }
  }
}

unity::PaintInfoCollector::PaintInfoCollector (CompWindow *w) :
  mWindow (w)
{
}

void
unity::PaintInfoCollector::collect()
{
  GLWindow *gWindow = GLWindow::get (mWindow);
  UnityDialogWindow *udw = UnityDialogWindow::get (mWindow);
  GLMatrix sTransform;

  sTransform.toScreenSpace(&screen->outputDevs()[screen->outputDeviceForGeometry(mWindow->geometry())], -DEFAULT_Z_CAMERA);

  gWindow->glDrawTextureSetEnabled(udw, true);
  gWindow->glAddGeometrySetEnabled(udw, true);
  gWindow->glDrawSetEnabled(udw, false);
  gWindow->glPaintSetEnabled(udw, false);

  mCollection.push_back (unity::TexGeometryCollection ());

  unity::PaintInfoCollector::active_collector = this;

  gWindow->glPaint(gWindow->lastPaintAttrib(), sTransform, infiniteRegion, 0);

  unity::PaintInfoCollector::active_collector = NULL;

  gWindow->glDrawTextureSetEnabled(udw, false);
  gWindow->glAddGeometrySetEnabled(udw, false);
  gWindow->glDrawSetEnabled(udw, true);
  gWindow->glPaintSetEnabled(udw, true);
}

void
unity::PaintInfoCollector::processGeometry (const GLTexture::MatrixList &ml,
				     const CompRegion            &r,
				     int                         min,
				     int                         max)
{
  mCollection.back ().addGeometry (ml, r, min, max);
}

void
unity::PaintInfoCollector::processTexture (GLTexture *tex)
{
  mCollection.back().setTexture (tex);
  mCollection.push_back (unity::TexGeometryCollection ());
}

void
unity::PaintInfoCollector::drawGeometriesForWindow(CompWindow *w,
                                                   const GLMatrix &transform,
                                                   unsigned int pm)
{
  for (unity::TexGeometryCollection &tcg : mCollection)
    tcg.addGeometriesAndDrawTextureForWindow (w, transform, pm);
}

unity::PaintInfoCollector * unity::PaintInfoCollector::active_collector = NULL;

unity::PaintInfoCollector *
unity::PaintInfoCollector::Active ()
{
  return active_collector;
}

/* Draw the window */

bool
UnityDialogWindow::glDraw(const GLMatrix& transform,
                          const GLWindowPaintAttrib& attrib,
                          const CompRegion& region,
                          unsigned int mask)
{
  /* We want to set the geometry of the dim to the window
   * region */
  CompRegion reg = CompRegion(window->x(), window->y(), window->width(), window->height());
  CompRegion        paintRegion(region);

  /* Draw the window on the bottom, we will be drawing the
   * dim render on top */
  bool status = gWindow->glDraw(transform,
                                attrib,
                                region, mask);

  UNITY_DIALOG_SCREEN(screen);

  for (GLTexture* tex : uds->tex())
  {
    GLTexture::MatrixList matl;
    GLTexture::Matrix     mat = tex->matrix();
    GLWindowPaintAttrib   wAttrib(attrib);

    /* We can reset the window geometry since it will be
     * re-added later */
    gWindow->vertexBuffer()->begin();

    /* Scale the dim render by the ratio of dim size
     * to window size */
    mat.xx *= (4) / window->width();
    mat.yy *= (4) / window->height();

    /* Not sure what this does, but it is necessary
     * (adjusts for scale?) */
    mat.x0 -= mat.xx * reg.boundingRect().x1();
    mat.y0 -= mat.yy * reg.boundingRect().y1();

    matl.push_back(mat);

    if (mask & PAINT_WINDOW_TRANSFORMED_MASK)
      paintRegion = infiniteRegion;

    /* Now allow plugins to mess with the geometry of our
     * dim (so we get a nice render for things like
     * wobbly etc etc */
    gWindow->glAddGeometry(matl, reg, paintRegion);
    if (gWindow->vertexBuffer()->end())
    {
      unsigned int glDrawTextureIndex = gWindow->glDrawTextureGetCurrentIndex();
      wAttrib.opacity = mShadeProgress;
      /* Texture rendering set-up */
      //    uds->gScreen->setTexEnvMode(GL_MODULATE);
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      /* Draw the dim texture with all of it's modified
       * geometry glory */
      gWindow->glDrawTextureSetCurrentIndex(MAXSHORT);
      gWindow->glDrawTexture(tex, transform, attrib, mask
                                                | PAINT_WINDOW_BLEND_MASK
                                                | PAINT_WINDOW_TRANSLUCENT_MASK
                                                | PAINT_WINDOW_TRANSFORMED_MASK);
      gWindow->glDrawTextureSetCurrentIndex(glDrawTextureIndex);
      /* Texture rendering tear-down */
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      uds->gScreen->setTexEnvMode(GL_REPLACE);
    }
  }

  for (CompWindow* w : mTransients)
  {
    if (GLWindow::get(w)->textures().empty())
      GLWindow::get(w)->bind();

    if (!UnityDialogWindow::get(w)->mIsAnimated)
    {
      unity::PaintInfoCollector pc (w);

      pc.collect();
      pc.drawGeometriesForWindow (window,
                                  transform,
                                  mask);
    }
  }

  return status;
}

void
UnityDialogWindow::getAllowedActions(unsigned int& setActions,
                                     unsigned int& clearActions)
{
  window->getAllowedActions(setActions, clearActions);

  if (!mParent)
    return;

  clearActions |= (CompWindowActionMoveMask |
                   CompWindowActionMaximizeVertMask |
                   CompWindowActionMaximizeHorzMask |
                   CompWindowActionResizeMask);
}

void
UnityDialogWindow::adjustIPW()
{
  XWindowChanges xwc;

  if (!mIpw)
    return;

  xwc.stack_mode = Above;
  xwc.x = window->input().left;
  xwc.y = window->input().top;
  xwc.width = window->width();
  xwc.height = window->height();

  XConfigureWindow(screen->dpy(), mIpw, CWStackMode | CWWidth | CWHeight | CWX | CWY, &xwc);
}

bool
UnityDialogWindow::addTransient(CompWindow* w)
{
  bool alreadyAdded = false;
  bool newParent = false;

  if (!mTransients.size())
  {
    gWindow->glDrawSetEnabled(this, true);
    gWindow->glPaintSetEnabled(this, true);
    window->grabNotifySetEnabled(this, true);
    window->ungrabNotifySetEnabled(this, true);
    window->moveNotifySetEnabled(this, true);
    cWindow->addDamage();

    if (!mIpw)
    {
      XSetWindowAttributes attr;

      attr.override_redirect = true;

      mIpw = XCreateWindow(screen->dpy(), window->frame(), window->input().left,
                           window->input().top, window->width(), window->height(),
                           0, 0, InputOnly, CopyFromParent, CWOverrideRedirect, &attr);

      XSelectInput(screen->dpy(), mIpw, StructureNotifyMask | ButtonPressMask);

      XMapWindow(screen->dpy(), mIpw);
      adjustIPW();
    }

    mCurrentPos = mTargetPos = window->serverBorderRect().pos();

    newParent = true;
  }
  else
  {
    for (CompWindow * tw : mTransients)
      if (tw->id() == w->id())
        alreadyAdded = true;
  }

  if (!alreadyAdded)
  {
    w->grabNotifySetEnabled(this, true);
    w->ungrabNotifySetEnabled(this, true);
    w->moveNotifySetEnabled(this, true);
    w->getAllowedActionsSetEnabled(this, true);
    UnityDialogWindow::get(w)->gWindow->glPaintSetEnabled(UnityDialogWindow::get(w), !mIsAnimated);
    UnityDialogWindow::get(w)->mParent = window;
    UnityDialogWindow::get(w)->setMaxConstrainingAreas();

    screen->matchPropertyChanged(window);
    w->recalcActions();
    mTransients.push_back(w);
  }

  return newParent;
}

bool
UnityDialogWindow::removeTransient(CompWindow* w)
{
  mTransients.remove(w);

  w->grabNotifySetEnabled(this, false);
  w->ungrabNotifySetEnabled(this, false);
  w->moveNotifySetEnabled(this, false);
  w->getAllowedActionsSetEnabled(this, false);
  UnityDialogWindow::get(w)->mParent = NULL;
  screen->matchPropertyChanged(window);
  w->recalcActions();

  if (!mTransients.size())
  {
    XWindowChanges xwc;
    unsigned int   mask = 0;

    window->ungrabNotifySetEnabled(this, false);
    window->grabNotifySetEnabled(this, false);
    window->moveNotifySetEnabled(this, false);

    if (mIpw)
    {
      XDestroyWindow(screen->dpy(), mIpw);
      mIpw = None;
    }

    if (mDiffXWC.width)
    {
      xwc.width = window->width() - mDiffXWC.width;
      mask |= CWWidth;
    }

    if (mDiffXWC.height)
    {
      xwc.height = window->height() - mDiffXWC.height;
      mask |= CWHeight;
    }

    if (mask)
      window->configureXWindow(mask, &xwc);

    cWindow->addDamage();
    window->move(-mOffset.x() - mDiffXWC.x, -mOffset.y() - mDiffXWC.y, true);
    cWindow->addDamage();

    memset(&mDiffXWC, 0, sizeof(XWindowChanges));

    mCurrentPos = CompPoint(window->serverBorderRect().x(), window->serverBorderRect().y());
    mTargetPos = mCurrentPos + mOffset;
    mOffset = CompPoint(0, 0);

    return true;
  }

  return false;
}

void
UnityDialogScreen::trackParent(CompWindow* w)
{
  unsigned long* data = new unsigned long;

  *data = 1;

  if (!mParentWindows.size())
  {
    cScreen->preparePaintSetEnabled(this, true);
    gScreen->glPaintOutputSetEnabled(this, true);
    cScreen->donePaintSetEnabled(this, true);
  }

  /* Set the _UNITY_IS_PARENT property */
  XChangeProperty(screen->dpy(), w->id(), mUnityIsParentAtom, XA_CARDINAL,
                  32, PropModeReplace, (const unsigned char*) data, 1);

  delete data;

  mParentWindows.push_back(w);
}

void
UnityDialogScreen::untrackParent(CompWindow* w)
{
  CompWindowList::iterator it = std::find(mParentWindows.begin(), mParentWindows.end(), w);

  if (it != mParentWindows.end())
  {
    mParentWindows.erase(it);

    /* Unset the _UNITY_IS_PARENT property */
    XDeleteProperty(screen->dpy(), w->id(), mUnityIsParentAtom);

    if (!mParentWindows.size())
    {
      cScreen->preparePaintSetEnabled(this, false);
      gScreen->glPaintOutputSetEnabled(this, false);
      cScreen->donePaintSetEnabled(this, false);
    }
  }
}

void
UnityDialogWindow::windowNotify(CompWindowNotify n)
{
  switch (n)
  {
    case CompWindowNotifyClose:

      /* If this window was a transient for some other window
         * then decrement the transient count */

      if (mParent)
        UnityDialogWindow::get(mParent)->removeTransient(window);

      break;

    case CompWindowNotifyFrameUpdate:

      if (mParent)
	moveToRect(mParent->serverBorderRect());

    default:
      break;
  }

  window->windowNotify(n);
}

void
UnityDialogWindow::grabNotify(int x, int y,
                              unsigned int state,
                              unsigned int mask)
{
  mGrabMask = mask;

  window->grabNotify(x, y, state, mask);

  if (!mSkipNotify && (mask & CompWindowGrabMoveMask))
  {
    moveTransientsToRect(NULL, window->serverBorderRect(), true);

    grabTransients(NULL, x, y, state, mask, true);
  }
}

void
UnityDialogWindow::ungrabNotify()
{
  mGrabMask = 0;

  window->ungrabNotify();

  if (!mSkipNotify)
  {
    moveTransientsToRect(NULL, window->serverBorderRect(), true);
    grabTransients(NULL, 0, 0, 0, 0, false);
  }
}

void
UnityDialogWindow::resizeNotify(int dx, int dy,
                                int dwidth,
                                int dheight)
{
  window->resizeNotify(dx, dy, dwidth, dheight);

  /* The window resized was a parent window, re-center transients */
  if (!mSkipNotify)
  {
    moveTransientsToRect(NULL, window->serverBorderRect(), true);

    if (mIpw)
      adjustIPW();

    if (mParent)
      UnityDialogWindow::get(mParent)->moveTransientsToRect(NULL, mParent->serverBorderRect(), true);
  }
}

void
UnityDialogWindow::moveNotify(int dx, int dy, bool immediate)
{
  window->moveNotify(dx, dy, immediate);

  if (!mSkipNotify)
  {
    if (mParent && UnityDialogScreen::get(screen)->switchingVp() &&
        !(mGrabMask & CompWindowGrabMoveMask))
    {
      moveParentToRect(window, window->serverBorderRect(), true);
    }
    else if (mParent)
    {
      moveToRect(mParent->serverBorderRect());
    }
    else
      moveTransientsToRect(window, window->serverBorderRect(), true);
  }
}

void
UnityDialogWindow::setMaxConstrainingAreas()
{
  /* Don't set maximum size hints if they are already set to a value
   * that is lower than ours (unlikely, but it has a chance of
   * happening so we need to handle this case)
   *
   * Also, core will set max_width to the same as min_width in the
   * case that somehow our requested max_width < min_width, so
   * detect this case too.
   *
   * In the case where we're goign to make the dialog window smaller
   * than it actually is, don't allow this, instead make parent window
   * at least 1.25x bigger than the dialog window and set the maximum
   * size of the dialog window to 0.8x the parent window
   */

  bool sizeHintsSet = false;
  bool needsWidth, needsHeight;
  XWindowChanges xwc;
  unsigned int   changeMask = 0;

  if (!(mParent->state() & MAXIMIZE_STATE ||
        mParent->state() & CompWindowStateFullscreenMask))
  {

    if (mParent->serverBorderRect().width() < window->serverBorderRect().width() * 1.25)
    {
      xwc.width = window->serverBorderRect().width() * 1.25;
      xwc.x = mParent->x() - ((window->serverBorderRect().width() * 1.25) -
                              (mParent->serverBorderRect().width())) / 2.0;

      /* Don't ever put the parent window offscreen */
      if (xwc.x < screen->workArea().left() + mParent->border().left)
        xwc.x = screen->workArea().left() + mParent->border().left;
      else if (xwc.x + xwc.width > screen->workArea().right() - mParent->border().right)
        xwc.x = screen->workArea().right() - xwc.width - mParent->border().right;

      if (!UnityDialogWindow::get(mParent)->mDiffXWC.width)
        UnityDialogWindow::get(mParent)->mDiffXWC.width = xwc.width - mParent->width();
      if (!UnityDialogWindow::get(mParent)->mDiffXWC.x)
      {
        UnityDialogWindow::get(mParent)->mDiffXWC.x = xwc.x - mParent->x();
        (UnityDialogWindow::get(mParent))->mCurrentPos += CompPoint(UnityDialogWindow::get(mParent)->mDiffXWC.x, 0);
        (UnityDialogWindow::get(mParent))->mTargetPos += CompPoint(UnityDialogWindow::get(mParent)->mDiffXWC.x, 0);
      }

      changeMask |= CWX | CWWidth;
    }

    if (mParent->serverBorderRect().height() < window->serverBorderRect().height() * 1.25)
    {
      xwc.height = window->serverBorderRect().height() * 1.25;
      xwc.y = mParent->y() - ((window->serverBorderRect().height() * 1.25) -
                              (mParent->serverBorderRect().height())) / 2.0;

      /* Don't ever put the parent window offscreen */
      if (xwc.y < screen->workArea().top() + mParent->border().top)
        xwc.y = screen->workArea().top() + mParent->border().top;
      else if (xwc.y + xwc.height > screen->workArea().bottom() - mParent->border().bottom)
        xwc.y = screen->workArea().bottom() - xwc.height - mParent->border().bottom;

      if (!UnityDialogWindow::get(mParent)->mDiffXWC.height)
        UnityDialogWindow::get(mParent)->mDiffXWC.height = xwc.height - mParent->height();
      if (!UnityDialogWindow::get(mParent)->mDiffXWC.y)
      {
        UnityDialogWindow::get(mParent)->mDiffXWC.y = xwc.y - mParent->y();
        (UnityDialogWindow::get(mParent))->mCurrentPos += CompPoint(0, UnityDialogWindow::get(mParent)->mDiffXWC.y);
        (UnityDialogWindow::get(mParent))->mTargetPos += CompPoint(0, UnityDialogWindow::get(mParent)->mDiffXWC.y);
      }

      changeMask |= CWY | CWHeight;
    }

  }

  if (changeMask)
    mParent->configureXWindow(changeMask, &xwc);

  needsWidth = mOldHintsSize.width() != window->sizeHints().max_width;

  if (needsWidth)
    needsWidth = !(mOldHintsSize.width() && window->sizeHints().max_width ==
                   window->sizeHints().min_width);

  needsHeight = mOldHintsSize.height() != window->sizeHints().max_height;

  if (needsHeight)
    needsHeight = !(mOldHintsSize.height() && window->sizeHints().max_height ==
                    window->sizeHints().min_height);

  if (mParent && (window->sizeHints().flags & PMaxSize) && needsWidth && needsHeight)
  {
    sizeHintsSet |= ((window->sizeHints().max_width <
                      mParent->serverGeometry().width() * 0.8) ||
                     (window->sizeHints().max_height <
                      mParent->serverGeometry().height() * 0.8));
  }

  if (mParent && (!sizeHintsSet))
  {
    XSizeHints sizeHints = window->sizeHints();

    sizeHints.flags |= PMaxSize;
    sizeHints.max_width = mParent->serverGeometry().width() * 0.8;
    sizeHints.max_height = mParent->serverGeometry().height() * 0.8;

    mOldHintsSize = CompSize(sizeHints.max_width, sizeHints.max_height);
    XSetWMNormalHints(screen->dpy(), window->id(), &sizeHints);
  }
}


CompPoint
UnityDialogWindow::getChildCenteredPositionForRect(CompRect currentRect)
{
  int centeredX = currentRect.x() + (currentRect.width() / 2 -
                                     window->serverBorderRect().width() / 2);
  int centeredY = currentRect.y() + (currentRect.height() / 2 -
                                     window->serverBorderRect().height() / 2);

  return CompPoint(centeredX, centeredY);
}

CompPoint
UnityDialogWindow::getParentCenteredPositionForRect(CompRect currentRect)
{
  int centeredX = currentRect.x() + (currentRect.width() / 2 -
                                     window->serverBorderRect().width() / 2);
  int centeredY = currentRect.y() + (currentRect.height() / 2 -
                                     window->serverBorderRect().height() / 2);

  return CompPoint(centeredX, centeredY);
}

void
UnityDialogWindow::grabTransients(CompWindow* skip, int x, int y,
                                  unsigned int state, unsigned int mask,
                                  bool grab)
{
  /* Center transients (leave a bit more space
   * below) */

  for (CompWindow* cw : mTransients)
  {
    UnityDialogWindow* udw = UnityDialogWindow::get(cw);

    if (cw == skip)
      return;

    udw->mSkipNotify = true;

    if (grab)
      udw->grabNotify(x, y, state, mask);
    else
      udw->ungrabNotify();

    udw->mSkipNotify = false;
  }
}

void
UnityDialogWindow::animateTransients(CompWindow* skip, CompPoint& orig, CompPoint& dest, bool cont)
{
  /* Center transients (leave a bit more space
   * below) */

  for (CompWindow* cw : mTransients)
  {
    UnityDialogWindow* udw = UnityDialogWindow::get(cw);
    CompRect newRect(dest.x(), dest.y(), window->serverBorderRect().width(), window->serverBorderRect().height());

    if (cw == skip)
      return;

    udw->mTargetPos = udw->getChildCenteredPositionForRect(newRect);
    udw->mCurrentPos = CompPoint(cw->serverBorderRect().x(), cw->serverBorderRect().y());
    udw->mOffset = udw->mTargetPos - udw->mCurrentPos;

    /* New transient position is centered to this window's target position */
    if (cont)
      udw->animateTransients(NULL, udw->mTargetPos, udw->mCurrentPos, true);
  }
}

void
UnityDialogWindow::animateParent(CompWindow* requestor, CompPoint& orig, CompPoint& dest)
{
  if (mParent)
  {
    if (!(mParent->state() & MAXIMIZE_STATE ||
          mParent->state() & CompWindowStateFullscreenMask))
    {
      UnityDialogWindow* udw = UnityDialogWindow::get(mParent);
      CompRect newRect(dest.x(), dest.y(), window->serverBorderRect().width(), window->serverBorderRect().height());

      udw->mTargetPos = udw->getParentCenteredPositionForRect(newRect);
      udw->mCurrentPos = CompPoint(mParent->serverBorderRect().x(), mParent->serverBorderRect().y());
      udw->mOffset = udw->mTargetPos - udw->mCurrentPos;

      udw->animateTransients(NULL, udw->mTargetPos, udw->mCurrentPos, false);
    }
  }
}

void
UnityDialogWindow::moveTransientsToRect(CompWindow* skip, CompRect currentRect, bool sync)
{
  /* Center transients (leave a bit more space
   * below) */

  for (CompWindow* cw : mTransients)
  {
    if (cw == skip)
      return;

    UnityDialogWindow::get(cw)->moveToRect(currentRect);

    /* Sync all of this window's transients */
    if (UnityDialogWindow::get(cw)->mTransients.size())
      UnityDialogWindow::get(cw)->moveTransientsToRect(NULL, cw->serverBorderRect(), sync);
  }
}

void
UnityDialogWindow::moveParentToRect(CompWindow*      requestor,
                                    CompRect      rect,
                                    bool      sync)
{
  if (mParent)
  {
    if (!(mParent->state() & MAXIMIZE_STATE ||
          mParent->state() & CompWindowStateFullscreenMask))
    {
      CompPoint centeredPos = UnityDialogWindow::get(mParent)->getParentCenteredPositionForRect(rect);
      UnityDialogWindow::get(mParent)->mSkipNotify = true;

      /* Move the parent window to the requested position */
      mParent->move(centeredPos.x() - mParent->borderRect().x(),
                    centeredPos.y() - mParent->borderRect().y(), true);

      UnityDialogWindow::get(mParent)->mSkipNotify = false;

      UnityDialogWindow::get(mParent)->moveTransientsToRect(requestor, window->serverBorderRect(), sync);
      UnityDialogWindow::get(mParent)->moveParentToRect(window, window->serverBorderRect(), sync);
    }
  }

}

CompWindow*
UnityDialogWindow::transientParent() const
{
  if (window->transientFor() &&
      window->state() & CompWindowStateModalMask &&
      !UnityDialogScreen::get(screen)->optionGetAvoidMatch().evaluate(window))
  {
    Window xid = window->transientFor();

    /* Some stupid applications set the WM_TRANSIENT_FOR to an unmapped window
     * but one that is strangely still managed *cough*qt*cough*. Those applications
     * deserve to be shot. In any case, we'll need to work around that by assuming
     * that this is a modal dialog with *a* parent, so go for the largest window
     * in this client group that's not a transient for anything */

    CompWindow* parent = screen->findWindow(xid);

    if (!parent->isViewable() || parent->overrideRedirect())
    {
      compLogMessage("unitydialog", CompLogLevelWarn, "window 0x%x sets WM_TRANSIENT_FOR" \
                     " in a strange way. Workaround around.\n", window->id());
      if (parent->clientLeader())
      {
        CompWindow* candidate = parent;

	for (CompWindow* w : screen->windows())
          if (w->clientLeader() == parent->clientLeader() &&
              w->isViewable() && !w->overrideRedirect() &&
              (w->geometry().width() * w->geometry().height()) >
              (candidate->geometry().width() * candidate->geometry().height()))
            candidate = w;

        if (candidate != parent)
          parent = candidate;
        else
          parent = NULL;
      }
    }

    if (parent)
      return parent;
  }

  return NULL;
}

bool
UnityDialogWindow::place(CompPoint& pos)
{
  CompWindow* parent;

  /* If this window is a transient for some other window,
   * increment the transient count and
   * enable the dimming on that other window */

  if ((parent = transientParent()) != NULL)
    if (UnityDialogWindow::get(parent)->addTransient(window))
      UnityDialogScreen::get(screen)->trackParent(parent);

  /* We need to check the final parent window */

  if (mParent)
  {
    CompWindow::Geometry transientGeometry;
    CompRegion transientPos, outputRegion, outsideArea, outsideRegion;
    pos = getChildCenteredPositionForRect(mParent->serverBorderRect());

    transientGeometry = CompWindow::Geometry(pos.x(),
                                             pos.y(),
                                             window->borderRect().width(),
                                             window->borderRect().height(), 0);

    transientPos = CompRegion((CompRect) transientGeometry);
    outputRegion = screen->workArea();
    outsideRegion = outputRegion;

    /* Create a w->width () px region outside of the output region */
    outsideRegion.shrink(-window->borderRect().width(), -window->borderRect().height());

    outsideRegion -= outputRegion;

    if (!(outsideArea = transientPos.intersected(outsideRegion)).isEmpty())
    {
      CompRect   transientRect;
      CompPoint  orig, dest;
      int width;
      int height;

      int hdirection = outsideArea.boundingRect().x() < 0 ? 1 : -1;
      int vdirection = outsideArea.boundingRect().y() < 0 ? 1 : -1;

      width  = outsideArea.boundingRect().width() >=
               window->serverBorderRect().width() ? 0 :
               outsideArea.boundingRect().width() * hdirection;
      height = outsideArea.boundingRect().height() >=
               window->serverBorderRect().height() ? 0 :
               outsideArea.boundingRect().height() * vdirection;

      pos += CompPoint(width, height);
      transientPos = CompRegion(pos.x(),
                                pos.y(),
                                window->serverBorderRect().width(), window->serverBorderRect().height());
      transientRect = transientPos.boundingRect();

      dest = CompPoint(transientRect.x(), transientRect.y());
      orig = CompPoint(mParent->serverBorderRect().x(), mParent->serverBorderRect().y());

      animateParent(window, orig, dest);

      moveParentToRect(window, transientRect, true);

    }

    pos -= CompPoint(window->border().left, window->border().top);

    return true;
  }

  return window->place(pos);
}

void
UnityDialogScreen::handleCompizEvent(const char*    plugin,
                                     const char*    event,
                                     CompOption::Vector&  o)
{
  if (strcmp(event, "start_viewport_switch") == 0)
  {
    mSwitchingVp = true;
  }
  else if (strcmp(event, "end_viewport_switch") == 0)
  {
    mSwitchingVp = false;
  }
  else if (strcmp(event, "window_animation") == 0)
  {
    CompWindow* w = screen->findWindow(CompOption::getIntOptionNamed(o, "window", 0));
    if (w)
    {
      if (UnityDialogWindow::get(w)->hasParent())
      {
        UnityDialogWindow::get(w)->setIsAnimated(CompOption::getBoolOptionNamed(o, "active", false));
        GLWindow::get(w)->glPaintSetEnabled(UnityDialogWindow::get(w), !UnityDialogWindow::get(w)->isAnimated());
      }
    }
  }

  screen->handleCompizEvent(plugin, event, o);
}

void
UnityDialogScreen::handleEvent(XEvent* event)
{
  CompWindow* w;

  screen->handleEvent(event);

  switch (event->type)
  {
    case ClientMessage:

      w = screen->findWindow(event->xclient.window);

      if (event->xclient.message_type == mCompizResizeAtom && w)
      {
        CompRect currentRect(event->xclient.data.l[0] - w->border().left,
                             event->xclient.data.l[1] - w->border().top ,
                             event->xclient.data.l[2] + w->border().left + w->border().right,
                             event->xclient.data.l[3] + w->border().top + w->border().bottom);
        UnityDialogWindow* udw = UnityDialogWindow::get(w);

        udw->moveTransientsToRect(NULL, currentRect, false);

      }
      break;
    case PropertyNotify:

      if (event->xproperty.type == XA_WM_TRANSIENT_FOR)
      {
        CompWindow* w = screen->findWindow(event->xproperty.window);

        if (w)
        {
          CompWindow* parent = screen->findWindow(w->transientFor());
          UnityDialogWindow* udw = UnityDialogWindow::get(parent);

          if (!udw->hasTransient(w))
          {
            /* This window got it's transient status updated
             * probably because the app was buggy and decided
             * to do so after it was mapped. Work around it
             */
            if (udw->addTransient(w))
              trackParent(parent);

            /* re-center all the transients */

            udw->moveTransientsToRect(w, parent->serverBorderRect(), true);
          }
        }
      }
      break;
    default:
      break;
  }
}

void
UnityDialogScreen::optionChanged(CompOption* option,
                                 Options    num)
{
  mTex->render(optionGetAlpha());
}

/* Constructors */

UnityDialogWindow::UnityDialogWindow(CompWindow* w) :
  PluginClassHandler <UnityDialogWindow, CompWindow> (w),
  window(w),
  cWindow(CompositeWindow::get(w)),
  gWindow(GLWindow::get(w)),
  mSkipNotify(false),
  mParent(NULL),
  mOldHintsSize(CompSize(0, 0)),
  mGrabMask(0),
  mShadeProgress(0),
  mIpw(None),
  mIsAnimated(false)
{
  WindowInterface::setHandler(window, true);
  GLWindowInterface::setHandler(gWindow, false);

  memset(&mDiffXWC, 0, sizeof(XWindowChanges));

  window->windowNotifySetEnabled(this, true);
}

UnityDialogWindow::~UnityDialogWindow()
{
  /* Handle weird circumstances where windows go away
   * without warning */

  UnityDialogScreen::get(screen)->untrackParent(window);

  if (mParent)
  {
    compLogMessage("unitydialog", CompLogLevelWarn, "Did not get a close notification before window was destroyed!");
    UnityDialogWindow::get(mParent)->removeTransient(window);
  }

  /* The parent went away, so make sure that
   * all the transients know about this one */
  if (mTransients.size())
  {
    compLogMessage("unitydialog", CompLogLevelWarn, "Parent got closed before transients. This is an indication of a buggy app!");
    for(CompWindow* w : mTransients)
      UnityDialogWindow::get(mParent)->removeTransient(w);
  }
}

UnityDialogScreen::UnityDialogScreen(CompScreen* s) :
  PluginClassHandler <UnityDialogScreen, CompScreen> (s),
  cScreen(CompositeScreen::get(s)),
  gScreen(GLScreen::get(s)),
  mSwitchingVp(false),
  mCompizResizeAtom(XInternAtom(screen->dpy(),
                                "_COMPIZ_RESIZE_NOTIFY", 0)),
  mUnityIsParentAtom(XInternAtom(screen->dpy(),
                                 "_UNITY_IS_PARENT", 0))
{
  ScreenInterface::setHandler(screen);
  CompositeScreenInterface::setHandler(cScreen, false);
  GLScreenInterface::setHandler(gScreen, false);

  optionSetAlphaNotify(boost::bind(&UnityDialogScreen::optionChanged, this, _1, _2));

  mTex = new UnityDialogShadeTexture();
  mTex->render(optionGetAlpha());
}

UnityDialogScreen::~UnityDialogScreen()
{
  delete mTex;
}

bool
UnityDialogPluginVTable::init()
{
  if (!CompPlugin::checkPluginABI("core", CORE_ABIVERSION) ||
      !CompPlugin::checkPluginABI("composite", COMPIZ_COMPOSITE_ABI) ||
      !CompPlugin::checkPluginABI("opengl", COMPIZ_OPENGL_ABI))
    return false;

  return true;
}
