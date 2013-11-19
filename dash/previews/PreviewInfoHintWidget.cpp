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

#include "config.h"
#include <glib/gi18n-lib.h>

#include "PreviewInfoHintWidget.h"
#include "unity-shared/IntrospectableWrappers.h"
#include <NuxCore/Logger.h>
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
DECLARE_LOGGER(logger, "unity.dash.preview.infohintwidget");
namespace
{
const int layout_spacing = 12;
}

NUX_IMPLEMENT_OBJECT_TYPE(PreviewInfoHintWidget);

PreviewInfoHintWidget::PreviewInfoHintWidget(dash::Preview::Ptr preview_model, int icon_size)
: View(NUX_TRACKER_LOCATION)
, icon_size_(icon_size)
, preview_model_(preview_model)
{
  SetupViews();
}

PreviewInfoHintWidget::~PreviewInfoHintWidget()
{
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

void PreviewInfoHintWidget::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
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
  info_hints_.clear();

  previews::Style& style = previews::Style::Instance();

  auto on_mouse_down = [this](int x, int y, unsigned long button_flags, unsigned long key_flags) { this->preview_container_.OnMouseDown(x, y, button_flags, key_flags); };

  nux::VLayout* layout = new nux::VLayout();
  layout->SetSpaceBetweenChildren(6);

  for (dash::Preview::InfoHintPtr info_hint : preview_model_->GetInfoHints())
  {
    nux::HLayout* hint_layout = new nux::HLayout();
    hint_layout->SetSpaceBetweenChildren(layout_spacing);

    StaticCairoTextPtr info_name;
    if (!info_hint->display_name.empty())
    {
      // The "%s" is used in the dash preview to display the "<hint>: <value>" infos
      std::string tmp_display_name = glib::String(g_strdup_printf (_("%s:"), info_hint->display_name.c_str())).Str();

      info_name = new StaticCairoText(tmp_display_name, true, NUX_TRACKER_LOCATION);
      info_name->SetFont(style.info_hint_bold_font());
      info_name->SetLines(-1);
      info_name->SetTextAlignment(StaticCairoText::NUX_ALIGN_RIGHT);
      info_name->mouse_click.connect(on_mouse_down);
      hint_layout->AddView(info_name.GetPointer(), 0, nux::MINOR_POSITION_CENTER);
    }

    StaticCairoTextPtr info_value(new StaticCairoText(StringFromVariant(info_hint->value), true, NUX_TRACKER_LOCATION));
    info_value->SetFont(style.info_hint_font());
    info_value->SetLines(-1);
    info_value->mouse_click.connect(on_mouse_down);
    hint_layout->AddView(info_value.GetPointer(), 1, nux::MINOR_POSITION_CENTER);

    InfoHint info_hint_views(info_name, info_value);
    info_hints_.push_back(info_hint_views);

    layout->AddLayout(hint_layout, 0);
  }

  mouse_click.connect(on_mouse_down);

  SetLayout(layout);
}


void PreviewInfoHintWidget::PreLayoutManagement()
{
  previews::Style& style = previews::Style::Instance();
  nux::Geometry const& geo = GetGeometry();
  
  int info_hint_width = 0;
  for (InfoHint const& info_hint : info_hints_)
  {
    int width = style.GetInfoHintNameMinimumWidth();
    if (info_hint.first)
    {
      width = info_hint.first->GetTextExtents().width;

      if (width < style.GetInfoHintNameMinimumWidth())
        width = style.GetInfoHintNameMinimumWidth();
      else if (width > style.GetInfoHintNameMaximumWidth())
        width = style.GetInfoHintNameMaximumWidth();
    }

    if (info_hint_width < width)
    {
      info_hint_width = width;
    }
  }

  int info_value_width = geo.width;
  info_value_width -= layout_spacing;
  info_value_width -= info_hint_width;
  info_value_width = MAX(0, info_value_width);

  for (InfoHint const& info_hint : info_hints_)
  {
    if (info_hint.first)
    {
      info_hint.first->SetMinimumWidth(info_hint_width);
      info_hint.first->SetMaximumWidth(info_hint_width);
    }
    if (info_hint.second)
    {
      info_hint.second->SetMaximumWidth(info_value_width);
    }
  }

  View::PreLayoutManagement();
}

} // namespace previews
} // namespace dash
} // namespace unity
