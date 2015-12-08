/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2014-2015 - Jay McCarthy
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>

#include <retro_inline.h>

#include "menu_driver.h"
#include "menu_cbs.h"
#include "menu_hash.h"

#include "../general.h"
#include "../system.h"

struct menu_list
{
   file_list_t **menu_stack;
   size_t menu_stack_size;
   file_list_t **selection_buf;
   size_t selection_buf_size;
};

struct menu_entries
{
   /* Flagged when menu entries need to be refreshed */
   bool need_refresh;
   bool nonblocking_refresh;

   size_t begin;
   menu_list_t *menu_list;
   rarch_setting_t *list_settings;
};

static menu_entries_t *menu_entries_data;

static void menu_list_free_list(file_list_t *list)
{
   unsigned i;

   for (i = 0; i < list->size; i++)
      menu_driver_list_free(list, i, list->size);

   if (list)
      file_list_free(list);
}

static void menu_list_free(menu_list_t *menu_list)
{
   unsigned i;
   if (!menu_list)
      return;

   for (i = 0; i < menu_list->menu_stack_size; i++)
   {
      if (!menu_list->menu_stack[i])
         continue;

      menu_list_free_list(menu_list->menu_stack[i]);
      menu_list->menu_stack[i]    = NULL;
   }
   for (i = 0; i < menu_list->selection_buf_size; i++)
   {
      if (!menu_list->selection_buf[i])
         continue;

      menu_list_free_list(menu_list->selection_buf[i]);
      menu_list->selection_buf[i] = NULL;
   }

   free(menu_list->menu_stack);
   free(menu_list->selection_buf);

   free(menu_list);
}

static menu_list_t *menu_list_new(void)
{
   unsigned i;
   menu_list_t           *list = (menu_list_t*)calloc(1, sizeof(*list));

   if (!list)
      return NULL;

   list->menu_stack          = (file_list_t**)calloc(1, sizeof(*list->menu_stack));

   if (!list->menu_stack)
      goto error;

   list->selection_buf       = (file_list_t**)calloc(1, sizeof(*list->selection_buf));

   if (!list->selection_buf)
      goto error;

   list->menu_stack_size      = 1;
   list->selection_buf_size   = 1;

   for (i = 0; i < list->menu_stack_size; i++)
      list->menu_stack[i]    = (file_list_t*)calloc(1, sizeof(*list->menu_stack[i]));

   for (i = 0; i < list->selection_buf_size; i++)
      list->selection_buf[i] = (file_list_t*)calloc(1, sizeof(*list->selection_buf[i]));

   return list;

error:
   menu_list_free(list);
   return NULL;
}

static size_t menu_list_get_stack_size(menu_list_t *list, size_t idx)
{
   if (!list)
      return 0;
   return file_list_get_size(list->menu_stack[idx]);
}

void menu_entries_get_at_offset(const file_list_t *list, size_t idx,
      const char **path, const char **label, unsigned *file_type,
      size_t *entry_idx, const char **alt)
{
   file_list_get_at_offset(list, idx, path, label, file_type, entry_idx);
   file_list_get_alt_at_offset(list, idx, alt);
}

void menu_entries_get_last(const file_list_t *list,
      const char **path, const char **label,
      unsigned *file_type, size_t *entry_idx)
{
   if (list)
      file_list_get_last(list, path, label, file_type, entry_idx);
}

void *menu_entries_get_userdata_at_offset(const file_list_t *list, size_t idx)
{
   if (!list)
      return NULL;
   return file_list_get_userdata_at_offset(list, idx);
}

menu_file_list_cbs_t *menu_entries_get_actiondata_at_offset(
      const file_list_t *list, size_t idx)
{
   if (!list)
      return NULL;
   return (menu_file_list_cbs_t*)
      file_list_get_actiondata_at_offset(list, idx);
}

static int menu_entries_flush_stack_type(const char *needle, const char *label,
      unsigned type, unsigned final_type)
{
   return needle ? strcmp(needle, label) : (type != final_type);
}

static bool menu_list_pop_stack(menu_list_t *list, size_t idx, size_t *directory_ptr)
{
   file_list_t *menu_list = NULL;
   if (!list)
      return false;

   menu_list = list->menu_stack[idx];

   if (menu_list_get_stack_size(list, idx) <= 1)
      return false;

   menu_driver_list_cache(MENU_LIST_PLAIN, 0);

   if (menu_list->size != 0)
      menu_driver_list_free(menu_list, menu_list->size - 1, menu_list->size - 1);

   file_list_pop(menu_list, directory_ptr);
   menu_driver_list_set_selection(menu_list);

   menu_entries_set_refresh(false);

   return true;
}

static void menu_list_flush_stack(menu_list_t *list,
      size_t idx, const char *needle, unsigned final_type)
{
   const char *path       = NULL;
   const char *label      = NULL;
   unsigned type          = 0;
   size_t entry_idx       = 0;
   if (!list)
      return;

   menu_entries_set_refresh(false);
   menu_entries_get_last(list->menu_stack[idx],
         &path, &label, &type, &entry_idx);

   while (menu_entries_flush_stack_type(
            needle, label, type, final_type) != 0)
   {
      size_t new_selection_ptr;

      menu_navigation_ctl(MENU_NAVIGATION_CTL_GET_SELECTION, &new_selection_ptr);

      if (!menu_list_pop_stack(list, idx, &new_selection_ptr))
         break;

      menu_navigation_ctl(MENU_NAVIGATION_CTL_SET_SELECTION, &new_selection_ptr);

      menu_entries_get_last(list->menu_stack[idx],
            &path, &label, &type, &entry_idx);
   }
}

void menu_entries_clear(file_list_t *list)
{
   unsigned i;
   const menu_ctx_driver_t *driver = menu_ctx_driver_get_ptr();

   if (driver->list_clear)
      driver->list_clear(list);

   for (i = 0; i < list->size; i++)
      file_list_free_actiondata(list, i);

   if (list)
      file_list_clear(list);
}

void menu_entries_set_alt_at_offset(file_list_t *list, size_t idx,
      const char *alt)
{
   file_list_set_alt_at_offset(list, idx, alt);
}

/**
 * menu_entries_elem_is_dir:
 * @list                     : File list handle.
 * @offset                   : Offset index of element.
 *
 * Is the current entry at offset @offset a directory?
 *
 * Returns: true (1) if entry is a directory, otherwise false (0).
 **/
static bool menu_entries_elem_is_dir(file_list_t *list,
      unsigned offset)
{
   unsigned type     = 0;

   menu_entries_get_at_offset(list, offset, NULL, NULL, &type, NULL, NULL);

   return type == MENU_FILE_DIRECTORY;
}

/**
 * menu_entries_elem_get_first_char:
 * @list                     : File list handle.
 * @offset                   : Offset index of element.
 *
 * Gets the first character of an element in the
 * file list.
 *
 * Returns: first character of element in file list.
 **/
static int menu_entries_elem_get_first_char(
      file_list_t *list, unsigned offset)
{
   int ret;
   const char *path = NULL;

   menu_entries_get_at_offset(list, offset,
         NULL, NULL, NULL, NULL, &path);

   ret = tolower((int)*path);

   /* "Normalize" non-alphabetical entries so they
    * are lumped together for purposes of jumping. */
   if (ret < 'a')
      ret = 'a' - 1;
   else if (ret > 'z')
      ret = 'z' + 1;
   return ret;
}

static void menu_entries_build_scroll_indices(file_list_t *list)
{
   int current;
   bool current_is_dir;
   size_t i, scroll_value   = 0;

   if (!list || !list->size)
      return;

   menu_navigation_ctl(MENU_NAVIGATION_CTL_CLEAR_SCROLL_INDICES, NULL);
   menu_navigation_ctl(MENU_NAVIGATION_CTL_ADD_SCROLL_INDEX, &scroll_value);

   current        = menu_entries_elem_get_first_char(list, 0);
   current_is_dir = menu_entries_elem_is_dir(list, 0);

   for (i = 1; i < list->size; i++)
   {
      int first   = menu_entries_elem_get_first_char(list, i);
      bool is_dir = menu_entries_elem_is_dir(list, i);

      if ((current_is_dir && !is_dir) || (first > current))
         menu_navigation_ctl(MENU_NAVIGATION_CTL_ADD_SCROLL_INDEX, &i);

      current        = first;
      current_is_dir = is_dir;
   }


   scroll_value = list->size - 1;
   menu_navigation_ctl(MENU_NAVIGATION_CTL_ADD_SCROLL_INDEX, &scroll_value);
}

/**
 * Before a refresh, we could have deleted a
 * file on disk, causing selection_ptr to
 * suddendly be out of range.
 *
 * Ensure it doesn't overflow.
 **/
void menu_entries_refresh(file_list_t *list)
{
   size_t list_size, selection;
   if (!list)
      return;
   if (!menu_navigation_ctl(MENU_NAVIGATION_CTL_GET_SELECTION, &selection))
      return;

   menu_entries_build_scroll_indices(list);

   list_size = menu_entries_get_size();

   if ((selection >= list_size) && list_size)
   {
      size_t idx  = list_size - 1;
      bool scroll = true;
      menu_navigation_ctl(MENU_NAVIGATION_CTL_SET_SELECTION, &idx);
      menu_navigation_ctl(MENU_NAVIGATION_CTL_SET, &scroll);
   }
   else if (!list_size)
   {
      bool pending_push = true;
      menu_navigation_ctl(MENU_NAVIGATION_CTL_CLEAR, &pending_push);
   }
}

rarch_setting_t *menu_setting_get_ptr(void)
{
   menu_entries_t *entries = menu_entries_data;

   if (!entries)
      return NULL;
   return entries->list_settings;
}

menu_list_t *menu_list_get_ptr(void)
{
   menu_entries_t *entries = menu_entries_data;
   if (!entries)
      return NULL;
   return entries->menu_list;
}

/* Sets the starting index of the menu entry list. */
void menu_entries_set_start(size_t i)
{
   menu_entries_t *entries = menu_entries_data;
   
   if (entries)
      entries->begin = i;
}

/* Returns the starting index of the menu entry list. */
size_t menu_entries_get_start(void)
{
   menu_entries_t *entries = menu_entries_data;
   
   if (!entries)
     return 0;

   return entries->begin;
}

/* Returns the last index (+1) of the menu entry list. */
size_t menu_entries_get_end(void)
{
   return menu_entries_get_size();
}

/* Get an entry from the top of the menu stack */
void menu_entries_get(size_t i, menu_entry_t *entry)
{
   const char *label          = NULL;
   const char *path           = NULL;
   const char *entry_label    = NULL;
   menu_file_list_cbs_t *cbs  = NULL;
   file_list_t *selection_buf = menu_entries_get_selection_buf_ptr(0);

   menu_entries_get_last_stack(NULL, &label, NULL, NULL);

   entry->path[0] = entry->value[0] = entry->label[0] = '\0';

   menu_entries_get_at_offset(selection_buf, i,
         &path, &entry_label, &entry->type, &entry->entry_idx, NULL);

   cbs = menu_entries_get_actiondata_at_offset(selection_buf, i);

   if (cbs && cbs->action_get_value)
      cbs->action_get_value(selection_buf,
            &entry->spacing, entry->type, i, label,
            entry->value,  sizeof(entry->value),
            entry_label, path,
            entry->path, sizeof(entry->path));

   entry->idx = i;

   if (entry_label)
      strlcpy(entry->label, entry_label, sizeof(entry->label));
}

/* Sets title to what the name of the current menu should be. */
int menu_entries_get_title(char *s, size_t len)
{
   unsigned menu_type        = 0;
   const char *path          = NULL;
   const char *label         = NULL;
   menu_file_list_cbs_t *cbs = menu_entries_get_last_stack_actiondata();
   
   if (!cbs)
      return -1;

   menu_entries_get_last_stack(&path, &label, &menu_type, NULL);

   if (cbs && cbs->action_get_title)
      return cbs->action_get_title(path, label, menu_type, s, len);
   return 0;
}

/* Returns true if a Back button should be shown (i.e. we are at least
 * one level deep in the menu hierarchy). */
bool menu_entries_show_back(void)
{
   return (menu_entries_get_stack_size(0) > 1);
}

/* Sets 's' to the name of the current core 
 * (shown at the top of the UI). */
int menu_entries_get_core_title(char *s, size_t len)
{
   settings_t *settings           = config_get_ptr();
   rarch_system_info_t      *info = rarch_system_info_get_ptr();
   const char *core_name          = g_system_menu.library_name;
   const char *core_version       = g_system_menu.library_version;

   if (!settings->menu.core_enable)
      return -1; 

   if (!core_name || core_name[0] == '\0')
      core_name = info->info.library_name;
   if (!core_name || core_name[0] == '\0')
      core_name = menu_hash_to_str(MENU_VALUE_NO_CORE);

   if (!core_version)
      core_version = info->info.library_version;
   if (!core_version)
      core_version = "";

   snprintf(s, len, "%s - %s %s", PACKAGE_VERSION,
         core_name, core_version);

   return 0;
}

file_list_t *menu_entries_get_menu_stack_ptr(size_t idx)
{
   menu_list_t *menu_list = menu_list_get_ptr();
   if (!menu_list)
      return NULL;
   return menu_list->menu_stack[idx];
}

file_list_t *menu_entries_get_selection_buf_ptr(size_t idx)
{
   menu_list_t *menu_list = menu_list_get_ptr();
   if (!menu_list)
      return NULL;
   return menu_list->selection_buf[idx];
}

bool menu_entries_needs_refresh(void)
{
   menu_entries_t *entries = menu_entries_data;

   if (!entries || entries->nonblocking_refresh)
      return false;
   if (entries->need_refresh)
      return true;
   return false;
}

void menu_entries_set_refresh(bool nonblocking)
{
   menu_entries_t *entries = menu_entries_data;
   if (entries)
   {
      if (nonblocking)
         entries->nonblocking_refresh = true;
      else
         entries->need_refresh        = true;
   }
}

void menu_entries_unset_refresh(bool nonblocking)
{
   menu_entries_t *entries = menu_entries_data;
   if (entries)
   {
      if (nonblocking)
         entries->nonblocking_refresh = false;
      else
         entries->need_refresh        = false;
   }
}

bool menu_entries_init(void *data)
{
   menu_entries_t *entries = (menu_entries_t*)
      calloc(1, sizeof(*entries));

   if (!entries)
      goto error;

   if (!(entries->menu_list = (menu_list_t*)menu_list_new()))
      goto error;

   menu_entries_data = (struct menu_entries*)entries;

   entries->list_settings      = menu_setting_new();

   return true;

error:
   if (entries)
      free(entries);
   return false;
}

void menu_entries_free(void)
{
   menu_entries_t *entries = menu_entries_data;

   if (!entries)
      return;

   if (entries->list_settings)
      menu_setting_free(entries->list_settings);
   entries->list_settings = NULL;

   menu_list_free(entries->menu_list);
   entries->menu_list     = NULL;

   free(menu_entries_data);
   menu_entries_data = NULL;
}

void menu_entries_push(file_list_t *list, const char *path, const char *label,
      unsigned type, size_t directory_ptr, size_t entry_idx)
{
   size_t idx;
   const menu_ctx_driver_t *driver = menu_ctx_driver_get_ptr();
   menu_file_list_cbs_t *cbs       = NULL;
   if (!list || !label)
      return;

   file_list_push(list, path, label, type, directory_ptr, entry_idx);

   idx = list->size - 1;

   if (driver->list_insert)
      driver->list_insert(list, path, label, idx);

   file_list_free_actiondata(list, idx);
   cbs = (menu_file_list_cbs_t*)
      calloc(1, sizeof(menu_file_list_cbs_t));

   if (!cbs)
      return;

   file_list_set_actiondata(list, idx, cbs);

   cbs->setting = menu_setting_find(label);

   menu_cbs_init(list, cbs, path, label, type, idx);
}

bool menu_entries_increment_selection_buf(void)
{
   file_list_t **selection_buf    = NULL;
   menu_list_t *menu_list         = menu_list_get_ptr();

   if (!menu_list)
      return false;

   selection_buf = (file_list_t**)realloc(selection_buf, (menu_list->selection_buf_size + 1) * sizeof(*menu_list->selection_buf));

   if (!selection_buf)
      goto error;

   menu_list->selection_buf = selection_buf;
   menu_list->selection_buf[menu_list->selection_buf_size] = (file_list_t*)calloc(1, sizeof(*menu_list->selection_buf[menu_list->selection_buf_size]));
   menu_list->selection_buf_size = menu_list->selection_buf_size + 1;

   return true;

error:
   if (selection_buf)
      free(selection_buf);
   return false;
}

bool menu_entries_increment_menu_stack(void)
{
   file_list_t **menu_stack       = NULL;
   menu_list_t *menu_list         = menu_list_get_ptr();

   if (!menu_list)
      return false;

   menu_stack = (file_list_t**)realloc(menu_stack, (menu_list->menu_stack_size + 1) * sizeof(*menu_list->menu_stack));

   if (!menu_stack)
      goto error;

   menu_list->menu_stack = menu_stack;
   menu_list->menu_stack[menu_list->menu_stack_size] = (file_list_t*)calloc(1, sizeof(*menu_list->menu_stack[menu_list->menu_stack_size]));
   menu_list->menu_stack_size = menu_list->menu_stack_size + 1;

   return true;

error:
   if (menu_stack)
      free(menu_stack);
   return false;
}

menu_file_list_cbs_t *menu_entries_get_last_stack_actiondata(void)
{
   menu_list_t *menu_list         = menu_list_get_ptr();
   if (!menu_list)
      return NULL;
   return (menu_file_list_cbs_t*)file_list_get_last_actiondata(menu_list->menu_stack[0]);
}

void menu_entries_get_last_stack(const char **path, const char **label,
      unsigned *file_type, size_t *entry_idx)
{
   menu_list_t *menu_list         = menu_list_get_ptr();
   if (menu_list)
      menu_entries_get_last(menu_list->menu_stack[0], path, label, file_type, entry_idx);
}

void menu_entries_flush_stack(const char *needle, unsigned final_type)
{
   menu_list_t *menu_list         = menu_list_get_ptr();
   if (menu_list)
      menu_list_flush_stack(menu_list, 0, needle, final_type);
}

void menu_entries_pop_stack(size_t *ptr, size_t idx)
{
   menu_list_t *menu_list         = menu_list_get_ptr();
   if (menu_list)
      menu_list_pop_stack(menu_list, idx, ptr);
}

size_t menu_entries_get_stack_size(size_t idx)
{
   menu_list_t *menu_list         = menu_list_get_ptr();
   if (!menu_list)
      return 0;
   return menu_list_get_stack_size(menu_list, idx);
}

size_t menu_entries_get_size(void)
{
   menu_list_t *menu_list         = menu_list_get_ptr();
   if (!menu_list)
      return 0;
   return file_list_get_size(menu_list->selection_buf[0]);
}

rarch_setting_t *menu_entries_get_setting(uint32_t i)
{
   file_list_t *selection_buf = menu_entries_get_selection_buf_ptr(0);
   menu_file_list_cbs_t *cbs  = menu_entries_get_actiondata_at_offset(selection_buf, i);

   if (!cbs)
      return NULL;
   return cbs->setting;
}
