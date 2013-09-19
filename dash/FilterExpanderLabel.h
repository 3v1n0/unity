// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#ifndef UNITYSHELL_FILTEREXPANDERLABEL_H
#define UNITYSHELL_FILTEREXPANDERLABEL_H

#include <memory>

#include <Nux/Nux.h>
#include <Nux/GridHLayout.h>
#include <Nux/HLayout.h>
#include <Nux/View.h>
#include <Nux/VLayout.h>
#include <UnityCore/Filter.h>

#include "unity-shared/IconTexture.h"
#include "unity-shared/Introspectable.h"
#include "unity-shared/StaticCairoText.h"

namespace nux
{
class AbstractPaintLayer;
}

namespace unity
{

class HSeparator;

namespace dash
{

class FilterExpanderLabel : public nux::View,  public debug::Introspectable
{
  NUX_DECLARE_OBJECT_TYPE(FilterExpanderLabel, nux::View);
public:
  FilterExpanderLabel(std::string const& label, NUX_FILE_LINE_PROTO);

  void SetRightHandView(nux::View* view);
  void SetLabel(std::string const& label);
  void SetContents(nux::Layout* layout);

  virtual void SetFilter(Filter::Ptr const& filter) = 0;
  virtual std::string GetFilterType() = 0;

  nux::View* expander_view() const { return expander_view_; }

  nux::Property<bool> expanded;

protected:
  virtual bool AcceptKeyNavFocus();
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

  virtual void ClearRedirectedRenderChildArea() = 0;

  // Introspection
  virtual std::string GetName() const;
  virtual void AddProperties(debug::IntrospectionData&);

private:
  void BuildLayout();
  void DoExpandChange(bool change);
  bool ShouldBeHighlighted();

  nux::LinearLayout* layout_;
  nux::LinearLayout* top_bar_layout_;
  nux::View* expander_view_;
  nux::LinearLayout* expander_layout_;
  nux::View* right_hand_contents_;
  StaticCairoText* cairo_label_;
  std::string raw_label_;
  std::string label_;
  nux::VLayout* arrow_layout_;
  nux::SpaceLayout* arrow_top_space_;
  nux::SpaceLayout* arrow_bottom_space_;
  IconTexture* expand_icon_;

  nux::ObjectPtr<nux::Layout> contents_;
  std::unique_ptr<nux::AbstractPaintLayer> focus_layer_;
};

} // namespace dash
} // namespace unity

#endif // UNITYSHELL_FILTEREXPANDERLABEL_H
