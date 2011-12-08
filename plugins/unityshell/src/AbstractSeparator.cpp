/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Jay Taoko <jaytaoko@inalogic.com>
 *
 */

#include "AbstractSeparator.h"

#include "Nux/Nux.h"

namespace unity
{

AbstractSeparator::AbstractSeparator(NUX_FILE_LINE_DECL)
  : nux::View(NUX_FILE_LINE_PARAM)
  , color_(0xFFFFFFFF)
  , alpha0_(0.0f)
  , alpha1_(0.592f)
  , border_size_(10)
{
}

// Maybe it's better to use default arguments?
AbstractSeparator::AbstractSeparator(nux::Color const& color, float Alpha0, 
                                     float Alpha1, int Border, NUX_FILE_LINE_DECL)
  : nux::View(NUX_FILE_LINE_PARAM)
  , color_(color)
  , alpha0_(Alpha0)
  , alpha1_(Alpha1)
  , border_size_(Border)
{
}

AbstractSeparator::~AbstractSeparator()
{

}

void AbstractSeparator::SetColor(nux::Color const &color)
{
  color_ = color;
}

void AbstractSeparator::SetAlpha(float Alpha0, float Alpha1)
{
  alpha0_ = Alpha0;
  alpha1_ = Alpha1;
}

void AbstractSeparator::SetBorderSize(int Border)
{
  border_size_ = Border;
}

} // namespace unity
