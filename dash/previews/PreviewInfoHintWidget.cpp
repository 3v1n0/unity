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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

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
nux::logging::Logger logger("unity.dash.previews.previewinfohintwidget");

NUX_IMPLEMENT_OBJECT_TYPE(PreviewInfoHintWidget);

PreviewInfoHintWidget::PreviewInfoHintWidget(dash::Preview::Ptr preview_model)
  : View(NUX_TRACKER_LOCATION)
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
  previews::Style& style = previews::Style::Instance();

  nux::VLayout* layout = new nux::VLayout();

  for (dash::Preview::InfoHintPtr info_hint : preview_model_->GetInfoHints())
  {
    nux::HLayout* hint_layout = new nux::HLayout();
    hint_layout->SetSpaceBetweenChildren(16);

    int icon_width = 32;

    IconTexture* info_icon = new IconTexture(info_hint->icon_hint, icon_width);
    info_icon->SetMinimumSize(icon_width, icon_width);
    info_icon->SetVisible(true);
    hint_layout->AddView(info_icon, 0);

    nux::StaticCairoText* info_name = new nux::StaticCairoText(info_hint->display_name, NUX_TRACKER_LOCATION);
    info_name->SetFont(style.info_hint_font());
    nux::Layout* info_name_layout = new nux::HLayout();
    info_name_layout->AddView(info_name, 1, nux::MINOR_POSITION_CENTER);
    info_name_layout->SetMaximumWidth(128 - (icon_width > 0 ? (icon_width + 16) : 0));


    nux::StaticCairoText* info_value = new nux::StaticCairoText(StringFromVariant(info_hint->value), NUX_TRACKER_LOCATION);
    info_value->SetFont(style.info_hint_font());
    nux::Layout* info_value_layout = new nux::HLayout();
    info_value_layout->AddView(info_value, 1, nux::MINOR_POSITION_CENTER);

    hint_layout->AddView(info_name_layout, 1);
    hint_layout->AddView(info_value_layout, 1);

    layout->AddLayout(hint_layout, 0);
  }

  SetLayout(layout);
}

} // namespace previews
} // namespace dash
} // namespace unity
