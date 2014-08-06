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
 *              Marco Trevisan <marco.trevisan@canonical.com>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "PreviewInfoHintWidget.h"
#include "unity-shared/IntrospectableWrappers.h"
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <unity-shared/StaticCairoText.h>
#include <unity-shared/IconTexture.h>
#include <unity-shared/PreviewStyle.h>

namespace unity
{
namespace dash
{
namespace previews
{
namespace
{
const RawPixel LAYOUT_SPACING = 12_em;
const RawPixel HINTS_SPACING = 6_em;
}

NUX_IMPLEMENT_OBJECT_TYPE(PreviewInfoHintWidget);

PreviewInfoHintWidget::PreviewInfoHintWidget(dash::Preview::Ptr preview_model, int icon_size)
: View(NUX_TRACKER_LOCATION)
, scale(1.0)
, icon_size_(icon_size)
, layout_(nullptr)
, info_names_layout_(nullptr)
, info_values_layout_(nullptr)
, preview_model_(preview_model)
{
  SetupViews();
  scale.changed.connect(sigc::mem_fun(this, &PreviewInfoHintWidget::UpdateScale));
}

void PreviewInfoHintWidget::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
}

void PreviewInfoHintWidget::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

  if (GetCompositionLayout())
  {
    unsigned int alpha, src, dest = 0;
    gfx_engine.GetRenderStates().GetBlend(alpha, src, dest);
    gfx_engine.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

    gfx_engine.GetRenderStates().SetBlend(alpha, src, dest);
  }

  gfx_engine.PopClippingRectangle();
}

std::string PreviewInfoHintWidget::GetName() const
{
  return "PreviewInfoHintWidget";
}

void PreviewInfoHintWidget::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
    .add(GetAbsoluteGeometry());
}

std::string StringFromVariant(GVariant* variant)
{
    std::stringstream ss;
    const GVariantType* info_hint_type = g_variant_get_type(variant);

    if (g_variant_type_equal(info_hint_type, G_VARIANT_TYPE_BOOLEAN))
    {
      ss << g_variant_get_int16(variant);
    }
    else if (g_variant_type_equal(info_hint_type, G_VARIANT_TYPE_INT16))
    {
      ss << g_variant_get_int16(variant);
    }
    else if (g_variant_type_equal(info_hint_type, G_VARIANT_TYPE_UINT16))
    {
      ss << g_variant_get_uint16(variant);
    }
    else if (g_variant_type_equal(info_hint_type, G_VARIANT_TYPE_INT32))
    {
      ss << g_variant_get_int32(variant);
    }
    else if (g_variant_type_equal(info_hint_type,  G_VARIANT_TYPE_UINT32))
    {
      ss << g_variant_get_uint32(variant);
    }
    else if (g_variant_type_equal(info_hint_type,  G_VARIANT_TYPE_INT64))
    {
      ss << g_variant_get_int64(variant);
    }
    else if (g_variant_type_equal(info_hint_type,   G_VARIANT_TYPE_UINT64))
    {
      ss << g_variant_get_uint64(variant);
    }
    else if (g_variant_type_equal(info_hint_type,   G_VARIANT_TYPE_DOUBLE))
    {
      ss << g_variant_get_double(variant);
    }
    else if (g_variant_type_equal(info_hint_type,   G_VARIANT_TYPE_STRING))
    {
      std::string str = g_variant_get_string(variant, NULL);
      ss << str;
    }
    else
    {
      ss << "unknown value";
    }
    return ss.str();
}

void PreviewInfoHintWidget::SetupViews()
{
  RemoveLayout();
  auto& style = previews::Style::Instance();

  auto on_mouse_down = [this](int x, int y, unsigned long button_flags, unsigned long key_flags) { this->preview_container_.OnMouseDown(x, y, button_flags, key_flags); };

  layout_ = new nux::HLayout();
  layout_->SetSpaceBetweenChildren(LAYOUT_SPACING.CP(scale));

  auto *hint_vlayout = new nux::VLayout();
  hint_vlayout->SetSpaceBetweenChildren(HINTS_SPACING.CP(scale));
  layout_->AddLayout(hint_vlayout);
  info_names_layout_ = hint_vlayout;

  hint_vlayout = new nux::VLayout();
  hint_vlayout->SetSpaceBetweenChildren(HINTS_SPACING.CP(scale));
  layout_->AddLayout(hint_vlayout);
  info_values_layout_ = hint_vlayout;

  for (dash::Preview::InfoHintPtr const& info_hint : preview_model_->GetInfoHints())
  {
    // The "%s" is used in the dash preview to display the "<hint>: <value>" infos
    auto const& name = glib::String(g_strdup_printf (_("%s:"), info_hint->display_name.c_str())).Str();
    auto* info_name = new StaticCairoText(name == ":" ? "" : name, true, NUX_TRACKER_LOCATION);
    info_name->SetFont(style.info_hint_bold_font());
    info_name->SetLines(-1);
    info_name->SetScale(scale);
    info_name->SetTextAlignment(StaticCairoText::NUX_ALIGN_RIGHT);
    info_name->SetMinimumWidth(style.GetInfoHintNameMinimumWidth().CP(scale));
    info_name->SetMaximumWidth(style.GetInfoHintNameMaximumWidth().CP(scale));
    info_name->mouse_click.connect(on_mouse_down);
    info_names_layout_->AddView(info_name, 1, nux::MINOR_POSITION_RIGHT);

    auto* info_value = new StaticCairoText(StringFromVariant(info_hint->value), true, NUX_TRACKER_LOCATION);
    info_value->SetFont(style.info_hint_font());
    info_value->SetLines(-1);
    info_value->SetScale(scale);
    info_value->mouse_click.connect(on_mouse_down);
    info_values_layout_->AddView(info_value, 1, nux::MINOR_POSITION_LEFT);
  }

  mouse_click.connect(on_mouse_down);

  SetLayout(layout_);
}


void PreviewInfoHintWidget::PreLayoutManagement()
{
  if (info_names_layout_ && info_values_layout_)
  {
    nux::Geometry const& geo = GetGeometry();
    info_names_layout_->SetMaximumWidth(info_names_layout_->GetContentWidth());
    int max_width = std::max(0, geo.width - info_names_layout_->GetWidth() - LAYOUT_SPACING.CP(scale) -1);

    for (auto value : info_values_layout_->GetChildren())
      value->SetMaximumWidth(max_width);
  }

  View::PreLayoutManagement();
}

void PreviewInfoHintWidget::UpdateScale(double scale)
{
  if (layout_)
    layout_->SetSpaceBetweenChildren(LAYOUT_SPACING.CP(scale));

  if (info_names_layout_)
  {
    info_names_layout_->SetSpaceBetweenChildren(HINTS_SPACING.CP(scale));

    for (auto* area : info_names_layout_->GetChildren())
      static_cast<StaticCairoText*>(area)->SetScale(scale);
  }

  if (info_values_layout_)
  {
    info_values_layout_->SetSpaceBetweenChildren(HINTS_SPACING.CP(scale));

    for (auto* area : info_values_layout_->GetChildren())
      static_cast<StaticCairoText*>(area)->SetScale(scale);
  }

  QueueRelayout();
  QueueDraw();
}

} // namespace previews
} // namespace dash
} // namespace unity
