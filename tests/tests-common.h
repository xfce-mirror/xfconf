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

#ifndef __XFCONF_TESTS_COMMON_H__
#define __XFCONF_TESTS_COMMON_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>
#include <xfconf/xfconf.h>

#define TEST_CHANNEL_NAME  "test-channel"

#define TEST_OPERATION(x) G_STMT_START{ \
    if(!(x)) { \
        g_critical("Test failed: " # x); \
        xfconf_tests_end(); \
        return 1; \
    } \
}G_STMT_END

static GPid xfconfd_pid = -1;

/* don't use static to avoid compiler warnings in tests that don't use
 * all of them */
const gchar *test_string_property = "/test/stringtest/string";
const gchar *test_string = "teststring";
const gchar *test_strlist_property = "/test/stringtest/strlist";
const gchar *test_strlist[] = { "teststring1", "teststring2", NULL };
const gchar *test_int_property = "/test/inttest/int";
const gint test_int = 42;
const gchar *test_uint64_property = "/test/uint64test/uint64";
const guint64 test_uint64 = 42000000000LL;
const gchar *test_double_property = "/test/doubletest/double";
const gdouble test_double = 42.4242;
const gchar *test_bool_property = "/test/booltest/bool";
const gboolean test_bool = TRUE;
const gchar *test_array_property = "/test/arrayprop";

static inline gboolean
xfconf_tests_start()
{
    gchar *argv[2] = { XFCONFD, NULL };
    GError *error = NULL;
    
    if(!g_spawn_async(NULL, argv, NULL, 0, NULL, NULL, &xfconfd_pid, &error)) {
        g_critical("Failed to launch xfconfd (%s): %s", XFCONFD, error->message);
        g_error_free(error);
        return FALSE;
    }
    
    /* this is lame */
    sleep(1);
    
    if(!xfconf_init(&error)) {
        g_critical("Failed to init libxfconf: %s", error->message);
        g_error_free(error);
        return FALSE;
    }
    
    return TRUE;
}

static inline void
xfconf_tests_end()
{
    xfconf_shutdown();
    
    if(xfconfd_pid != -1) {
        kill(xfconfd_pid, SIGTERM);
        waitpid(xfconfd_pid, NULL, 0);
        xfconfd_pid = -1;
    }
}

#endif  /* __XFCONF_TESTS_COMMON_H__ */
