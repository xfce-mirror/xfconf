/*
 *  xfconf
 *
 *  Copyright (c) 2007 Brian Tarricone <bjt23@cornell.edu>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "tests-common.h"

int
main(int argc,
     char **argv)
{
    XfconfChannel *channel;
    
    if(!xfconf_tests_start())
        return 1;
    
    channel = xfconf_channel_new("test-channel");
    
    TEST_OPERATION(xfconf_channel_set_string(channel, test_string_property, test_string));
    TEST_OPERATION(xfconf_channel_set_string_list(channel, test_strlist_property, test_strlist));
    TEST_OPERATION(xfconf_channel_set_int(channel, test_int_property, test_int));
    TEST_OPERATION(xfconf_channel_set_int64(channel, test_int64_property, test_int64));
    TEST_OPERATION(xfconf_channel_set_double(channel, test_double_property, test_double));
    TEST_OPERATION(xfconf_channel_set_bool(channel, test_bool_property, test_bool));
    
    g_object_unref(G_OBJECT(channel));
    
    xfconf_tests_end();
    
    return 0;
}
