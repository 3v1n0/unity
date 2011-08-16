/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#include <math.h>

#include <Nux/Nux.h>
#include <Nux/HLayout.h>

#include "UBusMessages.h"
#include "ubus-server.h"

#include "SwitcherController.h"

namespace unity
{
namespace switcher
{

SwitcherController::SwitcherController()
  :  view_(0)
  ,  view_window_(0)
  ,  visible_(false)
  ,  show_timer_(0)
{
  timeout_length = 150;

  blur = BLUR_ACTIVE;

  UBusServer *ubus = ubus_server_get_default();
  ubus_server_register_interest(ubus, UBUS_BACKGROUND_COLOR_CHANGED,
                                (UBusCallback)&SwitcherController::OnBackgroundUpdate,
                                this);
}

SwitcherController::~SwitcherController()
{
}

void SwitcherController::OnBackgroundUpdate (GVariant *data, SwitcherController *self)
{
  gdouble red, green, blue, alpha;
  g_variant_get(data, "(dddd)", &red, &green, &blue, &alpha);
  self->bg_color_ = nux::Color (red, green, blue, alpha);

  if (self->view_)
    self->view_->background_color = self->bg_color_;
}

void SwitcherController::Show(SwitcherController::ShowMode show, SwitcherController::SortMode sort, bool reverse, std::vector<AbstractLauncherIcon*> results)
{
  if (sort == FOCUS_ORDER)
    std::sort(results.begin(), results.end(), CompareSwitcherItemsPriority);

  model_ = SwitcherModel::Ptr(new SwitcherModel(results));
  model_->detail_inline = false;
  SelectFirstItem();

  visible_ = true;

  if (timeout_length > 0)
  {
    show_timer_ = g_timeout_add(timeout_length, &SwitcherController::OnShowTimer, this);
  }
  else
  {
    ConstructView ();
  }
}

gboolean SwitcherController::OnShowTimer(gpointer data)
{
  SwitcherController* self = static_cast<SwitcherController*>(data);

  if (self->visible_)
    self->ConstructView();

  self->show_timer_ = 0;
  return FALSE;
}

void SwitcherController::ConstructView()
{
  nux::HLayout* layout;

  layout = new nux::HLayout();

  view_ = new SwitcherView();
  view_->SetModel(model_);
  view_->background_color = bg_color_;
  view_->blur = blur();

  layout->AddView(view_, 1);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);

  view_window_ = new nux::BaseWindow("Switcher");
  view_window_->SinkReference();
  view_window_->SetLayout(layout);
  view_window_->SetBackgroundColor(nux::Color(0x00000000));
  view_window_->SetGeometry(workarea_);

  view_window_->ShowWindow(true);
}

void SwitcherController::SetWorkspace(nux::Geometry geo)
{
  workarea_ = geo;
}

void SwitcherController::Hide()
{
  if (!visible_)
    return;

  AbstractLauncherIcon* selection = model_->Selection();
  if (selection)
  {
    if (model_->detail_selection)
    {
      selection->Activate(ActionArg(ActionArg::SWITCHER, 0, model_->DetailSelectionWindow ()));
    }
    else
    {
      if (selection->GetQuirk (AbstractLauncherIcon::QUIRK_ACTIVE))
      {
        selection->Activate(ActionArg (ActionArg::SWITCHER, 0, model_->DetailXids()[0]));
      } 
      else
      {
        selection->Activate(ActionArg(ActionArg::SWITCHER, 0));
      }     
    }
  }

  model_.reset();
  visible_ = false;

  if (view_window_)
  {
    view_window_->ShowWindow(false);
    view_window_->UnReference();
    view_window_ = 0;
  }

  if (show_timer_)
    g_source_remove(show_timer_);
  show_timer_ = 0;

  view_ = 0;
}

bool SwitcherController::Visible()
{
  return visible_;
}

void SwitcherController::MoveNext()
{
  if (!model_)
    return;
  model_->Next();
}

void SwitcherController::MovePrev()
{
  if (!model_)
    return;
  model_->Prev();
}

SwitcherView * SwitcherController::GetView()
{
  return view_;
}

void SwitcherController::DetailCurrent()
{
  if (model_->detail_selection)
  {
    model_->NextDetail ();
  }
  else if (model_->Selection ()->RelatedWindows () > 0)
  {
    model_->detail_selection = true;
  }
}

LayoutWindowList SwitcherController::ExternalRenderTargets ()
{
  if (!view_)
  {
    LayoutWindowList result;
    return result;
  }
  return view_->ExternalTargets ();
}

bool SwitcherController::CompareSwitcherItemsPriority(AbstractLauncherIcon* first, AbstractLauncherIcon* second)
{
  if (first->Type() == second->Type())
    return first->SwitcherPriority() > second->SwitcherPriority();
  return first->Type() < second->Type();
}

void SwitcherController::SelectFirstItem()
{
  if (!model_)
    return;

  AbstractLauncherIcon *first  = model_->at (1);
  AbstractLauncherIcon *second = model_->at (2);

  if (!first)
  {
    model_->Select (0);
    return;
  }
  else if (!second)
  {
    model_->Select (1);
    return;
  }

  unsigned int first_highest = 0;
  unsigned int first_second = 0; // first icons second highest active
  unsigned int second_first = 0; // second icons first highest active

  for (guint32 xid : first->RelatedXids ())
  {
    unsigned int num = WindowManager::Default ()->GetWindowActiveNumber (xid);

    if (num > first_highest)
    {
      first_second = first_highest;
      first_highest = num;
    }
    else if (num > first_second)
    {
      first_second = num;
    }
  }

  for (guint32 xid : second->RelatedXids ())
  {
    second_first = MAX (WindowManager::Default ()->GetWindowActiveNumber (xid), second_first);
  }

  if (first_second > second_first)
    model_->Select (first);
  else
    model_->Select (second);
}

}
}
