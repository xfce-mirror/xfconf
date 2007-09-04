/*
 *  xfce-power-manager
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <libxfce4util/libxfce4util.h>

int
main(int argc,
     char **argv)
{
    GMainLoop *mloop;
    XfconfDaemon *xfconfd;
    GError *error = NULL;
    
    xfce_textdomain(PACKAGE, LOCALEDIR, "UTF-8");
    
    g_type_init();
    
    mloop = g_main_loop_new(NULL, FALSE);
    
    xfconfd = xfconf_daemon_new_unique(&error);
    if(!xfconfd) {
        g_printerr("Xfconfd failed to start: %s\n", error->message);
        return 1;
    }
    
    g_main_loop_run(mloop);
    
    return 0;
}
