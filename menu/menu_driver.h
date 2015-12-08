/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2015 - Daniel De Matteis
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

#ifndef __MENU_DRIVER_H__
#define __MENU_DRIVER_H__

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <boolean.h>
#include <retro_miscellaneous.h>

#include "menu_displaylist.h"
#include "menu_entries.h"
#include "menu_input.h"
#include "menu_navigation.h"
#include "menu_setting.h"

#include "../driver.h"
#include "../libretro.h"
#include "../playlist.h"
#include "../input/input_driver.h"
#include "../dynamic.h"
#include "../gfx/video_shader_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_COUNTERS
#define MAX_COUNTERS 64
#endif

#ifndef MAX_CHEAT_COUNTERS
#define MAX_CHEAT_COUNTERS 100
#endif

#define MENU_SETTINGS_CORE_INFO_NONE             0xffff
#define MENU_SETTINGS_CORE_OPTION_NONE           0xffff
#define MENU_SETTINGS_CHEEVOS_NONE               0xffff
#define MENU_SETTINGS_CORE_OPTION_CREATE         0x05000
#define MENU_SETTINGS_CORE_OPTION_START          0x10000
#define MENU_SETTINGS_PLAYLIST_ASSOCIATION_START 0x20000
#define MENU_SETTINGS_CHEEVOS_START              0x40000


#define MENU_KEYBOARD_BIND_TIMEOUT_SECONDS 5

typedef enum
{
   MENU_IMAGE_NONE = 0,
   MENU_IMAGE_WALLPAPER,
   MENU_IMAGE_BOXART
} menu_image_type_t;

typedef enum
{
   MENU_ENVIRON_NONE = 0,
   MENU_ENVIRON_RESET_HORIZONTAL_LIST,
   MENU_ENVIRON_LAST
} menu_environ_cb_t;

typedef enum
{
   MENU_HELP_NONE       = 0,
   MENU_HELP_WELCOME,
   MENU_HELP_EXTRACT,
   MENU_HELP_CONTROLS,
   MENU_HELP_CHEEVOS_DESCRIPTION,
   MENU_HELP_LOADING_CONTENT,
   MENU_HELP_WHAT_IS_A_CORE,
   MENU_HELP_CHANGE_VIRTUAL_GAMEPAD,
   MENU_HELP_AUDIO_VIDEO_TROUBLESHOOTING,
   MENU_HELP_SCANNING_CONTENT,
   MENU_HELP_LAST
} menu_help_type_t;

enum rarch_menu_ctl_state
{
   RARCH_MENU_CTL_NONE = 0,
   RARCH_MENU_CTL_DEINIT,
   RARCH_MENU_CTL_FRAME,
   RARCH_MENU_CTL_SET_PREVENT_POPULATE,
   RARCH_MENU_CTL_UNSET_PREVENT_POPULATE,
   RARCH_MENU_CTL_IS_PREVENT_POPULATE,
   RARCH_MENU_CTL_SET_TEXTURE,
   RARCH_MENU_CTL_SET_TOGGLE,
   RARCH_MENU_CTL_UNSET_TOGGLE,
   RARCH_MENU_CTL_SET_ALIVE,
   RARCH_MENU_CTL_UNSET_ALIVE,
   RARCH_MENU_CTL_IS_ALIVE,
   RARCH_MENU_CTL_DESTROY,
   RARCH_MENU_CTL_IS_SET_TEXTURE,
   RARCH_MENU_CTL_SET_OWN_DRIVER,
   RARCH_MENU_CTL_UNSET_OWN_DRIVER,
   RARCH_MENU_CTL_OWNS_DRIVER,
   RARCH_MENU_CTL_LOAD_NO_CONTENT_GET,
   RARCH_MENU_CTL_HAS_LOAD_NO_CONTENT,
   RARCH_MENU_CTL_SET_LOAD_NO_CONTENT,
   RARCH_MENU_CTL_UNSET_LOAD_NO_CONTENT
};

typedef enum
{
   MENU_FILE_NONE = 0,
   MENU_FILE_PLAIN,
   MENU_FILE_DIRECTORY,
   MENU_FILE_PARENT_DIRECTORY,
   MENU_FILE_PATH,
   MENU_FILE_DEVICE,
   MENU_FILE_CORE,
   MENU_FILE_PLAYLIST_ENTRY,
   MENU_FILE_CONTENTLIST_ENTRY,
   MENU_FILE_SHADER_PRESET,
   MENU_FILE_SHADER,
   MENU_FILE_VIDEOFILTER,
   MENU_FILE_AUDIOFILTER,
   MENU_FILE_CHEAT,
   MENU_FILE_OVERLAY,
   MENU_FILE_FONT,
   MENU_FILE_CONFIG,
   MENU_FILE_USE_DIRECTORY,
   MENU_FILE_SCAN_DIRECTORY,
   MENU_FILE_CARCHIVE,
   MENU_FILE_IN_CARCHIVE,
   MENU_FILE_IMAGE,
   MENU_FILE_IMAGEVIEWER,
   MENU_FILE_REMAP,
   MENU_FILE_DOWNLOAD_CORE,
   MENU_FILE_DOWNLOAD_CORE_CONTENT,
   MENU_FILE_DOWNLOAD_CORE_INFO,
   MENU_FILE_RDB,
   MENU_FILE_RDB_ENTRY,
   MENU_FILE_RPL_ENTRY,
   MENU_FILE_CURSOR,
   MENU_FILE_RECORD_CONFIG,
   MENU_FILE_PLAYLIST_COLLECTION,
   MENU_FILE_PLAYLIST_ASSOCIATION,
   MENU_FILE_MOVIE,
   MENU_FILE_MUSIC,
   MENU_SETTINGS,
   MENU_SETTINGS_TAB,
   MENU_HISTORY_TAB,
   MENU_ADD_TAB,
   MENU_PLAYLISTS_TAB,
   MENU_SETTING_NO_ITEM,
   MENU_SETTING_DRIVER,
   MENU_SETTING_ACTION,
   MENU_SETTING_ACTION_RUN,
   MENU_SETTING_ACTION_CLOSE,
   MENU_SETTING_ACTION_CORE_OPTIONS,
   MENU_SETTING_ACTION_CORE_INPUT_REMAPPING_OPTIONS,
   MENU_SETTING_ACTION_CORE_CHEAT_OPTIONS,
   MENU_SETTING_ACTION_CORE_INFORMATION,
   MENU_SETTING_ACTION_CORE_DISK_OPTIONS,
   MENU_SETTING_ACTION_CORE_SHADER_OPTIONS,
   MENU_SETTING_ACTION_SAVESTATE,
   MENU_SETTING_ACTION_LOADSTATE,
   MENU_SETTING_ACTION_SCREENSHOT,
   MENU_SETTING_ACTION_RESET,
   MENU_SETTING_STRING_OPTIONS,
   MENU_SETTING_GROUP,
   MENU_SETTING_SUBGROUP,
   MENU_SETTING_HORIZONTAL_MENU,
   MENU_INFO_MESSAGE,
   MENU_FILE_TYPE_T_LAST
} menu_file_type_t;

typedef enum
{
   MENU_SETTINGS_NONE       = MENU_FILE_TYPE_T_LAST + 1,
   MENU_SETTINGS_SHADER_PARAMETER_0,
   MENU_SETTINGS_SHADER_PARAMETER_LAST = MENU_SETTINGS_SHADER_PARAMETER_0 + (GFX_MAX_PARAMETERS - 1),
   MENU_SETTINGS_SHADER_PRESET_PARAMETER_0,
   MENU_SETTINGS_SHADER_PRESET_PARAMETER_LAST = MENU_SETTINGS_SHADER_PRESET_PARAMETER_0 + (GFX_MAX_PARAMETERS - 1),
   MENU_SETTINGS_SHADER_PASS_0,
   MENU_SETTINGS_SHADER_PASS_LAST = MENU_SETTINGS_SHADER_PASS_0 + (GFX_MAX_SHADERS - 1),
   MENU_SETTINGS_SHADER_PASS_FILTER_0,
   MENU_SETTINGS_SHADER_PASS_FILTER_LAST = MENU_SETTINGS_SHADER_PASS_FILTER_0  + (GFX_MAX_SHADERS - 1),
   MENU_SETTINGS_SHADER_PASS_SCALE_0,
   MENU_SETTINGS_SHADER_PASS_SCALE_LAST = MENU_SETTINGS_SHADER_PASS_SCALE_0  + (GFX_MAX_SHADERS - 1),

   MENU_SETTINGS_CORE_DISK_OPTIONS_DISK_INDEX,
   MENU_SETTINGS_CORE_DISK_OPTIONS_DISK_IMAGE_APPEND,
   MENU_SETTINGS_CORE_DISK_OPTIONS_DISK_CYCLE_TRAY_STATUS,

   MENU_SETTINGS_BIND_BEGIN,
   MENU_SETTINGS_BIND_LAST = MENU_SETTINGS_BIND_BEGIN + RARCH_ANALOG_RIGHT_Y_MINUS,
   MENU_SETTINGS_BIND_ALL_LAST = MENU_SETTINGS_BIND_BEGIN + RARCH_MENU_TOGGLE,

   MENU_SETTINGS_CUSTOM_BIND,
   MENU_SETTINGS_CUSTOM_BIND_KEYBOARD,
   MENU_SETTINGS_CUSTOM_BIND_ALL,
   MENU_SETTINGS_CUSTOM_BIND_DEFAULT_ALL,
   MENU_SETTINGS_LIBRETRO_PERF_COUNTERS_BEGIN,
   MENU_SETTINGS_LIBRETRO_PERF_COUNTERS_END = MENU_SETTINGS_LIBRETRO_PERF_COUNTERS_BEGIN + (MAX_COUNTERS - 1),
   MENU_SETTINGS_PERF_COUNTERS_BEGIN,
   MENU_SETTINGS_PERF_COUNTERS_END = MENU_SETTINGS_PERF_COUNTERS_BEGIN + (MAX_COUNTERS - 1),
   MENU_SETTINGS_CHEAT_BEGIN,
   MENU_SETTINGS_CHEAT_END = MENU_SETTINGS_CHEAT_BEGIN + (MAX_CHEAT_COUNTERS - 1),
   MENU_SETTINGS_INPUT_DESC_BEGIN,
   MENU_SETTINGS_INPUT_DESC_END = MENU_SETTINGS_INPUT_DESC_BEGIN + (MAX_USERS * (RARCH_FIRST_CUSTOM_BIND + 4)),
   MENU_SETTINGS_LAST
} menu_settings_t;

typedef struct
{
   void *userdata;

   float scroll_y;

   bool push_help_screen;
   unsigned         help_screen_id;
   menu_help_type_t help_screen_type;

   bool defer_core;
   char deferred_path[PATH_MAX_LENGTH];

   char scratch_buf[PATH_MAX_LENGTH];
   char scratch2_buf[PATH_MAX_LENGTH];

   uint64_t state;
   struct
   {
      char msg[PATH_MAX_LENGTH];
   } menu_state;

   /* Menu shader */
   char default_glslp[PATH_MAX_LENGTH];
   char default_cgp[PATH_MAX_LENGTH];
   struct video_shader *shader;

   content_playlist_t *playlist;
   char db_playlist_file[PATH_MAX_LENGTH];
} menu_handle_t;

typedef struct menu_ctx_driver
{
   void  (*set_texture)(void);
   void  (*render_messagebox)(const char *msg);
   int   (*iterate)(enum menu_action action);
   void  (*render)(void);
   void  (*frame)(void);
   void* (*init)(void);
   void  (*free)(void*);
   void  (*context_reset)(void);
   void  (*context_destroy)(void);
   void  (*populate_entries)(const char *path, const char *label,
         unsigned k);
   void  (*toggle)(bool);
   void  (*navigation_clear)(bool);
   void  (*navigation_decrement)(void);
   void  (*navigation_increment)(void);
   void  (*navigation_set)(bool);
   void  (*navigation_set_last)(void);
   void  (*navigation_descend_alphabet)(size_t *);
   void  (*navigation_ascend_alphabet)(size_t *);
   bool  (*lists_init)(void*);
   void  (*list_insert)(file_list_t *list, const char *, const char *, size_t);
   void  (*list_free)(file_list_t *list, size_t, size_t);
   void  (*list_clear)(file_list_t *list);
   void  (*list_cache)(menu_list_type_t, unsigned);
   int   (*list_push)(menu_displaylist_info_t*, unsigned);
   size_t(*list_get_selection)(void *data);
   size_t(*list_get_size)(void *data, menu_list_type_t type);
   void *(*list_get_entry)(void *data, menu_list_type_t type, unsigned i);
   void  (*list_set_selection)(file_list_t *list);
   int   (*bind_init)(menu_file_list_cbs_t *cbs,
         const char *path, const char *label, unsigned type, size_t idx,
         const char *elem0, const char *elem1,
         uint32_t label_hash, uint32_t menu_label_hash);
   bool  (*load_image)(void *data, menu_image_type_t type);
   const char *ident;
   int (*environ_cb)(menu_environ_cb_t type, void *data);
   int (*pointer_tap)(unsigned x, unsigned y, unsigned ptr,
         menu_file_list_cbs_t *cbs,
         menu_entry_t *entry, unsigned action);
} menu_ctx_driver_t;

extern menu_ctx_driver_t menu_ctx_rmenu;
extern menu_ctx_driver_t menu_ctx_rmenu_xui;
extern menu_ctx_driver_t menu_ctx_rgui;
extern menu_ctx_driver_t menu_ctx_mui;
extern menu_ctx_driver_t menu_ctx_xmb;
extern menu_ctx_driver_t menu_ctx_zarch;
extern menu_ctx_driver_t menu_ctx_null;

/**
 * menu_driver_find_handle:
 * @index              : index of driver to get handle to.
 *
 * Returns: handle to menu driver at index. Can be NULL
 * if nothing found.
 **/
const void *menu_driver_find_handle(int index);

/**
 * menu_driver_find_ident:
 * @index              : index of driver to get handle to.
 *
 * Returns: Human-readable identifier of menu driver at index. Can be NULL
 * if nothing found.
 **/
const char *menu_driver_find_ident(int index);

/**
 * config_get_menu_driver_options:
 *
 * Get an enumerated list of all menu driver names,
 * separated by '|'.
 *
 * Returns: string listing of all menu driver names,
 * separated by '|'.
 **/
const char* config_get_menu_driver_options(void);

void find_menu_driver(void);

void init_menu(void);

menu_handle_t *menu_driver_get_ptr(void);

void menu_driver_context_reset(void);

void menu_driver_free(menu_handle_t *menu);

bool menu_driver_load_image(void *data, menu_image_type_t type);

void menu_driver_list_cache(menu_list_type_t type, unsigned action);

void  menu_driver_list_free(file_list_t *list, size_t i, size_t list_size);

size_t menu_driver_list_get_size(menu_list_type_t type);

void  menu_driver_list_set_selection(file_list_t *list);

void *menu_driver_list_get_entry(menu_list_type_t type, unsigned i);

const menu_ctx_driver_t *menu_ctx_driver_get_ptr(void);

void  menu_driver_context_destroy(void);

bool menu_driver_list_push(menu_displaylist_info_t *info, unsigned type);

size_t  menu_driver_list_get_selection(void);

bool menu_environment_cb(menu_environ_cb_t type, void *data);

int menu_driver_iterate(enum menu_action action);

int menu_driver_bind_init(menu_file_list_cbs_t *cbs,
      const char *path, const char *label, unsigned type, size_t idx,
      const char *elem0, const char *elem1,
      uint32_t label_hash, uint32_t menu_label_hash);

int menu_driver_pointer_tap(unsigned x, unsigned y, unsigned ptr,
      menu_file_list_cbs_t *cbs,
      menu_entry_t *entry, unsigned action);

/* HACK */
extern unsigned int rdb_entry_start_game_selection_ptr;

bool menu_driver_ctl(enum rarch_menu_ctl_state state, void *data);

/**
 * menu_init:
 * @data                     : Menu context handle.
 *
 * Create and initialize menu handle.
 *
 * Returns: menu handle on success, otherwise NULL.
 **/
void *menu_init(const void *data);

/**
 * menu_iterate:
 * @render_this_frame        : Render this frame or not
 * @action                   : Associated action for this frame
 *
 * Runs RetroArch menu for one frame.
 *
 * Returns: 0 on success, -1 if we need to quit out of the loop.
 **/
int menu_iterate(bool render_this_frame, enum menu_action action);

int menu_iterate_render(void);

/**
 * menu_free:
 * @menu                     : Menu handle.
 *
 * Frees a menu handle
 **/
void menu_free(menu_handle_t *menu);

/**
 * menu_load_content:
 * type                      : Type of content to load.
 *
 * Loads content into currently selected core.
 * Will also optionally push the content entry to the history playlist.
 *
 * Returns: true (1) if successful, otherwise false (0).
 **/
bool menu_load_content(enum rarch_core_type type);

int menu_common_load_content(const char *core_path, const char *full_path,
      bool persist, enum rarch_core_type type);

#ifdef __cplusplus
}
#endif

#endif

