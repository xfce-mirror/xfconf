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
    
    xfconf_channel_remove_property(channel, test_string_property);
    xfconf_channel_remove_property(channel, test_strlist_property);
    xfconf_channel_remove_property(channel, test_int_property);
    xfconf_channel_remove_property(channel, test_int64_property);
    xfconf_channel_remove_property(channel, test_double_property);
    xfconf_channel_remove_property(channel, test_bool_property);
    
    TEST_OPERATION(xfconf_channel_get_string(channel, test_string_property, NULL) == NULL);
    TEST_OPERATION(xfconf_channel_get_string_list(channel, test_strlist_property, NULL) == NULL);
    TEST_OPERATION(xfconf_channel_get_int(channel, test_int_property, -1) == -1);
    TEST_OPERATION(xfconf_channel_get_int64(channel, test_int64_property, -1) == -1);
    /* FIXME: will this work everywhere? */
    TEST_OPERATION(xfconf_channel_get_double(channel, test_double_property, 0.0) == 0.0);
    TEST_OPERATION(xfconf_channel_get_bool(channel, test_bool_property, FALSE) == FALSE);
    
    g_object_unref(G_OBJECT(channel));
    
    xfconf_tests_end();
    
    return 0;
}
