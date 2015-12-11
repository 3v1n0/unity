// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012-2013 Canonical Ltd
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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef UNITYSHELL_FILEMANAGER_H
#define UNITYSHELL_FILEMANAGER_H

#include <memory>
#include <vector>
#include <string>
#include <vector>
#include <set>
#include <sigc++/sigc++.h>
#include "ApplicationManager.h"


namespace unity
{

class FileManager : public sigc::trackable
{
public:
  typedef std::shared_ptr<FileManager> Ptr;

  FileManager() = default;
  virtual ~FileManager() = default;

  virtual void Open(std::string const& uri, uint64_t timestamp = 0) = 0;
  virtual void OpenTrash(uint64_t timestamp) = 0;
  virtual void CopyFiles(std::set<std::string> const& uris, std::string const& dest, uint64_t timestamp = 0) = 0;
  virtual bool TrashFile(std::string const& uri) = 0;
  virtual void EmptyTrash(uint64_t timestamp = 0) = 0;
  virtual WindowList WindowsForLocation(std::string const& location) const = 0;
  virtual std::string LocationForWindow(ApplicationWindowPtr const&) const = 0;

  sigc::signal<void> locations_changed;

private:
  FileManager(FileManager const&) = delete;
  FileManager& operator=(FileManager const&) = delete;
};

} // namespace unity

#endif
