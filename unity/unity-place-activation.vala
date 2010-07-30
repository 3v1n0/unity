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
 * Authored by Mikkel Kamstrup Erlandsen <mikkel.kamstrup@canonical.com>
 *
 */

/*
 * IMPLEMENTATION NOTE:
 * We want the generatedd C API to be nice and not too Vala-ish. We must
 * anticipate that place daemons consuming libunity will be written in
 * both Vala and C.
 *
 */
namespace Unity.Place {

  /**
   * UnityPlaceActivation:
   *
   * Interface for async launching of URIs. Instances implementing
   * this interface can be registered with a Unity.Place.Controller by
   * calling set_activation (activation) on the controller object.
   */
  [DBus (name = "com.canonical.Unity.Activation")]
  public interface Activation : GLib.Object
  {
    /**
     * Returns true if the URI has been activated, false if activation should
     * be delegated to other activation services.
     */
    public async abstract bool activate (string uri) throws DBus.Error;
  }

} /* namespace */
