// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2013 Canonical Ltd.
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *              Manuel de la Pena <manuel.delapena@canonical.com>
 *
 */

#include <list>
#include <Nux/Nux.h>
#include <Nux/InputArea.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>

namespace unity
{
namespace dash
{
namespace previews
{

class TabIterator
{
public:
  TabIterator() {}

  void Prepend(nux::InputArea* area);
  void Append(nux::InputArea* area);
  void Insert(nux::InputArea* area, unsigned index);
  void InsertBefore(nux::InputArea* area, nux::InputArea* after);
  void InsertAfter(nux::InputArea* area, nux::InputArea* before);
  void Remove(nux::InputArea* area);

  std::list<nux::InputArea*> const& GetTabAreas() const;
  nux::InputArea* DefaultFocus() const;
  nux::InputArea* FindKeyFocusArea(unsigned int key_symbol,
                                   unsigned long x11_key_code,
                                   unsigned long special_keys_state);
  nux::Area* KeyNavIteration(nux::KeyNavDirection direction);

private:
  friend class TestTabIterator;
  std::list<nux::InputArea*> areas_;
};

class TabIteratorHLayout  : public nux::HLayout
{
public:
  TabIteratorHLayout(TabIterator* iterator)
  :tab_iterator_(iterator)
  {
  }

  nux::Area* KeyNavIteration(nux::KeyNavDirection direction)
  {
    return tab_iterator_->KeyNavIteration(direction);
  }

private:
  TabIterator* tab_iterator_;
};

class TabIteratorVLayout  : public nux::VLayout
{
public:
  TabIteratorVLayout(TabIterator* iterator)
  :tab_iterator_(iterator)
  {
  }

  nux::Area* KeyNavIteration(nux::KeyNavDirection direction)
  {
    return tab_iterator_->KeyNavIteration(direction);
  }

private:
  TabIterator* tab_iterator_;
};

} // previews

} // dash

} // unity
