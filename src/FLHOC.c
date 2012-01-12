// 2011 Ninjas
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>

#include "FLHOC_menu.h"

typedef struct {
  Ecore_Evas *ee;
  Evas *evas;
  Eina_List *themes;
  Theme *current_theme;
  Theme *preview_theme;
} FLHOC;

static int _add_game_items (Category *category);

static void
_key_down_cb (void *data, Evas *e, Evas_Object *obj, void *event)
{
  MainWindow *main_win = data;
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
  } else if (strcmp (ev->keyname, "Return") == 0) {
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
    _add_game_items (category);
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

static void
_category_action_quit (Category *category)
{
  Menu *menu = category->menu;
  MainWindow *main_win = menu->main_win;

  edje_object_signal_callback_add (main_win->edje, "FLHOC/primary,unset,end", "*",
      _quit_ready_cb, main_win);
  edje_object_signal_emit (main_win->edje, "FLHOC/primary,unset", "");
}

static Eina_Bool
_ee_signal_exit(void* data, int ev_type, void* ev)
{
  ecore_main_loop_quit();
  return 0;
}


static int
_add_game_items (Category *category)
{
  Item *item = NULL;

  item = item_new (category, ITEM_TYPE_GAME);
  edje_object_part_text_set (item->edje, "FLHOC/menu/item/game.title", "Awesome game!");
  category_append_item (category, item);
  item = item_new (category, ITEM_TYPE_GAME);
  edje_object_part_text_set (item->edje, "FLHOC/menu/item/game.title", "Another awesome game!");
  category_append_item (category, item);
  item = item_new (category, ITEM_TYPE_GAME);
  edje_object_part_text_set (item->edje, "FLHOC/menu/item/game.title", "A cool game I guess!");
  category_append_item (category, item);
  item = item_new (category, ITEM_TYPE_GAME);
  edje_object_part_text_set (item->edje, "FLHOC/menu/item/game.title", "Some game!");
  category_append_item (category, item);
  item = item_new (category, ITEM_TYPE_GAME);
  edje_object_part_text_set (item->edje, "FLHOC/menu/item/game.title", "Homebrew app!!");
  category_append_item (category, item);

  category_set_items (category, NULL);

  return EINA_TRUE;
}

static void
_get_dirlist_internal (DIR *fd, const char *base, const char *subdir,
    Eina_List **list, Eina_Bool dirs, Eina_Bool files, Eina_Bool recursive)
{
  struct dirent *dirent = NULL;
  char path[1024];

  while (1) {
    dirent = readdir (fd);
    if (!dirent)
      break;
    if (strcmp (dirent->d_name, ".") == 0 ||
        strcmp (dirent->d_name, "..") == 0)
      continue;

    if (dirent->d_type == DT_DIR) {
      DIR *dir_fd = NULL;

      snprintf (path, sizeof(path), "%s/%s/%s", base, subdir, dirent->d_name);
      if (dirs) {
        *list = eina_list_append (*list, strdup (path));
      }
      if (recursive) {
        dir_fd = opendir(path);
        if (dir_fd) {
          snprintf (path, sizeof(path), "%s/%s", subdir, dirent->d_name);
          _get_dirlist_internal (dir_fd, base, path, list, dirs, files, recursive);
          closedir (dir_fd);
        }
      }
    } else if (files) {
      snprintf (path, sizeof(path), "%s/%s/%s", base, subdir, dirent->d_name);
      *list = eina_list_append (*list, strdup (path));
    }
  }
}

static Eina_List *
_get_dirlist (const char *path, Eina_Bool dirs, Eina_Bool files, Eina_Bool recursive)
{
  DIR *dir_fd = NULL;
  Eina_List *list = NULL;

  dir_fd = opendir(path);
  if (!dir_fd)
    return NULL;

  _get_dirlist_internal (dir_fd, path, ".", &list, dirs, files, recursive);
  closedir (dir_fd);

  return list;
}

static Eina_Bool _add_main_categories (FLHOC *flhoc, Menu *menu);

static void
_theme_selection (Item *item, Eina_Bool selected)
{
  Theme *theme = item->priv;
  Evas_Object *obj;

  printf ("Theme '%s' %sselected\n", theme->name, selected?"":"de");

  obj = edje_object_part_swallow_get (item->edje, "FLHOC/menu/item/theme.preview");
  if (obj) {
    MainWindow *main_win = evas_object_data_get (obj, "MainWindow");

    edje_object_part_unswallow (item->edje, obj);
    main_window_free (main_win);
  }
  if (selected) {
    MainWindow *main_win = main_window_new (theme);
    main_win->menu = menu_new (main_win);
    _add_main_categories (NULL, main_win->menu);
    edje_object_part_swallow (item->edje,
        "FLHOC/menu/item/theme.preview", main_win->edje);
    evas_object_data_set (main_win->edje, "MainWindow", main_win);
    main_window_init (main_win);
  }
}

static void
_theme_selected (Item *item)
{
  FLHOC *flhoc;
  Category *category = item->category;
  Menu *menu = category->menu;
  MainWindow *main_win = menu->main_win;
  Theme *old_theme = NULL;
  Theme *new_theme = item->priv;
  Evas_Coord w, h;


  flhoc = evas_object_data_get (main_win->edje, "FLHOC");
  old_theme = flhoc->current_theme;
  flhoc->current_theme = new_theme;
  if (flhoc->current_theme == NULL) {
    flhoc->current_theme = old_theme;
    return;
  }

  _theme_selection (item, EINA_FALSE);
  new_theme->main_win = main_window_new (new_theme);

  evas_object_data_set (new_theme->main_win->edje, "FLHOC", flhoc);

  evas_output_size_get (flhoc->evas, &w, &h);
  evas_object_move (new_theme->main_win->edje, 0, 0);
  evas_object_resize (new_theme->main_win->edje, w, h);
  evas_object_show (new_theme->main_win->edje);

  ecore_evas_object_associate(flhoc->ee, new_theme->main_win->edje,
      ECORE_EVAS_OBJECT_ASSOCIATE_BASE);
  new_theme->main_win->menu = menu_new (new_theme->main_win);
  _add_main_categories (flhoc, new_theme->main_win->menu);

  evas_object_event_callback_add (new_theme->main_win->edje,
      EVAS_CALLBACK_KEY_DOWN, _key_down_cb, new_theme->main_win);
  evas_object_focus_set (new_theme->main_win->edje, EINA_TRUE);
  main_window_init (new_theme->main_win);

  main_window_free (old_theme->main_win);
  old_theme->main_win = NULL;
}

static void
_populate_themes (FLHOC *flhoc, Category *themes)
{
  Eina_List *l;
  Theme *theme;
  Item *this_theme = NULL;

  if (!flhoc)
    return;

  EINA_LIST_FOREACH (flhoc->themes, l, theme) {
    Item *item = item_new (themes, ITEM_TYPE_THEME);
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
    category_append_item (themes, item);
    if (flhoc->current_theme == theme)
      this_theme = item;
  }

  if (this_theme)
    themes->items_selection = eina_list_data_find_list (themes->items, this_theme);

  category_set_items (themes, NULL);

}

static Eina_Bool
_add_main_categories (FLHOC *flhoc, Menu *menu)
{
  Category *category = NULL;
  Category *homebrew = NULL;
  Item *item = NULL;

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
  _populate_themes (flhoc, category);
  menu_append_category (menu, category);
  homebrew = category_new (menu, CATEGORY_TYPE_HOMEBREW);
  _add_game_items (homebrew);
  menu_append_category (menu, homebrew);
  category = category_new (menu, CATEGORY_TYPE_DEVICE_HOMEBREW);
  _add_game_items (category);
  menu_append_category (menu, category);
  category = category_new (menu, CATEGORY_TYPE_DEVICE_PACKAGES);
  menu_append_category (menu, category);

  menu->categories_selection = eina_list_data_find_list (menu->categories, homebrew);

  menu_set_categories (menu, NULL);

  return EINA_TRUE;
}

int
main (int argc, char *argv[])
{
  FLHOC flhoc;
  Evas_Coord w, h;
  Eina_List *theme_files;
  Eina_List *l;
  char *theme_filename;
  Theme *theme = NULL;

  if (!ecore_init ()) {
    printf ("error setting up ecore\n");
    return 1;
  }
  ecore_app_args_set (argc, (const char **)argv);
  if (!ecore_evas_init ()) {
    printf ("error setting up ecore_evas\n");
    goto ecore_shutdown;
  }
  if (!edje_init ()) {
    printf ("error setting up edje\n");
    goto ecore_evas_shutdown;
  }

  //ee = ecore_evas_new (NULL, 0, 0, 1280, 720, NULL);
  //ecore_evas_fullscreen_set (ee, EINA_TRUE);
  memset (&flhoc, 0, sizeof(FLHOC));
  flhoc.ee = ecore_evas_new (NULL, 0, 0, 640, 480, NULL);
  flhoc.evas = ecore_evas_get (flhoc.ee);
  ecore_evas_show (flhoc.ee);
  evas_output_size_get (flhoc.evas, &w, &h);
  ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, _ee_signal_exit, NULL);

  theme_files = _get_dirlist ("data/themes", EINA_FALSE, EINA_TRUE, EINA_TRUE);
  EINA_LIST_FOREACH (theme_files, l, theme_filename) {
    if (eina_str_has_extension (theme_filename, ".edj") &&
        theme_file_is_valid (flhoc.evas, theme_filename)) {
      theme = theme_new (flhoc.evas, theme_filename);

      flhoc.themes = eina_list_append (flhoc.themes, theme);
      if (flhoc.current_theme == NULL)
        flhoc.current_theme = theme;
    }
  }

  if (flhoc.current_theme == NULL) {
    printf ("No valid themes found\n");
    goto edje_shutdown;
  }

  theme = flhoc.current_theme;
  theme->main_win = main_window_new (theme);
  if (theme->main_win == NULL) {
    printf ("error creating main window\n");
    goto edje_shutdown;
  }

  evas_object_data_set (theme->main_win->edje, "FLHOC", &flhoc);
  ecore_evas_object_associate(flhoc.ee, theme->main_win->edje,
      ECORE_EVAS_OBJECT_ASSOCIATE_BASE);

  evas_object_move (theme->main_win->edje, 0, 0);
  evas_object_resize (theme->main_win->edje, w, h);
  evas_object_show (theme->main_win->edje);

  theme->main_win->menu = menu_new (theme->main_win);
  if (theme->main_win->menu == NULL) {
    printf ("error creating menu\n");
    goto edje_shutdown;
  }

  _add_main_categories (&flhoc, theme->main_win->menu);

  /* Start the process */
  evas_object_event_callback_add(theme->main_win->edje,
      EVAS_CALLBACK_KEY_DOWN, _key_down_cb, theme->main_win);
  evas_object_focus_set (theme->main_win->edje, EINA_TRUE);
  main_window_init (theme->main_win);

  ecore_main_loop_begin ();

 edje_shutdown:
  EINA_LIST_FOREACH (flhoc.themes, l, theme)
    theme_free (theme);

  if (flhoc.ee)
    ecore_evas_free (flhoc.ee);
  edje_shutdown ();
 ecore_evas_shutdown:
  ecore_evas_shutdown ();
 ecore_shutdown:
  ecore_shutdown ();

  return 0;
}
