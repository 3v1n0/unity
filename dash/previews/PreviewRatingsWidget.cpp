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

#include <Nux/Nux.h>
#include <Nux/VLayout.h>
#include <glib.h>
#include "config.h"
#include <glib/gi18n-lib.h>

#include "unity-shared/RatingsButton.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/PreviewStyle.h"
#include "unity-shared/DashStyle.h"
#include "PreviewRatingsWidget.h"

namespace unity
{
namespace dash
{
namespace previews
{

namespace
{
  const RawPixel CHILDREN_SPACE = 3_em;
  const int RATINGS_SIZE = 18;
  const int RATINGS_GAP = 2;
}

NUX_IMPLEMENT_OBJECT_TYPE(PreviewRatingsWidget);

PreviewRatingsWidget::PreviewRatingsWidget(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , scale(1.0)
{
  layout_ = new nux::VLayout();
  layout_->SetSpaceBetweenChildren(CHILDREN_SPACE.CP(scale));

  previews::Style& style = previews::Style::Instance();

  auto on_mouse_down = [this](int x, int y, unsigned long button_flags, unsigned long key_flags) { this->preview_container_.OnMouseDown(x, y, button_flags, key_flags); };

  ratings_ = new RatingsButton(RATINGS_SIZE, RATINGS_GAP);
  ratings_->SetEditable(false);
  ratings_->mouse_click.connect(on_mouse_down);
  ratings_->scale = scale();
  layout_->AddView(ratings_);

  reviews_ = new StaticCairoText("", NUX_TRACKER_LOCATION);
  reviews_->SetFont(style.user_rating_font());
  reviews_->SetScale(scale);
  reviews_->mouse_click.connect(on_mouse_down);
  layout_->AddView(reviews_);

  mouse_click.connect(on_mouse_down);

  SetLayout(layout_);

  UpdateScale(scale);
  scale.changed.connect(sigc::mem_fun(this, &PreviewRatingsWidget::UpdateScale));
}

void PreviewRatingsWidget::SetRating(float rating)
{
  ratings_->SetRating(rating);
}

float PreviewRatingsWidget::GetRating() const
{
  return ratings_->GetRating();
}

void PreviewRatingsWidget::SetReviews(int count)
{
  std::stringstream out;
  out << count;
  out << " reviews";

  reviews_->SetText(out.str());
}

void PreviewRatingsWidget::Draw(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
}

void PreviewRatingsWidget::DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();
  gfx_engine.PushClippingRectangle(base);

  if (GetCompositionLayout())
    GetCompositionLayout()->ProcessDraw(gfx_engine, force_draw);

  gfx_engine.PopClippingRectangle();
}

std::string PreviewRatingsWidget::GetName() const
{
  return "PreviewRatingsWidget";
}

void PreviewRatingsWidget::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
    .add(GetAbsoluteGeometry());
}

void PreviewRatingsWidget::UpdateScale(double scale)
{
  reviews_->SetScale(scale);
  ratings_->scale = scale;
  preview_container_.scale = scale;
  layout_->SetSpaceBetweenChildren(CHILDREN_SPACE.CP(scale));

  QueueRelayout();
  QueueDraw();
}

} // namespace previews
} // namespace dash
} // namespace unity
