// 2011 Ninjas
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "FLHOC_menu.h"


static void _main_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);
static void _menu_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);
static void _category_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);
static void _item_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);
static void _secondary_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);
static void _init_menu_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);
static void _init_secondary_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);
static void _menu_ready_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);
static void _secondary_ready_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source);


static void _menu_reset (Menu *menu);
static Eina_Bool _menu_swallow_category (Menu *menu, Category *category,
    int index, const char *signal_suffix);

static void _category_reset (Category *category);
static Eina_Bool _category_swallow_item (Category *category, Item *item,
    int index, const char *signal_suffix);

static void
_main_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  //printf ("Main: Signal '%s' coming from part '%s'\n", emission, source);
}

static void
_menu_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  Menu *menu = data;

  //printf ("Menu: Signal '%s' coming from part '%s'\n", emission, source);

  if (strcmp (emission, "FLHOC/menu,category_previous,end") == 0 ||
      strcmp (emission, "FLHOC/menu,category_next,end") == 0) {
    Category *selection = menu_get_selected_category (menu);
    Item *item_selection = category_get_selected_item (selection);

    if (item_selection) {
      edje_object_signal_emit (item_selection->edje, "FLHOC/menu/item,selected", "");
      if (item_selection->selection)
        item_selection->selection (item_selection, EINA_TRUE);
    }

    _menu_reset (menu);
  } else if (strcmp (emission, "FLHOC/menu,category_unset,end") == 0) {
    char part[40] = "FLHOC/menu/category.";
    Evas_Object *obj;

    strncat (part, source, sizeof(part)-1);
    obj = edje_object_part_swallow_get (menu->edje, part);
    if (obj) {
      edje_object_part_unswallow (menu->edje, obj);
      evas_object_hide (obj);
    }
  }
}

static void
_category_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  Category *category = data;

  //printf ("Category: Signal '%s' coming from part '%s'!\n", emission, source);
  if (strcmp (emission, "FLHOC/menu/category,item_previous,end") == 0 ||
      strcmp (emission, "FLHOC/menu/category,item_next,end") == 0) {
    _category_reset (category);
  } else if (strcmp (emission, "FLHOC/menu/category,item_unset,end") == 0) {
    char part[40] = "FLHOC/menu/category/item.";
    Evas_Object *obj;

    strncat (part, source, sizeof(part)-1);
    obj = edje_object_part_swallow_get (category->edje, part);
    if (obj) {
      edje_object_part_unswallow (category->edje, obj);
      evas_object_hide (obj);
    }
  }
}

static void
_item_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  //printf ("Item: Signal '%s' coming from part '%s'!\n", emission, source);
}

static void
_secondary_signal_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  //printf ("Secondary: Signal '%s' coming from part '%s'\n", emission, source);
}

static void
_init_menu_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  MainWindow *main_win = data;
  Menu *menu = main_win->menu;

  edje_object_signal_callback_del_full (main_win->edje, "FLHOC/primary,set,end", "*",
      _init_menu_cb, main_win);
  edje_object_signal_callback_add (menu->edje, "FLHOC/menu,ready", "*",
      _menu_ready_cb, main_win);
  edje_object_signal_emit (menu->edje, "FLHOC/menu,init", "");
}

static void
_init_secondary_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  MainWindow *main_win = data;
  Secondary *secondary = main_win->secondary;

  edje_object_signal_callback_del_full (main_win->edje, "FLHOC/secondary,set,end", "*",
      _init_secondary_cb, main_win);
  if (secondary) {
    edje_object_signal_callback_add (secondary->edje, "FLHOC/secondary,ready", "*",
        _secondary_ready_cb, main_win);
    edje_object_signal_emit (secondary->edje, "FLHOC/secondary,init", "");
  }
}

static void
_menu_ready_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
  MainWindow *main_win = data;
  Menu *menu = main_win->menu;
  Category *selection = menu_get_selected_category (menu);
  Item *item_selection = category_get_selected_item (selection);

  edje_object_signal_callback_del_full (menu->edje, "FLHOC/menu,ready", "*",
      _menu_ready_cb, main_win);
  menu_set_categories (menu, "reset");

  if (selection) {
    edje_object_signal_emit (selection->edje, "FLHOC/menu/category,selected", "");
    if (selection->selection)
      selection->selection (selection, EINA_TRUE);
    if (item_selection) {
        edje_object_signal_emit (item_selection->edje, "FLHOC/menu/item,selected", "");
        if (item_selection->selection)
          item_selection->selection (item_selection, EINA_TRUE);
    }
  }
}

static void
_secondary_ready_cb (void *data, Evas_Object *obj,
    const char  *emission, const char  *source)
{
}

Eina_Bool
theme_file_is_valid (Evas *evas, const char *filename)
{
  Eina_List *groups = edje_file_collection_list (filename);
  Evas_Object *edje = NULL;
  Eina_Bool is_valid = EINA_FALSE;
  unsigned int i;

  const char *valid_categories[] = {
    "FLHOC/menu/category/quit",
    "FLHOC/menu/category/settings",
    "FLHOC/menu/category/homebrew",
    "FLHOC/menu/category/device_homebrew",
    "FLHOC/menu/category/device_packages",
  };

  if (eina_list_search_unsorted (groups,
          (Eina_Compare_Cb) strcmp, "main") == NULL ||
      eina_list_search_unsorted (groups,
          (Eina_Compare_Cb) strcmp, "FLHOC/menu") == NULL ||
      eina_list_search_unsorted (groups,
          (Eina_Compare_Cb) strcmp, "FLHOC/menu/item/game") == NULL ||
      eina_list_search_unsorted (groups,
          (Eina_Compare_Cb) strcmp, "FLHOC/menu/item/theme") == NULL ||
      eina_list_search_unsorted (groups,
          (Eina_Compare_Cb) strcmp, "FLHOC/menu/item/about") == NULL ||
      eina_list_search_unsorted (groups,
          (Eina_Compare_Cb) strcmp, "FLHOC/menu/item/help") == NULL ||
      eina_list_search_unsorted (groups,
          (Eina_Compare_Cb) strcmp, "FLHOC/menu/item/install_theme") == NULL ||
      eina_list_search_unsorted (groups,
          (Eina_Compare_Cb) strcmp, "FLHOC/menu/item/wallpaper") == NULL)
    goto end;

  for (i = 0; i < sizeof(valid_categories) / sizeof(char *); i++) {
    if (eina_list_search_unsorted (groups,
            (Eina_Compare_Cb) strcmp, valid_categories[i]) == NULL)
      goto end;
  }

  edje = edje_object_add (evas);
  if (!edje_object_file_set (edje, filename, "main"))
    goto end;
  if (!edje_object_part_exists (edje, "FLHOC/primary") ||
      !edje_object_part_exists (edje, "FLHOC/secondary_above") ||
      !edje_object_part_exists (edje, "FLHOC/secondary_below") ||
      !edje_object_part_exists (edje, "FLHOC/wallpaper"))
    goto end;

  if (!edje_object_file_set (edje, filename, "FLHOC/menu"))
    goto end;
  if (!edje_object_part_exists (edje, "FLHOC/menu/category.selection") ||
      edje_object_data_get (edje, "category_next") == NULL ||
      edje_object_data_get (edje, "category_previous") == NULL ||
      edje_object_data_get (edje, "item_next") == NULL ||
      edje_object_data_get (edje, "item_previous") == NULL)
    goto end;

  for (i = 0; i < sizeof(valid_categories) / sizeof(char *); i++) {
    if (!edje_object_file_set (edje, filename, valid_categories[i]))
      goto end;
    if (!edje_object_part_exists (edje, "FLHOC/menu/category/item.selection"))
      goto end;
  }
  if (!edje_object_file_set (edje, filename, "FLHOC/menu/item/game"))
    goto end;
  if (!edje_object_part_exists (edje, "FLHOC/menu/item/game.icon") ||
      !edje_object_part_exists (edje, "FLHOC/menu/item/game.title"))
    goto end;
  if (!edje_object_file_set (edje, filename, "FLHOC/menu/item/theme"))
    goto end;
  if (!edje_object_part_exists (edje, "FLHOC/menu/item/theme.name") ||
      !edje_object_part_exists (edje, "FLHOC/menu/item/theme.author") ||
      !edje_object_part_exists (edje, "FLHOC/menu/item/theme.version") ||
      !edje_object_part_exists (edje, "FLHOC/menu/item/theme.description") ||
      !edje_object_part_exists (edje, "FLHOC/menu/item/theme.preview"))
    goto end;

  is_valid = EINA_TRUE;
 end:
  evas_object_del (edje);
  return is_valid;
}

Theme *
theme_new (Evas *evas, const char *filename)
{
  Theme *theme = calloc (1, sizeof (Theme));

  theme->edje_file = strdup (filename);
  theme->evas = evas;

  theme->name = edje_file_data_get (filename, "name");
  theme->author = edje_file_data_get (filename, "author");
  theme->version = edje_file_data_get (filename, "version");
  theme->description = edje_file_data_get (filename, "description");
  theme->compatible = edje_file_data_get (filename, "compatible");

  return theme;
}

void
theme_free (Theme *theme)
{
  if (theme->edje_file)
    free (theme->edje_file);
  if (theme->main_win)
    main_window_free (theme->main_win);
  free (theme);
}

MainWindow *
main_window_new (Theme *theme)
{
  MainWindow *main_win = calloc (1, sizeof (MainWindow));

  main_win->theme = theme;
  main_win->edje = edje_object_add (theme->evas);
  if (!edje_object_file_set (main_win->edje, theme->edje_file, "main")) {
    main_window_free (main_win);
    return NULL;
  }
  edje_object_signal_callback_add (main_win->edje, "*", "*",
      _main_signal_cb, main_win);

  return main_win;
}

void
main_window_free (MainWindow *main_win)
{
  if (main_win->menu)
    menu_free (main_win->menu);
  evas_object_del (main_win->edje);
  free (main_win);
}

void
main_window_init (MainWindow *main_win)
{
  edje_object_signal_callback_add (main_win->edje, "FLHOC/primary,set,end", "*",
      _init_menu_cb, main_win);
  edje_object_part_swallow (main_win->edje, "FLHOC/primary", main_win->menu->edje);
  edje_object_signal_emit (main_win->edje, "FLHOC/primary,set", "");
}

void
main_window_set_secondary (MainWindow *main_win, Secondary *secondary)
{
  char part[100];

  snprintf (part, sizeof(part), "FLHOC/secondary_%s", secondary->secondary);

  main_win->secondary = secondary;
  edje_object_signal_callback_add (main_win->edje, "FLHOC/secondary,set,end", "*",
      _init_secondary_cb, main_win);
  edje_object_part_swallow (main_win->edje, part, main_win->secondary->edje);
  edje_object_signal_emit (main_win->edje, "FLHOC/secondary,set", secondary->secondary);
}

Menu *
menu_new (MainWindow *main_win)
{
  Menu *menu = calloc (1, sizeof(Menu));

  menu->main_win = main_win;
  menu->edje = edje_object_add (main_win->theme->evas);
  if (!edje_object_file_set (menu->edje, main_win->theme->edje_file, "FLHOC/menu")) {
    menu_free (menu);
    return NULL;
  }
  menu->category_next = edje_object_data_get (menu->edje, "category_next");
  menu->category_previous = edje_object_data_get (menu->edje, "category_previous");
  menu->item_next = edje_object_data_get (menu->edje, "item_next");
  menu->item_previous = edje_object_data_get (menu->edje, "item_previous");
  edje_object_signal_callback_add (menu->edje, "*", "*", _menu_signal_cb, menu);

  return menu;
}

void
menu_free (Menu *menu)
{
  Eina_List *cur = NULL;
  Category *category;

  EINA_LIST_FOREACH (menu->categories, cur, category)
      category_free (category);
  evas_object_del (menu->edje);
  free (menu);
}

void
menu_set_categories (Menu *menu, const char *signal)
{
  Eina_List *cur = NULL;
  Category *category;
  int index = 0;

  EINA_LIST_FOREACH (menu->categories, cur, category) {
      edje_object_part_unswallow (menu->edje, category->edje);
      evas_object_hide (category->edje);
  }

  if (menu->categories_selection) {
    category = eina_list_data_get (menu->categories_selection);
    _menu_swallow_category (menu, category, 0, signal);
    for (cur = eina_list_prev (menu->categories_selection), index = -1; cur;
         cur =  eina_list_prev (cur), index--) {
      category = eina_list_data_get (cur);
      if (!_menu_swallow_category (menu, category, index, signal))
        break;
    }
    while (_menu_swallow_category (menu, NULL, index, signal)) index--;
    for (cur = eina_list_next (menu->categories_selection), index = 1; cur;
         cur =  eina_list_next (cur), index++) {
      category = eina_list_data_get (cur);
      if (!_menu_swallow_category (menu, category, index, signal))
        break;
    }
    while (_menu_swallow_category (menu, NULL, index, signal)) index++;
  }
}

Eina_Bool
menu_append_category (Menu *menu, Category *category)
{
  if (eina_list_data_find (menu->categories, category) != NULL)
    return EINA_FALSE;

  menu->categories = eina_list_append (menu->categories, category);
  if (menu->categories_selection == NULL)
    menu->categories_selection = menu->categories;

  return EINA_TRUE;
}

Eina_Bool
menu_delete_category (Menu *menu, Category *category)
{
  Eina_List *list = eina_list_data_find_list (menu->categories, category);

  if (list == NULL)
    return EINA_FALSE;

  if (menu->categories_selection == list) {
    Category *old_selection = menu_get_selected_category (menu);
    Category *new_selection;
    Item *item_selection;

    if (eina_list_prev (list) != NULL)
      menu->categories_selection = eina_list_prev (list);
    else if (eina_list_next (list) != NULL)
      menu->categories_selection = eina_list_next (list);
    else
      return EINA_FALSE; /* Can't remove last element */

    _menu_reset (menu);
    if (old_selection->selection)
      old_selection->selection (old_selection, EINA_FALSE);
    new_selection = menu_get_selected_category (menu);
    item_selection = category_get_selected_item (new_selection);

    edje_object_signal_emit (new_selection->edje, "FLHOC/menu/category,selected", "");
    if (new_selection->selection)
      new_selection->selection (new_selection, EINA_TRUE);
    if (item_selection) {
      edje_object_signal_emit (item_selection->edje, "FLHOC/menu/item,selected", "");
      if (item_selection->selection)
        item_selection->selection (item_selection, EINA_TRUE);
    }
  }

  menu->categories = eina_list_remove (menu->categories, category);
  return EINA_TRUE;
}

Category *
menu_get_selected_category (Menu *menu)
{
  if (menu)
    return eina_list_data_get (menu->categories_selection);
  return NULL;
}

void
menu_scroll_next (Menu *menu)
{
  Category *old = eina_list_data_get (menu->categories_selection);
  Eina_List *next = eina_list_next (menu->categories_selection);
  Category *new = eina_list_data_get (next);

  _category_reset (old);
  _menu_reset (menu);
  if (next && new) {
    Item *item_selection = category_get_selected_item (old);

    edje_object_signal_emit (old->edje, "FLHOC/menu/category,deselected", "");
    if (old->selection)
      old->selection (old, EINA_FALSE);
    edje_object_signal_emit (new->edje, "FLHOC/menu/category,selected", "");
    if (new->selection)
      new->selection (new, EINA_TRUE);

    if (item_selection) {
      edje_object_signal_emit (item_selection->edje, "FLHOC/menu/item,deselected", "");
      if (item_selection->selection)
        item_selection->selection (item_selection, EINA_FALSE);
    }

    menu->categories_selection = next;

    edje_object_signal_emit (menu->edje, "FLHOC/menu,category_next", "");
  }
}

void
menu_scroll_previous (Menu *menu)
{
  Category *old = eina_list_data_get (menu->categories_selection);
  Eina_List *previous = eina_list_prev (menu->categories_selection);
  Category *new = eina_list_data_get (previous);
  Item *item_selection = category_get_selected_item (old);

  _category_reset (old);
  _menu_reset (menu);
  if (previous && new) {
    edje_object_signal_emit (old->edje, "FLHOC/menu/category,deselected", "");
    if (old->selection)
      old->selection (old, EINA_FALSE);
    edje_object_signal_emit (new->edje, "FLHOC/menu/category,selected", "");
    if (new->selection)
      new->selection (new, EINA_TRUE);

    if (item_selection) {
      edje_object_signal_emit (item_selection->edje, "FLHOC/menu/item,deselected", "");
      if (item_selection->selection)
        item_selection->selection (item_selection, EINA_FALSE);
    }

    menu->categories_selection = previous;

    edje_object_signal_emit (menu->edje, "FLHOC/menu,category_previous", "");
  }
}

static Eina_Bool
_menu_swallow_category (Menu *menu, Category *category, int index, const char *signal_suffix)
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
    Evas_Object *obj = edje_object_part_swallow_get (menu->edje, part);

    if (obj) {
      edje_object_part_unswallow (menu->edje, obj);
      evas_object_hide (obj);
    }

    edje_object_part_swallow (menu->edje, part, category->edje);
    if (signal_suffix) {
      snprintf (signal, sizeof(signal),  "FLHOC/menu,category_set%s%s",
          signal_suffix[0] == 0 ? "" : ",", signal_suffix);
      edje_object_signal_emit (menu->edje, signal, name);
    }
  } else {
    if (signal_suffix) {
      snprintf (signal, sizeof(signal),  "FLHOC/menu,category_unset%s%s",
          signal_suffix[0] == 0 ? "" : ",", signal_suffix);
      edje_object_signal_emit (menu->edje, signal, name);
      if (strcmp (signal_suffix, "reset") == 0) {
        Evas_Object *obj = edje_object_part_swallow_get (menu->edje, part);

        //printf ("Doing a reset, checking if there's a part there already (%s) : %p\n", name, obj);
        if (obj) {
          edje_object_part_unswallow (menu->edje, obj);
          evas_object_hide (obj);
        }
      }
    }
  }
  return EINA_TRUE;
}

static void
_menu_reset (Menu *menu)
{
  edje_object_signal_emit (menu->edje, "FLHOC/menu,category_reset", "");
  menu_set_categories (menu, "reset");
}

static const char *
category_type_to_group (CategoryType type)
{
  switch(type)
    {
      case CATEGORY_TYPE_QUIT:
        return "FLHOC/menu/category/quit";
      case CATEGORY_TYPE_SETTINGS:
        return "FLHOC/menu/category/settings";
      case CATEGORY_TYPE_THEME:
        return "FLHOC/menu/category/theme";
      case CATEGORY_TYPE_HOMEBREW:
        return "FLHOC/menu/category/homebrew";
      case CATEGORY_TYPE_DEVICE_HOMEBREW:
        return "FLHOC/menu/category/device_homebrew";
      case CATEGORY_TYPE_DEVICE_PACKAGES:
        return "FLHOC/menu/category/device_packages";
      default:
        return "";
    }
}

Category *
category_new (Menu *menu, CategoryType type)
{
  Category *category = calloc (1, sizeof(Category));
  MainWindow *main_win = menu->main_win;

  category->menu = menu;
  category->type = type;
  category->edje = edje_object_add (main_win->theme->evas);
  if (!edje_object_file_set (category->edje, main_win->theme->edje_file,
          category_type_to_group (type))) {
    category_free (category);
    return NULL;
  }
  edje_object_signal_callback_add (category->edje, "*", "*",
      _category_signal_cb, category);

  return category;
}

void
category_free (Category *category)
{
  Eina_List *cur = NULL;
  Item *item;

  EINA_LIST_FOREACH (category->items, cur, item)
      item_free (item);
  evas_object_del (category->edje);
  free (category);
}

void
category_set_items (Category *category, const char *signal)
{
  Eina_List *cur = NULL;
  Item *item;
  int index = 0;

  EINA_LIST_FOREACH (category->items, cur, item) {
      edje_object_part_unswallow (category->edje, item->edje);
      evas_object_hide (item->edje);
  }

  if (category->items_selection) {
    item = eina_list_data_get (category->items_selection);
    _category_swallow_item (category, item, 0, signal);
    for (cur = eina_list_prev (category->items_selection), index = -1; cur;
         cur =  eina_list_prev (cur), index--) {
      item = eina_list_data_get (cur);
      if (!_category_swallow_item (category, item, index, signal))
        break;
    }
    while (_category_swallow_item (category, NULL, index, signal)) index--;
    for (cur = eina_list_next (category->items_selection), index = 1; cur;
         cur =  eina_list_next (cur), index++) {
      item = eina_list_data_get (cur);
      if (!_category_swallow_item (category, item, index, signal))
        break;
    }
    while (_category_swallow_item (category, NULL, index, signal)) index++;
  }
}

Eina_Bool
category_append_item (Category *category, Item *item)
{
  if (eina_list_data_find (category->items, item) != NULL)
    return EINA_FALSE;

  category->items = eina_list_append (category->items, item);
  if (category->items_selection == NULL)
    category->items_selection = category->items;

  return EINA_TRUE;
}

Eina_Bool
category_delete_item (Category *category, Item *item)
{
  Eina_List *list = eina_list_data_find_list (category->items, item);

  if (list == NULL)
    return EINA_FALSE;

  if (category->items_selection == list) {
    Item *new_selection;
    Item *old_selection = category_get_selected_item (category);

    if (eina_list_prev (list) != NULL)
      category->items_selection = eina_list_prev (list);
    else if (eina_list_next (list) != NULL)
      category->items_selection = eina_list_next (list);
    else
      return EINA_FALSE; /* Can't remove last element */

    _category_reset (category);
    if (old_selection->selection)
      old_selection->selection (old_selection, EINA_FALSE);
    new_selection = category_get_selected_item (category);

    edje_object_signal_emit (new_selection->edje, "FLHOC/menu/item,selected", "");
    if (new_selection->selection)
      new_selection->selection (new_selection, EINA_TRUE);
  }

  category->items = eina_list_remove (category->items, item);
  return EINA_TRUE;
}

Item *
category_get_selected_item (Category *category)
{
  if (category)
    return eina_list_data_get (category->items_selection);
  return NULL;
}

void
category_scroll_next (Category *category)
{
  Item *old = eina_list_data_get (category->items_selection);
  Eina_List *next = eina_list_next (category->items_selection);
  Item *new = eina_list_data_get (next);

  _category_reset (category);
  if (next && new) {
    edje_object_signal_emit (old->edje, "FLHOC/menu/item,deselected", "");
    if (old->selection)
      old->selection (old, EINA_FALSE);
    edje_object_signal_emit (new->edje, "FLHOC/menu/item,selected", "");
    if (new->selection)
      new->selection (new, EINA_TRUE);
    category->items_selection = next;

    edje_object_signal_emit (category->edje, "FLHOC/menu/category,item_next", "");
  }
}

void
category_scroll_previous (Category *category)
{
  Item *old = eina_list_data_get (category->items_selection);
  Eina_List *previous = eina_list_prev (category->items_selection);
  Item *new = eina_list_data_get (previous);

  _category_reset (category);
  if (previous && new) {
    edje_object_signal_emit (old->edje, "FLHOC/menu/item,deselected", "");
    if (old->selection)
      old->selection (old, EINA_FALSE);
    edje_object_signal_emit (new->edje, "FLHOC/menu/item,selected", "");
    if (new->selection)
      new->selection (new, EINA_TRUE);
    category->items_selection = previous;

    edje_object_signal_emit (category->edje, "FLHOC/menu/category,item_previous", "");
  }
}

static Eina_Bool
_category_swallow_item (Category *category, Item *item,
    int index, const char *signal_suffix)
{
  char signal[40];
  char part[40] = "FLHOC/menu/category/item.";
  int prefix_len = strlen (part);
  char *name = part + prefix_len;

  if (index == 0)
    snprintf (name, sizeof(part) - prefix_len, "selection");
  else if (index < 0)
    snprintf (name, sizeof(part) - prefix_len, "before_%d", -index - 1);
  else
    snprintf (name, sizeof(part) - prefix_len, "after_%d", index - 1);

  if (!edje_object_part_exists (category->edje, part))
    return EINA_FALSE;

  if (item) {
    edje_object_part_swallow (category->edje, part, item->edje);
    if (signal_suffix) {
      snprintf (signal, sizeof(signal),  "FLHOC/menu/category,item_set%s%s",
          signal_suffix[0] == 0 ? "" : ",", signal_suffix);
      edje_object_signal_emit (category->edje, signal, name);
    }
  } else {
    if (signal_suffix) {
      if (strcmp (signal_suffix, "reset") == 0) {
        Evas_Object *obj = edje_object_part_swallow_get (category->edje, part);

        if (obj) {
          edje_object_part_unswallow (category->edje, obj);
          evas_object_hide (obj);
        }
      }
      snprintf (signal, sizeof(signal),  "FLHOC/menu/category,item_unset%s%s",
          signal_suffix[0] == 0 ? "" : ",", signal_suffix);
      edje_object_signal_emit (category->edje, signal, name);
    }
  }
  return EINA_TRUE;
}

static void
_category_reset (Category *category)
{
  edje_object_signal_emit (category->edje, "FLHOC/menu/category,item_reset", "");
  category_set_items (category, "reset");
}

static const char *
item_type_to_group (ItemType type)
{
  switch(type)
    {
      case ITEM_TYPE_GAME:
        return "FLHOC/menu/item/game";
      case ITEM_TYPE_THEME:
        return "FLHOC/menu/item/theme";
      case ITEM_TYPE_ABOUT:
        return "FLHOC/menu/item/about";
      case ITEM_TYPE_HELP:
        return "FLHOC/menu/item/help";
      case ITEM_TYPE_INSTALL_THEME:
        return "FLHOC/menu/item/install_theme";
      case ITEM_TYPE_WALLPAPER:
        return "FLHOC/menu/item/wallpaper";
      default:
        return "";
    }
}

Item *
item_new (Category *category, ItemType type)
{
  Item *item = calloc (1, sizeof(Item));
  Menu *menu = category->menu;
  MainWindow *main_win = menu->main_win;

  item->category = category;
  item->type = type;
  item->edje = edje_object_add (main_win->theme->evas);
  if (!edje_object_file_set (item->edje, main_win->theme->edje_file,
          item_type_to_group (type))) {
    item_free (item);
    return NULL;
  }
  edje_object_signal_callback_add (item->edje, "*", "*",
      _item_signal_cb, item);

  return item;
}

void
item_free (Item *item)
{
  evas_object_del (item->edje);
  free (item);
}


Secondary *
secondary_new (MainWindow *main_win, const char *group_name)
{
  Secondary *secondary = calloc (1, sizeof(Secondary));

  secondary->main_win = main_win;
  secondary->group = strdup (group_name);
  secondary->edje = edje_object_add (main_win->theme->evas);
  if (!edje_object_file_set (secondary->edje, main_win->theme->edje_file,
          group_name)) {
    secondary_free (secondary);
    return NULL;
  }
  secondary->secondary = edje_object_data_get (secondary->edje, "secondary");

  edje_object_signal_callback_add (secondary->edje, "*", "*",
      _secondary_signal_cb, secondary);

  return secondary;
}

void
secondary_free (Secondary *secondary)
{
  evas_object_del (secondary->edje);
  free (secondary->group);
  free (secondary);
}
