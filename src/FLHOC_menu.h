/*
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 *
 */

#ifndef __FLHOC_MENU_H__
#define __FLHOC_MENU_H__

#include <Edje.h>
#include <Evas.h>

typedef struct _Theme Theme;
typedef struct _MainWindow MainWindow;
typedef struct _Secondary Secondary;
typedef struct _Menu Menu;
typedef struct _Category Category;
typedef struct _Item Item;

struct _Theme {
  Evas *evas;
  char *edje_file;
  Eina_Bool installed;
  const char *name;
  const char *author;
  const char *version;
  const char *description;
  const char *compatible;
  MainWindow *main_win;
  void *priv;
};

struct _MainWindow {
  Theme *theme;
  Evas_Object *edje;
  Menu *menu;
  Secondary *secondary;
  void *priv;
};

struct _Secondary {
  MainWindow *main_win;
  Evas_Object *edje;
  const char *secondary;
  char *group;
  void *priv;
};

struct _Menu {
  MainWindow *main_win;
  Evas_Object *edje;
  const char *category_next;
  const char *category_previous;
  const char *item_next;
  const char *item_previous;
  Eina_List *categories;
  Eina_List *categories_selection;
  void *priv;
};

typedef enum {
  CATEGORY_TYPE_QUIT,
  CATEGORY_TYPE_SETTINGS,
  CATEGORY_TYPE_THEME,
  CATEGORY_TYPE_HOMEBREW,
  CATEGORY_TYPE_DEVICE_HOMEBREW,
  CATEGORY_TYPE_DEVICE_PACKAGES,
} CategoryType;

typedef void (*CategoryActionCb) (Category *Category);
typedef void (*CategorySelectionCb) (Category *Category, Eina_Bool selected);
struct _Category {
  Menu *menu;
  Evas_Object *edje;
  CategoryType type;
  Eina_List *items;
  Eina_List *items_selection;
  CategorySelectionCb selection;
  CategoryActionCb action;
  void *priv;
};

typedef enum {
  ITEM_TYPE_GAME,
  ITEM_TYPE_PACKAGE,
  ITEM_TYPE_THEME,
  ITEM_TYPE_USB_THEME,
  ITEM_TYPE_ABOUT,
  ITEM_TYPE_HELP,
  ITEM_TYPE_WALLPAPER,
} ItemType;

typedef void (*ItemActionCb) (Item *item);
typedef void (*ItemSelectionCb) (Item *item, Eina_Bool selected);
struct _Item {
  Category *category;
  Evas_Object *edje;
  ItemType type;
  ItemSelectionCb selection;
  ItemActionCb action;
  void *priv;
};

Eina_Bool theme_file_is_valid (Evas *evas, const char *filename);

Theme *theme_new (Evas *evas, const char *filename);
void theme_free (Theme *theme);

MainWindow *main_window_new (Theme *theme);
void main_window_free (MainWindow *main_win);
void main_window_init (MainWindow *main_win);
void main_window_set_secondary (MainWindow *main_win, Secondary *secondary);

Menu *menu_new (MainWindow *main_win);
void menu_free (Menu *menu);
void menu_set_categories (Menu *menu, const char *signal);
Eina_Bool menu_append_category (Menu *menu, Category *category);
Eina_Bool menu_delete_category (Menu *menu, Category *category);
Category *menu_get_selected_category (Menu *menu);
void menu_scroll_previous (Menu *menu);
void menu_scroll_next (Menu *menu);
void menu_reset (Menu *menu);

Category *category_new (Menu *menu, CategoryType type);
void category_free (Category *category);
void category_set_items (Category *category, const char *signal);
Eina_Bool category_append_item (Category *category, Item *item);
Eina_Bool category_delete_item (Category *category, Item *item);
Item *category_get_selected_item (Category *category);
void category_scroll_previous (Category *category);
void category_scroll_next (Category *category);
void category_reset (Category *category);

Item *item_new (Category *category, ItemType type);
void item_free (Item *item);

Secondary *secondary_new (MainWindow *main_win, const char *group_name);
void secondary_free (Secondary *secondary);

#endif /* __FLHOC_MENU_H__ */

