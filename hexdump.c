/*

Copyright  2013, Chris Barts.

This file is part of gopher.

gopher is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

gopher is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with gopher.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

void hexdump(FILE * of, const unsigned char *buf, size_t len,
             size_t noffset)
{
    size_t i, j;

    for (i = 0; i < len; i += 16) {
        fprintf(of, "%08x: ", i + noffset);
        for (j = i; (j < len) && (j < i + 16); j++) {
            if (j % 8 == 0)
                fprintf(of, " ");
            fprintf(of, "%02x ", buf[j]);
        }

        if ((i + 16) > len) {   /* We have extra to space-fill. */
            for (; j < i + 16; j++) {
                if (j % 8 == 0)
                    fprintf(of, " ");
                fprintf(of, "   ");
            }
        }

        for (j = i; (j < len) && (j < i + 16); j++) {
            if (j % 8 == 0)
                fprintf(of, " ");
            fprintf(of, "%c", isgraph(buf[j]) ? buf[j] : '.');
        }

        fprintf(of, "\n");
    }
}
