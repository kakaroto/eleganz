/*
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tools.h"
#include "paged_file.h"
#include "pkg.h"
#include "keys.h"

Key *keys = NULL;
int num_keys = 0;

static int
pkg_debug_decrypt (PagedFile *f, PagedFileCryptOperation operation,
    u8 *ptr, u32 len, void *user_data)
{
  u64 *crypt_offset = user_data;
  u8 key[0x40];
  u8 bfr[0x14];
  u64 i;
  s64 seek;

  if (operation == PAGED_FILE_CRYPT_DECRYPT ||
      operation == PAGED_FILE_CRYPT_ENCRYPT) {
    memset(key, 0, 0x40);
    memcpy(key, f->key, 8);
    memcpy(key + 0x08, f->key, 8);
    memcpy(key + 0x10, f->key + 8, 8);
    memcpy(key + 0x18, f->key + 8, 8);
    seek = (signed) ((f->page_pos + f->pos) - *crypt_offset) / 0x10;
    if (seek > 0)
      wbe64(key + 0x38, be64(key + 0x38) + seek);

    for (i = 0; i < len; i++) {
      if (i % 16 == 0) {
        sha1(key, 0x40, bfr);
        wbe64(key + 0x38, be64(key + 0x38) + 1);
      }
      ptr[i] ^= bfr[i & 0xf];
    }
  }

  return TRUE;
}

int
pkg_open (const char *filename, PagedFile *in,
    PKG_HEADER *header, PKG_FILE_HEADER **files)
{
  Key *gpkg_key;
  u32 i;

  if (!paged_file_open (in, filename, TRUE)) {
    printf ("Unable to open package file\n");
    return FALSE;
  }

  paged_file_read (in, header, sizeof(PKG_HEADER));
  PKG_HEADER_FROM_BE (*header);

  if (header->magic != 0x7f504b47)
    goto error;

  paged_file_seek (in, header->data_offset);
  if (header->pkg_type == 0x80000001) {
    if (keys == NULL) {
      keys = keys_load (&num_keys);
      if (keys == NULL) {
        printf ("Unable to load necessary keys\n");
        return FALSE;
      }
    }

    gpkg_key = keys_find_by_name (keys, num_keys, "Game PKG");
    paged_file_crypt (in, gpkg_key->key, header->k_licensee,
        PAGED_FILE_CRYPT_AES_128_CTR, NULL, NULL);
  } else {
    paged_file_crypt (in, header->digest, header->k_licensee,
        PAGED_FILE_CRYPT_CUSTOM, pkg_debug_decrypt, &header->data_offset);
  }

  *files = malloc (header->item_count * sizeof(PKG_FILE_HEADER));
  paged_file_read (in, *files, header->item_count * sizeof(PKG_FILE_HEADER));

  for (i = 0; i < header->item_count; i++) {
    PKG_FILE_HEADER_FROM_BE ((*files)[i]);
  }

  return TRUE;

 error:
  if (*files)
    free (*files);
  *files = NULL;

  paged_file_close (in);

  return FALSE;
}

int
pkg_list (const char *filename)
{
  PagedFile in = {0};
  PKG_HEADER header;
  PKG_FILE_HEADER *files = NULL;
  char *pkg_file_path = NULL;
  u32 i;

  if (!pkg_open (filename, &in, &header, &files))
    die ("Unable to open pkg file\n");

  printf ("PKG type : %X\n", header.pkg_type);
  printf ("Item count : %d\n", header.item_count);
  printf ("Content ID : %s\n", header.contentid);
  printf ("Digest : ");
  print_hash (header.digest, 0x10);
  printf ("\nKLicensee : ");
  print_hash (header.k_licensee, 0x10);
  printf ("\n\n");

  for (i = 0; i < header.item_count; i++) {
    paged_file_seek (&in, files[i].filename_offset + header.data_offset);
    pkg_file_path = malloc (files[i].filename_size + 1);
    paged_file_read (&in, pkg_file_path, files[i].filename_size);
    pkg_file_path[files[i].filename_size] = 0;
    printf ("File %d : %s\n\tSize : %lu\n\tFlags : %X\n", i, pkg_file_path,
        files[i].data_size, files[i].flags);
    free (pkg_file_path);
  }

  paged_file_close (&in);
  return TRUE;

}

int
pkg_unpack (const char *filename, const char *destination)
{
  PagedFile in = {0};
  PagedFile out = {0};
  char out_dir[1024];
  char *pkg_file_path = NULL;
  char path[1024];
  PKG_HEADER header;
  PKG_FILE_HEADER *files = NULL;
  int ret = TRUE;
  u32 i;

  if (!pkg_open (filename, &in, &header, &files))
    die ("Unable to open pkg file\n");

  if (destination == NULL) {
    strncpy (out_dir, header.contentid, sizeof(out_dir));
  } else {
    strncpy (out_dir, destination, sizeof(out_dir));
  }
  mkdir_recursive (out_dir);

  for (i = 0; i < header.item_count; i++) {
    int j;

    paged_file_seek (&in, files[i].filename_offset + header.data_offset);
    pkg_file_path = malloc (files[i].filename_size + 1);
    paged_file_read (&in, pkg_file_path, files[i].filename_size);
    pkg_file_path[files[i].filename_size] = 0;

    snprintf (path, sizeof(path), "%s/%s", out_dir, pkg_file_path);
    if ((files[i].flags & 0xFF) == 4) {
      mkdir_recursive (path);
    } else {
      j = strlen (path);
      while (j > 0 && path[j] != '/') j--;
      if (j > 0) {
        path[j] = 0;
        mkdir_recursive (path);
        path[j] = '/';
      }
      paged_file_seek (&in, files[i].data_offset + header.data_offset);
      printf ("Opening file %s\n", path);
      if (!paged_file_open (&out, path, FALSE))
        die ("Unable to open output file\n");
      paged_file_splice (&out, &in, files[i].data_size);
      paged_file_close (&out);
    }
  }

  paged_file_close (&in);
  return ret;
}
