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

#include <gmock/gmock.h>
#include "FileManager.h"

namespace unity
{

struct MockFileManager : FileManager
{
  typedef std::shared_ptr<MockFileManager> Ptr;
  typedef testing::NiceMock<MockFileManager> Nice;

  MOCK_METHOD2(Open, void(std::string const& uri, uint64_t time));
  MOCK_METHOD1(OpenTrash, void(uint64_t time));
  MOCK_METHOD1(TrashFile, bool(std::string const& uri));
  MOCK_METHOD1(EmptyTrash, void(uint64_t time));
  MOCK_METHOD3(CopyFiles, void(std::set<std::string> const& files, std::string const& dest, uint64_t time));
  MOCK_CONST_METHOD1(WindowsForLocation, WindowList(std::string const&));
  MOCK_CONST_METHOD1(LocationForWindow, std::string(ApplicationWindowPtr const&));

  MockFileManager()
  {
    using namespace testing;
    ON_CALL(*this, WindowsForLocation(_)).WillByDefault(Return(WindowList()));
  }
};

}

#endif
