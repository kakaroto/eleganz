/* COBRA manager parsing library. v1.6
 * This software is released into the public domain.
 */

#include "manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <lv2/sysfs.h>

#define be32(x) (x & 0xff000000) >> 24 | \
                (x & 0x00ff0000) >> 8  | \
                (x & 0x0000ff00) << 8  | \
                (x & 0x000000ff) << 24

#define be64(x) (x & 0xff00000000000000ull) >> 56 |  \
                (x & 0x00ff000000000000ull) >> 40 |  \
                (x & 0x0000ff0000000000ull) >> 24 |  \
                (x & 0x000000ff00000000ull) >> 8  |  \
                (x & 0x00000000ff000000ull) << 8  |  \
                (x & 0x0000000000ff0000ull) << 24 |  \
                (x & 0x000000000000ff00ull) << 40 |  \
                (x & 0x00000000000000ffull) << 56

static inline void set_name (char *out, char *name)
{
  strcpy (out, COBRA_PATH);
  strcat (out, "/");
  strcat (out, name);
}

static inline void cobra_iso_free (CobraIso *iso)
{
  free (iso->filename);
  free (iso->png_path);
  free (iso->sfo_path);
}

int cobra_manager_init (CobraManager *manager)
{
  DIR *d;
  FILE *f;
  struct dirent *e;
  int i;
  char path[MAX_FILENAME_LENGTH+1];

  memset (manager, 0, sizeof(CobraManager));
  manager->info.major = 1;
  manager->info.minor = 0;

  // Read ODE information
  f = fopen (COBRA_PATH "/COBRA.NFO", "rb");
  if (f)
  {
    fread (&manager->info, 1, sizeof(CobraInfo), f);
    manager->info.disc_lba = be32 (manager->info.disc_lba);
    fclose (f);
  }

  d = opendir(COBRA_PATH);
  if (!d)
  {
    return -1;
  }

  // Find disc.iso and game ids
  while((e = readdir(d)) != NULL)
  {
    if(e->d_type != DT_REG)
      continue;

    if (strcmp (e->d_name, "DISC.ISO") == 0)
      set_name (manager->disc_iso, e->d_name);
    if (strcmp (e->d_name, "EJECT") == 0)
    {
      set_name (path, e->d_name);
      manager->eject = strdup (path);
    }
    else if (strlen (e->d_name) == 9)
    {
      if (manager->num_iso < MAX_ISO)
      {
        FILE *f;

        strcpy (manager->iso[manager->num_iso].game_id, e->d_name);
        set_name (path, e->d_name);
        f = fopen (path, "rb");
        if (f)
        {
          char zero = 0;

          fread (&zero, 1, 1, f);
          if (zero == 0)
          {
            u8 len;

            fread (&len, 1, 1, f);
            manager->iso[manager->num_iso].filename = malloc (len + 1);
            fread (manager->iso[manager->num_iso].filename, len, 1, f);
            manager->iso[manager->num_iso].filename[len] = 0;
            fread (&manager->iso[manager->num_iso].size, sizeof(u64), 1, f);

	    if (manager->info.major > 1 || manager->info.minor >= 9)
	    {
	      fread (&manager->iso[manager->num_iso].type, 1, 1, f);
	      fread (manager->iso[manager->num_iso].volume_name, 32, 1, f);
	    }
	    else
	    {
	      manager->iso[manager->num_iso].type = COBRA_ISO_TYPE_PS3_GAME;
	      memset(manager->iso[manager->num_iso].volume_name, ' ', 32);
	    }
            manager->iso[manager->num_iso].size = be64 (manager->iso[manager->num_iso].size);
          }
          else
          {
            manager->iso[manager->num_iso].filename = malloc (2049);
            manager->iso[manager->num_iso].filename[0] = zero;
            fread (manager->iso[manager->num_iso].filename + 1, 2047, 1, f);
            manager->iso[manager->num_iso].filename[2048] = 0;
          }
          fclose (f);
        }
        manager->num_iso++;
      }
    }
  }

  // rewinddir does not work on the ps3, so we reopen the dir
  // rewinddir (d);

  // now go look for the sfo/png/run of each game found
  closedir(d);
  d = opendir(COBRA_PATH);
  if (!d)
  {
    memset (manager, 0, sizeof(CobraManager));
    return -1;
  }

  while((e = readdir(d)) != NULL)
  {
    if(e->d_type != DT_REG)
      continue;

    for (i = 0; i < manager->num_iso; i++)
    {
      if (strlen (e->d_name) > 9 &&
          memcmp (e->d_name, manager->iso[i].game_id, 9) == 0)
      {
        if (strcmp (e->d_name + 9, ".PNG") == 0)
        {
          set_name (path, e->d_name);
          manager->iso[i].png_path = strdup (path);
        }
        else if (strcmp (e->d_name + 9, ".SFO") == 0)
	{
          set_name (path, e->d_name);
          manager->iso[i].sfo_path = strdup (path);
        }
	else if (strcmp (e->d_name + 9, ".RUN") == 0)
          set_name (manager->iso[i].run_path, e->d_name);
        break;
      }
    }
  }
  closedir(d);

  for (i = 0; i < manager->num_iso; i++)
  {
    if ((manager->iso[i].type == COBRA_ISO_TYPE_PS3_GAME && manager->iso[i].sfo_path[0] == 0) ||
        manager->iso[i].run_path[0] == 0)
    {
      // Remove iso files with missing sfo/run
      cobra_iso_free (&manager->iso[i]);
      memmove (&manager->iso[i], &manager->iso[i+1], sizeof(CobraIso) * (manager->num_iso-i-1));
      memset (&manager->iso[manager->num_iso - 1], 0, sizeof(CobraIso));
      manager->num_iso--;
      i--;
    }
  }

  return manager->num_iso;
}

void cobra_manager_free (CobraManager *manager)
{
  int i;

  for (i = 0; i < manager->num_iso; i++)
    cobra_iso_free (&manager->iso[i]);
  free (manager->eject);
}

u8 cobra_iso_run (CobraIso *iso)
{
  FILE *f = fopen (iso->run_path, "rb");

  if (f)
  {
    char buffer[2048];

    fread (buffer, 1, 2048, f);
    fclose (f);
    return 1;
  }
  return 0;
}

CobraDisc * cobra_disc_open (CobraManager *manager)
{
  CobraDisc *disc = NULL;

  // disc_lba is required and only availale in v1.3+
  if ((manager->info.major > 1 || manager->info.minor >= 3) &&
      manager->info.disc_lba != 0)
  {
    disc = malloc (sizeof(CobraDisc));

    if (sysFsOpen (manager->disc_iso, SYS_O_RDONLY, &disc->fd, NULL, 0) == 0)
    {
      u64 read, pos;

      // Read encrypted sector information in sector 0
      disc->disc_lba = manager->info.disc_lba;
      disc->sector = 0;
      sysFsRead(disc->fd, &disc->sector0, 2048, &read);
      sysFsLseek(disc->fd, 0, SEEK_SET, &pos);
    }
    else
    {
      free (disc);
      return NULL;
    }
  }

  return disc;
}

u32 cobra_disc_write (CobraDisc *disc, s32 output)
{
  u8 is_encrypted = 1;
  int i;
  u32 sectors, bytes;
  u64 read, written;

  // Determine from sector 0 if the current 0x20 sectors are encrypted or not
  // PS3 always reads on 0x20 sector boundaries
  for (i = 0; i < disc->sector0.num_plain; i++)
  {
    if (disc->sector >= disc->sector0.plain_regions[i].start &&
        disc->sector <= disc->sector0.plain_regions[i].end)
    {
      is_encrypted = 0;
      break;
    }
  }

  if (sysFsRead(disc->fd, disc->buffer, 2048 * 32, &read) == 0)
  {
    sectors = (u32) read / 2048;
    if (is_encrypted)
    {
      // If encrypted, post-process buffer to fix encryption IV
      for (i = 0; i < sectors; i++)
      {
        u8 real_xor[4];
        u8 fake_xor[4];
        u32 real_lba = disc->sector + i;
        u32 fake_lba = real_lba + disc->disc_lba;
        int j;

        real_xor[3] = real_lba & 0xFF;
        real_xor[2] = (real_lba >> 8) & 0xFF;
        real_xor[1] = (real_lba >> 16) & 0xFF;
        real_xor[0] = (real_lba >> 24) & 0xFF;
        fake_xor[3] = fake_lba & 0xFF;
        fake_xor[2] = (fake_lba >> 8) & 0xFF;
        fake_xor[1] = (fake_lba >> 16) & 0xFF;
        fake_xor[0] = (fake_lba >> 24) & 0xFF;

        for (j = 0; j < 4; j++)
          disc->buffer[i*2048 + j + 12] = (disc->buffer[i*2048 + j + 12] ^ fake_xor[j]) ^ real_xor[j];
      }
    }
    // Write to file
    disc->sector = disc->sector + sectors;
    if (sysFsWrite(output, disc->buffer, read, &written) == 0)
      bytes = (u32) written;
    else
      bytes = 0;
  }
  else
    bytes = 0;

  return bytes;
}

void cobra_disc_close (CobraDisc *disc)
{
  sysFsClose (disc->fd);
  free (disc);
}

u8 cobra_manager_eject (CobraManager *manager)
{
  FILE *f = NULL;

  if (manager->eject)
    f = fopen (manager->eject, "rb");

  if (f)
  {
    char buffer[2048];

    fread (buffer, 1, 2048, f);
    fclose (f);
    return 1;
  }
  return 0;
}
