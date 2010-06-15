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
 * Authored by Mirco "MacSlow" Müller <mirco.mueller@canonical.com>
 *             Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

namespace Unity.Places
{
  public class View : Ctk.Box
  {
    private PlaceModel _model;

    /* Properties */
    public Shell shell { get; construct; }

    public PlaceModel model {
      get { return _model; }
      set { _model = value; }
    }

    private PlaceBar place_bar;

    public View (Shell shell)
    {
      Object (shell:shell, orientation:Ctk.Orientation.VERTICAL);
    }

    construct
    {
      _model = new PlaceFileModel () as PlaceModel;

      Idle.add (() => {
        place_bar = new PlaceBar (shell, _model);
        pack (place_bar, false, true);
        place_bar.show ();

        return false;
      });
    }
  }
}

