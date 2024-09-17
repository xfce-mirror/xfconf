/*
 *  xfconf
 *
 *  Copyright (c) 2008 Brian Tarricone <bjt23@cornell.edu>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License ONLY.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "tests-common.h"

int
main(int argc,
     char **argv)
{
    gchar **channels;
    gint i;

    if (!xfconf_tests_start()) {
        return 1;
    }

    TEST_OPERATION((channels = xfconf_list_channels()));

    for (i = 0; channels[i]; ++i) {
        g_print("Channel %d: %s\n", i, channels[i]);
    }
    TEST_OPERATION((i == 1));

    xfconf_tests_end();

    return 0;
}
