// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2012 Canonical Ltd.
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#ifndef UNITYSHELL_PREVIEWRATINGSWIDGET_H
#define UNITYSHELL_PREVIEWRATINGSWIDGET_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include "PreviewContainer.h"

namespace nux
{
class StaticCairoText;
}

namespace unity
{
class RatingsButton;
namespace dash
{
namespace previews
{

class PreviewRatingsWidget : public debug::Introspectable, public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(PreviewRatingsWidget, nux::View);
public:
  PreviewRatingsWidget(NUX_FILE_LINE_PROTO);
  virtual ~PreviewRatingsWidget();

  void SetRating(float rating);
  float GetRating() const;

  void SetReviews(int count);

  sigc::signal<void> request_close() const { return preview_container_.request_close; }

protected:
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  
  virtual bool AcceptKeyNavFocus() { return false; }

  // From debug::Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder*);

private:
  RatingsButton* ratings_;
  StaticCairoText* reviews_;

  PreviewContainer preview_container_;
};

} // namespace previews
} // namespace dash
} // namespace unity

#endif // UNITYSHELL_PREVIEWRATINGSWIDGET_H
