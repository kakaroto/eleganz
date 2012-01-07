// 2011 Ninjas
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include <stdio.h>
#include <stdlib.h>

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>

#include "FLHOC_menu.h"

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
    Ecore_Evas *ee = ecore_evas_ecore_evas_get (main_win->evas);
    ecore_evas_fullscreen_set (ee, !ecore_evas_fullscreen_get (ee));
  } else if (strcmp (ev->keyname, "a") == 0) {
    Category *category;

    category = category_new (menu, "FLHOC/menu/category/device_homebrew");
    _add_game_items (category);
    menu_append_category (menu, category);
    category = category_new (menu, "FLHOC/menu/category/device_packages");
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
  Menu *menu = category->parent;
  MainWindow *main_win = menu->parent;

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

  item = item_new (category, "FLHOC/menu/item");
  edje_object_part_text_set (item->edje, "FLHOC/menu/item/title", "Awesome game!");
  category_append_item (category, item);
  item = item_new (category, "FLHOC/menu/item");
  edje_object_part_text_set (item->edje, "FLHOC/menu/item/title", "Another awesome game!");
  category_append_item (category, item);
  item = item_new (category, "FLHOC/menu/item");
  edje_object_part_text_set (item->edje, "FLHOC/menu/item/title", "A cool game I guess!");
  category_append_item (category, item);
  item = item_new (category, "FLHOC/menu/item");
  edje_object_part_text_set (item->edje, "FLHOC/menu/item/title", "Some game!");
  category_append_item (category, item);
  item = item_new (category, "FLHOC/menu/item");
  edje_object_part_text_set (item->edje, "FLHOC/menu/item/title", "Homebrew app!!");
  category_append_item (category, item);

  category_set_items (category, NULL);

  return EINA_TRUE;
}

static void
_populate_themes (Category *themes)
{
  /* TODO */
}

static Eina_Bool
_add_main_categories (Menu *menu)
{
  Category *category = NULL;
  Category *homebrew = NULL;
  Item *item = NULL;

  category = category_new (menu, "FLHOC/menu/category/quit");
  category->action = _category_action_quit;
  menu_append_category (menu, category);
  category = category_new (menu, "FLHOC/menu/category/update");
  menu_append_category (menu, category);
  category = category_new (menu, "FLHOC/menu/category/theme");
  _populate_themes (category);
  menu_append_category (menu, category);
  category = category_new (menu, "FLHOC/menu/category/settings");
  menu_append_category (menu, category);
  item = item_new (category, "FLHOC/menu/item");
  edje_object_part_text_set (item->edje, "FLHOC/menu/item/title",
      "Press 'A' to add icons and 'D' or 'S' to delete them");
  category_append_item (category, item);
  item = item_new (category, "FLHOC/menu/item");
  edje_object_part_text_set (item->edje, "FLHOC/menu/item/title",
      "Press 'F' for fullscreen and 'Q' to exit");
  category_append_item (category, item);
  category_set_items (category, NULL);
  homebrew = category_new (menu, "FLHOC/menu/category/homebrew");
  _add_game_items (homebrew);
  menu_append_category (menu, homebrew);

  menu->categories_selection = eina_list_data_find_list (menu->categories, homebrew);

  menu_set_categories (menu, NULL);

  return EINA_TRUE;
}

int
main (int argc, char *argv[])
{
  Ecore_Evas *ee = NULL;
  Evas *evas = NULL;
  Evas_Coord w, h;
  MainWindow *main_win = NULL;
  Menu *menu = NULL;

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
  ee = ecore_evas_new (NULL, 0, 0, 640, 480, NULL);
  ecore_evas_show (ee);
  evas = ecore_evas_get (ee);
  evas_output_size_get (evas, &w, &h);
  ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, _ee_signal_exit, NULL);

  if (!theme_file_is_valid (evas, "data/themes/default/default.edj")) {
    printf ("Invalid theme file\n");
    goto edje_shutdown;
  }

  main_win = main_window_new (evas, "data/themes/default/default.edj");
  if (main_win == NULL) {
    printf ("error creating main window\n");
    goto edje_shutdown;
  }

  ecore_evas_object_associate(ee, main_win->edje, ECORE_EVAS_OBJECT_ASSOCIATE_BASE);

  evas_object_move (main_win->edje, 0, 0);
  evas_object_resize (main_win->edje, w, h);
  evas_object_show (main_win->edje);

  menu = menu_new (main_win);
  if (menu == NULL) {
    printf ("error creating menu\n");
    goto edje_shutdown;
  }

  _add_main_categories (menu);

  /* Start the process */
  evas_object_event_callback_add(main_win->edje, EVAS_CALLBACK_KEY_DOWN,
      _key_down_cb, main_win);
  evas_object_focus_set (main_win->edje, EINA_TRUE);
  main_window_init (main_win, menu);

  ecore_main_loop_begin ();

 edje_shutdown:
  if (menu)
    menu_free (menu);
  if (main_win)
    main_window_free (main_win);

  if (ee)
    ecore_evas_free (ee);
  edje_shutdown ();
 ecore_evas_shutdown:
  ecore_evas_shutdown ();
 ecore_shutdown:
  ecore_shutdown ();

  return 0;
}
