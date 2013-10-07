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

#ifndef UNITYSHELL_GNOME_FILEMANAGER_H
#define UNITYSHELL_GNOME_FILEMANAGER_H

#include "FileManager.h"

namespace unity
{

class GnomeFileManager : public FileManager
{
public:
  static FileManager::Ptr Get();
  ~GnomeFileManager();

  void Open(std::string const& uri, uint64_t timestamp);
  void OpenActiveChild(std::string const& uri, uint64_t timestamp);
  void OpenTrash(uint64_t timestamp);

  void CopyFiles(std::set<std::string> const& uris, std::string const& dest, uint64_t timestamp);
  bool TrashFile(std::string const& uri);
  void EmptyTrash(uint64_t timestamp);

  std::vector<std::string> OpenedLocations() const;
  bool IsPrefixOpened(std::string const& uri) const;
  bool IsTrashOpened() const;
  bool IsDeviceOpened() const;

private:
  GnomeFileManager();
  void Activate(uint64_t timestamp);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}

#endif
