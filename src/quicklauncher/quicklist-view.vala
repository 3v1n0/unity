/* -*- Mode: vala; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/*
 * Copyright (C) 2009 Canonical Ltd
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
 * Authored by Gordon Allott <gord.allott@canonical.com>
 *
 */
 
namespace Unity.Quicklauncher
{

  /* we call this instead of Ctk.Menu so you can alter this to look right */
  public class QuicklistMenu : Ctk.Menu
  {
    Ctk.EffectDropShadow drop_shadow;
    Clutter.Rectangle ql_background;
    construct 
    {
      Clutter.Color color = Clutter.Color () {
        red = 0x20,
        green = 0x30,
        blue = 0x40,
        alpha = 0xff
      };
      this.ql_background = new Clutter.Rectangle.with_color (color);
      
      this.set_background (this.ql_background);
      Ctk.Padding padding = Ctk.Padding () {
        left = 6,
        right = 6,
        top = 6,
        bottom = 6
      };
      this.set_padding (padding);
      
      // triggers a bug in the effects framework, only first frame is cached.
      // can't do the work-around of waiting for the menu to have all its 
      // elements before attaching the effects anymore as we need to account
      // for the menu being a small label.
      this.drop_shadow = new Ctk.EffectDropShadow(3, 5, 5);
      add_effect(this.drop_shadow);
    }
  } 
}
