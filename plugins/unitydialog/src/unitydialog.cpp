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

COMPIZ_PLUGIN_20090315 (unitydialog, UnityDialogPluginVTable);

class UnityDialogExp :
    public CompMatch::Expression
{
    public:
	UnityDialogExp (const CompString &str);

	bool evaluate (CompWindow *w);

	bool value;
};


UnityDialogExp::UnityDialogExp (const CompString &str) :
    value (boost::lexical_cast<int> (str) != 0)
{
}

bool
UnityDialogExp::evaluate (CompWindow *w)
{
    UnityDialogWindow *udw = UnityDialogWindow::get (w);

    return ((value && udw->transientParent ()) || (!value && !udw->transientParent ()));
}

CompMatch::Expression *
UnityDialogScreen::matchInitExp (const CompString &str)
{
    CompString matchString ("transient-dialog=");
    /* Create a new match object */

    if (str.find (matchString) == 0)
	return new UnityDialogExp (str.substr (matchString.size ()));

    return screen->matchInitExp (str);
}

void
UnityDialogScreen::matchExpHandlerChanged ()
{
    screen->matchExpHandlerChanged ();

    foreach (CompWindow *w, screen->windows ())
    {
	if (UnityDialogWindow::get (w)->hasParent () ||
	    UnityDialogWindow::get (w)->hasTransients ())
	    screen->matchPropertyChanged (w);
    }
}

void
UnityDialogWindow::moveToRect (CompRect currentRect, bool sync)
{
    CompPoint pos = getChildCenteredPositionForRect (currentRect);

    mSkipNotify = true;
    window->move (pos.x () - window->x () + window->input ().left,
		  pos.y () - window->y () + window->input ().top, true);

    if (sync)
	window->syncPosition ();

    setMaxConstrainingAreas ();
    mSkipNotify = false;
}

CompWindow *
UnityDialogWindow::findTopParentWindow ()
{
    CompWindow	      *parent = window, *nextParent = NULL;

    /* Go to the bottom most window in the stack */
    while ((nextParent = UnityDialogWindow::get (parent)->mParent))
	parent = nextParent;

    /* parent now refers to the top most window */
    return parent;
}

UnityDialogShadeTexture::UnityDialogShadeTexture () :
    mPixmap (None),
    mTexture (0),
    mSurface (NULL),
    mCairo (NULL),
    mDamage (None)
{
    mOffscreenContainer = gtk_offscreen_window_new ();
    gtk_widget_set_name (mOffscreenContainer, "UnityPanelWidget");
    gtk_style_context_add_class (gtk_widget_get_style_context (mOffscreenContainer),
                                 "menubar");
    gtk_widget_set_size_request (mOffscreenContainer, 100, 24);
    gtk_widget_show_all (mOffscreenContainer);

    g_signal_connect (gtk_settings_get_default (), "notify::gtk-theme-name",
                      G_CALLBACK (UnityDialogShadeTexture::onThemeChanged), this);

    g_object_get (gtk_settings_get_default (), "gtk-theme-name", &mThemeName, NULL);
    mStyle = gtk_widget_get_style (mOffscreenContainer);

    context ();
}

UnityDialogShadeTexture::~UnityDialogShadeTexture ()
{
    gtk_widget_destroy (mOffscreenContainer);
    g_free (mThemeName);

    destroy ();
}

void
UnityDialogShadeTexture::destroy ()
{
    if (mPixmap)
    {
        XFreePixmap (screen->dpy (), mPixmap);
        mPixmap = None;
    }
    if (mSurface)
    {
        cairo_surface_destroy (mSurface);
        mSurface = NULL;
    }
    if (mCairo)
    {
        cairo_destroy (mCairo);
        mCairo = NULL;
    }
}

void
UnityDialogShadeTexture::clear ()
{
    if (mCairo)
    {
        cairo_save (mCairo);
        cairo_set_operator (mCairo, CAIRO_OPERATOR_CLEAR);
        cairo_paint (mCairo);
        cairo_restore (mCairo);
    }
}

void
UnityDialogShadeTexture::context ()
{
    if (!mCairo)
    {
	Screen		  *xScreen;
	XRenderPictFormat *format;
	int		  w, h;

	xScreen = ScreenOfDisplay (screen->dpy (), screen->screenNum ());

	format = XRenderFindStandardFormat (screen->dpy (),
					    PictStandardARGB32);

	w = 1;
	h = 1;

	mPixmap = XCreatePixmap (screen->dpy (), screen->root (), w, h, 32);

	mTexture = GLTexture::bindPixmapToTexture (mPixmap, w, h, 32);

	if (mTexture.empty ())
	{
	    compLogMessage ("unitydialog", CompLogLevelError,
			    "Couldn't bind pixmap 0x%x to texture",
			    (int) mPixmap);

	    XFreePixmap (screen->dpy (), mPixmap);

	    return;
	}

	mDamage = XDamageCreate (screen->dpy (), mPixmap,
				 XDamageReportRawRectangles);

	mSurface =
	    cairo_xlib_surface_create_with_xrender_format (screen->dpy (),
							   mPixmap, xScreen,
							   format, w, h);

        if (!mSurface)
        {
            compLogMessage ("unitydialog", CompLogLevelError, "Couldn't create surface");

            XFreePixmap (screen->dpy (), mPixmap);
            XDamageDestroy (screen->dpy (), mDamage);
            mTexture.clear ();

            return;
        }

        mCairo = cairo_create (mSurface);

        if (!mCairo)
        {
            compLogMessage ("unitydialog", CompLogLevelError, "Couldn't create cairo context");

            cairo_surface_destroy (mSurface);
            XFreePixmap (screen->dpy (), mPixmap);
            XDamageDestroy (screen->dpy (), mDamage);
            mTexture.clear ();
        }
    }
}

void
UnityDialogShadeTexture::render (float alpha)
{
    float divisorMask = 0xffff;
    mAlpha = alpha;

    clear ();

    cairo_set_line_width (mCairo, 2);
    cairo_set_source_rgba (mCairo,
                           (mStyle->bg[1].red / divisorMask),
                           (mStyle->bg[1].green / divisorMask),
                           (mStyle->bg[1].blue / divisorMask),
                           (alpha));

    cairo_move_to (mCairo, 0, 0);
    cairo_rectangle (mCairo, 0, 0, 1, 1);

    cairo_fill (mCairo);
}

void
UnityDialogShadeTexture::render ()
{
    render (mAlpha);
}

void
UnityDialogShadeTexture::onThemeChanged (GObject	  *obj,
				       GParamSpec *pspec,
				       gpointer	  data)
{
    UnityDialogShadeTexture *self = static_cast<UnityDialogShadeTexture *> (data);

    g_object_get (gtk_settings_get_default (), "gtk-theme-name", &self->mThemeName, NULL);
    self->mStyle = gtk_widget_get_style (self->mOffscreenContainer);

    self->render ();
}

const GLTexture::List &
UnityDialogShadeTexture::texture ()
{
    return mTexture;
}

bool
UnityDialogWindow::animate (int   ms,
			    float fadeTime)
{
    if (mTransients.size () && mShadeProgress < OPAQUE)
    {
	mShadeProgress += OPAQUE * (ms / fadeTime);

	if (mShadeProgress >= OPAQUE)
	    mShadeProgress = OPAQUE;

	return true;
    }
    else if (!mTransients.size () && mShadeProgress > 0)
    {
	mShadeProgress -=  OPAQUE * (ms / fadeTime);

	if (mShadeProgress <= 0)
	    mShadeProgress = 0;

	return true;
    }

    return false;
}

void
UnityDialogScreen::preparePaint (int ms)
{
    cScreen->preparePaint (ms);

    foreach (CompWindow *w, mParentWindows)
	UnityDialogWindow::get (w)->animate (ms, optionGetFadeTime ());

}

bool
UnityDialogScreen::glPaintOutput (const GLScreenPaintAttrib &attrib,
				  const GLMatrix	    &transform,
				  const CompRegion	    &region,
				  CompOutput		    *output,
				  unsigned int		    mask)
{
    mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS_MASK;

    return gScreen->glPaintOutput (attrib, transform, region, output, mask);
}

void
UnityDialogScreen::donePaint ()
{
    CompWindowList::iterator it = mParentWindows.begin ();

    cScreen->donePaint ();

    while (it != mParentWindows.end ())
    {
	CompWindow *w = (*it);
	UnityDialogWindow *udw = UnityDialogWindow::get (w);

	if (udw->animate (0, optionGetFadeTime ()))
	    udw->cWindow->addDamage ();
	else if (!udw->hasTransients ())
	{
	    untrackParent (w);
	    udw->gWindow->glDrawSetEnabled (udw, false);
	    udw->gWindow->glPaintSetEnabled (udw, false);

	    it = mParentWindows.begin ();
	    continue;
	}

	it++;
    }
}

/* Paint the window transformed */

bool
UnityDialogWindow::glPaint (const GLWindowPaintAttrib &attrib,
			    const GLMatrix	      &transform,
			    const CompRegion	      &region,
			    unsigned int	      mask)
{
    GLMatrix wTransform (transform);

    if (mTargetPos != mCurrentPos)
    {
	float progress = mShadeProgress / (float) OPAQUE;
	mask |= PAINT_WINDOW_TRANSFORMED_MASK;

	int dx = ((mTargetPos.x () - mCurrentPos.x ()) * progress);
	int dy = ((mTargetPos.y () - mCurrentPos.y ()) * progress);

	dx += mCurrentPos.x () - window->x ();
	dy += mCurrentPos.y () - window->y ();

	wTransform.translate (dx, dy, 0);

	if (mShadeProgress == OPAQUE)
	    mTargetPos = mCurrentPos;
    }

    return gWindow->glPaint (attrib, wTransform, region, mask);
}

/* Draw the window */

bool
UnityDialogWindow::glDraw (const GLMatrix &transform,
			   GLFragment::Attrib &fragment,
			   const CompRegion &region,
			   unsigned int mask)
{
    /* We want to set the geometry of the dim to the window
     * region */
    CompRegion reg = CompRegion (window->x (), window->y (), window->width (), window->height ());

    /* Draw the window on the bottom, we will be drawing the
     * dim render on top */
    bool status = gWindow->glDraw (transform, fragment, region, mask);

    UNITY_DIALOG_SCREEN (screen);

    foreach (GLTexture *tex, uds->tex ())
    {
	GLTexture::MatrixList matl;
	GLTexture::Matrix     mat = tex->matrix ();
	CompRegion	      paintRegion (region);

	/* We can reset the window geometry since it will be
	 * re-added later */
	gWindow->geometry ().reset ();

	/* Scale the dim render by the ratio of dim size
	 * to window size */
	mat.xx *= (4) / window->width ();
	mat.yy *= (4) / window->height ();

	/* Not sure what this does, but it is necessary
	 * (adjusts for scale?) */
	mat.x0 -= mat.xx * reg.boundingRect ().x1 ();
	mat.y0 -= mat.yy * reg.boundingRect ().y1 ();

	matl.push_back (mat);

	if (mask & PAINT_WINDOW_TRANSFORMED_MASK)
	    paintRegion = infiniteRegion;

	/* Now allow plugins to mess with the geometry of our
	 * dim (so we get a nice render for things like
	 * wobbly etc etc */
	gWindow->glAddGeometry (matl, reg, paintRegion);

	/* Did it succeed? */
	if (gWindow->geometry ().vertices)
	{
	    unsigned int glDrawTextureIndex = gWindow->glDrawTextureGetCurrentIndex ();
	    fragment.setOpacity (mShadeProgress);
	    /* Texture rendering set-up */
	    uds->gScreen->setTexEnvMode (GL_MODULATE);
	    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	    /* Draw the dim texture with all of it's modified
	     * geometry glory */
	    gWindow->glDrawTextureSetCurrentIndex (MAXSHORT);
	    gWindow->glDrawTexture (tex, fragment, mask | PAINT_WINDOW_BLEND_MASK
				    | PAINT_WINDOW_TRANSLUCENT_MASK |
				      PAINT_WINDOW_TRANSFORMED_MASK);
	    gWindow->glDrawTextureSetCurrentIndex (glDrawTextureIndex);
	    /* Texture rendering tear-down */
	    glBlendFunc (GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	    uds->gScreen->setTexEnvMode (GL_REPLACE);
	}
     }

    return status;
}

void
UnityDialogWindow::getAllowedActions (unsigned int &setActions,
				      unsigned int &clearActions)
{
    window->getAllowedActions (setActions, clearActions);

    if (!mParent)
	return;

    clearActions |= (CompWindowActionMoveMask |
		     CompWindowActionMaximizeVertMask |
		     CompWindowActionMaximizeHorzMask |
		     CompWindowActionResizeMask);
}

void
UnityDialogWindow::adjustIPW ()
{
    XWindowChanges xwc;

    if (!mIpw)
	return;

    xwc.stack_mode = Above;
    xwc.x = window->input ().left;
    xwc.y = window->input ().top;
    xwc.width = window->width ();
    xwc.height = window->height ();

    XConfigureWindow (screen->dpy (), mIpw, CWStackMode | CWWidth | CWHeight | CWX | CWY, &xwc);
}

bool
UnityDialogWindow::addTransient (CompWindow *w)
{
    bool alreadyAdded = false;
    bool newParent = false;

    if (!mTransients.size ())
    {
	gWindow->glDrawSetEnabled (this, true);
	gWindow->glPaintSetEnabled (this, true);
	window->grabNotifySetEnabled (this, true);
	window->ungrabNotifySetEnabled (this, true);
	window->moveNotifySetEnabled (this, true);
	cWindow->addDamage ();

	if (!mIpw)
	{
	    XSetWindowAttributes attr;

	    attr.override_redirect = true;

	    mIpw = XCreateWindow (screen->dpy (), window->frame (), window->input ().left,
				  window->input ().top, window->width (), window->height (),
				  0, 0, InputOnly, CopyFromParent, CWOverrideRedirect, &attr);

	    XSelectInput (screen->dpy (), mIpw, StructureNotifyMask | ButtonPressMask);

	    XMapWindow (screen->dpy (), mIpw);
	    adjustIPW ();
	}

	newParent = true;
    }
    else
    {
	foreach (CompWindow *tw, mTransients)
	    if (tw->id () == w->id ())
	    	alreadyAdded = true;
    }

    if (!alreadyAdded)
    {
	w->grabNotifySetEnabled (this, true);
	w->ungrabNotifySetEnabled (this, true);
	w->moveNotifySetEnabled (this, true);
	w->getAllowedActionsSetEnabled (this, true);
	UnityDialogWindow::get (w)->mParent = window;
	UnityDialogWindow::get (w)->setMaxConstrainingAreas ();

	screen->matchPropertyChanged (window);
	w->recalcActions ();
	mTransients.push_back (w);
    }

    return newParent;
}

bool
UnityDialogWindow::removeTransient (CompWindow *w)
{
    mTransients.remove (w);

    w->grabNotifySetEnabled (this, false);
    w->ungrabNotifySetEnabled (this, false);
    w->moveNotifySetEnabled (this, false);
    w->getAllowedActionsSetEnabled (this, false);
    UnityDialogWindow::get (w)->mParent = NULL;
    screen->matchPropertyChanged (window);
    w->recalcActions ();

    if (!mTransients.size ())
    {
	window->ungrabNotifySetEnabled (this, false);
	window->grabNotifySetEnabled (this, false);
	window->moveNotifySetEnabled (this, false);

	if (mIpw)
	{
	    XDestroyWindow (screen->dpy (), mIpw);
	    mIpw = None;
	}

	cWindow->addDamage ();

	return true;
    }

    return false;
}

void
UnityDialogScreen::trackParent (CompWindow *w)
{
    unsigned long *data = new unsigned long;

    *data = 1;

    if (!mParentWindows.size ())
    {
	cScreen->preparePaintSetEnabled (this, true);
	gScreen->glPaintOutputSetEnabled (this, true);
	cScreen->donePaintSetEnabled (this, true);
    }

    /* Set the _UNITY_IS_PARENT property */
    XChangeProperty (screen->dpy (), w->id (), mUnityIsParentAtom, XA_CARDINAL,
		     32, PropModeReplace, (const unsigned char *) data, 1);

    delete data;

    mParentWindows.push_back (w);
}

void
UnityDialogScreen::untrackParent (CompWindow *w)
{
    CompWindowList::iterator it = std::find (mParentWindows.begin (), mParentWindows.end (), w);

    if (it != mParentWindows.end ())
    {
	mParentWindows.erase (it);

	/* Unset the _UNITY_IS_PARENT property */
	XDeleteProperty (screen->dpy (), w->id (), mUnityIsParentAtom);

	if (!mParentWindows.size ())
	{
	    cScreen->preparePaintSetEnabled (this, false);
	    gScreen->glPaintOutputSetEnabled (this, false);
	    cScreen->donePaintSetEnabled (this, false);
	}
    }
}

void
UnityDialogWindow::windowNotify (CompWindowNotify n)
{
    switch (n)
    {
	case CompWindowNotifyClose:

	    /* If this window was a transient for some other window
	     * then decrement the transient count */

	    if (mParent)
		UnityDialogWindow::get (mParent)->removeTransient (window);

	    break;

	default:
	    break;
    }

    window->windowNotify (n);
}

void
UnityDialogWindow::grabNotify (int x, int y,
			       unsigned int state,
			       unsigned int mask)
{
    mGrabMask = mask;

    window->grabNotify (x, y, state, mask);

    if (!mSkipNotify && (mask & CompWindowGrabMoveMask))
    {
	moveTransientsToRect (NULL, window->serverBorderRect (), true);

	grabTransients (NULL, x, y, state, mask, true);
    }
}

void
UnityDialogWindow::ungrabNotify ()
{
    mGrabMask = 0;

    window->ungrabNotify ();

    if (!mSkipNotify)
    {
        moveTransientsToRect (NULL, window->serverBorderRect (), true);
	grabTransients (NULL, 0, 0, 0, 0, false);
    }
}

void
UnityDialogWindow::resizeNotify (int dx, int dy,
				 int dwidth,
				 int dheight)
{
    window->resizeNotify (dx, dy, dwidth, dheight);

    /* The window resized was a parent window, re-center transients */
    if (!mSkipNotify)
    {
        moveTransientsToRect (NULL, window->serverBorderRect (), true);

	if (mIpw)
	    adjustIPW ();

	if (mParent)
	    UnityDialogWindow::get (mParent)->moveTransientsToRect (NULL, mParent->serverBorderRect (), true);
    }
}

void
UnityDialogWindow::moveNotify (int dx, int dy, bool immediate)
{
    window->moveNotify (dx, dy, immediate);

    if (!mSkipNotify)
    {
	if ((mGrabMask & CompWindowGrabMoveMask ||
	    UnityDialogScreen::get (screen)->switchingVp ()) &&
	    !(mGrabMask & CompWindowGrabMoveMask &&
	      UnityDialogScreen::get (screen)->switchingVp ()))
	{
	    moveTransientsToRect (NULL, window->serverBorderRect (), UnityDialogScreen::get (screen)->switchingVp ());

	    if (UnityDialogScreen::get (screen)->switchingVp ())
		window->syncPosition ();
	}
	/* Not a valid reason for the transient to be moved -
	 * force it back to the main window */
	else if (mParent)
	    moveToRect (mParent->serverBorderRect (), true);
    }
}

void
UnityDialogWindow::setMaxConstrainingAreas ()
{
    /* Don't set maximum size hints if they are already set to a value
     * that is lower than ours (unlikely, but it has a chance of
     * happening so we need to handle this case)
     *
     * Also, core will set max_width to the same as min_width in the
     * case that somehow our requested max_width < min_width, so
     * detect this case too.
     *
     */

    bool sizeHintsSet = false;
    bool needsWidth, needsHeight;

    needsWidth = mOldHintsSize.width () != window->sizeHints ().max_width;

    if (needsWidth)
	needsWidth = !(mOldHintsSize.width () && window->sizeHints ().max_width ==
				      		 window->sizeHints ().min_width);

    needsHeight = mOldHintsSize.height () != window->sizeHints ().max_height;

    if (needsHeight)
	needsHeight = !(mOldHintsSize.height () && window->sizeHints ().max_height ==
				      		   window->sizeHints ().min_height);

    if (mParent && (window->sizeHints ().flags & PMaxSize) && needsWidth && needsHeight)
    {
	sizeHintsSet |=  ((window->sizeHints ().max_width <
			  mParent->width () * 0.8) ||
			  (window->sizeHints ().max_height <
			  mParent->height () * 0.8));
    }

    if (mParent && (!sizeHintsSet))
    {
	XSizeHints sizeHints = window->sizeHints ();

	sizeHints.flags |= PMaxSize;
	sizeHints.max_width = mParent->width () * 0.8;
	sizeHints.max_height = mParent->height () * 0.8;

	mOldHintsSize = CompSize (sizeHints.max_width, sizeHints.max_height);
	XSetWMNormalHints (screen->dpy (), window->id (), &sizeHints);
    }
}


CompPoint
UnityDialogWindow::getChildCenteredPositionForRect (CompRect currentRect)
{
    int centeredX = currentRect.x () + (currentRect.width () / 2 -
                                        window->serverBorderRect ().width () / 2);
    int centeredY = currentRect.y () + (currentRect.height () -
                                        window->serverBorderRect ().height ()) / 2;

    return CompPoint (centeredX, centeredY);
}

CompPoint
UnityDialogWindow::getParentCenteredPositionForRect (CompRect currentRect)
{
    int centeredX = currentRect.x () + (currentRect.width () / 2 -
				        window->width () / 2) -
				        window->input ().left;
    int centeredY = currentRect.y () + (currentRect.height () -
				        window->height ()) * (0.5) -
				        window->input ().top;

    return CompPoint (centeredX, centeredY);
}

void
UnityDialogWindow::grabTransients (CompWindow *skip, int x, int y,
				   unsigned int state, unsigned int mask,
				   bool grab)
{
    /* Center transients (leave a bit more space
     * below) */

    foreach (CompWindow *cw, mTransients)
    {
	UnityDialogWindow *udw = UnityDialogWindow::get (cw);

	if (cw == skip)
	    return;

	udw->mSkipNotify = true;
	
	if (grab)
	    udw->grabNotify (x, y, state, mask);
	else
	    udw->ungrabNotify ();

	udw->mSkipNotify = false;
    }
}

void
UnityDialogWindow::animateTransients (CompWindow *skip, CompPoint &orig, CompPoint &dest, bool cont)
{
    /* Center transients (leave a bit more space
     * below) */

    foreach (CompWindow *cw, mTransients)
    {
	UnityDialogWindow *udw = UnityDialogWindow::get (cw);
	CompRect newRect (dest.x (), dest.y (), window->width (), window->height ());

	if (cw == skip)
	    return;

	udw->mTargetPos = udw->getChildCenteredPositionForRect (newRect);
	udw->mCurrentPos = CompPoint (cw->x (), cw->y ());

	/* New transient position is centered to this window's target position */
	if (cont)
	    udw->animateTransients (NULL, udw->mTargetPos, udw->mCurrentPos, true);
    }
}

void
UnityDialogWindow::animateParent (CompWindow *requestor, CompPoint &orig, CompPoint &dest)
{
    if (mParent)
    {
	UnityDialogWindow *udw = UnityDialogWindow::get (mParent);
	CompRect newRect (dest.x (), dest.y (), window->width (), window->height ());

	udw->mTargetPos = udw->getParentCenteredPositionForRect (newRect);
	udw->mCurrentPos = CompPoint (mParent->x (), mParent->y ());

	udw->animateTransients (NULL, udw->mTargetPos, udw->mCurrentPos, false);
    }
}

void
UnityDialogWindow::moveTransientsToRect (CompWindow *skip, CompRect currentRect, bool sync)
{
    /* Center transients (leave a bit more space
     * below) */

    foreach (CompWindow *cw, mTransients)
    {
	if (cw == skip)
	    return;

	UnityDialogWindow::get (cw)->moveToRect (currentRect, sync);

	/* Sync all of this window's transients */
	if (UnityDialogWindow::get (cw)->mTransients.size ())
	    UnityDialogWindow::get (cw)->moveTransientsToRect (NULL, cw->serverBorderRect (), sync);
    }
}

void
UnityDialogWindow::moveParentToRect (CompWindow      *requestor,
				     CompRect	     rect,
				     bool	     sync)
{
    if (mParent)
    {
	CompPoint centeredPos = UnityDialogWindow::get (mParent)->getParentCenteredPositionForRect (rect);
	UnityDialogWindow::get (mParent)->mSkipNotify = true;

	/* Move the parent window to the requested position */

	mParent->move (centeredPos.x () - mParent->serverBorderRect ().x (),
		       centeredPos.y () - mParent->serverBorderRect ().y (), true);

	if (sync)
	    mParent->syncPosition ();

	UnityDialogWindow::get (mParent)->mSkipNotify = false;

	UnityDialogWindow::get (mParent)->moveTransientsToRect (requestor, window->serverBorderRect (), sync);
	UnityDialogWindow::get (mParent)->moveParentToRect (window, window->serverBorderRect (), sync);
    }

}

CompWindow *
UnityDialogWindow::transientParent ()
{
    if (window->transientFor () &&
	window->state () & CompWindowStateModalMask &&
	!UnityDialogScreen::get (screen)->optionGetAvoidMatch ().evaluate (window))
    {
	CompWindow *parent = screen->findWindow (window->transientFor ());

	if (parent)
	    return parent;
    }

    return NULL;
}

bool
UnityDialogWindow::place (CompPoint &pos)
{
    CompWindow *parent;

    /* If this window is a transient for some other window,
     * increment the transient count and
     * enable the dimming on that other window */

    if ((parent = transientParent ()) != NULL)
	if (UnityDialogWindow::get (parent)->addTransient (window))
	    UnityDialogScreen::get (screen)->trackParent (parent);

    /* We need to check the final parent window */

    if (mParent)
    {
	CompWindow::Geometry transientGeometry;
	CompRegion transientPos, outputRegion, outsideArea, outsideRegion;
	pos = getChildCenteredPositionForRect (mParent->serverBorderRect ());
	int	   hdirection, vdirection;

	transientGeometry = CompWindow::Geometry (pos.x () - window->input ().left,
						  pos.y () - window->input ().top,
						  window->inputRect ().width (),
						  window->inputRect ().height (), 0);

	transientPos = CompRegion ((CompRect) transientGeometry);
	outputRegion = screen->workArea ();
	outsideRegion = outputRegion;

	/* Create a w->width () px region outside of the output region */
	outsideRegion.shrink (-window->inputRect ().width (), -window->inputRect ().height ());

	outsideRegion -= outputRegion;

	if (!(outsideArea = transientPos.intersected (outsideRegion)).isEmpty ())
	{
	    CompRect   transientRect;
	    CompPoint  orig, dest;
	    int width;
	    int height;

	    hdirection = outsideArea.boundingRect ().x () < 0 ? 1 : -1;
	    vdirection = outsideArea.boundingRect ().y () < 0 ? 1 : -1;

	    width  = outsideArea.boundingRect ().width () >=
			window->width () ? 0 :
			outsideArea.boundingRect ().width () * hdirection;
	    height = outsideArea.boundingRect ().height () >=
			 window->height () ? 0 :
			 outsideArea.boundingRect ().height () * vdirection;

	    pos += CompPoint (width, height);
	    transientPos = CompRegion (pos.x (), pos.y (), window->width (), window->height ());
	    transientRect = transientPos.boundingRect ();
	    setMaxConstrainingAreas ();

	    dest = CompPoint (transientRect.x (), transientRect.y ());
	    orig = CompPoint (mParent->x (), mParent->y ());

	    animateParent (window, orig, dest);

	    moveParentToRect (window, transientRect, true);

	}

	return true;
    }

    return window->place (pos);
}

void
UnityDialogScreen::handleCompizEvent (const char	  *plugin,
				      const char	  *event,
				      CompOption::Vector  &o)
{
    if (strcmp (event, "start_viewport_switch") == 0)
    {
	mSwitchingVp = true;
    }
    else if (strcmp (event, "end_viewport_switch") == 0)
    {
	mSwitchingVp = false;

	foreach (CompWindow *w, mParentWindows)
	{
	    UnityDialogWindow *udw = UnityDialogWindow::get (w);

	    /* Only move bottomlevels */
	    if (udw->hasParent ())
		continue;

	    udw->moveTransientsToRect (NULL, w->serverBorderRect (), true);
	}
    }

    screen->handleCompizEvent (plugin, event, o);
}

void
UnityDialogScreen::handleEvent (XEvent *event)
{
    CompWindow *w;

    screen->handleEvent (event);

    switch (event->type)
    {
	case ClientMessage:

	    w = screen->findWindow (event->xclient.window);

	    if (event->xclient.message_type == mCompizResizeAtom && w)
	    {
		CompRect currentRect (event->xclient.data.l[0] - w->border ().left,
				      event->xclient.data.l[1] - w->border ().top ,
				      event->xclient.data.l[2] + w->border ().left + w->border ().right,
				      event->xclient.data.l[3] + w->border ().top + w->border ().bottom);
		UnityDialogWindow *udw = UnityDialogWindow::get (w);

		udw->moveTransientsToRect (NULL, currentRect, false);

	    }
	    break;
	case PropertyNotify:

	    if (event->xproperty.type == XA_WM_TRANSIENT_FOR)
	    {
		CompWindow *w = screen->findWindow (event->xproperty.window);

		if (w)
		{
		    CompWindow *parent = screen->findWindow (w->transientFor ());
		    UnityDialogWindow *udw = UnityDialogWindow::get (parent);

		    if (!udw->hasTransient (w))
		    {
			/* This window got it's transient status updated
			 * probably because the app was buggy and decided
			 * to do so after it was mapped. Work around it
			 */
			if (udw->addTransient (w))
			    trackParent (parent);

			/* re-center all the transients */

			udw->moveTransientsToRect (w, parent->serverBorderRect (), true);
		    }
		}
	    }
	    break;
	default:
	    break;
    }
}

void
UnityDialogScreen::optionChanged (CompOption *option,
                                  Options    num)
{
    mTex->render (optionGetAlpha ());
}

/* Constructors */

UnityDialogWindow::UnityDialogWindow (CompWindow *w) :
    PluginClassHandler <UnityDialogWindow, CompWindow> (w),
    window (w),
    cWindow (CompositeWindow::get (w)),
    gWindow (GLWindow::get (w)),
    mSkipNotify (false),
    mParent (NULL),
    mOldHintsSize (CompSize (0, 0)),
    mGrabMask (0),
    mShadeProgress (0),
    mIpw (None)
{
    WindowInterface::setHandler (window, true);
    GLWindowInterface::setHandler (gWindow, false);

    window->windowNotifySetEnabled (this, true);
}

UnityDialogWindow::~UnityDialogWindow ()
{
    /* Handle weird circumstances where windows go away
     * without warning */

    UnityDialogScreen::get (screen)->untrackParent (window);

    if (mParent)
    {
	compLogMessage ("unitydialog", CompLogLevelWarn, "Did not get a close notification before window was destroyed!");
	UnityDialogWindow::get (mParent)->removeTransient (window);
    }

    /* The parent went away, so make sure that
     * all the transients know about this one */
    if (mTransients.size ())
    {
	compLogMessage ("unitydialog", CompLogLevelWarn, "Parent got closed before transients. This is an indication of a buggy app!");
	foreach (CompWindow *w, mTransients)
	    UnityDialogWindow::get (mParent)->removeTransient (w);
    }
}

UnityDialogScreen::UnityDialogScreen (CompScreen *s) :
    PluginClassHandler <UnityDialogScreen, CompScreen> (s),
    cScreen (CompositeScreen::get (s)),
    gScreen (GLScreen::get (s)),
    mSwitchingVp (false),
    mCompizResizeAtom (XInternAtom (screen->dpy (),
				   "_COMPIZ_RESIZE_NOTIFY", 0)),
    mUnityIsParentAtom (XInternAtom (screen->dpy (),
				   "_UNITY_IS_PARENT", 0))
{
    ScreenInterface::setHandler (screen);
    CompositeScreenInterface::setHandler (cScreen, false);
    GLScreenInterface::setHandler (gScreen, false);

    optionSetAlphaNotify (boost::bind (&UnityDialogScreen::optionChanged, this, _1, _2));

    mTex = new UnityDialogShadeTexture ();
    mTex->render (optionGetAlpha ());
}

UnityDialogScreen::~UnityDialogScreen ()
{
    delete mTex;
}

bool
UnityDialogPluginVTable::init ()
{	
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION) ||
	!CompPlugin::checkPluginABI ("composite", COMPIZ_COMPOSITE_ABI) ||
	!CompPlugin::checkPluginABI ("opengl", COMPIZ_OPENGL_ABI))
	return false;

    return true;
}
