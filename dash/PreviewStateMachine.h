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

#ifndef UNITY_PREVIEW_STATE_MACHINE_H_
#define UNITY_PREVIEW_STATE_MACHINE_H_
#include "previews/Preview.h"
#include "previews/PreviewContainer.h"
#include <unordered_map>

namespace unity
{
namespace dash
{

typedef enum 
{
  START,
  CONTENT_AREA,
  FILTER_BAR,
  SCOPE_BAR,
  SEARCH_BAR,
  END
} SplitPosition;

class PreviewStateMachine
{
public:
  PreviewStateMachine();
  ~PreviewStateMachine();

  void ActivatePreview(Preview::Ptr preview); // potentially async call.
  void Reset(); // resets the state but does not close the preview
  void ClosePreview();

  void SetSplitPosition(SplitPosition position, int coord);
  int  GetSplitPosition(SplitPosition position);

  nux::Property<bool> preview_active;
  nux::Property<int> left_results;
  nux::Property<int> right_results;

  sigc::signal<void, Preview::Ptr> PreviewActivated;

private:
  void CheckPreviewRequirementsFulfilled();

  // all stored co-ordinates are absolute geometry
  // to make dealing with the views inside the scrollview easier to understand
  std::unordered_map<int, int> split_positions_;

  Preview::Ptr stored_preview_;
  bool requires_activation_;
  bool requires_new_position_;
};

}
}

#endif
