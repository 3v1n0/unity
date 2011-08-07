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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */



#ifndef RESULTVIEWGRID_H
#define RESULTVIEWGRID_H

#include <UnityCore/Categories.h>

#include "ResultView.h"

namespace unity
{
namespace dash
{

// will order its elements in to a grid that expands horizontally before vertically
class ResultViewGrid : public ResultView
{
public:
    ResultViewGrid(NUX_FILE_LINE_DECL);
    ~ResultViewGrid();

    void SetModelRenderer(ResultRenderer* renderer);
    void AddResult(Result& result);
    void RemoveResult(Result& result);

  void SetPreview (PreviewBase *preview, Result& related_result);

    nux::Property<int> horizontal_spacing;
    nux::Property<int> vertical_spacing;
    nux::Property<int> padding;

protected:
    void MouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
    void MouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags);

    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);;
    virtual long int ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

private:
  void SizeReallocate ();
  uint GetIndexAtPosition (int x, int y);

  int mouse_over_index_;
  int active_index_;
  int selected_index_;
};

}
}
#endif // RESULTVIEWGRID_H
