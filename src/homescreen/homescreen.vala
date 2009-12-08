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
 * Authored by Mirco "MacSlow" Müller <mirco.mueller@canonical.com>
 *
 */

namespace Unity.Homescreen
{
	private float points        = 10.0f;
	private float dpi           = 72.0f;
	private float pixels_per_em = 10.0f;

	private float get_dpi ()
	{
		float dpi;

		dpi = 72.0f; //get_dpi_from_gconf ();
		if (dpi == 0.0f)
			dpi = 72.0f;

		return dpi;
	}

	private float get_points ()
	{
		float points;

		points = 10.0f; //get_points_from_gconf ();
		if (points == 0.0f)
			points = 10.0f;

		return points;
	}

	private void update_pixels_per_em ()
	{
		points = get_points ();
		dpi = get_dpi ();
		pixels_per_em = points * dpi / 72.0f;
	}

	public float pixel2em (int pixel_value)
	{
		return (float) pixel_value / pixels_per_em;
	}

	public int em2pixel (float em_value)
	{
		return (int) em_value * (int) pixels_per_em;
	}

	public class View : Ctk.Bin
	{
		private Shell         shell;
		private ActivityStore store;

		public View (Shell shell)
		{ 
			this.shell = shell;
		}

		construct
		{
			stdout.printf ("View() constructor called\n");
			this.store = new ActivityStore ();
			this.add_actor (store);
		}
	}
}
