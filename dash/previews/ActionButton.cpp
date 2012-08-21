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

#include "unity-shared/DashStyle.h"
#include "ActionButton.h"
#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include "unity-shared/IconTexture.h"
#include "unity-shared/StaticCairoText.h"

namespace
{
const int kMinButtonHeight = 36;
const int kMinButtonWidth  = 48;

const int icon_size  = 24;

nux::logging::Logger logger("unity.dash.actionbutton");
}

namespace unity
{
namespace dash
{

ActionButton::ActionButton(std::string const& label, std::string const& icon_hint, NUX_FILE_LINE_DECL)
  : nux::AbstractButton(NUX_FILE_LINE_PARAM)
  , image_(nullptr)
{
  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptKeyNavFocusOnMouseEnter(false);
  Init();
  BuildLayout(label, icon_hint);
}

ActionButton::~ActionButton()
{
}

void ActionButton::Init()
{
  InitTheme();

  key_nav_focus_change.connect([&] (nux::Area*, bool, nux::KeyNavDirection)
  {
    QueueDraw();
  });

  key_nav_focus_activate.connect([&](nux::Area*)
  {
    if (GetInputEventSensitivity())
      Active() ? Deactivate() : Activate();
  });
}

void ActionButton::InitTheme()
{
  if (!cr_active_)
  {
    nux::Geometry const& geo = GetGeometry();

    cr_prelight_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &ActionButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)));
    cr_active_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &ActionButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_PRESSED)));
    cr_normal_.reset(new nux::CairoWrapper(geo, sigc::bind(sigc::mem_fun(this, &ActionButton::RedrawTheme), nux::ButtonVisualState::VISUAL_STATE_NORMAL)));
    cr_focus_.reset(new nux::CairoWrapper(geo, sigc::mem_fun(this, &ActionButton::RedrawFocusOverlay)));
  }

  SetMinimumHeight(kMinButtonHeight);
  SetMinimumWidth(kMinButtonWidth);
}

void ActionButton::BuildLayout(std::string const& label, std::string const& icon_hint)
{
  if (icon_hint != icon_hint_)
  {
    icon_hint_ = icon_hint;
    if (image_)
    {
      image_.Release();
      image_ = NULL;
    }

    if (!icon_hint_.empty())
    {
      image_ = new IconTexture(icon_hint, icon_size);
      image_->texture_updated.connect([&](nux::BaseTexture*)
      {
        BuildLayout(label_, icon_hint_);
      });
      image_->SetInputEventSensitivity(false);
      image_->SetMinMaxSize(icon_size, icon_size);
    }
  }

  if (label != label_)
  {
    label_ = label;
    if (static_text_)
    {
      static_text_.Release();
      static_text_ = NULL;
    }

    if (!label_.empty())
    {
      static_text_ = new nux::StaticCairoText(label_, true, NUX_TRACKER_LOCATION);
      if (!font_hint_.empty())
        static_text_->SetFont(font_hint_);
      static_text_->SetInputEventSensitivity(false);
      static_text_->SetTextAlignment(nux::StaticCairoText::NUX_ALIGN_CENTRE);
    }
  }

  RemoveLayout();

  nux::HLayout* layout = new nux::HLayout();
  layout->SetHorizontalInternalMargin(6);
  layout->SetPadding(2, 11, 2, 11);
  layout->AddSpace(0,1);
  if (image_)
    layout->AddView(image_.GetPointer(), 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  if (static_text_)
    layout->AddView(static_text_.GetPointer(), 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);
  layout->AddSpace(0,1);
  SetLayout(layout);

  ComputeContentSize();
  QueueDraw();
}

void ActionButton::RedrawTheme(nux::Geometry const& geom, cairo_t* cr, nux::ButtonVisualState faked_state)
{
  Style::Instance().Button(cr, faked_state, "", -1, Alignment::CENTER, true);
}

void ActionButton::RedrawFocusOverlay(nux::Geometry const& geom, cairo_t* cr)
{
  Style::Instance().ButtonFocusOverlay(cr);
}

long ActionButton::ComputeContentSize()
{
  long ret = nux::View::ComputeContentSize();

  nux::Geometry const& geo = GetGeometry();

  if (cached_geometry_ != geo && geo.width > 0 && geo.height > 0)
  {
    if (cr_prelight_) cr_prelight_->Invalidate(geo);
    if (cr_active_) cr_active_->Invalidate(geo);
    if (cr_normal_) cr_normal_->Invalidate(geo);
    if (cr_focus_) cr_focus_->Invalidate(geo);

    cached_geometry_ = geo;
  }

  return ret;
}

void ActionButton::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();

  gPainter.PaintBackground(GfxContext, geo);
  // set up our texture mode
  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  // clear what is behind us
  unsigned int alpha = 0, src = 0, dest = 0;
  GfxContext.GetRenderStates().GetBlend(alpha, src, dest);
  GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  nux::Color col = nux::color::Black;
  col.alpha = 0;
  GfxContext.QRP_Color(geo.x,
                       geo.y,
                       geo.width,
                       geo.height,
                       col);

  nux::BaseTexture* texture = cr_normal_->GetTexture();
  if (Active())
    texture = cr_active_->GetTexture();
  else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)
    texture = cr_prelight_->GetTexture();
  else if (GetVisualState() == nux::ButtonVisualState::VISUAL_STATE_PRESSED)
    texture = cr_active_->GetTexture();

  GfxContext.QRP_1Tex(geo.x,
                      geo.y,
                      geo.width,
                      geo.height,
                      texture->GetDeviceTexture(),
                      texxform,
                      nux::Color(1.0f, 1.0f, 1.0f, 1.0f));

  if (IsMouseInside() || HasKeyboardFocus())
  {
    GfxContext.QRP_1Tex(geo.x,
                        geo.y,
                        geo.width,
                        geo.height,
                        cr_focus_->GetTexture()->GetDeviceTexture(),
                        texxform,
                        nux::Color(1.0f, 1.0f, 1.0f, 1.0f));
  }

  GfxContext.GetRenderStates().SetBlend(alpha, src, dest);


  if (GetCompositionLayout())
  {
    gPainter.PushPaintLayerStack();
    {
      nux::Geometry clip_geo = geo;

      GfxContext.PushClippingRectangle(clip_geo);
      gPainter.PushPaintLayerStack();
      GetCompositionLayout()->ProcessDraw(GfxContext, force_draw);
      gPainter.PopPaintLayerStack();
      GfxContext.PopClippingRectangle();
    }
    gPainter.PopPaintLayerStack();
  }
}

void ActionButton::RecvClick(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  click.emit(this);
}

void ActionButton::SetFont(std::string const& font_hint)
{
  if (static_text_)
  {
    static_text_->SetFont(font_hint);
    ComputeContentSize();
    QueueDraw();
  }
}


void ActionButton::Activate()
{
  if (active_ == true)
  {
    // already active
    return;
  }

  active_ = true;
  QueueDraw();
}

void ActionButton::Deactivate()
{
  if (active_ == false)
  {
    // already deactivated
    return;
  }

  active_ = false;
  QueueDraw();
}

} // namespace dash
} // namespace unity
