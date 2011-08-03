// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "MultiRangeFilter.h"

#include <NuxCore/Logger.h>

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.multirangefilter");
}

MultiRangeFilter::MultiRangeFilter(DeeModel* model, DeeModelIter* iter)
  : Filter(model, iter)
  , left_pos_(-1)
  , right_pos_(-1)
{
  buttons.SetGetterFunction(sigc::mem_fun(this, &MultiRangeFilter::get_buttons));
  Refresh();
}

void MultiRangeFilter::Clear()
{
  left_pos_ = -1;
  right_pos_ = -1;

  for(auto button: buttons_)
  {
    button->active = false;
  }
  UpdateState(false);

  buttons.EmitChanged(buttons_);
}

void MultiRangeFilter::Toggle(std::string const& id)
{
  int position = PositionOfId(id);

  if (left_pos_ == -1 && right_pos_ == -1)
  {
    left_pos_ = position;
    right_pos_ = position;
  }
  else if (left_pos_ > position)
  {
    left_pos_ = position;
  }
  else if (right_pos_ < position)
  {
    right_pos_ = position;
  }

  int i = 0;
  for(auto button: buttons_)
  {
    if (i < left_pos_)
      button->active = false;
    else if (i <= right_pos_)
      button->active = true;
    else
      button->active = false;

    i++;
  }
  UpdateState(true);

  buttons.EmitChanged(buttons_);
}

void MultiRangeFilter::Update(Filter::Hints& hints)
{
  GVariant* buttons_variant = hints["buttons"];
  GVariantIter* buttons_iter;

  g_variant_get(buttons_variant, "(sssb)", &buttons_iter);

  char *id = NULL;
  char *name = NULL;
  char *icon_hint = NULL;
  gboolean active = false;

  for (auto button: buttons_)
    button_removed.emit(button);
  buttons_.clear();

  while (g_variant_iter_loop(buttons_iter, "sssb", &id, &name, &icon_hint, &active))
  {
    FilterButton::Ptr button(new FilterButton(id, name, icon_hint, active));
    buttons_.push_back(button);
    button_added.emit(button);
  }

  buttons.EmitChanged(buttons_);
}

MultiRangeFilter::Buttons const& MultiRangeFilter::get_buttons() const
{
  return buttons_;
}

void MultiRangeFilter::UpdateState(bool raw_filtering)
{
  if (!IsValid())
    return;

  GVariantBuilder buttons;
  g_variant_builder_init(&buttons, G_VARIANT_TYPE("a(sssb)"));

  for(auto button: buttons_)
  {
    std::string id = button->id;
    std::string name = button->name;
    std::string icon_hint = button->icon_hint;
    bool active = button->active;

    g_variant_builder_add(&buttons, "sssb",
                          id.c_str(), name.c_str(),
                          icon_hint.c_str(), active ? TRUE : FALSE);
  }
  
  GVariantBuilder hints;
  g_variant_builder_init(&hints, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&hints, "{sv}", "buttons", g_variant_builder_end(&buttons));

  dee_model_set_value(model_,iter_,
                      FilterColumn::RENDERER_STATE,
                      g_variant_builder_end(&hints));
  dee_model_set_value(model_, iter_,
                      FilterColumn::FILTERING,
                      g_variant_new("b", raw_filtering ? TRUE : FALSE));
}

int MultiRangeFilter::PositionOfId(std::string const& id)
{
  int i = 0;
  for (auto button: buttons_)
  {
    if (button->id == id)
      return i;
    i++;
  }
  return -1;
}


}
}
