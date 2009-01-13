/* GStreamer Media Player
 * (c) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * disc.h: media content detection
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __DISC_H__
#define __DISC_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
  CD_TYPE_ERROR = -1, /* error */
  CD_TYPE_DATA = 1,
  CD_TYPE_CDDA,
  CD_TYPE_VCD,
  CD_TYPE_DVD
} CdType;

CdType	cd_detect_type	(const gchar *device,
			 GError     **error);

G_END_DECLS

#endif /* __DISC_H__ */
