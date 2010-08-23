/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */
 
using Unity.Launcher;
using Unity.Testing;
 
namespace Unity.Places
{
  public class VolumeController : Object
  {
    private VolumeMonitor monitor;

    public VolumeController ()
    {
      Object ();
    }

    construct
    {
      monitor = VolumeMonitor.get ();
      monitor.volume_added.connect (on_volume_added);

      var volumes = monitor.get_volumes ();
      foreach (Volume volume in volumes)
        {
          on_volume_added_real (volume);

          /* Because get_volumes () returns a list of ref'd volumes */
          volume.unref ();
        }
    }

    private bool on_volume_added_real (Volume volume)
    {
      if (volume.can_eject ())
        {
          debug ("VOLUME_ADDED: %s", volume.get_name ());

          var child = new VolumeChildController (volume);
          
          ScrollerModel s;
          
          s = ObjectRegistry.get_default ().lookup ("UnityScrollerModel")[0] as ScrollerModel;
          s.add (child.child);

          return true;
        }
      else
        return false;
    }

    private void on_volume_added (Volume volume)
    {
      on_volume_added_real (volume);
    }
  }
}
