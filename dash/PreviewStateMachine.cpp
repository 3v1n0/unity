/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Gord Allott <gord.allott@canonical.com>>
 */

#include "PreviewStateMachine.h"

namespace unity
{
namespace dash
{

PreviewStateMachine::PreviewStateMachine()
  : preview_active(false)
  , stored_preview_(nullptr)
{
  for (int pos = SplitPosition::START; pos != SplitPosition::END; pos++)
  {
    split_positions_[static_cast<SplitPosition>(pos)] = -1;
  }
}

PreviewStateMachine::~PreviewStateMachine()
{
}

void PreviewStateMachine::ActivatePreview(Preview::Ptr preview)
{
  stored_preview_ = preview;
  CheckPreviewRequirementsFulfilled();
}

void PreviewStateMachine::ClosePreview()
{
  stored_preview_ = nullptr;
  SetSplitPosition(SplitPosition::CONTENT_AREA, -1); 
}

void PreviewStateMachine::SetSplitPosition(SplitPosition position, int coord)
{
  split_positions_[position] = coord;
  CheckPreviewRequirementsFulfilled();
}

int PreviewStateMachine::GetSplitPosition(SplitPosition position)
{
  return split_positions_[position];
}

void PreviewStateMachine::CheckPreviewRequirementsFulfilled()
{
  if (preview_active())
    return;

  if (stored_preview_ == nullptr)
    return;

  if (GetSplitPosition(CONTENT_AREA) < 0) return;
  if (GetSplitPosition(FILTER_BAR) < 0) return;
  if (GetSplitPosition(LENS_BAR) < 0) return;
  if (GetSplitPosition(SEARCH_BAR) < 0) return;

  preview_active = true;
  PreviewActivated(stored_preview_);
}

}
}
