#include <stdio.h>
#include <stdlib.h>


#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>

static void
_main_signal_cb (void *data,
    Evas_Object *obj,
    const char  *emission,
    const char  *source)
{
  printf ("Main: Signal %s coming from part %s (%p == %p)!\n",
      emission, source, obj, data);

  if (strcmp (source, "forward") == 0) {
    Evas_Object *flhoc = edje_object_part_swallow_get (obj, "FLHOC");

    printf ("Forwarding message to %p\n", flhoc);
    if (flhoc)
      edje_object_signal_emit (flhoc, emission, source);
  }
}

static void
_flhoc_signal_cb (void *data,
    Evas_Object *obj,
    const char  *emission,
    const char  *source)
{
  printf ("FLHOC: Signal %s coming from part %s (%p == %p)!\n",
      emission, source, obj, data);

  if (strcmp (source, "forward") == 0) {
    Evas_Object *flhoc = edje_object_part_object_get (obj, "FLHOC");

    printf ("Forwarding message to %p\n", flhoc);
    if (flhoc)
      edje_object_signal_emit (flhoc, emission, "");
  }
}

static void
_category_signal_cb (void *data,
    Evas_Object *obj,
    const char  *emission,
    const char  *source)
{
  printf ("Signal %s coming from part %s (%p == %p)!\n",
      emission, source, obj, data);

  if (strcmp (source, "FLHOC") == 0) {
    Evas_Object *box = edje_object_part_object_get (obj, "FLHOC/items");
    Eina_List *children, *l;

    children = evas_object_box_children_get (box);
    EINA_LIST_FOREACH(children, l, data) {
        edje_object_signal_emit (data, emission, source);
    }
  }
}


static Evas_Object *
_add_categories (Ecore_Evas *ee, Evas *evas, Evas_Object *edje)
{
  Evas_Object *category = NULL;
  Evas_Coord w, h;

  category = edje_object_add (evas);
  edje_object_file_set (category, "data/themes/default/default.edj", "FLHOC/category/settings");
  edje_object_part_swallow (edje, "FLHOC/category.before_2", category);
  category = edje_object_add (evas);
  edje_object_file_set (category, "data/themes/default/default.edj", "FLHOC/category/settings");
  edje_object_part_swallow (edje, "FLHOC/category.before_1", category);
  category = edje_object_add (evas);
  edje_object_file_set (category, "data/themes/default/default.edj", "FLHOC/category/homebrew");
  edje_object_part_swallow (edje, "FLHOC/category.before_0", category);
  category = edje_object_add (evas);
  edje_object_file_set (category, "data/themes/default/default.edj", "FLHOC/category/homebrew");
  edje_object_part_swallow (edje, "FLHOC/category.selection", category);
  edje_object_signal_emit (category, "FLHOC/category_selected", "");
  category = edje_object_add (evas);
  edje_object_file_set (category, "data/themes/default/default.edj", "FLHOC/category/device");
  edje_object_part_swallow (edje, "FLHOC/category.after_0", category);
  category = edje_object_add (evas);
  edje_object_file_set (category, "data/themes/default/default.edj", "FLHOC/category/device");
  edje_object_part_swallow (edje, "FLHOC/category.after_1", category);
  category = edje_object_add (evas);

  return category;
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

  return item;
}

int
main (int argc, char *argv[])
{
  Ecore_Evas *ee = NULL;
  Evas *evas = NULL;
  Evas_Object *edje = NULL;
  Evas_Object *flhoc = NULL;
  Evas_Coord w, h;
  Evas_Object *hb;

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
  ee = ecore_evas_new (NULL, 0, 0, 640, 480, NULL);
  //ecore_evas_fullscreen_set (ee, EINA_TRUE);
  ecore_evas_show (ee);
  evas = ecore_evas_get (ee);
  evas_font_path_append (evas, "./data/");
  edje = edje_object_add (evas);
  flhoc = edje_object_add (evas);
  edje_object_file_set (edje, "data/themes/default/default.edj", "main");
  edje_object_file_set (flhoc, "data/themes/default/default.edj", "FLHOC");

  ecore_evas_object_associate(ee, edje, ECORE_EVAS_OBJECT_ASSOCIATE_BASE);
  edje_object_signal_callback_add (edje, "*", "*", _main_signal_cb, flhoc);
  edje_object_signal_callback_add (flhoc, "*", "*", _flhoc_signal_cb, flhoc);

  evas_object_move (edje, 0, 0);
  evas_output_size_get (evas, &w, &h);
  evas_object_resize (edje, w, h);
  evas_object_show (edje);

  edje_object_part_swallow (edje, "FLHOC", flhoc);
  _add_categories (ee, evas, flhoc);

  ecore_main_loop_begin ();
 edje_shutdown:
  edje_shutdown ();
 ecore_shutdown:
  ecore_shutdown ();

  return 0;
}
