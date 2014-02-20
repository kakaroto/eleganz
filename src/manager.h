/* COBRA manager parsing library. v1.6
 * This software is released into the public domain.
 */
#ifndef _MANAGER_H_
#define _MANAGER_H_

#include <ppu-types.h>
#include <lv2/sysfs.h>

/*
 * This library provides an API to parse the available ISO files
 * provided by the COBRA ODE.
 * You can communicate with the COBRA ODE firmware by accessing those
 * files. Please refer to user manual for more information.
 *
 * You must allocate a CobraManager structure and call cobra_manager_init to get
 * a list of all valid ISO files and other information.
 * It will fill the structure and return the number of games found.
 * You must call cobra_manager_free to free resources when done.
 * You can use cobra_iso_run to tell COBRA ODE to select an iso.
 * Game dumping requires post-processing that needs to be done via the appropriate API.
 * You can create a CobraDisc structure with cobra_disc_open, then use
 * cobra_disc_write to copy 64KB of the iso into an output file.
 * It will return the number of bytes written or 0 in case of error or EOF.
 * Use cobra_disc_close to close and free the CobraDisc structure.
 *
 * Example of use :
 *
  CobraManager manager;
  int i;

  cobra_manager_init(&manager);

  // Print firmware version
  printf ("COBRA ODE Firmware : v%d.%d\n", manager.info.major, manager.info.minor);

  // Parse all ISO files
  for (i = 0; i < manager.num_iso; i++)
  {
    if (manager.iso[i].sfo_path != NULL)
    parse_sfo_file(manager.iso[i].sfo_path);
    if (manager.iso[i].png_path != NULL)
      load_png_from_file(manager.iso[i].png_path);
    show_filename(manager.iso[i].filename);
    show_iso_type(manager.iso[i].type);
    if (manager.info.major > 1 || manager.info.minor >= 3)
      show_iso_size(manager.iso[i].size);
  }

  // User selects game
  cobra_iso_run(&manager.iso[selection]);

  // User dumps disc
  CobraDisc *disc = cobra_disc_open(&manager);
  s32 out;

  sysFsOpen ("/dev_usb000/DISC.ISO", SYS_O_WRONLY, &out, NULL, 0);

  while (1)
  {
    int bytes = cobra_disc_write (disc, out);
    // Show progress
    // ...
    //////
    if (bytes == 0)
      break;
  }
  sysFsClose (out);
  cobra_disc_close (disc);

  // Eject and load game
  cobra_manager_eject(&manager);

  // Exit
  cobra_manager_free(&manager);

 *
 */

#define MAX_ISO 500
#define MAX_FILENAME_LENGTH 128
#define DEV_BDVD "/dev_bdvd"
#define COBRA_PATH DEV_BDVD "/COBRA"

enum {
  COBRA_ISO_TYPE_PS1_GAME = 1,
  COBRA_ISO_TYPE_PS2_GAME = 2,
  COBRA_ISO_TYPE_PS3_GAME = 3,
  COBRA_ISO_TYPE_BD_MOVIE = 4,
  COBRA_ISO_TYPE_DVD_MOVIE = 5,
};

typedef struct {
  char game_id[10];
  char *filename;
  char volume_name[32]; // Since 1.9
  u64 size; // Since 1.3
  u8 type; // Since 1.9
  char *png_path; // NULL if iso is not a PS3 game or has no ICON0.PNG
  char *sfo_path; // NULL if iso not a PS3 game;
  char run_path[MAX_FILENAME_LENGTH+1];
} CobraIso;

typedef struct {
  u8 major;
  u8 minor; // 1.0 and 1.1 appear as 1.0
  u32 disc_lba; // Since 1.3
  u8 quick_eject; // Since 1.6
} __attribute__ ((packed)) CobraInfo;

typedef struct {
  CobraIso iso[MAX_ISO];
  int num_iso;
  char disc_iso[MAX_FILENAME_LENGTH+1];
  char *eject;
  CobraInfo info;
} CobraManager;

typedef struct {
  s32 fd;
  u32 disc_lba;
  u32 sector;
  struct {
    u32 num_plain;
    u32 zero;
    struct {
      u32 start;
      u32 end;
    } plain_regions[255];
  } sector0;
  u8 buffer[32 * 2048];
} CobraDisc;

// Parse the manager files and generate the listing
int cobra_manager_init (CobraManager *manager);
// Free resources
void cobra_manager_free (CobraManager *manager);
// Eject manager disc and load game
u8 cobra_manager_eject (CobraManager *manager);
// Run the selected iso
u8 cobra_iso_run (CobraIso *iso);
// Open DISC.ISO for dumping. Requires MCU firmware v1.3 or later
CobraDisc * cobra_disc_open (CobraManager *manager);
// Read 32 sectors from DISC.ISO and write them to the output file.
// Returns number of bytes written or 0 in case of error or EOF
u32 cobra_disc_write (CobraDisc *disc, s32 output);
// Close file and free resources
void cobra_disc_close (CobraDisc *disc);


#endif
