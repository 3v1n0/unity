// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011-2012 Canonical Ltd
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
 *              Michal Hruby <michal.hruby@canonical.com>
 */

#ifndef UNITY_MUSIC_PREVIEW_H
#define UNITY_MUSIC_PREVIEW_H

#include <memory>

#include <sigc++/trackable.h>

#include "Preview.h"
#include "Tracks.h"

namespace unity
{
namespace dash
{

class MusicPreview : public Preview
{
public:
  typedef std::shared_ptr<MusicPreview> Ptr;

  MusicPreview(unity::glib::Object<GObject> const& proto_obj);
  ~MusicPreview();

  Tracks::Ptr GetTracksModel() const;

private:
  class Impl;
  std::unique_ptr<Impl> pimpl;
};

}
}

#endif
