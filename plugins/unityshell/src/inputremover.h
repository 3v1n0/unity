/*
 * Copyright (C) 2011 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authored By:
 * Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#ifndef _COMPIZ_INPUTREMOVER_H
#define _COMPIZ_INPUTREMOVER_H

#include <memory>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

// Will be merged back into compiz
namespace compiz {

class WindowInputRemoverInterface
{
  public:

    typedef std::shared_ptr <WindowInputRemoverInterface> Ptr;

    bool save () { return saveInput (); }
    bool remove () { return removeInput (); }
    bool restore () { return restoreInput (); }

    virtual ~WindowInputRemoverInterface ();

  protected:

    virtual bool saveInput () = 0;
    virtual bool removeInput () = 0;
    virtual bool restoreInput () = 0;
};

class WindowInputRemover :
  public WindowInputRemoverInterface
{
public:

  WindowInputRemover (Display *,
                      Window shapeWindow,
                      Window propWindow);
  ~WindowInputRemover ();

private:

  bool saveInput ();
  bool removeInput ();
  bool restoreInput ();

  void sendShapeNotify ();

  bool queryShapeRectangles(XRectangle **input,
                            int *nInput,
                            int *inputOrdering,
                            unsigned int *width,
                            unsigned int *height,
                            unsigned int *border);

  bool queryProperty(XRectangle **input,
                     int *nInput,
                     int *inputOrdering);

  bool writeProperty(XRectangle *input,
                     int nInput,
                     int inputOrdering);

  bool checkRectangles(XRectangle *input,
                       int *nInput,
                       int inputOrdering,
                       unsigned int width,
                       unsigned int height,
                       unsigned int border);

  bool saveRectangles(XRectangle *input,
                      int nInput,
                      int inputOrdering);

  void clearProperty ();
  void clearRectangles ();

  Display       *mDpy;
  Window        mShapeWindow;
  Window        mPropWindow;
  unsigned long mShapeMask;

  XRectangle    *mInputRects;
  int           mNInputRects;
  int           mInputRectOrdering;

  bool          mRemoved;

  int           mShapeEvent;
  int           mShapeError;

};

class WindowInputRemoverLock
{
  public:

    typedef std::shared_ptr <WindowInputRemoverLock> Ptr;
    typedef std::weak_ptr <WindowInputRemoverLock> Weak;

    WindowInputRemoverLock (WindowInputRemoverInterface *remover) :
        remover_ (remover)
    {
      remover->save();
      remover->remove();
    }

    ~WindowInputRemoverLock ()
    {
      remover_->restore();
      delete remover_;
    }

    void refresh ()
    {
      remover_->save();
      remover_->remove();
    }

  private:
    WindowInputRemoverInterface *remover_;
};

class WindowInputRemoverLockAcquireInterface
{
public:

  virtual ~WindowInputRemoverLockAcquireInterface () {}

  WindowInputRemoverLock::Ptr InputRemover () { return GetInputRemover (); }

private:

  virtual WindowInputRemoverLock::Ptr GetInputRemover () = 0;
};

}

#endif
