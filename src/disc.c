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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <mntent.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>

#include <glib.h>

#include "disc.h"

typedef struct _CdCache {
  /* device node and mountpoint */
  gchar *device, *mountpoint;

  /* file descriptor to the device */
  gint fd;

  /* capabilities of the device */
  gint cap;

  /* indicates if we mounted this mountpoint ourselves or if it
   * was already mounted. */
  gboolean self_mounted;
  gboolean mounted;
} CdCache;

/*
 * So, devices can be symlinks and that screws up.
 */

static gchar *
get_device (const gchar *device,
	    GError     **error)
{
  gchar buf[256];
  gchar *dev = g_strdup (device);
  struct stat st;
  gint read;

  while (1) {
    if (lstat (dev, &st) != 0) {
      g_set_error (error, 0, 0,
          "Failed to find real device node for %s: %s",
          dev, g_strerror (errno));
      g_free (dev);
      return NULL;
    }

    if (!S_ISLNK (st.st_mode))
      break;

    if ((read = readlink (dev, buf, 255)) < 0) {
      g_set_error (error, 0, 0,
          "Failed to read symbolic link %s: %s",
          dev, g_strerror (errno));
      g_free (dev);
      return NULL;
    }
    g_free (dev);
    dev = g_strndup (buf, read);
  }

  return dev;
}

static CdCache *
cd_cache_new (const gchar *dev,
	      GError     **error)
{
  CdCache *cache;
  FILE *f;
  gchar *mountpoint = NULL, buf[256], *device;
  struct mntent mnt;

  /* retrieve mountpoint (/etc/fstab). We could also use HAL for this,
   * I think (gnome-volume-manager does that). */
  if (!(device = get_device (dev, error)))
    return NULL;
  if (!(f = setmntent ("/etc/fstab", "r"))) {
    g_set_error (error, 0, 0,
        "Opening /etc/fstab failed: %s",
        g_strerror (errno));
    return NULL;
  }
  while (getmntent_r (f, &mnt, buf, 256)) {
    gchar *pdev;

    if (!(pdev = get_device (mnt.mnt_fsname, NULL)))
      continue;

    if (!strcmp (pdev, device)) {
      mountpoint = g_strdup (mnt.mnt_dir);
      g_free (pdev);
      break;
    }
    g_free (pdev);
  }
  endmntent (f);
  if (!mountpoint) {
    g_set_error (error, 0, 0,
        "Failed to find mountpoint for device %s in /etc/fstab",
        device);
    return NULL;
  }

  /* create struture */
  cache = g_new0 (CdCache, 1);
  cache->device = device;
  cache->mountpoint = mountpoint;
  cache->fd = -1;
  cache->self_mounted = FALSE;

  return cache;
}

static gboolean
cd_cache_open_device (CdCache *cache,
		      GError **error)
{
  gint drive;

  /* already open? */
  if (cache->fd > 0)
    return TRUE;

  /* try to open the CD before creating anything */
  if ((cache->fd = open (cache->device, O_RDONLY)) < 0) {
    g_set_error (error, 0, 0,
        "Failed to open device %s for reading: %s",
        cache->device, g_strerror (errno));
    return FALSE;
  }

  /* get capabilities */
  if ((cache->cap = ioctl (cache->fd, CDROM_GET_CAPABILITY, NULL)) < 0) {
    close (cache->fd);
    cache->fd = -1;
    g_set_error (error, 0, 0,
        "Failed to retrieve capabilities of device %s: %s",
        cache->device, g_strerror (errno));
    return FALSE;
  }

  /* is there a disc in the tray? */
  if ((drive = ioctl (cache->fd, CDROM_DRIVE_STATUS, NULL)) != CDS_DISC_OK) {
    const gchar *drive_s;

    close (cache->fd);
    cache->fd = -1;

    switch (drive) {
      case CDS_NO_INFO:
        drive_s = "Not implemented";
        break;
      case CDS_NO_DISC:
        drive_s = "No disc in tray";
        break;
      case CDS_TRAY_OPEN:
        drive_s = "Tray open";
        break;
      case CDS_DRIVE_NOT_READY:
        drive_s = "Drive not ready";
        break;
      case CDS_DISC_OK:
        drive_s = "OK";
        break;
      default:
        drive_s = "Unknown";
        break;
    }
    g_set_error (error, 0, 0,
        "Drive status 0x%x (%s) - check disc",
        drive, drive_s);
    return FALSE;
  }

  return TRUE;
}

static GDir *
cd_cache_open_mountpoint (CdCache *cache,
			  GError **error)
{
  FILE *f;
  gchar buf[256];
  struct mntent mnt;

  /* already opened? */
  if (cache->mounted)
    goto opendir;

  /* check for mounting - assume we'll mount ourselves */
  if (!(f = setmntent ("/etc/mtab", "r"))) {
    g_set_error (error, 0, 0,
        "Opening /etc/mtab failed: %s",
        g_strerror (errno));
    return NULL;
  }
  cache->self_mounted = TRUE;
  while (getmntent_r (f, &mnt, buf, 256)) {
    if (!strcmp (mnt.mnt_dir, cache->mountpoint)) {
      cache->self_mounted = FALSE;
      break;
    }
  }
  endmntent (f);

  /* mount if we have to */
  if (cache->self_mounted) {
    gchar *command;
    gint status;

    command = g_strdup_printf ("mount %s", cache->mountpoint);
    if (!g_spawn_command_line_sync (command,
             NULL, NULL, &status, error)) {
      g_free (command);
      return NULL;
    }
    g_free (command);
    if (status != 0) {
      g_set_error (error, 0, 0,
          "Unexpected error status %d while mounting %s",
          status, cache->mountpoint);
      return NULL;
    }
  }

  cache->mounted = TRUE;

opendir:
  return g_dir_open (cache->mountpoint, 0, error);
}

static void
cd_cache_free (CdCache *cache)
{
  /* umount if we mounted */
  if (cache->self_mounted && cache->mounted) {
    gchar *command;

    command = g_strdup_printf ("umount %s", cache->mountpoint);
    g_spawn_command_line_sync (command, NULL, NULL, NULL, NULL);
    g_free (command);
  }

  /* close file descriptor to device */
  if (cache->fd > 0) {
    close (cache->fd);
  }

  /* free mem */
  g_free (cache->mountpoint);
  g_free (cache->device);
  g_free (cache);
}

static CdType
cd_cache_disc_is_cdda (CdCache *cache,
		       GError **error)
{
  CdType type = CD_TYPE_DATA;
  gint disc;
  const gchar *disc_s;

  /* open disc and open mount */
  if (!cd_cache_open_device (cache, error))
    return CD_TYPE_ERROR;

  if ((disc = ioctl (cache->fd, CDROM_DISC_STATUS, NULL)) < 0) {
    g_set_error (error, 0, 0,
        "Error getting %s disc status: %s",
        cache->device, g_strerror (errno));
    return CD_TYPE_ERROR;
  }

  switch (disc) {
    case CDS_NO_INFO:
      disc_s = "Not implemented";
      type = CD_TYPE_ERROR;
      break;
    case CDS_NO_DISC:
      disc_s = "No disc in tray";
      type = CD_TYPE_ERROR;
      break;
    case CDS_AUDIO:
    case CDS_MIXED:
      type = CD_TYPE_CDDA;
      break;
    case CDS_DATA_1:
    case CDS_DATA_2:
    case CDS_XA_2_1:
    case CDS_XA_2_2:
      break;
    default:
      disc_s = "Unknown";
      type = CD_TYPE_ERROR;
      break;
  }
  if (type == CD_TYPE_ERROR) {
    g_set_error (error, 0, 0,
        "Unexpected/unknown cd type 0x%x (%s)",
        disc, disc_s);
    return CD_TYPE_ERROR;
  }

  return type;
}

static CdType
cd_cache_disc_is_vcd (CdCache *cache,
                      GError **error)
{
  GDir *dir;
  CdType type = CD_TYPE_DATA;

  /* open disc and open mount */
  if (!cd_cache_open_device (cache, error) ||
      !(dir = cd_cache_open_mountpoint (cache, error)))
    return CD_TYPE_ERROR;

  //..

  g_dir_close (dir);

  return type;
}

static CdType
cd_cache_disc_is_dvd (CdCache *cache,
		      GError **error)
{
  GDir *dir;
  const gchar *name;
  gboolean have_vts = FALSE, have_vtsifo = FALSE, have_ats = FALSE;

  /* open disc, check capabilities and open mount */
  if (!cd_cache_open_device (cache, error))
    return CD_TYPE_ERROR;
  if (!(cache->cap & CDC_DVD))
    return CD_TYPE_DATA;
  if (!(dir = cd_cache_open_mountpoint (cache, error)))
    return CD_TYPE_ERROR;

  /* check directory structure */
  while ((name = g_dir_read_name (dir)) != NULL) {
    if (!strcmp (name, "VIDEO_TS")) {
      GDir *subdir;
      gchar *subdirname = g_build_path (G_DIR_SEPARATOR_S,
                               cache->mountpoint, name, NULL);

      have_vts = TRUE;
      if (!(subdir = g_dir_open (subdirname, 0, error)))
        return CD_TYPE_ERROR;
      while ((name = g_dir_read_name (subdir)) != NULL) {
        if (!strcmp (name, "VIDEO_TS.IFO"))
          have_vtsifo = TRUE;
      }
      g_dir_close (subdir);
      g_free (subdirname);
    } else if (!strcmp (name, "AUDIO_TS")) {
      have_ats = TRUE;
    }
  }
  g_dir_close (dir);

  return (have_vts && have_ats && have_vtsifo) ?
      CD_TYPE_DVD : CD_TYPE_DATA;
}

CdType
cd_detect_type (const gchar *device,
		GError     **error)
{
  CdCache *cache;
  CdType type;

  if (!(cache = cd_cache_new (device, error)))
    return CD_TYPE_ERROR;
  if ((type = cd_cache_disc_is_cdda (cache, error)) == CD_TYPE_DATA &&
      (type = cd_cache_disc_is_vcd (cache, error)) == CD_TYPE_DATA &&
      (type = cd_cache_disc_is_dvd (cache, error)) == CD_TYPE_DATA) {
    /* crap, nothing found */
  }
  cd_cache_free (cache);

  return type;
}
