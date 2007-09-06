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

#ifndef __XFCONF_H__
#define __XFCONF_H__

#include <glib.h>

#define XFCONF_IN_XFCONF_H

#include <xfconf/xfconf-channel.h>

G_BEGIN_DECLS

gboolean xfconf_init(GError **error);
void xfconf_shutdown();

G_END_DECLS

#endif  /* __XFCONF_H__ */
