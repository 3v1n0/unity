// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2016 Canonical Ltd
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
* Authored by: Marco Trevisan <marco@ubuntu.com>
*/

#include "SpreadWidgets.h"

#include "DashStyle.h"
#include "PanelStyle.h"
#include "RawPixel.h"
#include "SearchBar.h"
#include "UnitySettings.h"
#include "UScreen.h"

namespace unity
{
namespace spread
{
namespace
{
const RawPixel LEFT_CORNER_OFFSET = 10_em;
}

class Decorations : public nux::BaseWindow
{
public:
  nux::Property<int> monitor;

  Decorations(int monitor_)
    : monitor(monitor_)
  {
    monitor.changed.connect(sigc::mem_fun(this, &Decorations::Update));
    SetBackgroundColor(nux::color::Transparent);

    Update(monitor);
    PushToFront();
    ShowWindow(true);
  }

  ~Decorations()
  {
    ShowWindow(false);
    object_destroyed.emit(this);
  }

  void Update(int monitor)
  {
    auto& settings = Settings::Instance();
    auto abs_geo = UScreen::GetDefault()->GetMonitorGeometry(monitor);
    int panel_height = panel::Style::Instance().PanelHeight(monitor);
    int launcher_size = settings.LauncherSize(monitor);
    scale_ = settings.em(monitor)->DPIScale();

    if (settings.launcher_position() == LauncherPosition::LEFT)
    {
      abs_geo.x += launcher_size;
      abs_geo.width -= launcher_size;
    }
    else
    {
      abs_geo.height -= launcher_size;
    }

    abs_geo.y += panel_height;
    abs_geo.height -= panel_height;
    SetGeometry(abs_geo);

    auto& dash_style = dash::Style::Instance();
    corner_tex_ = dash_style.GetDashTopLeftTile(scale_);
    left_edge_tex_ = dash_style.GetDashLeftTile(scale_);
    horizontal_tex_ = dash_style.GetDashTopTile(scale_);
  }

  void DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw) override
  {
    auto const& geo = GetGeometry();
    auto launcher_position = Settings::Instance().launcher_position();
    int x_offset = 0;

    nux::TexCoordXForm texxform;

    if (launcher_position == LauncherPosition::LEFT)
    {
      // Corner
      texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
      texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

      gfx_context.QRP_1Tex(0,
                           0,
                           corner_tex_->GetWidth(),
                           corner_tex_->GetHeight(),
                           corner_tex_->GetDeviceTexture(),
                           texxform,
                           nux::color::White);

      x_offset = corner_tex_->GetWidth();
    }

    // Top Edge
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

    gfx_context.QRP_1Tex(x_offset,
                         -LEFT_CORNER_OFFSET.CP(scale_),
                         geo.width - x_offset,
                         horizontal_tex_->GetHeight(),
                         horizontal_tex_->GetDeviceTexture(),
                         texxform,
                         nux::color::White);

    if (launcher_position == LauncherPosition::LEFT)
    {
      // Left edge
      texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
      texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

      gfx_context.QRP_1Tex(-LEFT_CORNER_OFFSET.CP(scale_),
                           corner_tex_->GetHeight(),
                           left_edge_tex_->GetWidth(),
                           geo.height,
                           left_edge_tex_->GetDeviceTexture(),
                           texxform,
                           nux::color::White);
    }
    else if (launcher_position == LauncherPosition::BOTTOM)
    {
      texxform.flip_v_coord = true;

      // Bottom Edge
      texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
      texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

      gfx_context.QRP_1Tex(0,
                           geo.height - horizontal_tex_->GetHeight() + LEFT_CORNER_OFFSET.CP(scale_),
                           geo.width,
                           horizontal_tex_->GetHeight(),
                           horizontal_tex_->GetDeviceTexture(),
                           texxform,
                           nux::color::White);
    }
  }

  double scale_;
  dash::BaseTexturePtr corner_tex_;
  dash::BaseTexturePtr left_edge_tex_;
  dash::BaseTexturePtr horizontal_tex_;
};


Widgets::Widgets()
  : filter_(std::make_shared<Filter>())
{
  auto const& uscreen = UScreen::GetDefault();
  auto num_monitors = uscreen->GetPluggedMonitorsNumber();

  for (auto i = 0; i < num_monitors; ++i)
    decos_.push_back(std::make_shared<Decorations>(i));

  uscreen->changed.connect(sigc::track_obj([this] (int, std::vector<nux::Geometry> const& monitors) {
    auto num_monitors = monitors.size();
    decos_.reserve(num_monitors);

    while (decos_.size() < num_monitors)
      decos_.emplace_back(std::make_shared<Decorations>(decos_.size()-1));

    decos_.resize(num_monitors);
    for (auto i = 0u; i < num_monitors; ++i)
    {
      decos_[i]->monitor = i;
      decos_[i]->monitor.changed.emit(i);
    }
  }, *this));
}

Filter::Ptr Widgets::GetFilter() const
{
  return filter_;
}

} // namespace spread
} // namespace unity
