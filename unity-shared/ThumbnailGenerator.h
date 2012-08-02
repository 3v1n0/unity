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
 **
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#ifndef UNITYSHARED_THUMBNAILGENERATOR_H
#define UNITYSHARED_THUMBNAILGENERATOR_H

#include <Nux/Nux.h>

namespace unity
{

class Thumbnailer
{
public:
  typedef std::shared_ptr<Thumbnailer> Ptr;

  virtual std::string GetName() const = 0;

  virtual bool Run(int size, std::string const& input_file, std::string& output_file, std::string& error_hint) = 0;
};

class ThumbnailGeneratorImpl;
class ThumbnailGenerator
{
public:
  ThumbnailGenerator();
  virtual ~ThumbnailGenerator();

  static ThumbnailGenerator& Instance();

  static void RegisterThumbnailer(std::list<std::string> mime_types, Thumbnailer::Ptr thumbnailer);

  unsigned int GetThumbnail(std::string const& uri, int size);

  sigc::signal<void, unsigned int, std::string> ready;
  sigc::signal<void, unsigned int, std::string> error;

protected:
  std::unique_ptr<ThumbnailGeneratorImpl> pimpl;

  friend class Thumbnail;
};

} // namespace unity

#endif // UNITYSHARED_THUMBNAILGENERATOR_H

