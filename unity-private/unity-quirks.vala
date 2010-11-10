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
 * Authored by David Barth <david.barth@canonical.com>
 *
 */

namespace Unity.Quirks
{
	public bool cache_disabled ()
	{
		string? cache = Environment.get_variable ("UNITY_DISABLE_CACHE");
		return (cache != null) ? true : false;
	}

	public bool effects_disabled ()
	{
		string? effects = Environment.get_variable ("CLUTK_DISABLE_EFFECTS");
		return (effects != null) ? true : false;
	}
}
