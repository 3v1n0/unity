/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#ifndef TEST_MOCK_FILEMANAGER_H
#define TEST_MOCK_FILEMANAGER_H

#include "FileManager.h"

namespace unity
{

struct MockFileManager : FileManager
{
  typedef std::shared_ptr<MockFileManager> Ptr;

  MOCK_METHOD2(Open, void(std::string const& uri, uint64_t time));
  MOCK_METHOD2(OpenActiveChild, void(std::string const& uri, uint64_t time));
  MOCK_METHOD1(OpenTrash, void(uint64_t time));
  MOCK_METHOD1(TrashFile, bool(std::string const& uri));
  MOCK_METHOD1(EmptyTrash, void(uint64_t time));
  MOCK_CONST_METHOD0(OpenedLocations, std::vector<std::string>());
  MOCK_CONST_METHOD1(IsPrefixOpened, bool(std::string const& uri));
  MOCK_CONST_METHOD0(IsTrashOpened, bool());
  MOCK_CONST_METHOD0(IsDeviceOpened, bool());
};

}

#endif
