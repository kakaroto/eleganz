// 2011 Ninjas
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#ifndef __FLHOC_MENU_H__
#define __FLHOC_MENU_H__

#include <Edje.h>
#include <Evas.h>

typedef struct _MainWindow MainWindow;
typedef struct _Menu Menu;
typedef struct _Category Category;
typedef struct _Item Item;

struct _MainWindow {
  char *edje_file;
  Evas *evas;
  Evas_Object *edje;
  Menu *menu;
};

struct _Menu {
  MainWindow *parent;
  Evas_Object *edje;
  const char *category_next;
  const char *category_previous;
  const char *item_next;
  const char *item_previous;
  Eina_List *categories;
  Eina_List *categories_selection;
};

struct _Category {
  Menu *parent;
  Evas_Object *edje;
  char *group;
  Eina_List *items;
  Eina_List *items_selection;
};

struct _Item {
  Category *parent;
  Evas_Object *edje;
  char *group;
};


Eina_Bool theme_file_is_valid (Evas *evas, const char *filename);

MainWindow *main_window_new (Evas *evas, const char *filename);
void main_window_free (MainWindow *main_win);
void main_window_set_menu (MainWindow *main, Menu *menu);
void main_window_init (MainWindow *main_win, Menu *menu);

Menu *menu_new (MainWindow *main);
void menu_free (Menu *menu);
void menu_set_categories (Menu *menu, const char *signal);
Eina_Bool menu_append_category (Menu *menu, Category *category);
Eina_Bool menu_delete_category (Menu *menu, Category *category);
Category *menu_get_selected_category (Menu *menu);
void menu_scroll_previous (Menu *menu);
void menu_scroll_next (Menu *menu);
void menu_reset (Menu *menu);

Category *category_new (Menu *menu, const char *group);
void category_free (Category *category);
void category_set_items (Category *category, const char *signal);
Eina_Bool category_append_item (Category *category, Item *item);
Eina_Bool category_delete_item (Category *category, Item *item);
Item *category_get_selected_item (Category *category);
void category_scroll_previous (Category *category);
void category_scroll_next (Category *category);
void category_reset (Category *category);

Item *item_new (Category *category, const char *group);
void item_free (Item *item);

#endif /* __FLHOC_MENU_H__ */

