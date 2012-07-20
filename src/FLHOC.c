/*
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <Ecore.h>
#include <Ecore_File.h>
#include <Ecore_Evas.h>
#include <Edje.h>

#include "common.h"
#include "FLHOC_menu.h"

#ifdef __lv2ppu__
#define DEV_HDD0 "/dev_hdd0/"
#define DEV_USB_FMT "/dev_usb%03d/"
#define THEMES_DIRECTORY "/dev_hdd0/game/FLHOC0000/USRDIR/data/themes/"
#define HOMEBREW_DIRECTORY "/dev_hdd0/game/"
#define USB_HOMEBREW_DIRECTORY_FMT "/dev_usb%03d/PS3/HOMEBREW/"
#define USB_PACKAGES_DIRECTORY_FMT "/dev_usb%03d/PS3/PACKAGES/"
#define USB_THEMES_DIRECTORY_FMT "/dev_usb%03d/PS3/THEMES/"
#define FLHOC_WIDTH 1280
#define FLHOC_HEIGHT 720
#else
#define DEV_HDD0 "./dev_hdd0/"
#define DEV_USB_FMT "./dev_usb%03d/"
#define THEMES_DIRECTORY "data/themes/"
#define HOMEBREW_DIRECTORY "dev_hdd0/game/FLHOC0000/USRDIR/HOMEBREW/"
#define USB_HOMEBREW_DIRECTORY_FMT "dev_usb%03d/PS3/HOMEBREW/"
#define USB_PACKAGES_DIRECTORY_FMT "dev_usb%03d/PS3/PACKAGES/"
#define USB_THEMES_DIRECTORY_FMT "dev_usb%03d/PS3/THEMES/"
#define FLHOC_WIDTH 640
#define FLHOC_HEIGHT 480
#endif

#define MAX_USB_DEVICES 10

typedef struct {
  char *path;
  char *name;
  char *title;
  char *icon;
  char *picture;
} Game;

typedef struct {
  char *path;
  char *name;
} Package;

typedef struct {
  Ecore_Evas *ee;
  Evas *evas;
  Eina_List *themes;
  Theme *current_theme;
  Theme *preview_theme;
  Eina_List *homebrew;
  Eina_List *usb_homebrew[MAX_USB_DEVICES];
  Eina_List *usb_packages[MAX_USB_DEVICES];
  Eina_List *usb_themes[MAX_USB_DEVICES];
  Ecore_Timer *device_check_timer;
} FLHOC;

static int _add_game_items (Eina_List *games, Category *category);
static void _populate_games (FLHOC *flhoc);
static void _populate_themes (FLHOC *flhoc, Theme *current_theme, Eina_Bool only_usb);
static Eina_Bool _add_main_categories (FLHOC *flhoc, Menu *menu);
static Eina_Bool load_theme (FLHOC *flhoc, Theme *theme);
static void set_current_theme (FLHOC *flhoc, Theme *theme);

/* Generic callbacks */
static void
_key_down_cb (void *data, Evas *e, Evas_Object *obj, void *event)
{
  FLHOC *flhoc = data;
  MainWindow *main_win = flhoc->current_theme->main_win;
  Menu *menu = main_win->menu;
  Evas_Event_Key_Down *ev = event;

  //printf ("Key Down: '%s' - '%s' - '%s' - '%s'\n", ev->keyname, ev->key, ev->string, ev->compose);
  if (strcmp (ev->keyname, menu->category_previous) == 0) {
    menu_scroll_previous (menu);
  } else if (strcmp (ev->keyname, menu->category_next) == 0) {
    menu_scroll_next (menu);
  } else if (strcmp (ev->keyname, menu->item_previous) == 0) {
    category_scroll_previous (menu_get_selected_category (menu));
  } else if (strcmp (ev->keyname, menu->item_next) == 0) {
    category_scroll_next (menu_get_selected_category (menu));
  } else if (strcmp (ev->keyname, "f") == 0) {
    Ecore_Evas *ee = ecore_evas_ecore_evas_get (main_win->theme->evas);
    ecore_evas_fullscreen_set (ee, !ecore_evas_fullscreen_get (ee));
  } else if (strcmp (ev->keyname, "Escape") == 0 ||
      strcmp (ev->keyname, "q") == 0) {
    ecore_main_loop_quit();
  } else if (strcmp (ev->keyname, "Return") == 0 ||
      strcmp (ev->keyname, "Cross") == 0) {
    Category *category = menu_get_selected_category (menu);
    Item *item = category_get_selected_item (category);

    if (item) {
      if (item->action)
        item->action (item);
    } else {
      if (category->action)
        category->action (category);
    }
  }
}

static void
_quit_ready_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  ecore_main_loop_quit ();
}

static Eina_Bool
_ee_signal_exit (void* data, int ev_type, void* ev)
{
  ecore_main_loop_quit();
  return 0;
}


static void
_ee_delete_request (Ecore_Evas *ee)
{
  ecore_main_loop_quit();
}

/* Menu callbacks */
static void
_category_action_quit (Category *category)
{
  Menu *menu = category->menu;
  MainWindow *main_win = menu->main_win;

  edje_object_signal_callback_add (main_win->edje, "FLHOC/primary,unset,end", "*",
      _quit_ready_cb, main_win);
  edje_object_signal_emit (main_win->edje, "FLHOC/primary,unset", "");
}

static void
_theme_selection (Item *item, Eina_Bool selected)
{
  Theme *theme = item->priv;
  FLHOC *flhoc = NULL;
  Evas_Object *obj;

  flhoc = evas_object_data_get (item->category->menu->main_win->edje, "FLHOC");
  obj = edje_object_part_swallow_get (item->edje, "FLHOC/menu/item/theme.preview");
  if (obj) {
    edje_object_part_unswallow (item->edje, obj);
    evas_object_hide (obj);
    theme_free (flhoc->preview_theme);
    flhoc->preview_theme = NULL;
  }
  if (selected) {
    flhoc->preview_theme = theme_new (flhoc->evas, theme->edje_file);
    if (load_theme (flhoc, flhoc->preview_theme) == EINA_FALSE) {
      theme_free (flhoc->preview_theme);
      flhoc->preview_theme = NULL;
    }
  }
  if (flhoc->preview_theme) {
    edje_object_part_swallow (item->edje,
        "FLHOC/menu/item/theme.preview", flhoc->preview_theme->main_win->edje);
    main_window_init (flhoc->preview_theme->main_win);
  }
}

static void
_theme_selected (Item *item)
{
  FLHOC *flhoc;
  Theme *new_theme = item->priv;

  flhoc = evas_object_data_get (item->category->menu->main_win->edje, "FLHOC");
  if (new_theme != NULL && new_theme != flhoc->current_theme) {
    if (!new_theme->installed) {
      Eina_List *l;
      char *old_path = strdup (new_theme->edje_file);
      Theme *old_theme = new_theme;
      Eina_Bool installed = EINA_FALSE;

      new_theme = NULL;
      EINA_LIST_FOREACH (flhoc->themes, l, new_theme) {
        printf ("old theme : %s -- new theme : %s\n", old_theme->name, new_theme->name);
        if (strcmp (new_theme->name, old_theme->name) == 0)
          break;
      }
      printf ("new theme is : %p\n", new_theme);
      if (new_theme) {
        installed = ecore_file_cp (old_theme->edje_file, new_theme->edje_file);
      } else {
        Eina_Array *ea;
        char path[1024];

        ea = eina_file_split (old_path);
        snprintf (path, sizeof(path), THEMES_DIRECTORY "%s",
            (char *) eina_array_pop (ea));
        eina_array_free (ea);
        free (old_path);

        if (ecore_file_cp (old_theme->edje_file, path)) {
          new_theme = theme_new (old_theme->evas, path);
          new_theme->installed = EINA_TRUE;
          flhoc->themes = eina_list_append (flhoc->themes, new_theme);
          installed = EINA_TRUE;
        }
      }

      if (installed) {
        printf ("New theme installed successfully to %s\n", new_theme->edje_file);
      } else {
        printf ("Unable to install new theme\n");
        return;
      }
    }
    _theme_selection (item, EINA_FALSE);
    if (flhoc->preview_theme)
      theme_free (flhoc->preview_theme);
    flhoc->preview_theme = NULL;

    set_current_theme (flhoc, new_theme);
  }
}

static void
_game_secondary_unset_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  Secondary *secondary = data;

  edje_object_signal_callback_del_full (secondary->main_win->edje,
      "FLHOC/secondary,unset,end", "*", _game_secondary_unset_cb, secondary);
  secondary_free (secondary);
}

static void
_game_selection (Item *item, Eina_Bool selected)
{
  Game *game = item->priv;
  MainWindow *main_win = item->category->menu->main_win;
  Secondary *secondary = NULL;

  printf ("Game '%s' is %sselected\n", game->title, selected?"":"de");

  if (main_win->secondary) {
    secondary = main_win->secondary;
    edje_object_part_unswallow (main_win->edje, secondary->edje);
    edje_object_signal_callback_add (main_win->edje, "FLHOC/secondary,unset,end", "*",
      _game_secondary_unset_cb, secondary);
    edje_object_signal_emit (main_win->edje, "FLHOC/secondary,unset", secondary->secondary);
    main_win->secondary = NULL;
  }
  if (selected) {
    secondary = secondary_new (main_win, "FLHOC/secondary/game");
    if (edje_object_part_exists (secondary->edje, "FLHOC/secondary/game/title"))
      edje_object_part_text_set (secondary->edje,
          "FLHOC/secondary/game/title", game->title);
    if (game->icon &&
        edje_object_part_exists (secondary->edje, "FLHOC/secondary/game/icon")) {
      Evas_Object *img = (Evas_Object *) edje_object_part_object_get (
          secondary->edje, "FLHOC/secondary/game/icon");
      evas_object_image_file_set (img, game->icon, NULL);
    }
    if (game->picture &&
        edje_object_part_exists (secondary->edje, "FLHOC/secondary/game/picture")) {
      Evas_Object *img = (Evas_Object *) edje_object_part_object_get (
          secondary->edje, "FLHOC/secondary/game/picture");
      evas_object_image_file_set (img, game->picture, NULL);
    }

    main_window_set_secondary (main_win, secondary);
  }
}

static void
_game_selected (Item *item)
{
  Game *game = item->priv;

  /* TODO: do it! */
  printf ("Game '%s' is launched!!!\n", game->title);
}

static void
_package_selected (Item *item)
{
  Package *package = item->priv;

  /* TODO: do it! */
  printf ("Package '%s' is to be installed!!!\n", package->name);
}

/* Utility functions */
static int
_file_exists (const char *path)
{
  struct stat stat_buf;

  return stat (path, &stat_buf) == 0;
}

static char *
_strdup_printf (const char *format, ...)
{
  va_list args;
  int len;
  char *str = NULL;

  va_start (args, format);

  len = vsnprintf (str, 0, format, args);
  if (len > 0) {
    str = malloc (len + 1);
    va_start (args, format);
    vsnprintf (str, len + 1, format, args);
  }
  va_end (args);

  return str;
}

static void
__get_dirlist_internal (Eina_Iterator *iter, const char *base, const char *subdir,
    Eina_List **list, Eina_Bool dirs, Eina_Bool files, Eina_Bool recursive)
{
  Eina_File_Direct_Info *info = NULL;
  char path[1024];

  EINA_ITERATOR_FOREACH (iter, info) {
    if (info->type == EINA_FILE_DIR) {
      if (dirs)
        *list = eina_list_append (*list, eina_file_path_sanitize (info->path));

      if (recursive) {
        Eina_Iterator *dir_iter = eina_file_stat_ls (info->path);
        if (dir_iter) {
          __get_dirlist_internal (dir_iter, base, path, list, dirs, files, recursive);
          eina_iterator_free (dir_iter);
        }
      }
    } else if (files) {
      *list = eina_list_append (*list, eina_file_path_sanitize (info->path));
    }
  }
}

static Eina_List *
_get_dirlist (const char *path, Eina_Bool dirs, Eina_Bool files, Eina_Bool recursive)
{
  Eina_Iterator *dir_iter = NULL;
  Eina_List *list = NULL;

  dir_iter = eina_file_stat_ls (path);
  if (!dir_iter)
    return NULL;

  __get_dirlist_internal (dir_iter, path, ".", &list, dirs, files, recursive);
  eina_iterator_free (dir_iter);

  return list;
}

/* Theme functions */
static Eina_Bool
load_theme (FLHOC *flhoc, Theme *theme)
{
  theme->main_win = main_window_new (theme);
  if (theme->main_win == NULL)
    return EINA_FALSE;

  evas_object_data_set (theme->main_win->edje, "FLHOC", flhoc);
  evas_object_data_set (theme->main_win->edje, "MainWindow", theme->main_win);

  theme->main_win->menu = menu_new (theme->main_win);
  if (theme->main_win->menu == NULL)
    return EINA_FALSE;

  return _add_main_categories (flhoc, theme->main_win->menu);
}

static void
set_current_theme (FLHOC *flhoc, Theme *theme)
{
  Evas_Coord w, h;

  if (theme->main_win) {
    /* Prevent our main window from closing */
    if (theme == flhoc->current_theme)
      ecore_evas_object_dissociate(flhoc->ee, theme->main_win->edje);

    main_window_free (theme->main_win);
    theme->main_win = NULL;
  }

  if (load_theme (flhoc, theme) == EINA_TRUE) {
    evas_output_size_get (flhoc->evas, &w, &h);
    evas_object_move (theme->main_win->edje, 0, 0);
    evas_object_resize (theme->main_win->edje, w, h);
    evas_object_show (theme->main_win->edje);

    ecore_evas_object_associate(flhoc->ee, theme->main_win->edje,
        ECORE_EVAS_OBJECT_ASSOCIATE_BASE);

    if (flhoc->current_theme && flhoc->current_theme->main_win) {
      evas_object_event_callback_del(flhoc->current_theme->main_win->edje,
          EVAS_CALLBACK_KEY_DOWN, _key_down_cb);
      if (flhoc->current_theme != theme) {
        evas_object_hide (flhoc->current_theme->main_win->edje);
        main_window_free (flhoc->current_theme->main_win);
        flhoc->current_theme->main_win = NULL;
      }
    }

    /* Start the process */
    flhoc->current_theme = theme;
    _populate_themes (flhoc, theme, EINA_FALSE);
    evas_object_event_callback_add(theme->main_win->edje,
        EVAS_CALLBACK_KEY_DOWN, _key_down_cb, flhoc);
    evas_object_focus_set (theme->main_win->edje, EINA_TRUE);
    main_window_init (theme->main_win);
  }
}

static Item *
_add_theme (FLHOC *flhoc, Category *category, Theme *theme, ItemType type)
{
  Item *item;

  item = item_new (category, type);
  edje_object_part_text_set (item->edje,
      "FLHOC/menu/item/theme.name", theme->name);
  edje_object_part_text_set (item->edje,
      "FLHOC/menu/item/theme.version", theme->version);
  edje_object_part_text_set (item->edje,
      "FLHOC/menu/item/theme.author", theme->author);
  edje_object_part_text_set (item->edje,
      "FLHOC/menu/item/theme.description", theme->description);
  item->selection = _theme_selection;
  item->action = _theme_selected;
  item->priv = theme;
  category_append_item (category, item);

  return item;
}

static void
_populate_themes (FLHOC *flhoc, Theme *current_theme, Eina_Bool only_usb)
{
  Eina_List *l, *l2;
  Menu *menu = current_theme->main_win->menu;
  Theme *theme;
  Category *category;
  Item *item;
  int i;

  EINA_LIST_FOREACH (menu->categories, l, category) {
    if (category->type == CATEGORY_TYPE_THEME) {
      Eina_List *l;
      Eina_List *l_next;
      EINA_LIST_FOREACH_SAFE (category->items, l, l_next, item) {
        theme = item->priv;
        if (!only_usb || !theme->installed) {
          category_delete_item (category, item);
          item_free (item);
        }
      }
      if (!only_usb) {
        EINA_LIST_FOREACH (flhoc->themes, l2, theme) {
          item = _add_theme (flhoc, category, theme, ITEM_TYPE_THEME);
          if (flhoc->current_theme == theme)
            category->items_selection =
                eina_list_data_find_list (category->items, item);
        }
      }
      for (i = 0; i < MAX_USB_DEVICES; i++) {
        EINA_LIST_FOREACH (flhoc->usb_themes[i], l2, theme)
            _add_theme (flhoc, category, theme, ITEM_TYPE_USB_THEME);
      }

      if (menu_get_selected_category (menu) == category)
        category_set_items (category, "");
      else
        category_set_items (category, NULL);
      break;
    }
  }
}

/* Game functions */
typedef struct
{
  uint32_t magic; /* PSF */
  uint32_t version; /* 1.1 */
  uint32_t name_table_offset;
  uint32_t data_table_offset;
  uint32_t num_entries;
} SFOHeader;
#define SFO_HEADER_FROM_LE(h)                                     \
  h.magic = FROM_LE (32, h.magic);                                \
  h.version = FROM_LE (32, h.version);                            \
  h.name_table_offset = FROM_LE (32, h.name_table_offset);        \
  h.data_table_offset = FROM_LE (32, h.data_table_offset);        \
  h.num_entries = FROM_LE (32, h.num_entries);

typedef struct
{
  uint16_t name_offset;
  uint16_t type;
  uint32_t length;
  uint32_t total_size;
  uint32_t data_offset;
} SFOEntry;
#define SFO_ENTRY_FROM_LE(h)                               \
  h.name_offset = FROM_LE (16, h.name_offset);             \
  h.type = FROM_LE (16, h.type);                           \
  h.length = FROM_LE (32, h.length);                       \
  h.total_size = FROM_LE (32, h.total_size);               \
  h.data_offset = FROM_LE (32, h.data_offset);

static char *
_param_sfo_get_string (const char *path, const char *key)
{
  FILE *fd = fopen (path, "rb");
  SFOHeader h;
  SFOEntry *entries = NULL;
  char *result = NULL;
  uint32_t i;
  char *names = NULL;
  uint32_t name_table_len;

  if (!fd)
    return NULL;

  if (fread (&h, sizeof(SFOHeader), 1, fd) != 1)
    goto error;
  SFO_HEADER_FROM_LE (h);

  if (h.magic != 0x46535000 || h.version != 0x101)
    goto error;

  entries = calloc (h.num_entries, sizeof(SFOEntry));
  if (fread (entries, sizeof(SFOEntry), h.num_entries, fd) != h.num_entries)
    goto error;

  name_table_len = h.data_table_offset - h.name_table_offset;
  names = malloc (name_table_len);
  if (fread (names, name_table_len, 1, fd) != 1)
    goto error;

  for (i = 0; i < h.num_entries; i++) {
    SFO_ENTRY_FROM_LE (entries[i]);
    if (entries[i].type == 0x204 &&
        strcmp (names + entries[i].name_offset, key) == 0) {
      result = malloc (entries[i].total_size);
      fseek (fd, h.data_table_offset + entries[i].data_offset, SEEK_SET);
      if (fread (result, 1, entries[i].total_size, fd) != entries[i].total_size) {
        free (result);
        result = NULL;
        goto error;
      }
    }
  }

 error:
  free (entries);
  free (names);
  fclose (fd);
  return result;
}

static int
_add_game_items (Eina_List *games, Category *category)
{
  Eina_List *l;
  Game *game;
  Item *item = NULL;

  EINA_LIST_FOREACH (games, l, game) {
    Evas_Object *img = NULL;

    item = item_new (category, ITEM_TYPE_GAME);
    edje_object_part_text_set (item->edje, "FLHOC/menu/item/game.title", game->title);
    if (game->icon) {
      img = (Evas_Object *) edje_object_part_object_get (item->edje, "FLHOC/menu/item/game.icon");
      evas_object_image_file_set (img, game->icon, NULL);
    }
    item->selection = _game_selection;
    item->action = _game_selected;
    item->priv = game;
    category_append_item (category, item);
  }

  category->priv = games;

  category_set_items (category, NULL);

  return EINA_TRUE;
}

static int
_add_package_items (Eina_List *packages, Category *category)
{
  Eina_List *l;
  Package *package;
  Item *item = NULL;

  EINA_LIST_FOREACH (packages, l, package) {
    item = item_new (category, ITEM_TYPE_PACKAGE);
    edje_object_part_text_set (item->edje, "FLHOC/menu/item/package.name",
        package->name);
    item->action = _package_selected;
    item->priv = package;
    category_append_item (category, item);
  }

  category->priv = packages;

  category_set_items (category, NULL);

  return EINA_TRUE;
}

static Eina_Bool
_add_main_categories (FLHOC *flhoc, Menu *menu)
{
  Category *category = NULL;
  Category *homebrew = NULL;
  int i;

  category = category_new (menu, CATEGORY_TYPE_QUIT);
  category->action = _category_action_quit;
  menu_append_category (menu, category);
  /*
  category = category_new (menu, CATEGORY_TYPE_SETTINGS);
  menu_append_category (menu, category);
  item = item_new (category, ITEM_TYPE_ABOUT);
  category_append_item (category, item);
  item = item_new (category, ITEM_TYPE_HELP);
  category_append_item (category, item);
  item = item_new (category, ITEM_TYPE_WALLPAPER);
  category_append_item (category, item);
  category_set_items (category, NULL);
  */
  category = category_new (menu, CATEGORY_TYPE_THEME);
  menu_append_category (menu, category);

  homebrew = category_new (menu, CATEGORY_TYPE_HOMEBREW);
  if (flhoc->homebrew)
    _add_game_items (flhoc->homebrew, homebrew);
  menu_append_category (menu, homebrew);
  for (i = 0; i < MAX_USB_DEVICES; i++) {
    if (flhoc->usb_homebrew[i]) {
      category = category_new (menu, CATEGORY_TYPE_DEVICE_HOMEBREW);
      _add_game_items (flhoc->usb_homebrew[i], category);
      menu_append_category (menu, category);
    }
    if (flhoc->usb_packages[i]) {
      category = category_new (menu, CATEGORY_TYPE_DEVICE_PACKAGES);
      _add_package_items (flhoc->usb_packages[i], category);
      menu_append_category (menu, category);
    }
  }

  menu->categories_selection = eina_list_data_find_list (menu->categories, homebrew);

  menu_set_categories (menu, NULL);

  return EINA_TRUE;
}

static Eina_List *
_games_new (const char *path)
{
  Eina_List *dirlist;
  Eina_List *l;
  Eina_List *games = NULL;
  char *game_dir;

  dirlist = _get_dirlist (path, EINA_TRUE, EINA_FALSE, EINA_FALSE);
  EINA_LIST_FOREACH (dirlist, l, game_dir) {
    Game *game = NULL;
    Eina_Array *ea;
    char *param_sfo = NULL;

    param_sfo = _strdup_printf ("%s/PARAM.SFO", game_dir);
    if (_file_exists (param_sfo)) {
      game = calloc (1, sizeof(Game));
      game->path = strdup (game_dir);

      ea = eina_file_split (game_dir);
      game->name = strdup (eina_array_pop (ea));
      eina_array_free (ea);

      game->icon = _strdup_printf ("%s/ICON0.PNG", game->path);
      if (!_file_exists (game->icon)) {
        free (game->icon);
        game->icon = NULL;
      }
      game->picture = _strdup_printf ("%s/PIC1.PNG", game->path);
      if (!_file_exists (game->picture)) {
        free (game->picture);
        game->picture = NULL;
      }
      game->title = _param_sfo_get_string (param_sfo, "TITLE");
      if (game->title == NULL)
        game->title = strdup (game->name);

      games = eina_list_append (games, game);
    }
    free (param_sfo);
  }
  EINA_LIST_FREE (dirlist, game_dir)
      free (game_dir);

  return games;
}

static void
_games_free (Eina_List *games)
{
  Game *game = NULL;

  if (games == NULL)
    return;

  EINA_LIST_FREE (games, game) {
    free (game->path);
    free (game->name);
    free (game->title);
    free (game->icon);
    free (game->picture);
    free (game);
  }
}

static Eina_List *
_usb_themes_new (FLHOC *flhoc, const char *path)
{
  Eina_List *theme_files = NULL;
  Eina_List *l;
  Eina_List *themes = NULL;
  Theme *theme = NULL;
  char *theme_filename;

  theme_files = _get_dirlist (path, EINA_FALSE, EINA_TRUE, EINA_TRUE);
  EINA_LIST_FOREACH (theme_files, l, theme_filename) {
    if (eina_str_has_extension (theme_filename, ".edj") &&
        theme_file_is_valid (flhoc->evas, theme_filename)) {
      theme = theme_new (flhoc->evas, theme_filename);
      theme->installed = EINA_FALSE;
      if (load_theme (flhoc, theme) == EINA_TRUE) {
        main_window_free (theme->main_win);
        theme->main_win = NULL;
        themes = eina_list_append (themes, theme);
      } else {
        /* Error loading the theme */
        theme_free (theme);
      }
    }
  }
  EINA_LIST_FREE (theme_files, theme_filename)
      free (theme_filename);

  return themes;
}

static void
_usb_themes_free (Eina_List *themes)
{
  Theme *theme = NULL;

  if (themes == NULL)
    return;

  EINA_LIST_FREE (themes, theme) {
    theme_free (theme);
  }
}


static Eina_List *
_packages_new (const char *path)
{
  Eina_List *dirlist;
  Eina_List *l;
  Eina_List *packages = NULL;
  char *pkg_filename;

  dirlist = _get_dirlist (path, EINA_FALSE, EINA_TRUE, EINA_FALSE);
  EINA_LIST_FOREACH (dirlist, l, pkg_filename) {
    if (eina_str_has_extension (pkg_filename, ".pkg")) {
      Package *package = NULL;
      Eina_Array *ea;

      package = calloc (1, sizeof(Package));
      package->path = strdup (pkg_filename);

      ea = eina_file_split (pkg_filename);
      package->name = strdup (eina_array_pop (ea));
      eina_array_free (ea);

      packages = eina_list_append (packages, package);
    }
  }
  EINA_LIST_FREE (dirlist, pkg_filename)
      free (pkg_filename);

  return packages;
}

static void
_packages_free (Eina_List *packages)
{
  Package *package = NULL;

  if (packages == NULL)
    return;

  EINA_LIST_FREE (packages, package) {
    free (package->path);
    free (package->name);
    free (package);
  }
}

static Eina_Bool
_device_check_homebrew (FLHOC *flhoc, Menu *menu, Eina_List **list, char *path)
{
  if (*list == NULL) {
    *list = _games_new (path);
    if (*list) {
      Category *category = category_new (menu, CATEGORY_TYPE_DEVICE_HOMEBREW);

      printf ("USB device was inserted: %s\n", path);
      _add_game_items (*list, category);
      menu_append_category (menu, category);
      return EINA_TRUE;
    }
  } else {
    Eina_Iterator *dir_iter = NULL;

    dir_iter = eina_file_stat_ls (path);
    if (dir_iter) {
      eina_iterator_free (dir_iter);
    } else {
      Eina_List *l;
      Category *category = NULL, *c;

      printf ("USB device was removed: %s\n", path);
      EINA_LIST_FOREACH (menu->categories, l, c) {
        if (c->priv == *list) {
          category = c;
          break;
        }
      }
      if (category) {
        menu_delete_category (menu, category);
        _games_free (*list);
        *list = NULL;
        return EINA_TRUE;
      }
    }
  }

  return EINA_FALSE;
}


static Eina_Bool
_device_check_packages (FLHOC *flhoc, Menu *menu, Eina_List **list,
    char *path1, char *path2)
{
  if (*list == NULL) {
    Eina_List *list1 = _packages_new (path1);
    Eina_List *list2 = _packages_new (path2);

    *list = eina_list_merge (list1, list2);
    if (*list) {
      Category *category = category_new (menu, CATEGORY_TYPE_DEVICE_PACKAGES);

      printf ("USB device was inserted: %s\n", path1);
      _add_package_items (*list, category);
      menu_append_category (menu, category);
      return EINA_TRUE;
    }
  } else {
    Eina_Iterator *dir_iter = NULL;

    dir_iter = eina_file_stat_ls (path1);
    if (dir_iter) {
      eina_iterator_free (dir_iter);
    } else {
      Eina_List *l;
      Category *category = NULL, *c;

      printf ("USB device was removed: %s\n", path1);
      EINA_LIST_FOREACH (menu->categories, l, c) {
        if (c->priv == *list) {
          category = c;
          break;
        }
      }
      if (category) {
        menu_delete_category (menu, category);
        _packages_free (*list);
        *list = NULL;
        return EINA_TRUE;
      }
    }
  }

  return EINA_FALSE;
}

static void
_device_check_themes (FLHOC *flhoc, Menu *menu, Eina_List **list, char *path)
{
  if (*list == NULL) {
    *list = _usb_themes_new (flhoc, path);
    if (*list) {
      _populate_themes (flhoc, flhoc->current_theme, EINA_TRUE);
      printf ("USB device was inserted: %s\n", path);
    }
  } else {
    Eina_Iterator *dir_iter = NULL;

    dir_iter = eina_file_stat_ls (path);
    if (dir_iter) {
      eina_iterator_free (dir_iter);
    } else {
      printf ("USB device was removed : %s\n", path);
      _usb_themes_free (*list);
      *list = NULL;
      _populate_themes (flhoc, flhoc->current_theme, EINA_TRUE);
    }
  }
}

static Eina_Bool
_device_check_cb (void *data)
{
  FLHOC *flhoc = data;
  Menu *menu = flhoc->current_theme->main_win->menu;
  char dev_path[1024], path[1024];
  Eina_Bool changed = EINA_FALSE;
  int i;

  for (i = 0; i < MAX_USB_DEVICES; i++) {
    snprintf (path, sizeof(path), USB_HOMEBREW_DIRECTORY_FMT, i);
    changed |=  _device_check_homebrew (flhoc, menu, &(flhoc->usb_homebrew[i]),
        path);

    snprintf (dev_path, sizeof(dev_path), DEV_USB_FMT, i);
    snprintf (path, sizeof(path), USB_PACKAGES_DIRECTORY_FMT, i);
    changed |= _device_check_packages (flhoc, menu, &(flhoc->usb_packages[i]),
        dev_path, path);

    snprintf (path, sizeof(path), USB_THEMES_DIRECTORY_FMT, i);
    _device_check_themes (flhoc, menu, &(flhoc->usb_themes[i]), path);

  }

  if (changed)
    menu_set_categories (menu, "");

  return ECORE_CALLBACK_RENEW;
}

static void
_populate_games (FLHOC *flhoc)
{
  char buffer[1024];
  int i;

  flhoc->homebrew = _games_new (HOMEBREW_DIRECTORY);
  for (i = 0; i < MAX_USB_DEVICES; i++) {
    snprintf (buffer, sizeof(buffer), USB_HOMEBREW_DIRECTORY_FMT, i);
    flhoc->usb_homebrew[i] = _games_new (buffer);
  }
}

static void
_flhoc_init (FLHOC *flhoc)
{
  Eina_List *theme_files = NULL;
  Eina_List *l;
  Theme *theme = NULL;
  char *theme_filename;

  memset (flhoc, 0, sizeof(FLHOC));
  flhoc->ee = ecore_evas_new (NULL, 0, 0, FLHOC_WIDTH, FLHOC_HEIGHT, NULL);
  flhoc->evas = ecore_evas_get (flhoc->ee);
  ecore_evas_show (flhoc->ee);
  ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, _ee_signal_exit, NULL);
  ecore_evas_callback_delete_request_set(flhoc->ee, _ee_delete_request);

  theme_files = _get_dirlist (THEMES_DIRECTORY, EINA_FALSE, EINA_TRUE, EINA_TRUE);
  EINA_LIST_FOREACH (theme_files, l, theme_filename) {
    if (eina_str_has_extension (theme_filename, ".edj") &&
        theme_file_is_valid (flhoc->evas, theme_filename)) {
      theme = theme_new (flhoc->evas, theme_filename);
      theme->installed = EINA_TRUE;
      if (load_theme (flhoc, theme) == EINA_TRUE) {
        main_window_free (theme->main_win);
        theme->main_win = NULL;
        flhoc->themes = eina_list_append (flhoc->themes, theme);
      } else {
        /* Error loading the theme */
        theme_free (theme);
      }
    }
  }
  EINA_LIST_FREE (theme_files, theme_filename)
      free (theme_filename);

  _populate_games (flhoc);

  flhoc->device_check_timer = ecore_timer_add (1.0, _device_check_cb, flhoc);
}

static void
_flhoc_deinit (FLHOC *flhoc)
{
  Theme *theme = NULL;
  int i;

  EINA_LIST_FREE (flhoc->themes, theme)
    theme_free (theme);

  _games_free (flhoc->homebrew);
  for (i = 0; i < MAX_USB_DEVICES; i++)
    _games_free (flhoc->usb_homebrew[i]);

  ecore_timer_del (flhoc->device_check_timer);

  if (flhoc->ee)
    ecore_evas_free (flhoc->ee);
}

int
main (int argc, char *argv[])
{
  FLHOC flhoc;
  int ret = 1;

  if (!eina_init ()) {
    printf ("Error setting up eina\n");
    return 1;
  }
  if (!ecore_init ()) {
    printf ("Error setting up ecore\n");
    goto eina_shutdown;
  }
  ecore_app_args_set (argc, (const char **)argv);
  if (!ecore_evas_init ()) {
    printf ("Error setting up ecore_evas\n");
    goto ecore_shutdown;
  }
  if (!edje_init ()) {
    printf ("Error setting up edje\n");
    goto ecore_evas_shutdown;
  }

  _flhoc_init (&flhoc);

  if (flhoc.themes == NULL) {
    printf ("No valid themes found\n");
    goto edje_shutdown;
  }

  set_current_theme (&flhoc, flhoc.themes->data);

  ecore_main_loop_begin ();

  ret = 0;

 edje_shutdown:
  _flhoc_deinit (&flhoc);

  edje_shutdown ();
 ecore_evas_shutdown:
  ecore_evas_shutdown ();
 ecore_shutdown:
  ecore_shutdown ();
 eina_shutdown:
  eina_shutdown ();

  return ret;
}
