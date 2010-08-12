/*
 * Copyright (C) 2010 Canonical, Ltd.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * version 3.0 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3.0 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity.Place
{
  public interface Renderer : Ctk.Actor
  {
    /* Main function that is called after initialisation, and also can be
     * called n number of times during the renderers lifetime (if the place
     * entry wants to change it models but keep the same renderer
     */
    public abstract void set_models (Dee.Model                   groups,
                                     Dee.Model                   results,
                                     Gee.HashMap<string, string> hints);

    public signal void activated (string uri, string mimetype);
  }
}
