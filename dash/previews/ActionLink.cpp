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
 * Authored by: Manuel de la Pena <manuel.delapena@canonical.com>
 *
 */

#include "ActionLink.h"
#include <NuxCore/Logger.h>
#include <Nux/VLayout.h>
#include "unity-shared/DashStyle.h"
#include "unity-shared/IconTexture.h"
#include "unity-shared/StaticCairoText.h"

namespace
{
nux::logging::Logger logger("unity.dash.actionlink");
const double LINK_NORMAL_ALPHA_VALUE = 4;
const double LINK_HIGHLIGHTED_ALPHA_VALUE = 1;
}

namespace unity
{
namespace dash
{

ActionLink::ActionLink(std::string const& action_hint, std::string const& label, NUX_FILE_LINE_DECL)
  : nux::AbstractButton(NUX_FILE_LINE_PARAM)
  , action_hint_(action_hint)
  , aligment_(StaticCairoText::NUX_ALIGN_CENTRE)
  , underline_(StaticCairoText::NUX_UNDERLINE_SINGLE)
{
  Init();
  BuildLayout(label);
}

std::string ActionLink::GetName() const
{
  return "ActionLink";
}

void ActionLink::AddProperties(debug::IntrospectionData& introspection)
{
  introspection
    .add(GetAbsoluteGeometry())
    .add("action", action_hint_)
    .add("label", label_)
    .add("font-hint", font_hint)
    .add("active", active_)
    .add("text-aligment", text_aligment)
    .add("underline-state", underline_state);
}

void ActionLink::Init()
{
  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptKeyNavFocusOnMouseEnter(true);

  // set properties to ensure that we do redraw when one of them changes
  text_aligment.SetSetterFunction(sigc::mem_fun(this, &ActionLink::set_aligment));
  text_aligment.SetGetterFunction(sigc::mem_fun(this, &ActionLink::get_aligment));

  underline_state.SetSetterFunction(sigc::mem_fun(this, &ActionLink::set_underline));
  underline_state.SetGetterFunction(sigc::mem_fun(this, &ActionLink::get_underline));

  font_hint.SetSetterFunction(sigc::mem_fun(this, &ActionLink::set_font_hint));
  font_hint.SetGetterFunction(sigc::mem_fun(this, &ActionLink::get_font_hint));

  key_nav_focus_change.connect([this] (nux::Area*, bool, nux::KeyNavDirection)
  {
    QueueDraw();
  });

  key_nav_focus_activate.connect([this](nux::Area*)
  {
    if (GetInputEventSensitivity())
      activate.emit(this, action_hint_);
  });
}

void ActionLink::BuildLayout(std::string const& label)
{

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
      static_text_ = new StaticCairoText(label_, true, NUX_TRACKER_LOCATION);
      if (!font_hint_.empty())
        static_text_->SetFont(font_hint_);
      static_text_->SetInputEventSensitivity(false);
      static_text_->SetTextAlignment(aligment_);
      static_text_->SetUnderline(underline_);
    }
  }

  RemoveLayout();

  nux::VLayout* layout = new nux::VLayout();
  if (static_text_)
  {
    layout->AddView(static_text_.GetPointer(),
                    1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL,
                    100.0f, nux::NUX_LAYOUT_END);
  }
  SetLayout(layout);

  ComputeContentSize();
  QueueDraw();
}

int ActionLink::GetLinkAlpha(nux::ButtonVisualState state, bool keyboardFocus)
{
  if (keyboardFocus || state == nux::ButtonVisualState::VISUAL_STATE_PRELIGHT)
    return LINK_HIGHLIGHTED_ALPHA_VALUE;
  else
    return LINK_NORMAL_ALPHA_VALUE;
}

void ActionLink::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();

  gPainter.PaintBackground(GfxContext, geo);
  // set up our texture mode
  nux::TexCoordXForm texxform;
  texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);
  texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);

  // clear what is behind us
  unsigned int alpha = 0, src = 0, dest = 0;

  // set the alpha of the text according to its state
  static_text_->SetTextAlpha(GetLinkAlpha(GetVisualState(), HasKeyboardFocus()));

  GfxContext.GetRenderStates().GetBlend(alpha, src, dest);
  GfxContext.GetRenderStates().SetBlend(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  nux::Color col = nux::color::Black;
  col.alpha = 0;
  GfxContext.QRP_Color(geo.x,
                       geo.y,
                       geo.width,
                       geo.height,
                       col);

  GfxContext.GetRenderStates().SetBlend(alpha, src, dest);

  if (GetCompositionLayout())
  {
    gPainter.PushPaintLayerStack();
    {

      GfxContext.PushClippingRectangle(geo);
      gPainter.PushPaintLayerStack();
      GetCompositionLayout()->ProcessDraw(GfxContext, force_draw);
      gPainter.PopPaintLayerStack();
      GfxContext.PopClippingRectangle();
    }
    gPainter.PopPaintLayerStack();
  }
}

void ActionLink::RecvClick(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  activate.emit(this, action_hint_);
}

bool ActionLink::set_aligment(StaticCairoText::AlignState aligment)
{
  if(static_text_ && aligment_ != aligment)
  {
    static_text_->SetTextAlignment(aligment_);
    aligment_ = aligment;
    ComputeContentSize();
    QueueDraw();
  }
  return true;
}

StaticCairoText::AlignState ActionLink::get_aligment()
{
  return aligment_;
}

bool ActionLink::set_underline(StaticCairoText::UnderlineState underline)
{
  if(static_text_ && underline_ != underline)
  {
    static_text_->SetUnderline(underline_);
    underline_ = underline;
    ComputeContentSize();
    QueueDraw();
  }
  return true;
}

StaticCairoText::UnderlineState ActionLink::get_underline()
{
  return underline_;
}

bool ActionLink::set_font_hint(std::string font_hint)
{
  if(static_text_ && font_hint_ != font_hint)
  {
    static_text_->SetFont(font_hint_);
    font_hint_ = font_hint;
    ComputeContentSize();
    QueueDraw();
  }
  return true;
}

std::string ActionLink::get_font_hint()
{
  return font_hint_;
}

std::string ActionLink::GetLabel() const
{
  return label_;
}

} // namespace dash
} // namespace unity
