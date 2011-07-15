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

#include "SwitcherController.h"

namespace unity {
namespace switcher {

SwitcherController::SwitcherController()
  :  visible_(false)
{
  
}

void SwitcherController::Show (SwitcherController::ShowMode show, SwitcherController::SortMode sort, bool reverse, std::vector<AbstractLauncherIcon*> results)
{
  if (sort == FOCUS_ORDER)
    std::sort (results.begin (), results.end (), CompareSwitcherItemsPriority);
    
  model_ = SwitcherModel::Ptr (new SwitcherModel (results));
  SelectFirstItem ();
  
  visible_ = true;

  ConstructView ();
}

void SwitcherController::ConstructView ()
{
  nux::HLayout *layout;

  layout = new nux::HLayout();

  view_ = new SwitcherView ();
  view_->SetModel (model_);

  layout->AddView (view_, 1);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);

  view_window_ = new nux::BaseWindow ("Switcher");
  view_window_->SinkReference ();
  view_window_->SetLayout (layout);
  view_window_->SetBackgroundColor (nux::Color(0x00000000));
  view_window_->ShowWindow(true);

  view_window_->SetGeometry (workarea_);

}

void SwitcherController::SetWorkspace (nux::Geometry geo)
{
  workarea_ = geo;  
}

void SwitcherController::Hide ()
{
  if (!visible_)
    return;

  model_->Selection ()->Activate ();

  model_.reset ();
  visible_ = false;

  view_window_->ShowWindow(false);
  view_window_->UnReference ();
}

bool SwitcherController::Visible ()
{
  return visible_;
}

void SwitcherController::MoveNext ()
{
  if (!model_)
    return;
  model_->Next ();
}

void SwitcherController::MovePrev ()
{
  if (!model_)
    return;
  model_->Prev ();
}

void SwitcherController::DetailCurrent ()
{

}

bool SwitcherController::CompareSwitcherItemsPriority (AbstractLauncherIcon *first, AbstractLauncherIcon *second)
{
  return first->SwitcherPriority () > second->SwitcherPriority ();
}

void SwitcherController::SelectFirstItem ()
{
  if (!model_)
    return;
  
  // Hack
  model_->Select (1);
}

}
}
