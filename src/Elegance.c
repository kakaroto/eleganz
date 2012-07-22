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
#include <Exquisite.h>

#include "common.h"
#include "menu.h"
#include "pkg.h"
#include "keys.h"

#ifdef __lv2ppu__
#define DEV_HDD0 "/dev_hdd0/"
#define DEV_USB_FMT "/dev_usb%03d/"
#define THEMES_DIRECTORY "/dev_hdd0/game/ELEGANCE0/USRDIR/data/themes/"
#define GAMES_DIRECTORY "/dev_hdd0/game/"
#define HOMEBREW_DIRECTORY "/dev_hdd0/game/ELEGANCE0/USRDIR/HOMEBREW/"
#define USB_HOMEBREW_DIRECTORY_FMT "/dev_usb%03d/PS3/HOMEBREW/"
#define USB_PACKAGES_DIRECTORY_FMT "/dev_usb%03d/PS3/PACKAGES/"
#define USB_THEMES_DIRECTORY_FMT "/dev_usb%03d/PS3/THEMES/"
#define KEYS_PATH "/dev_hdd0/game/ELEGANCE0/USRDIR/keys.conf"
#define ELEGANCE_WIDTH 1280
#define ELEGANCE_HEIGHT 720
#else
#define DEV_HDD0 "./dev_hdd0/"
#define DEV_USB_FMT "./dev_usb%03d/"
#define THEMES_DIRECTORY "data/themes/"
#define GAMES_DIRECTORY "dev_hdd0/game/"
#define HOMEBREW_DIRECTORY "dev_hdd0/game/ELEGANCE0/USRDIR/HOMEBREW/"
#define USB_HOMEBREW_DIRECTORY_FMT "dev_usb%03d/PS3/HOMEBREW/"
#define USB_PACKAGES_DIRECTORY_FMT "dev_usb%03d/PS3/PACKAGES/"
#define USB_THEMES_DIRECTORY_FMT "dev_usb%03d/PS3/THEMES/"
#define KEYS_PATH "data/keys.conf"
#define ELEGANCE_WIDTH 640
#define ELEGANCE_HEIGHT 480
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
  Evas_Object *exquisite;
  Eina_Bool exquisite_done;
  Eina_List *themes;
  Theme *current_theme;
  Theme *preview_theme;
  Eina_List *games;
  Eina_List *homebrew;
  Eina_List *usb_homebrew[MAX_USB_DEVICES];
  Eina_List *usb_packages[MAX_USB_DEVICES];
  Eina_List *usb_themes[MAX_USB_DEVICES];
  Ecore_Timer *device_check_timer;
} Elegance;

static Eina_List *_games_new (const char *path);
static void _games_free (Eina_List *games);
static int _add_game_items (Eina_List *games, Category *category);
static void _populate_themes (Elegance *self, Theme *current_theme, Eina_Bool only_usb);
static Eina_Bool _add_main_categories (Elegance *self, Menu *menu);
static Eina_Bool load_theme (Elegance *self, Theme *theme);
static void set_current_theme (Elegance *self, Theme *theme);
static Eina_Bool _device_check_cb (void *data);
static void _exquisite_default_exit_cb (void *data);
static Evas_Object *_exquisite_new (Elegance *self, const char *title, const char *message);
static void _menu_ready_cb (Menu *menu);

/* Generic callbacks */
static void
_key_down_cb (void *data, Evas *e, Evas_Object *obj, void *event)
{
  Elegance *self = data;
  MainWindow *main_win = self->current_theme->main_win;
  Menu *menu = main_win->menu;
  Evas_Event_Key_Down *ev = event;

  _menu_ready_cb (menu);
  //printf ("Key Down: '%s' - '%s' - '%s' - '%s'\n", ev->keyname, ev->key, ev->string, ev->compose);
  if (strcmp (ev->keyname, "f") == 0) {
    Ecore_Evas *ee = ecore_evas_ecore_evas_get (main_win->theme->evas);
    ecore_evas_fullscreen_set (ee, !ecore_evas_fullscreen_get (ee));
  } else if (strcmp (ev->keyname, "Escape") == 0 ||
      strcmp (ev->keyname, "q") == 0) {
    ecore_main_loop_quit();
  } else if (strcmp (ev->keyname, "Return") == 0 ||
      strcmp (ev->keyname, "Cross") == 0) {
    Category *category = menu_get_selected_category (menu);
    Item *item = category_get_selected_item (category);

    if (self->exquisite) {
      if (self->exquisite_done)
        exquisite_object_exit (self->exquisite);
    } else if (item) {
      if (item->action)
        item->action (item);
    } else {
      if (category->action)
        category->action (category);
    }
  } else if (!self->exquisite) {
    if (strcmp (ev->keyname, menu->category_previous) == 0) {
      menu_scroll_previous (menu);
    } else if (strcmp (ev->keyname, menu->category_next) == 0) {
      menu_scroll_next (menu);
    } else if (strcmp (ev->keyname, menu->item_previous) == 0) {
      category_scroll_previous (menu_get_selected_category (menu));
    } else if (strcmp (ev->keyname, menu->item_next) == 0) {
      category_scroll_next (menu_get_selected_category (menu));
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

  edje_object_signal_callback_add (main_win->edje, "Elegance/primary,unset,end", "*",
      _quit_ready_cb, main_win);
  edje_object_signal_emit (main_win->edje, "Elegance/primary,unset", "");
}

static void
_theme_selection (Item *item, Eina_Bool selected)
{
  Theme *theme = item->priv;
  Elegance *self = NULL;
  Evas_Object *obj;

  self = evas_object_data_get (item->category->menu->main_win->edje, "Elegance");
  obj = edje_object_part_swallow_get (item->edje, "Elegance/menu/item/theme.preview");
  if (obj) {
    edje_object_part_unswallow (item->edje, obj);
    evas_object_hide (obj);
    theme_free (self->preview_theme);
    self->preview_theme = NULL;
  }
  if (selected) {
    self->preview_theme = theme_new (self->evas, theme->edje_file);
    self->preview_theme->priv = self;
    if (load_theme (self, self->preview_theme) == EINA_FALSE) {
      theme_free (self->preview_theme);
      self->preview_theme = NULL;
    }
  }
  if (self->preview_theme) {
    edje_object_part_swallow (item->edje,
        "Elegance/menu/item/theme.preview", self->preview_theme->main_win->edje);
    main_window_init (self->preview_theme->main_win);
  }
}

static void
_continue_set_theme (void *data)
{
  Theme *new_theme = data;
  Elegance *self = new_theme->priv;

  if (self->preview_theme)
    theme_free (self->preview_theme);
  self->preview_theme = NULL;

  set_current_theme (self, new_theme);
  _exquisite_default_exit_cb (self);
}

static void
_theme_selected (Item *item)
{
  Elegance *self;
  Theme *new_theme = item->priv;

  self = evas_object_data_get (item->category->menu->main_win->edje, "Elegance");
  if (new_theme != NULL && new_theme != self->current_theme) {
    if (!new_theme->installed) {
      Eina_List *l;
      char *old_path = strdup (new_theme->edje_file);
      Theme *old_theme = new_theme;
      Eina_Bool installed = EINA_FALSE;

      new_theme = NULL;
      EINA_LIST_FOREACH (self->themes, l, new_theme) {
        if (strcmp (new_theme->name, old_theme->name) == 0)
          break;
      }
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
          new_theme->priv = self;
          new_theme->installed = EINA_TRUE;
          self->themes = eina_list_append (self->themes, new_theme);
          installed = EINA_TRUE;
        }
      }

      if (installed) {
        printf ("New theme installed successfully to %s\n",
            new_theme->edje_file);
        _theme_selection (item, EINA_FALSE);
        _exquisite_new (self, "Theme Install", "Theme Installed successfully");
        exquisite_object_exit_callback_set (self->exquisite,
            _continue_set_theme, new_theme);
        exquisite_object_progress_set (self->exquisite, 1.0);
        exquisite_object_status_set (self->exquisite,
            exquisite_object_text_add (self->exquisite, "Copying theme file"),
            "OK", EXQUISITE_STATUS_TYPE_SUCCESS);
        exquisite_object_status_set (self->exquisite,
            exquisite_object_text_add (self->exquisite, "Installing theme"),
            "OK", EXQUISITE_STATUS_TYPE_SUCCESS);
        self->exquisite_done = EINA_TRUE;
        return;
      } else {
        printf ("Unable to install new theme\n");
        _exquisite_new (self, "Theme Install", "Error Installing theme");
        exquisite_object_status_set (self->exquisite,
            exquisite_object_text_add (self->exquisite, "Copying theme file"),
            "FAIL", EXQUISITE_STATUS_TYPE_FAILURE);
        self->exquisite_done = EINA_TRUE;
        return;
      }
    }
    _theme_selection (item, EINA_FALSE);
    if (self->preview_theme)
      theme_free (self->preview_theme);
    self->preview_theme = NULL;

    set_current_theme (self, new_theme);
  }
}

static void
_game_selection (Item *item, Eina_Bool selected)
{
  Game *game = item->priv;
  MainWindow *main_win = item->category->menu->main_win;
  Secondary *secondary = NULL;

  printf ("Game '%s' is %sselected\n", game->title, selected?"":"de");

  if (main_win->secondary)
    main_window_set_secondary (main_win, NULL);

  if (selected) {
    secondary = secondary_new (main_win, "Elegance/secondary/game");
    if (edje_object_part_exists (secondary->edje, "Elegance/secondary/game/title"))
      edje_object_part_text_set (secondary->edje,
          "Elegance/secondary/game/title", game->title);
    if (game->icon &&
        edje_object_part_exists (secondary->edje, "Elegance/secondary/game/icon")) {
      Evas_Object *img = (Evas_Object *) edje_object_part_object_get (
          secondary->edje, "Elegance/secondary/game/icon");
      evas_object_image_file_set (img, game->icon, NULL);
    }
    if (game->picture &&
        edje_object_part_exists (secondary->edje, "Elegance/secondary/game/picture")) {
      Evas_Object *img = (Evas_Object *) edje_object_part_object_get (
          secondary->edje, "Elegance/secondary/game/picture");
      evas_object_image_file_set (img, game->picture, NULL);
    }

    main_window_set_secondary (main_win, secondary);
  }
}

static void
_game_selected (Item *item)
{
  Game *game = item->priv;
  Elegance *self;

  self = evas_object_data_get (item->category->menu->main_win->edje, "Elegance");

  /* TODO: do it! */
  printf ("Game '%s' is launched!!!\n", game->title);
  _exquisite_new (self, "Homebrew launcher", game->title);
  exquisite_object_pulsate (self->exquisite);
  exquisite_object_status_set (self->exquisite,
      exquisite_object_text_add (self->exquisite, "Launching game!"),
      "FAIL", EXQUISITE_STATUS_TYPE_FAILURE);
  exquisite_object_text_add (self->exquisite, "Oh right, I didn't implement it yet!");
  exquisite_object_status_set (self->exquisite,
      exquisite_object_text_add (self->exquisite, "Please do it, thanks!!"),
      "OK", EXQUISITE_STATUS_TYPE_SUCCESS);
  self->exquisite_done = EINA_TRUE;
}

static Eina_Bool
_ecore_sleep_cb (void *data)
{
  Eina_Bool *done = data;

  *done = EINA_TRUE;

  return EINA_FALSE;
}

static void
_ecore_sleep (double delay)
{
  Eina_Bool done = EINA_FALSE;

  ecore_timer_add (delay, _ecore_sleep_cb, &done);
  while (!done)
    ecore_main_loop_iterate ();
}

static int
_pkg_install (Elegance *self, Package *package)
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
  u64 current_size = 0;
  u64 total_size = 0;
  int last_text_id = -1;

  exquisite_object_message_set (self->exquisite, "Inspecting package!");

  last_text_id = exquisite_object_text_add (self->exquisite, "Opening package");
  _ecore_sleep (1.0);
  if (!pkg_open (package->path, &in, &header, &files)) {
    exquisite_object_status_set (self->exquisite,
        last_text_id, "FAIL", EXQUISITE_STATUS_TYPE_FAILURE);
    exquisite_object_message_set (self->exquisite, "Error extracting package!");
    exquisite_object_progress_set (self->exquisite, 0);
    return FALSE;
  }
  exquisite_object_status_set (self->exquisite,
      last_text_id, "OK", EXQUISITE_STATUS_TYPE_SUCCESS);

  last_text_id = exquisite_object_text_add (self->exquisite, "Listing files");
  _ecore_sleep (0.25);
  for (i = 0; i < header.item_count; i++)
    total_size += files[i].data_size;

  exquisite_object_status_set (self->exquisite,
      last_text_id, "OK", EXQUISITE_STATUS_TYPE_SUCCESS);

  last_text_id = exquisite_object_text_add (self->exquisite,
      "Preparing destination");
  _ecore_sleep (0.25);
  if (strcmp (header.contentid, "UP0001-ELEGANCE0-0000000000000000") == 0)
    snprintf (out_dir, sizeof(out_dir), "%s/ELEGANCE0", GAMES_DIRECTORY);
  else
    snprintf (out_dir, sizeof(out_dir), "%s/%s", HOMEBREW_DIRECTORY,
        header.contentid);
  mkdir_recursive (out_dir);
  exquisite_object_status_set (self->exquisite,
      last_text_id, "OK", EXQUISITE_STATUS_TYPE_SUCCESS);

  exquisite_object_message_set (self->exquisite, "Extracting files!");
  exquisite_object_text_add (self->exquisite, "Extracting files : ");
  _ecore_sleep (0.25);
  for (i = 0; i < header.item_count; i++) {
    int j;

    paged_file_seek (&in, files[i].filename_offset + header.data_offset);
    pkg_file_path = malloc (files[i].filename_size + 1);
    paged_file_read (&in, pkg_file_path, files[i].filename_size);
    pkg_file_path[files[i].filename_size] = 0;

    last_text_id = exquisite_object_text_add (self->exquisite, pkg_file_path);
    _ecore_sleep (0.25);

    snprintf (path, sizeof(path), "%s/%s", out_dir, pkg_file_path);
    if ((files[i].flags & 0xFF) == 4) {
      mkdir_recursive (path);
      exquisite_object_status_set (self->exquisite,
          last_text_id, "OK", EXQUISITE_STATUS_TYPE_SUCCESS);
    } else {
      j = strlen (path);
      while (j > 0 && path[j] != '/') j--;
      if (j > 0) {
        path[j] = 0;
        mkdir_recursive (path);
        path[j] = '/';
      }
      paged_file_seek (&in, files[i].data_offset + header.data_offset);
      if (!paged_file_open (&out, path, FALSE)) {
        exquisite_object_status_set (self->exquisite,
            last_text_id, "FAIL", EXQUISITE_STATUS_TYPE_FAILURE);
        ret = FALSE;
        break;
      } else {
        paged_file_splice (&out, &in, files[i].data_size);
        paged_file_close (&out);

        exquisite_object_status_set (self->exquisite,
            last_text_id, "OK", EXQUISITE_STATUS_TYPE_SUCCESS);
      }
    }
    current_size += files[i].data_size;
    exquisite_object_progress_set (self->exquisite,
        (double) current_size / (double) total_size);
  }

  if (ret) {
    exquisite_object_message_set (self->exquisite,
        "Package extracted successfully!");
    exquisite_object_text_add (self->exquisite, "Done!");
  } else {
    exquisite_object_message_set (self->exquisite, "Error extracting package!");
  }
  paged_file_close (&in);
  return ret;
}

static void
_package_selected (Item *item)
{
  Package *package = item->priv;
  Elegance *self;
  char buffer[1024];

  self = evas_object_data_get (item->category->menu->main_win->edje, "Elegance");

  printf ("Package '%s' is to be installed!!!\n", package->name);
  snprintf (buffer, sizeof(buffer), "Installing package %s", package->name);
  _exquisite_new (self, buffer, "Preparing package");
  exquisite_object_pulsate (self->exquisite);
  if (_pkg_install (self, package)) {
    Menu *menu = self->current_theme->main_win->menu;
    Category *category;
    Item *item;
    Eina_List *l;

    _games_free (self->homebrew);
    self->homebrew = _games_new (HOMEBREW_DIRECTORY);
    EINA_LIST_FOREACH (menu->categories, l, category) {
      if (category->type == CATEGORY_TYPE_HOMEBREW) {
        EINA_LIST_FREE (category->items, item)
          item_free (item);
        category->items_selection = NULL;
        _add_game_items (self->homebrew, category);
      }
    }
  }
  self->exquisite_done = EINA_TRUE;
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

static void
_exquisite_default_exit_cb (void *data)
{
  Elegance *self = data;
  MainWindow *main_win = self->current_theme->main_win;

  main_window_set_secondary (main_win, NULL);
  self->exquisite = NULL;
}

static Evas_Object *
_exquisite_new (Elegance *self, const char *title, const char *message)
{
  MainWindow *main_win = self->current_theme->main_win;
  Secondary *secondary;

  secondary = secondary_exquisite_new (main_win,
      self->current_theme->edje_file);
  self->exquisite = secondary->edje;

  exquisite_object_exit_callback_set (self->exquisite,
      _exquisite_default_exit_cb, self);

  exquisite_object_title_set (self->exquisite, title);
  exquisite_object_message_set (self->exquisite, message);
  main_window_set_secondary (main_win, secondary);

  self->exquisite_done = EINA_FALSE;
  return self->exquisite;
}

/* Theme functions */

static void
_menu_ready_cb (Menu *menu)
{
  Elegance *self = evas_object_data_get (menu->main_win->edje, "Elegance");

  if (!self->device_check_timer)
    self->device_check_timer = ecore_timer_add (1.0, _device_check_cb, self);
  _device_check_cb (self);
}

static Eina_Bool
load_theme (Elegance *self, Theme *theme)
{
  theme->main_win = main_window_new (theme);
  if (theme->main_win == NULL)
    return EINA_FALSE;

  evas_object_data_set (theme->main_win->edje, "Elegance", self);
  evas_object_data_set (theme->main_win->edje, "MainWindow", theme->main_win);

  theme->main_win->menu = menu_new (theme->main_win);
  theme->main_win->menu->ready = _menu_ready_cb;
  if (theme->main_win->menu == NULL)
    return EINA_FALSE;

  return _add_main_categories (self, theme->main_win->menu);
}

static void
set_current_theme (Elegance *self, Theme *theme)
{
  Evas_Coord w, h;

  if (theme->main_win) {
    /* Prevent our main window from closing */
    if (theme == self->current_theme)
      ecore_evas_object_dissociate(self->ee, theme->main_win->edje);

    main_window_free (theme->main_win);
    theme->main_win = NULL;
  }

  if (load_theme (self, theme) == EINA_TRUE) {
    evas_output_size_get (self->evas, &w, &h);
    evas_object_move (theme->main_win->edje, 0, 0);
    evas_object_resize (theme->main_win->edje, w, h);
    evas_object_show (theme->main_win->edje);

    ecore_evas_object_associate(self->ee, theme->main_win->edje,
        ECORE_EVAS_OBJECT_ASSOCIATE_BASE);

    if (self->current_theme && self->current_theme->main_win) {
      evas_object_event_callback_del(self->current_theme->main_win->edje,
          EVAS_CALLBACK_KEY_DOWN, _key_down_cb);
      if (self->current_theme != theme) {
        evas_object_hide (self->current_theme->main_win->edje);
        main_window_free (self->current_theme->main_win);
        self->current_theme->main_win = NULL;
      }
    }

    /* Start the process */
    self->current_theme = theme;
    _populate_themes (self, theme, EINA_FALSE);
    evas_object_event_callback_add(theme->main_win->edje,
        EVAS_CALLBACK_KEY_DOWN, _key_down_cb, self);
    evas_object_focus_set (theme->main_win->edje, EINA_TRUE);
    main_window_init (theme->main_win);
  }
}

static Item *
_add_theme (Elegance *self, Category *category, Theme *theme, ItemType type)
{
  Item *item;

  item = item_new (category, type);
  edje_object_part_text_set (item->edje,
      "Elegance/menu/item/theme.name", theme->name);
  edje_object_part_text_set (item->edje,
      "Elegance/menu/item/theme.version", theme->version);
  edje_object_part_text_set (item->edje,
      "Elegance/menu/item/theme.author", theme->author);
  edje_object_part_text_set (item->edje,
      "Elegance/menu/item/theme.description", theme->description);
  item->selection = _theme_selection;
  item->action = _theme_selected;
  item->priv = theme;
  category_append_item (category, item);

  return item;
}

static void
_populate_themes (Elegance *self, Theme *current_theme, Eina_Bool only_usb)
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
        EINA_LIST_FOREACH (self->themes, l2, theme) {
          item = _add_theme (self, category, theme, ITEM_TYPE_THEME);
          if (self->current_theme == theme)
            category->items_selection =
                eina_list_data_find_list (category->items, item);
        }
      }
      for (i = 0; i < MAX_USB_DEVICES; i++) {
        EINA_LIST_FOREACH (self->usb_themes[i], l2, theme)
            _add_theme (self, category, theme, ITEM_TYPE_USB_THEME);
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
    edje_object_part_text_set (item->edje, "Elegance/menu/item/game.title", game->title);
    if (game->icon) {
      img = (Evas_Object *) edje_object_part_object_get (item->edje, "Elegance/menu/item/game.icon");
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
    edje_object_part_text_set (item->edje, "Elegance/menu/item/package.name",
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
_add_main_categories (Elegance *self, Menu *menu)
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

  category = category_new (menu, CATEGORY_TYPE_GAMES);
  if (self->games)
    _add_game_items (self->games, category);
  menu_append_category (menu, category);
  category = category_new (menu, CATEGORY_TYPE_HOMEBREW);
  if (self->homebrew)
    _add_game_items (self->homebrew, category);
  menu_append_category (menu, category);
  homebrew = category;
  for (i = 0; i < MAX_USB_DEVICES; i++) {
    if (self->usb_homebrew[i]) {
      category = category_new (menu, CATEGORY_TYPE_DEVICE_HOMEBREW);
      _add_game_items (self->usb_homebrew[i], category);
      menu_append_category (menu, category);
    }
    if (self->usb_packages[i]) {
      category = category_new (menu, CATEGORY_TYPE_DEVICE_PACKAGES);
      _add_package_items (self->usb_packages[i], category);
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
_usb_themes_new (Elegance *self, const char *path)
{
  Eina_List *theme_files = NULL;
  Eina_List *l;
  Eina_List *themes = NULL;
  Theme *theme = NULL;
  char *theme_filename;

  theme_files = _get_dirlist (path, EINA_FALSE, EINA_TRUE, EINA_TRUE);
  EINA_LIST_FOREACH (theme_files, l, theme_filename) {
    if (eina_str_has_extension (theme_filename, ".edj") &&
        theme_file_is_valid (self->evas, theme_filename)) {
      theme = theme_new (self->evas, theme_filename);
      theme->priv = self;
      theme->installed = EINA_FALSE;
      if (load_theme (self, theme) == EINA_TRUE) {
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
_device_check_homebrew (Elegance *self, Menu *menu, Eina_List **list, char *path)
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
_device_check_packages (Elegance *self, Menu *menu, Eina_List **list,
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
_device_check_themes (Elegance *self, Menu *menu, Eina_List **list, char *path)
{
  if (*list == NULL) {
    *list = _usb_themes_new (self, path);
    if (*list) {
      _populate_themes (self, self->current_theme, EINA_TRUE);
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
      _populate_themes (self, self->current_theme, EINA_TRUE);
    }
  }
}

static Eina_Bool
_device_check_cb (void *data)
{
  Elegance *self = data;
  Menu *menu = self->current_theme->main_win->menu;
  char dev_path[1024], path[1024];
  Eina_Bool changed = EINA_FALSE;
  int i;

  for (i = 0; i < MAX_USB_DEVICES; i++) {
    snprintf (path, sizeof(path), USB_HOMEBREW_DIRECTORY_FMT, i);
    changed |=  _device_check_homebrew (self, menu, &(self->usb_homebrew[i]),
        path);

    snprintf (dev_path, sizeof(dev_path), DEV_USB_FMT, i);
    snprintf (path, sizeof(path), USB_PACKAGES_DIRECTORY_FMT, i);
    changed |= _device_check_packages (self, menu, &(self->usb_packages[i]),
        dev_path, path);

    snprintf (path, sizeof(path), USB_THEMES_DIRECTORY_FMT, i);
    _device_check_themes (self, menu, &(self->usb_themes[i]), path);

  }

  if (changed)
    menu_set_categories (menu, "");

  return ECORE_CALLBACK_RENEW;
}

static void
_elegance_init (Elegance *self)
{
  Eina_List *theme_files = NULL;
  Eina_List *l;
  Theme *theme = NULL;
  char *theme_filename;

  keys_set_path (KEYS_PATH);

  memset (self, 0, sizeof(Elegance));
  self->ee = ecore_evas_new (NULL, 0, 0, ELEGANCE_WIDTH, ELEGANCE_HEIGHT, NULL);
  self->evas = ecore_evas_get (self->ee);
  ecore_evas_show (self->ee);
  ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, _ee_signal_exit, NULL);
  ecore_evas_callback_delete_request_set(self->ee, _ee_delete_request);

  theme_files = _get_dirlist (THEMES_DIRECTORY, EINA_FALSE, EINA_TRUE, EINA_TRUE);
  EINA_LIST_FOREACH (theme_files, l, theme_filename) {
    if (eina_str_has_extension (theme_filename, ".edj") &&
        theme_file_is_valid (self->evas, theme_filename)) {
      theme = theme_new (self->evas, theme_filename);
      theme->priv = self;
      theme->installed = EINA_TRUE;
      if (load_theme (self, theme) == EINA_TRUE) {
        main_window_free (theme->main_win);
        theme->main_win = NULL;
        self->themes = eina_list_append (self->themes, theme);
      } else {
        /* Error loading the theme */
        theme_free (theme);
      }
    }
  }
  EINA_LIST_FREE (theme_files, theme_filename)
      free (theme_filename);

  self->games = _games_new (GAMES_DIRECTORY);
  self->homebrew = _games_new (HOMEBREW_DIRECTORY);
}

static void
_elegance_deinit (Elegance *self)
{
  Theme *theme = NULL;
  int i;

  EINA_LIST_FREE (self->themes, theme)
    theme_free (theme);

  _games_free (self->games);
  _games_free (self->homebrew);
  for (i = 0; i < MAX_USB_DEVICES; i++)
    _games_free (self->usb_homebrew[i]);

  if (self->device_check_timer)
    ecore_timer_del (self->device_check_timer);
  self->device_check_timer = NULL;

  if (self->ee)
    ecore_evas_free (self->ee);

  self->exquisite = NULL;
}

int
main (int argc, char *argv[])
{
  Elegance self;
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

  _elegance_init (&self);

  if (self.themes == NULL) {
    printf ("No valid themes found\n");
    goto edje_shutdown;
  }

  set_current_theme (&self, self.themes->data);

  ecore_main_loop_begin ();

  ret = 0;

 edje_shutdown:
  _elegance_deinit (&self);

  edje_shutdown ();
 ecore_evas_shutdown:
  ecore_evas_shutdown ();
 ecore_shutdown:
  ecore_shutdown ();
 eina_shutdown:
  eina_shutdown ();

  return ret;
}
