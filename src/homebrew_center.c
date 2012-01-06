#include <stdio.h>
#include <stdlib.h>

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>

Evas_Object *edje = NULL;
Evas_Object *menu = NULL;
Evas_Object *category_selection = NULL;

typedef struct {
  char *edje_file;
  Evas *evas;
  Evas_Object *edje;
  void *primary;
  void *secondary;
  int state;
} MainWindow;

typedef struct {
  MainWindow *parent;
  Evas_Object *edje;
  Eina_List *categories;
  Eina_List *categories_selection;
} Menu;

typedef struct {
  Menu *parent;
  Evas_Object *edje;
  char *group;
  char *part;
  Eina_List *items;
  Eina_List *items_selection;
} Category;

typedef struct {
  Category *parent;
  Evas_Object *edje;
} Item;

static void _main_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);
static void _menu_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);
static void _menu_ready_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);
static void _init_menu_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);
static void _category_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);
static void _menu_scroll_previous_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);
static void _menu_scroll_next_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);
static MainWindow *main_window_new (Evas *evas, const char *filename);
static void main_window_set_primary (MainWindow *main, Evas_Object *edje,
    void *primary);
static Menu *menu_new (MainWindow *main);
static Category *category_new (Menu *menu, const char *group);
static int menu_swallow_category (Menu *menu, Category *category,
    int index, const char *signal_suffix);
static void menu_set_categories (Menu *menu, const char *signal);
static int _add_main_categories (Menu *menu);
static void menu_scroll_previous (Menu *menu);
static void menu_scroll_next (Menu *menu);
static void menu_reset (Menu *menu);


static void
_main_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  printf ("Main: Signal '%s' coming from part '%s'\n", emission, source);
}

static void
_menu_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  printf ("Menu: Signal '%s' coming from part '%s'\n", emission, source);
}

static void
_category_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  Category *c = data;
  printf ("Category %s: Signal '%s' coming from part '%s'!\n", c->part, emission, source);
}

static void
_menu_ready_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  MainWindow *main_win = data;
  Menu *menu = main_win->primary;
  Category *selection = eina_list_data_get (menu->categories_selection);


  edje_object_signal_callback_del_full (menu->edje, "FLHOC/menu,ready", "*",
      _menu_ready_cb, main_win);
  menu_set_categories (menu, "reset");

  if (selection)
    edje_object_signal_emit (selection->edje, "FLHOC/menu/category,selected", "");
}

static void
_init_menu_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  MainWindow *main_win = data;
  Menu *menu = main_win->primary;

  edje_object_signal_callback_del_full (main_win->edje, "FLHOC/primary,set,end", "*",
      _init_menu_cb, main_win);
  edje_object_signal_callback_add (menu->edje, "FLHOC/menu,ready", "*",
      _menu_ready_cb, main_win);
  edje_object_signal_emit (menu->edje, "FLHOC/menu,init", "");
}

static void
_menu_scroll_next_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  Menu *menu = data;

  edje_object_signal_callback_del_full (menu->edje, "FLHOC/menu,category_next,end", "*",
      _menu_scroll_next_cb, menu);
  menu_reset (menu);
}

static void
_menu_scroll_previous_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  Menu *menu = data;

  edje_object_signal_callback_del_full (menu->edje, "FLHOC/menu,category_previous,end", "*",
      _menu_scroll_previous_cb, menu);
  menu_reset (menu);
}

static void
_key_down_cb (void *data, Evas *e, Evas_Object *obj, void *event_info)
{
  MainWindow *main_win = data;
  Evas_Event_Key_Down *ev = event_info;

  printf ("Key Down: '%s' - '%s' - '%s' - '%s'\n", ev->keyname, ev->key, ev->string, ev->compose);
  if (strcmp (ev->keyname, "Left") == 0) {
    Menu *menu = main_win->primary;
    menu_scroll_previous (menu);
  } else if (strcmp (ev->keyname, "Right") == 0) {
    Menu *menu = main_win->primary;
    menu_scroll_next (menu);
  }
}

static void
_key_up_cb (void *data, Evas *e, Evas_Object *obj, void *event_info)
{
  Evas_Event_Key_Up *ev = event_info;

  printf ("Key Up: '%s' - '%s' - '%s' - '%s'\n", ev->keyname, ev->key, ev->string, ev->compose);
}

static MainWindow *
main_window_new (Evas *evas, const char *filename)
{
  MainWindow *main_win = calloc (1, sizeof (MainWindow));

  main_win->edje_file = strdup (filename);
  main_win->evas = evas;
  main_win->edje = edje_object_add (evas);
  edje_object_file_set (main_win->edje, main_win->edje_file, "main");
  edje_object_signal_callback_add (main_win->edje, "*", "*",
      _main_signal_cb, main_win);
  evas_object_event_callback_add(main_win->edje, EVAS_CALLBACK_KEY_DOWN,
      _key_down_cb, main_win);
  evas_object_event_callback_add(main_win->edje, EVAS_CALLBACK_KEY_UP,
      _key_up_cb, main_win);
  evas_object_focus_set (main_win->edje, EINA_TRUE);

  return main_win;
}

static void
main_window_set_primary (MainWindow *main_win, Evas_Object *edje, void *primary)
{
  main_win->primary = primary;
  edje_object_part_swallow (main_win->edje, "FLHOC/primary", edje);
  edje_object_signal_emit (main_win->edje, "FLHOC/primary,set", "");
}

static Menu *
menu_new (MainWindow *main_win)
{
  Menu *menu = calloc (1, sizeof(Menu));

  menu->parent = main_win;
  menu->edje = edje_object_add (main_win->evas);
  edje_object_file_set (menu->edje, main_win->edje_file, "FLHOC/menu");
  edje_object_signal_callback_add (menu->edje, "*", "*", _menu_signal_cb, menu);

  return menu;
}

static Category *
category_new (Menu *menu, const char *group)
{
  Category *category = calloc (1, sizeof(Category));
  MainWindow *main_win = menu->parent;
  static int idx = 0;

  category->parent = menu;
  category->group = strdup (group);
  category->part = calloc (2, 1);
  category->part[0] = '0' + idx++;

  category->edje = edje_object_add (main_win->evas);
  edje_object_file_set (category->edje, main_win->edje_file, group);
  edje_object_signal_callback_add (category->edje, "*", "*",
      _category_signal_cb, category);

  return category;
}

static int
menu_swallow_category (Menu *menu, Category *category, int index, const char *signal_suffix)
{
  char signal[40];
  char part[40] = "FLHOC/menu/category.";
  int prefix_len = strlen (part);
  char *name = part + prefix_len;

  if (index == 0)
    snprintf (name, sizeof(part) - prefix_len, "selection");
  else if (index < 0)
    snprintf (name, sizeof(part) - prefix_len, "before_%d", -index - 1);
  else
    snprintf (name, sizeof(part) - prefix_len, "after_%d", index - 1);

  if (!edje_object_part_exists (menu->edje, part))
    return EINA_FALSE;

  if (category) {
    printf ("Swallow category %s into %s\n", category->part, part);
    edje_object_part_swallow (menu->edje, part, category->edje);
    if (signal_suffix) {
      snprintf (signal, sizeof(signal),  "FLHOC/menu,category_set%s%s",
          signal_suffix[0] == 0 ? "" : ",", signal_suffix);
      edje_object_signal_emit (menu->edje, signal, name);
    }
  } else {
    /* TODO: need to unswallow after animation.. in edje? */
    // edje_object_part_unswallow (menu->edje, edje);
    if (signal_suffix) {
      snprintf (signal, sizeof(signal),  "FLHOC/menu,category_unset%s%s",
          signal_suffix[0] == 0 ? "" : ",", signal_suffix);
      edje_object_signal_emit (menu->edje, signal, name);
    }
  }
  return EINA_TRUE;
}

static void
menu_set_categories (Menu *menu, const char *signal)
{
  Eina_List *cur = NULL;
  Category *category;
  int index = 0;
  Eina_Bool *ret;

  if (menu->categories_selection) {
    category = eina_list_data_get (menu->categories_selection);
    menu_swallow_category (menu, category, 0, signal);
    for (cur = eina_list_prev (menu->categories_selection), index = -1; cur;
         cur =  eina_list_prev (cur), index--) {
      category = eina_list_data_get (cur);
      if (!menu_swallow_category (menu, category, index, signal))
        break;
    }
    while (menu_swallow_category (menu, NULL, index, signal)) index--;
    for (cur = eina_list_next (menu->categories_selection), index = 1; cur;
         cur =  eina_list_next (cur), index++) {
      category = eina_list_data_get (cur);
      if (!menu_swallow_category (menu, category, index, signal))
        break;
    }
    while (menu_swallow_category (menu, NULL, index, signal)) index++;
  }
}

static void
menu_reset (Menu *menu)
{
  Eina_List *l;
  Category *category;

  EINA_LIST_FOREACH (menu->categories, l, category)
      edje_object_part_unswallow (menu->edje, category->edje);

  edje_object_signal_emit (menu->edje, "FLHOC/menu,category_reset", "");
  menu_set_categories (menu, "reset");
}

static void
menu_scroll_next (Menu *menu)
{
  Category *old = eina_list_data_get (menu->categories_selection);
  Eina_List *next = eina_list_next (menu->categories_selection);
  Category *new = eina_list_data_get (next);

  menu_reset (menu);
  if (next && new) {
    edje_object_signal_emit (old->edje, "FLHOC/menu/category,deselected", "");
    edje_object_signal_emit (new->edje, "FLHOC/menu/category,selected", "");
    menu->categories_selection = next;

    edje_object_signal_callback_add (menu->edje, "FLHOC/menu,category_next,end", "*",
        _menu_scroll_next_cb, menu);
    edje_object_signal_emit (menu->edje, "FLHOC/menu,category_next", "");
  }
}

static void
menu_scroll_previous (Menu *menu)
{
  Category *old = eina_list_data_get (menu->categories_selection);
  Eina_List *previous = eina_list_prev (menu->categories_selection);
  Category *new = eina_list_data_get (previous);

  menu_reset (menu);
  if (previous && new) {
    edje_object_signal_emit (old->edje, "FLHOC/menu/category,deselected", "");
    edje_object_signal_emit (new->edje, "FLHOC/menu/category,selected", "");
    menu->categories_selection = previous;

    edje_object_signal_callback_add (menu->edje, "FLHOC/menu,category_previous,end", "*",
        _menu_scroll_previous_cb, menu);
    edje_object_signal_emit (menu->edje, "FLHOC/menu,category_previous", "");
  }
}

static int
_add_main_categories (Menu *menu)
{
  Category *category = NULL;

  category = category_new (menu, "FLHOC/menu/category/settings");
  menu->categories = eina_list_append (menu->categories, category);
  category = category_new (menu, "FLHOC/menu/category/homebrew");
  menu->categories = eina_list_append (menu->categories, category);
  category = category_new (menu, "FLHOC/menu/category/device");
  menu->categories = eina_list_append (menu->categories, category);
  category = category_new (menu, "FLHOC/menu/category/device");
  menu->categories = eina_list_append (menu->categories, category);

  menu->categories_selection = eina_list_next (menu->categories);

  return EINA_TRUE;
}

static void
_add_game (Ecore_Evas *ee, Evas *evas, Evas_Object *edje, const char *name)
{
  Evas_Object *item = NULL;
  Evas_Object *box = NULL;
  Evas_Coord w, h;

  item = edje_object_add (evas);
  if (!edje_object_file_set (item, "data/themes/default/default.edj",
          "FLHOC/item")) {
    int err = edje_object_load_error_get (edje);
    const char *errmsg = edje_load_error_str (err);
    printf ("could not load 'FLHOC/item' from themes/default.edj: %s\n",
        errmsg);
  }
  box = edje_object_part_object_get (edje, "FLHOC/items");
  evas_object_geometry_get (box, NULL, NULL, &w, &h);
  printf ("%dx%d\n", w, h);
  evas_object_move (item, 0, 0);
  //evas_object_resize (item, 200, 200);
  evas_object_show (item);
  if (!edje_object_part_box_append(edje, "FLHOC/items", item))
    printf("An error ocurred when appending item to the box.\n");

  edje_object_part_text_set (item, "FLHOC/item/title", name);
}

int
main (int argc, char *argv[])
{
  Ecore_Evas *ee;
  Evas *evas;
  Evas_Coord w, h;
  MainWindow *main_win;
  Menu *menu;

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
    goto ecore_shutdown;
  }

  //ee = ecore_evas_new (NULL, 0, 0, 1280, 720, NULL);
  //ecore_evas_fullscreen_set (ee, EINA_TRUE);
  ee = ecore_evas_new (NULL, 0, 0, 640, 480, NULL);
  ecore_evas_show (ee);
  evas = ecore_evas_get (ee);
  evas_output_size_get (evas, &w, &h);

  main_win = main_window_new (evas, "data/themes/default/default.edj");
  ecore_evas_object_associate(ee, main_win->edje, ECORE_EVAS_OBJECT_ASSOCIATE_BASE);

  evas_object_move (main_win->edje, 0, 0);
  evas_object_resize (main_win->edje, w, h);
  evas_object_show (main_win->edje);

  menu = menu_new (main_win);
  _add_main_categories (menu);
  menu_set_categories (menu, NULL);

  /* Start the process */
  edje_object_signal_callback_add (main_win->edje, "FLHOC/primary,set,end", "*",
      _init_menu_cb, main_win);
  main_window_set_primary (main_win, menu->edje, menu);

  ecore_main_loop_begin ();
 edje_shutdown:
  edje_shutdown ();
 ecore_shutdown:
  ecore_shutdown ();

  return 0;
}
