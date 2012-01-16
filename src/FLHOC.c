// 2011 Ninjas
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <Ecore.h>
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
  char *directory;
  Eina_List *games;
} HomebrewGames;

typedef struct {
  Category *homebrew;
  Category *usb_homebrew[MAX_USB_DEVICES];
  Category *usb_packages[MAX_USB_DEVICES];
} ThemePriv;

typedef struct {
  Ecore_Evas *ee;
  Evas *evas;
  Eina_List *themes;
  Theme *current_theme;
  Theme *preview_theme;
  HomebrewGames *homebrew;
  HomebrewGames *usb_homebrew[MAX_USB_DEVICES];
  Ecore_Timer *device_check_timer;
} FLHOC;

static int _add_game_items (HomebrewGames *games, Category *category);
static void _populate_games (FLHOC *flhoc);
static void _populate_themes (FLHOC *flhoc, Theme *current_theme);
static Eina_Bool _add_main_categories (FLHOC *flhoc, Menu *menu);
static Eina_Bool load_theme (FLHOC *flhoc, Theme *theme);
static Eina_Bool reload_theme (FLHOC *flhoc, Theme *theme);
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
  } else if (strcmp (ev->keyname, "f") == 0) {
    Ecore_Evas *ee = ecore_evas_ecore_evas_get (main_win->theme->evas);
    ecore_evas_fullscreen_set (ee, !ecore_evas_fullscreen_get (ee));
  } else if (strcmp (ev->keyname, "a") == 0) {
    Category *category;

    category = category_new (menu, CATEGORY_TYPE_DEVICE_HOMEBREW);
    if (flhoc->homebrew)
      _add_game_items (flhoc->homebrew, category);
    menu_append_category (menu, category);
    category = category_new (menu, CATEGORY_TYPE_DEVICE_PACKAGES);
    menu_append_category (menu, category);

    menu_set_categories (menu, "");
  } else if (strcmp (ev->keyname, "d") == 0) {
    Category *category;
    Eina_List *list;

    list = eina_list_last (menu->categories);
    category = eina_list_data_get (list);
    menu_delete_category (menu, category);

    menu_set_categories (menu, "");
  } else if (strcmp (ev->keyname, "s") == 0) {
    Category *category;

    category = eina_list_data_get (menu->categories_selection);
    menu_set_categories (menu, "reset");
    menu_delete_category (menu, category);

    menu_set_categories (menu, "");
  } else if (strcmp (ev->keyname, "u") == 0) {
    Eina_List *cur;
    Category *category;

    EINA_LIST_FOREACH (menu->categories, cur, category) {
      edje_object_part_unswallow (menu->edje, category->edje);
      evas_object_hide (category->edje);
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
    _theme_selection (item, EINA_FALSE);
    if (flhoc->preview_theme)
      theme_free (flhoc->preview_theme);
    flhoc->preview_theme = NULL;
    reload_theme (flhoc, new_theme);
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

  printf ("Game '%s' is launched!!!\n", game->title);
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

static Eina_Bool
reload_theme (FLHOC *flhoc, Theme *theme)
{
  if (theme->main_win) {
    main_window_free (theme->main_win);
    theme->main_win = NULL;
  }

  return load_theme (flhoc, theme);
}

static void
set_current_theme (FLHOC *flhoc, Theme *theme)
{
  Evas_Coord w, h;

  if (load_theme (flhoc, theme) == EINA_TRUE) {
    evas_output_size_get (flhoc->evas, &w, &h);
    evas_object_move (theme->main_win->edje, 0, 0);
    evas_object_resize (theme->main_win->edje, w, h);
    evas_object_show (theme->main_win->edje);

    ecore_evas_object_associate(flhoc->ee, theme->main_win->edje,
        ECORE_EVAS_OBJECT_ASSOCIATE_BASE);

    if (flhoc->current_theme) {
      evas_object_hide (flhoc->current_theme->main_win->edje);
      evas_object_event_callback_del(flhoc->current_theme->main_win->edje,
          EVAS_CALLBACK_KEY_DOWN, _key_down_cb);
      if (flhoc->current_theme->main_win) {
        main_window_free (flhoc->current_theme->main_win);
        flhoc->current_theme->main_win = NULL;
      }
    }

    /* Start the process */
    flhoc->current_theme = theme;
    _populate_themes (flhoc, theme);
    evas_object_event_callback_add(theme->main_win->edje,
        EVAS_CALLBACK_KEY_DOWN, _key_down_cb, flhoc);
    evas_object_focus_set (theme->main_win->edje, EINA_TRUE);
    main_window_init (theme->main_win);
  }
}

static void
_populate_themes (FLHOC *flhoc, Theme *current_theme)
{
  Eina_List *l, *l2;
  Theme *theme;
  Item *this_theme = NULL;
  Category *category;
  Item *item;

  EINA_LIST_FOREACH (current_theme->main_win->menu->categories, l, category) {
    if (category->type == CATEGORY_TYPE_THEME) {
      EINA_LIST_FREE (category->items, item)
          item_free (item);
      EINA_LIST_FOREACH (flhoc->themes, l2, theme) {
        item = item_new (category, ITEM_TYPE_THEME);
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
        if (flhoc->current_theme == theme)
          this_theme = item;
      }

      if (this_theme) {
        category->items_selection =
            eina_list_data_find_list (category->items, this_theme);
      }

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
  printf ("SFO header : %d entries (%X - %X)\n", h.num_entries,
      h.name_table_offset, h.data_table_offset);

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
_add_game_items (HomebrewGames *games, Category *category)
{
  Eina_List *l;
  Game *game;
  Item *item = NULL;

  EINA_LIST_FOREACH (games->games, l, game) {
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

static Eina_Bool
_add_main_categories (FLHOC *flhoc, Menu *menu)
{
  Category *category = NULL;
  Category *homebrew = NULL;
  Item *item = NULL;
  int i;

  category = category_new (menu, CATEGORY_TYPE_QUIT);
  category->action = _category_action_quit;
  menu_append_category (menu, category);
  category = category_new (menu, CATEGORY_TYPE_SETTINGS);
  menu_append_category (menu, category);
  item = item_new (category, ITEM_TYPE_ABOUT);
  category_append_item (category, item);
  item = item_new (category, ITEM_TYPE_HELP);
  category_append_item (category, item);
  item = item_new (category, ITEM_TYPE_GAME);
  edje_object_part_text_set (item->edje, "FLHOC/menu/item/game.title",
      "Press 'A' to add icons and 'D' or 'S' to delete them");
  category_append_item (category, item);
  item = item_new (category, ITEM_TYPE_GAME);
  edje_object_part_text_set (item->edje, "FLHOC/menu/item/game.title",
      "Press 'F' for fullscreen and 'Q' to exit");
  category_append_item (category, item);
  item = item_new (category, ITEM_TYPE_INSTALL_THEME);
  category_append_item (category, item);
  item = item_new (category, ITEM_TYPE_WALLPAPER);
  category_append_item (category, item);
  category_set_items (category, NULL);
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
  }

  menu->categories_selection = eina_list_data_find_list (menu->categories, homebrew);

  menu_set_categories (menu, NULL);

  return EINA_TRUE;
}

static HomebrewGames *
_games_new (const char *path)
{
  HomebrewGames *hb = calloc (1, sizeof(HomebrewGames));
  Eina_List *dirlist;
  Eina_List *l;
  char *game_dir;

  dirlist = _get_dirlist (path, EINA_TRUE, EINA_FALSE, EINA_FALSE);
  if (dirlist == NULL) {
    free (hb);
    return NULL;
  }
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

      hb->games = eina_list_append (hb->games, game);
    }
    free (param_sfo);
  }
  EINA_LIST_FREE (dirlist, game_dir)
      free (game_dir);

  if (hb->games == NULL) {
    free (hb);
    return NULL;
  }

  hb->directory = strdup (path);

  return hb;
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
_games_free (HomebrewGames *games)
{
  Game *game = NULL;

  if (games == NULL)
    return;

  EINA_LIST_FREE (games->games, game) {
    free (game->path);
    free (game->name);
    free (game->title);
    free (game->icon);
    free (game->picture);
    free (game);
  }
  free (games->directory);
}

static Eina_Bool
_device_check_cb (void *data)
{
  FLHOC *flhoc = data;
  Menu *menu = flhoc->current_theme->main_win->menu;
  char buffer[1024];
  Eina_Bool changed = EINA_FALSE;
  int i;

  for (i = 0; i < MAX_USB_DEVICES; i++) {
    snprintf (buffer, sizeof(buffer), USB_HOMEBREW_DIRECTORY_FMT, i);
    if (flhoc->usb_homebrew[i] == NULL) {
      flhoc->usb_homebrew[i] = _games_new (buffer);
      if (flhoc->usb_homebrew[i]) {
        Category *category = category_new (menu, CATEGORY_TYPE_DEVICE_HOMEBREW);

        printf ("USB device %d was inserted\n", i);
        _add_game_items (flhoc->usb_homebrew[i], category);
        menu_append_category (menu, category);
        changed = EINA_TRUE;
      }
    } else {
      Eina_Iterator *dir_iter = NULL;

      dir_iter = eina_file_stat_ls (buffer);
      if (dir_iter) {
        eina_iterator_free (dir_iter);
      } else {
        Eina_List *l;
        Category *category = NULL, *c;

        printf ("USB device %d was removed\n", i);
        EINA_LIST_FOREACH (menu->categories, l, c) {
          if (c->priv == flhoc->usb_homebrew[i]) {
            category = c;
            break;
          }
        }
        if (category) {
          menu_delete_category (menu, category);
          _games_free (flhoc->usb_homebrew[i]);
          flhoc->usb_homebrew[i] = NULL;
          changed = EINA_TRUE;
        }
      }
    }
  }

  if (changed)
    menu_set_categories (menu, "");

  return ECORE_CALLBACK_RENEW;
}

static void
_flhoc_init (FLHOC *flhoc)
{
  Eina_List *theme_files;
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
