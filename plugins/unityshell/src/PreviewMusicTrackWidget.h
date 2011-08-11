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



#ifndef PREVIEWMUSICTRACKWIDGET_H
#define PREVIEWMUSICTRACKWIDGET_H

#include <string>
#include <Nux/Nux.h>
#include "Nux/Button.h"

namespace unity {

  class PreviewMusicTrackWidget : public nux::View {
  public:
    PreviewMusicTrackWidget (std::string number, std::string name,
                             std::string time, std::string play_uri,
                             std::string pause_uri, NUX_FILE_LINE_PROTO);
    virtual ~PreviewMusicTrackWidget();

    nux::Property<bool> track_is_active;
    nux::Property<bool> is_paused;

    sigc::signal<void, std::string> UriActivated;

  protected:
    virtual long int ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo);
    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);



  private:
    void InitTheme ();

    std::string number_;
    std::string name_;
    std::string time_;
    std::string play_uri_;
    std::string pause_uri_;

    nux::Button *play_button_;

  };

}
#endif // PREVIEWMUSICTRACKWIDGET_H
