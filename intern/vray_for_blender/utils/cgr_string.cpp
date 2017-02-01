/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor(s): Andrei Izrantcev <andrei.izrantcev@chaosgroup.com>
 *
 * * ***** END GPL LICENSE BLOCK *****
 */

#include "cgr_config.h"
#include "cgr_string.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>


void StripString(char *str)
{
	int nChars = strlen(str);
	int i      = 0;

	for(i = 0; i < nChars; i++) {
		if(str[i]) {
			if(str[i] == '+')
				str[i] = 'p';
			else if(str[i] == '-')
				str[i] = 'm';
			else if(str[i] == '|' ||
					str[i] == '@')
				continue;
			else if(!((str[i] >= 'A' && str[i] <= 'Z') || (str[i] >= 'a' && str[i] <= 'z') || (str[i] >= '0' && str[i] <= '9')))
				str[i] = '_';
		}
	}
}


std::string StripString(const std::string &str)
{
	static char buf[CGR_MAX_PLUGIN_NAME];
	strncpy(buf, str.c_str(), CGR_MAX_PLUGIN_NAME);
	StripString(buf);
	return buf;
}


bool IsStdStringDigit(const std::string &str)
{
	return std::count_if(str.begin(), str.end(), ::isdigit) == str.size();
}