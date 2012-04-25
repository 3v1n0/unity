/*
 * Copyright (C) 2010 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Gord Allott <gord.allott@canonical.com>
 */

#include "HudView.h"

#include <math.h>

#include <glib/gi18n-lib.h>
#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Variant.h>
#include <Nux/HLayout.h>

#include <NuxCore/Logger.h>
#include "HudButton.h"
#include "UBusMessages.h"
#include "DashStyle.h"

namespace unity
{
namespace hud
{

namespace
{
nux::logging::Logger logger("unity.hud.view");
const int grow_anim_length = 90 * 1000;
const int pause_before_grow_length = 32 * 1000;

const int default_width = 960;
const int default_height = 276;
const int content_width = 941;

const int top_padding = 11;
const int bottom_padding = 9;
const int left_padding = 11;
const int right_padding = 0;
}

NUX_IMPLEMENT_OBJECT_TYPE(View);

View::View()
  : nux::View(NUX_TRACKER_LOCATION)
  , button_views_(NULL)
  , timeline_id_(0)
  , start_time_(0)
  , last_known_height_(0)
  , current_height_(0)
  , timeline_need_more_draw_(false)
  , selected_button_(0)
  , show_embedded_icon_(true)
{
  renderer_.SetOwner(this);
  renderer_.need_redraw.connect([this] () {
    QueueDraw();
  });

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  SetupViews();
  search_bar_->key_down.connect (sigc::mem_fun (this, &View::OnKeyDown));

  search_bar_->activated.connect (sigc::mem_fun (this, &View::OnSearchbarActivated));

  search_bar_->text_entry()->SetLoseKeyFocusOnKeyNavDirectionUp(false);
  search_bar_->text_entry()->SetLoseKeyFocusOnKeyNavDirectionDown(false);

  search_bar_->text_entry()->key_nav_focus_change.connect([&](nux::Area *area, bool receiving, nux::KeyNavDirection direction)
  {
    // We get here when the Hud closes.
    // The TextEntry should always have the keyboard focus as long as the hud is open.

    if (buttons_.empty())
      return;// early return on empty button list

    if (receiving)
    {
      if (!buttons_.empty())
      {
        // If the search_bar gets focus, fake focus the first button if it exists
        buttons_.back()->fake_focused = true;
      }
    }
    else
    {
      // The hud is closing and there are HudButtons visible. Remove the fake_focus.
      // There should be only one HudButton with the fake_focus set to true.
      std::list<HudButton::Ptr>::iterator it;
      for(it = buttons_.begin(); it != buttons_.end(); ++it)
      {
        if ((*it)->fake_focused)
        {
          (*it)->fake_focused = false;
        }
      }
    }
  });

  mouse_down.connect(sigc::mem_fun(this, &View::OnMouseButtonDown));

  Relayout();
}

View::~View()
{
  for (auto button = buttons_.begin(); button != buttons_.end(); button++)
  {
    RemoveChild((*button).GetPointer());
  }

  if (timeline_id_)
    g_source_remove(timeline_id_);
}

void View::ProcessGrowShrink()
{
  float diff = g_get_monotonic_time() - start_time_;
  int target_height = content_layout_->GetGeometry().height;
  // only animate if we are after our defined pause time
  if (diff > pause_before_grow_length)
  {
   float progress = (diff - pause_before_grow_length) / grow_anim_length;
   int last_height = last_known_height_;
   int new_height = 0;

   if (last_height < target_height)
   {
     // grow
     new_height = last_height + ((target_height - last_height) * progress);
   }
   else
   {
     //shrink
     new_height = last_height - ((last_height - target_height) * progress);
   }

   LOG_DEBUG(logger) << "resizing to " << target_height << " (" << new_height << ")"
                     << "View height: " << GetGeometry().height;
   current_height_ = new_height;
  }

  QueueDraw();

  if (diff > grow_anim_length + pause_before_grow_length)
  {
    // ensure we are at our final location and update last known height
    current_height_ = target_height;
    last_known_height_ = target_height;
    timeline_need_more_draw_ = false;
  }
}


void View::ResetToDefault()
{
  search_bar_->search_string = "";
  search_bar_->search_hint = _("Type your command");
}

void View::Relayout()
{
  nux::Geometry const& geo = GetGeometry();
  content_geo_ = GetBestFitGeometry(geo);
  LOG_DEBUG(logger) << "content_geo: " << content_geo_.width << "x" << content_geo_.height;

  layout_->SetMinimumWidth(content_geo_.width);
  layout_->SetMaximumSize(content_geo_.width, content_geo_.height);

  QueueDraw();
}

long View::PostLayoutManagement(long LayoutResult)
{
  Relayout();
  if (GetGeometry().height != last_known_height_)
  {
    // Start the timeline of drawing the dash resize
    if (timeline_need_more_draw_)
    {
      // already started, just reset the last known height
      last_known_height_ = current_height_;
    }

    timeline_need_more_draw_ = true;
    start_time_ = g_get_monotonic_time();
    QueueDraw();
  }

  return LayoutResult;
}


nux::View* View::default_focus() const
{
  return search_bar_->text_entry();
}

void View::SetQueries(Hud::Queries queries)
{
  // early exit, if the user is key navigating on the hud, we don't want to set new
  // queries under them, that is just rude
  if (!buttons_.empty() && buttons_.back()->fake_focused == false)
    return;

  // remove the previous children
  for (auto button : buttons_)
  {
    RemoveChild(button.GetPointer());
  }

  selected_button_ = 0;
  queries_ = queries_;
  buttons_.clear();
  button_views_->Clear();
  int found_items = 0;
  for (auto query = queries.begin(); query != queries.end(); query++)
  {
    if (found_items >= 5)
      break;

    HudButton::Ptr button(new HudButton());
    buttons_.push_front(button);
    button->SetQuery(*query);

    button_views_->AddView(button.GetPointer(), 0, nux::MINOR_POSITION_LEFT);

    button->click.connect([&](nux::View* view) {
      query_activated.emit(dynamic_cast<HudButton*>(view)->GetQuery());
    });

    button->key_nav_focus_activate.connect([&](nux::Area *area) {
      query_activated.emit(dynamic_cast<HudButton*>(area)->GetQuery());
    });

    button->key_nav_focus_change.connect([&](nux::Area *area, bool recieving, KeyNavDirection direction){
      if (recieving)
        query_selected.emit(dynamic_cast<HudButton*>(area)->GetQuery());
    });

    // You should never decrement end().  We should fix this loop.
    button->is_rounded = (query == --(queries.end())) ? true : false;
    button->fake_focused = (query == (queries.begin())) ? true : false;

    button->SetMinimumWidth(content_width);
    found_items++;
  }
  if (found_items)
    selected_button_ = 1;

  QueueRelayout();
  QueueDraw();
}

void View::SetIcon(std::string icon_name, unsigned int tile_size, unsigned int size, unsigned int padding)
{
  if (!icon_)
    return;

  LOG_DEBUG(logger) << "Setting icon to " << icon_name;

  icon_->SetIcon(icon_name, size, tile_size);
  icon_->SetMinimumWidth(tile_size + padding);

  /* We need to compute this value manually, since the _content_layout height changes */
  int content_height = search_bar_->GetBaseHeight() + top_padding + bottom_padding;
  icon_->SetMinimumHeight(std::max(icon_->GetMinimumHeight(), content_height));

  QueueDraw();
}

void View::ShowEmbeddedIcon(bool show)
{
  LOG_DEBUG(logger) << "Hide icon called";
  if (show == show_embedded_icon_)
    return;

  show_embedded_icon_ = show;

  if (show_embedded_icon_)
  {
    layout_->AddView(icon_.GetPointer(), 0, nux::MINOR_POSITION_LEFT,
                     nux::MINOR_SIZE_FULL, 100.0f, nux::LayoutPosition::NUX_LAYOUT_BEGIN);
    AddChild(icon_.GetPointer());
  }
  else
  {
    layout_->RemoveChildObject(icon_.GetPointer());
    RemoveChild(icon_.GetPointer());
  }

  Relayout();
}

// Gives us the width and height of the contents that will give us the best "fit",
// which means that the icons/views will not have uneccessary padding, everything will
// look tight
nux::Geometry View::GetBestFitGeometry(nux::Geometry const& for_geo)
{
  //FIXME - remove magic values, replace with scalable text depending on DPI
  // requires smarter font settings really...
  int width = default_width;
  int height = default_height;

  if (show_embedded_icon_)
    width += icon_->GetGeometry().width;

  LOG_DEBUG (logger) << "best fit is, " << width << ", " << height;

  return nux::Geometry(0, 0, width, height);
}

void View::AboutToShow()
{
  renderer_.AboutToShow();
}

void View::AboutToHide()
{
  renderer_.AboutToHide();
}

void View::SetWindowGeometry(nux::Geometry const& absolute_geo, nux::Geometry const& geo)
{
  window_geometry_ = geo;
  window_geometry_.x = 0;
  window_geometry_.y = 0;
  absolute_window_geometry_ = absolute_geo;
}

void View::SetupViews()
{
  dash::Style& style = dash::Style::Instance();

  nux::VLayout* super_layout = new nux::VLayout(); 
  layout_ = new nux::HLayout();
  {
    // fill layout with icon
    icon_ = new Icon();
    {
      AddChild(icon_.GetPointer());
      layout_->AddView(icon_.GetPointer(), 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
    }

    // fill the content layout
    content_layout_ = new nux::VLayout();
    {
      // Set the layout paddings
      content_layout_->SetLeftAndRightPadding(left_padding, right_padding);
      content_layout_->SetTopAndBottomPadding(top_padding, bottom_padding);

      // add the search bar to the composite
      search_bar_ = new unity::SearchBar(true);
      search_bar_->SetMinimumHeight(style.GetSearchBarHeight());
      search_bar_->SetMaximumHeight(style.GetSearchBarHeight());
      search_bar_->search_hint = _("Type your command");
      search_bar_->live_search_reached.connect(sigc::mem_fun(this, &View::OnSearchChanged));
      AddChild(search_bar_.GetPointer());
      content_layout_->AddView(search_bar_.GetPointer(), 0, nux::MINOR_POSITION_LEFT);

      button_views_ = new nux::VLayout();
      button_views_->SetMaximumWidth(content_width);

      content_layout_->AddLayout(button_views_.GetPointer(), 1, nux::MINOR_POSITION_LEFT);
    }

    layout_->AddLayout(content_layout_.GetPointer(), 1, nux::MINOR_POSITION_TOP);
  }

  super_layout->AddLayout(layout_.GetPointer(), 0);
  SetLayout(super_layout);
}

void View::OnSearchChanged(std::string const& search_string)
{
  LOG_DEBUG(logger) << "got search change";
  search_changed.emit(search_string);

  std::list<HudButton::Ptr>::iterator it;
  for(it = buttons_.begin(); it != buttons_.end(); ++it)
  {
    (*it)->fake_focused = false;
  }
  
  if (!buttons_.empty())
    buttons_.back()->fake_focused = true;
}


void View::OnKeyDown (unsigned long event_type, unsigned long keysym,
                      unsigned long event_state, const TCHAR* character,
                      unsigned short key_repeat_count)
{
  if (keysym == NUX_VK_ESCAPE)
  {
    LOG_DEBUG(logger) << "got escape key";
    ubus.SendMessage(UBUS_HUD_CLOSE_REQUEST);
  }
}

void View::OnMouseButtonDown(int x, int y, unsigned long button, unsigned long key)
{
  if (!content_geo_.IsPointInside(x, y))
  {
    ubus.SendMessage(UBUS_HUD_CLOSE_REQUEST);
  }
}

void View::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  if (timeline_need_more_draw_)
  {
    ProcessGrowShrink();
  }

  nux::Geometry draw_content_geo(layout_->GetGeometry());
  draw_content_geo.height = current_height_;
  renderer_.DrawFull(gfx_context, draw_content_geo, absolute_window_geometry_, window_geometry_, true);
}

void View::DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  nux::Geometry draw_content_geo(layout_->GetGeometry());
  draw_content_geo.height = current_height_;

  renderer_.DrawInner(gfx_context, draw_content_geo, absolute_window_geometry_, window_geometry_);

  gfx_context.PushClippingRectangle(draw_content_geo);
  if (IsFullRedraw())
  {
    nux::GetPainter().PushBackgroundStack();
    GetLayout()->ProcessDraw(gfx_context, force_draw);
    nux::GetPainter().PopBackgroundStack();
  }
  else
  {
    GetLayout()->ProcessDraw(gfx_context, force_draw);
  }
  gfx_context.PopClippingRectangle();

  renderer_.DrawInnerCleanup(gfx_context, draw_content_geo, absolute_window_geometry_, window_geometry_);

  if (timeline_need_more_draw_ && !timeline_id_)
  {
    timeline_id_ = g_idle_add([] (gpointer data) -> gboolean
    {
      View *self = static_cast<View*>(data);
      self->QueueDraw();
      self->timeline_id_ = 0;
      return FALSE;
    }, this);
  }
}

// Keyboard navigation
bool View::AcceptKeyNavFocus()
{
  return false;
}

// Introspectable
std::string View::GetName() const
{
  return "HudView";
}

void View::AddProperties(GVariantBuilder* builder)
{
  unsigned num_buttons = buttons_.size();
  variant::BuilderWrapper(builder)
    .add(GetGeometry())
    .add("selected_button", selected_button_)
    .add("num_buttons", num_buttons);
}

bool View::InspectKeyEvent(unsigned int eventType,
                           unsigned int key_sym,
                           const char* character)
{
  if ((eventType == nux::NUX_KEYDOWN) && (key_sym == NUX_VK_ESCAPE))
  {
    if (search_bar_->search_string == "")
    {
      ubus.SendMessage(UBUS_HUD_CLOSE_REQUEST);
    }
    else
    {
      search_bar_->search_string = "";
    }
    return true;
  }
  return false;
}

void View::SearchFinished()
{
  search_bar_->SearchFinished();
}

void View::OnSearchbarActivated()
{
  // The "Enter" key has been received and the text entry has the key focus.
  // If one of the button has the fake_focus, we get it to emit the query_activated signal.
  if (!buttons_.empty())
  {
    std::list<HudButton::Ptr>::iterator it;
    for(it = buttons_.begin(); it != buttons_.end(); ++it)
    {
      if ((*it)->fake_focused)
      {
        query_activated.emit((*it)->GetQuery());
        return;
      }
    }
  }
  search_activated.emit(search_bar_->search_string);
}

nux::Area* View::FindKeyFocusArea(unsigned int event_type,
      unsigned long x11_key_code,
      unsigned long special_keys_state)
{
  nux::KeyNavDirection direction = nux::KEY_NAV_NONE;
  switch (x11_key_code)
  {
  case NUX_VK_UP:
    direction = nux::KEY_NAV_UP;
    break;
  case NUX_VK_DOWN:
    direction = nux::KEY_NAV_DOWN;
    break;
  case NUX_VK_LEFT:
    direction = nux::KEY_NAV_LEFT;
    break;
  case NUX_VK_RIGHT:
    direction = nux::KEY_NAV_RIGHT;
    break;
  case NUX_VK_LEFT_TAB:
    direction = nux::KEY_NAV_TAB_PREVIOUS;
    break;
  case NUX_VK_TAB:
    direction = nux::KEY_NAV_TAB_NEXT;
    break;
  case NUX_VK_ENTER:
  case NUX_KP_ENTER:
    // Not sure if Enter should be a navigation key
    direction = nux::KEY_NAV_ENTER;
    break;
  default:
    direction = nux::KEY_NAV_NONE;
    break;
  }


  if ((event_type == nux::NUX_KEYDOWN) && (x11_key_code == NUX_VK_ESCAPE))
  {
    // Escape key! This is how it works:
    //    -If there is text, clear it and give the focus to the text entry view.
    //    -If there is no text text, then close the hud.

    if (search_bar_->search_string == "")
    {
      ubus.SendMessage(UBUS_HUD_CLOSE_REQUEST);
    }
    else
    {
      search_bar_->search_string = "";
      return search_bar_->text_entry();
    }
    return NULL;
  }

  if (search_bar_->text_entry()->HasKeyFocus() && !search_bar_->im_preedit)
  {
    if (direction == nux::KEY_NAV_NONE ||
        direction == nux::KEY_NAV_UP ||
        direction == nux::KEY_NAV_DOWN ||
        direction == nux::KEY_NAV_LEFT ||
        direction == nux::KEY_NAV_RIGHT)
    {
      // We have received a key character or a keyboard arrow Up or Down (navigation keys).
      // Because we have called "SetLoseKeyFocusOnKeyNavDirectionUp(false);" and "SetLoseKeyFocusOnKeyNavDirectionDown(false);"
      // on the text entry, the text entry will not loose the keyboard focus.
      // All that we need to do here is set the fake_focused value on the HudButton.

      if (!buttons_.empty())
      {
        if (event_type == nux::NUX_KEYDOWN && direction == nux::KEY_NAV_UP)
        {
          std::list<HudButton::Ptr>::iterator it;
          for(it = buttons_.begin(); it != buttons_.end(); ++it)
          {
            if ((*it)->fake_focused)
            {
              std::list<HudButton::Ptr>::iterator next = it;
              ++next;
              if (next != buttons_.end())
              {
                // The button with the current fake_focus looses it.
                (*it)->fake_focused = false;
                // The next button gets the fake_focus
                (*next)->fake_focused = true;
                query_selected.emit((*next)->GetQuery());
                --selected_button_;
              }
              break;
            }
          }
        }

        if (event_type == nux::NUX_KEYDOWN && direction == nux::KEY_NAV_DOWN)
        {
          std::list<HudButton::Ptr>::reverse_iterator rit;
          for(rit = buttons_.rbegin(); rit != buttons_.rend(); ++rit)
          {
            if ((*rit)->fake_focused)
            {
              std::list<HudButton::Ptr>::reverse_iterator next = rit;
              ++next;
              if(next != buttons_.rend())
              {
                // The button with the current fake_focus looses it.
                (*rit)->fake_focused = false;
                // The next button bellow gets the fake_focus.
                (*next)->fake_focused = true;
                query_selected.emit((*next)->GetQuery());
                ++selected_button_;
              }
              break;
            }
          }
        }
      }
      return search_bar_->text_entry();
    }

    if (event_type == nux::NUX_KEYDOWN && direction == nux::KEY_NAV_ENTER)
    {
      // We still choose the text_entry as the receiver of the key focus.
      return search_bar_->text_entry();
    }
  }
  else if (direction == nux::KEY_NAV_NONE || search_bar_->im_preedit)
  {
    return search_bar_->text_entry();
  }
  else if (next_object_to_key_focus_area_)
  {
    return next_object_to_key_focus_area_->FindKeyFocusArea(event_type, x11_key_code, special_keys_state);
  }
  return search_bar_->text_entry();
}

}
}
