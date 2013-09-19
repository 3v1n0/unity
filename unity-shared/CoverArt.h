// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Andrea Cimitan <andrea.cimitan@canonical.com>
 *              Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#ifndef COVERART_H
#define COVERART_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <UnityCore/ApplicationPreview.h>
#include <UnityCore/GLibSource.h>
#include <NuxCore/ObjectPtr.h>
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/Introspectable.h"
#include "ThumbnailGenerator.h"

namespace unity
{
namespace dash
{
namespace previews
{

class CoverArt : public nux::View, public unity::debug::Introspectable
{
public:
  typedef nux::ObjectPtr<CoverArt> Ptr;
  NUX_DECLARE_OBJECT_TYPE(CoverArt, nux::View);

  CoverArt();
  virtual ~CoverArt();

  // Use for setting an image which is already an image (path to iamge, gicon).
  void SetImage(std::string const& image_hint);
  // Use for generating an image for a uri which is not necessarily an image.
  void GenerateImage(std::string const& uri);
  
  void SetNoImageAvailable();

  void SetFont(std::string const& font);

protected:
  virtual void Draw(nux::GraphicsEngine& gfx_engine, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& gfx_engine, bool force_draw);
  
  virtual bool AcceptKeyNavFocus() { return false; }

  void SetupViews();

  void OnThumbnailGenerated(std::string const& uri);
  void OnThumbnailError(std::string const& error_hint);
  bool OnFrameTimeout();

  void IconLoaded(std::string const& texid, int max_width, int max_height, glib::Object<GdkPixbuf> const& pixbuf);
  void TextureLoaded(std::string const& texid, int max_width, int max_height, glib::Object<GdkPixbuf> const& pixbuf);

  void StartWaiting();
  void StopWaiting();

  virtual std::string GetName() const;
  virtual void AddProperties(debug::IntrospectionData&);

private:
  nux::ObjectPtr<nux::BaseTexture> texture_screenshot_;
  StaticCairoText* overlay_text_;

  std::string image_hint_;
  unsigned int thumb_handle_;
  int slot_handle_;
  bool stretch_image_;
  ThumbnailNotifier::Ptr notifier_;
  
  // Spinner
  bool waiting_;
  nux::BaseTexture* spin_;
  glib::Source::UniquePtr spinner_timeout_;
  glib::Source::UniquePtr frame_timeout_;
  nux::Matrix4 rotate_matrix_;
  float rotation_;

  typedef std::unique_ptr<nux::AbstractPaintLayer> LayerPtr;
  LayerPtr bg_layer_;
};

}
}
}

#endif // APPLICATIONSCREENSHOT_H
