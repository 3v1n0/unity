/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */



#ifndef PREVIEWGENERIC_H
#define PREVIEWGENERIC_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <UnityCore/GenericPreview.h>

#include "PreviewBase.h"

namespace unity {

  class PreviewGeneric : public PreviewBase
  {
  public:
    PreviewGeneric (dash::Preview::Ptr preview, NUX_FILE_LINE_PROTO);
    virtual ~PreviewGeneric ();

    virtual void SetPreview(dash::Preview::Ptr preview);

  protected:
    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

  private:
    void BuildLayout();
    dash::GenericPreview::Ptr preview_;

  };

}
#endif // PREVIEWGENERIC_H
