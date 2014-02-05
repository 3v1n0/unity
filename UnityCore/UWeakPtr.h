// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2014 Canonical Ltd
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
* Authored by: Marco Trevisan <marco.trevisan@canonical.com>
*/

#ifndef UNITY_WEAK_PTR
#define UNITY_WEAK_PTR

#include <memory>

namespace unity
{

template<typename T>
struct uweak_ptr : std::weak_ptr<T>
{
  uweak_ptr() {}
  uweak_ptr(std::shared_ptr<T> const& p) : std::weak_ptr<T>(p) {}
  uweak_ptr(uweak_ptr<T> const& p) : std::weak_ptr<T>(p) {}
  T* get() const { return this->lock().get(); }
  operator std::shared_ptr<T>() const { return this->lock(); }
  operator bool() const { return !this->expired(); }
  T* operator->() const { return this->lock().get(); }
  T const& operator*() const { return *(this->lock().get()); }
};

} // unity namespace

namespace std
{
template<class T>
void swap(unity::uweak_ptr<T> &lhs, unity::uweak_ptr<T> &rhs)
{
  lhs.swap(rhs);
}
}

#endif // UNITY_WEAK_PTR
