/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
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

#include <unistd.h>

#include <sys/ioctl.h>

/* Includes and defines for framebuffer size retrieval */
#include <linux/fb.h>
#include <linux/vt.h>

#include <retro_file.h>

#include "../../driver.h"
#include "../../general.h"
#include "../../runloop.h"
#include "../common/egl_common.h"
#include "../common/gl_common.h"

struct fbdev_window native_window;
static bool g_resize;
static unsigned g_width, g_height;

static void gfx_ctx_mali_fbdev_destroy(void *data)
{
   int fb;
   RFILE *fd;

   egl_destroy(data);

   g_resize       = false;

   /* Clear framebuffer and set cursor on again */
   fd = retro_fopen("/dev/tty", RFILE_MODE_READ_WRITE, -1);
   fb = retro_get_fd(fd);

   ioctl(fb, VT_ACTIVATE,5);
   ioctl(fb, VT_ACTIVATE,1);
   retro_fclose(fd);
   system("setterm -cursor on");
}

static void gfx_ctx_mali_fbdev_get_video_size(void *data,
      unsigned *width, unsigned *height)
{
   (void)data;

   *width  = g_width;
   *height = g_height;
}

static bool gfx_ctx_mali_fbdev_init(void *data)
{
   EGLint n;
   EGLint major, minor;
   EGLint format;
   static const EGLint attribs[] = {
      EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
      EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
      EGL_BLUE_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_RED_SIZE, 8,
      EGL_ALPHA_SIZE, 8,
      EGL_NONE
   };

   egl_install_sighandlers();

   /* Disable cursor blinking so it's not visible in RetroArch. */
   system("setterm -cursor off");
   
   if (!egl_init_context(EGL_DEFAULT_DISPLAY,
            &major, &minor, &n, attribs))
   {
      egl_report_error();
      goto error;
   }

   return true;

error:
   RARCH_ERR("[Mali fbdev]: EGL error: %d.\n", eglGetError());
   gfx_ctx_mali_fbdev_destroy(data);
   return false;
}

static void gfx_ctx_mali_fbdev_check_window(void *data, bool *quit,
      bool *resize, unsigned *width, unsigned *height, unsigned frame_count)
{
   unsigned new_width, new_height;

   (void)frame_count;

   gfx_ctx_mali_fbdev_get_video_size(data, &new_width, &new_height);

   if (new_width != *width || new_height != *height)
   {
      *width  = new_width;
      *height = new_height;
      *resize = true;
   }

   *quit = g_egl_quit;
}

static void gfx_ctx_mali_fbdev_set_resize(void *data,
      unsigned width, unsigned height)
{
   (void)data;
   (void)width;
   (void)height;
}

static void gfx_ctx_mali_fbdev_update_window_title(void *data)
{
   char buf[128]        = {0};
   char buf_fps[128]    = {0};
   settings_t *settings = config_get_ptr();

   (void)data;

   video_monitor_get_fps(buf, sizeof(buf),
         buf_fps, sizeof(buf_fps));
   if (settings->fps_show)
      runloop_msg_queue_push(buf_fps, 1, 1, false);
}

static bool gfx_ctx_mali_fbdev_set_video_mode(void *data,
      unsigned width, unsigned height,
      bool fullscreen)
{
   struct fb_var_screeninfo vinfo;
   static const EGLint attribs[] = {
      EGL_CONTEXT_CLIENT_VERSION, 2, /* Use version 2, even for GLES3. */
      EGL_NONE
   };
   RFILE *fd = retro_fopen("/dev/fb0", RFILE_MODE_READ_WRITE, -1);
   int fb    = retro_get_fd(fd);

   if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) < 0)
   {
      RARCH_ERR("Error obtaining framebuffer info.\n");
      goto error;
   }
   retro_fclose(fd);
   
   width                = vinfo.xres;
   height               = vinfo.yres;

   g_width              = width;
   g_height             = height;

   native_window.width  = vinfo.xres;
   native_window.height = vinfo.yres;

   if (!egl_create_context(attribs))
   {
      egl_report_error();
      goto error;
   }

   if (!egl_create_surface(&native_window))
      goto error;

   return true;

error:
   if (fd)
      retro_fclose(fd);
   RARCH_ERR("[Mali fbdev]: EGL error: %d.\n", eglGetError());
   gfx_ctx_mali_fbdev_destroy(data);
   return false;
}

static void gfx_ctx_mali_fbdev_input_driver(void *data,
      const input_driver_t **input, void **input_data)
{
   (void)data;
   *input = NULL;
   *input_data = NULL;
}

static bool gfx_ctx_mali_fbdev_bind_api(void *data,
      enum gfx_ctx_api api, unsigned major, unsigned minor)
{
   (void)data;
   return api == GFX_CTX_OPENGL_ES_API;
}

static bool gfx_ctx_mali_fbdev_has_focus(void *data)
{
   (void)data;
   return true;
}

static bool gfx_ctx_mali_fbdev_suppress_screensaver(void *data, bool enable)
{
   (void)data;
   (void)enable;
   return false;
}

static bool gfx_ctx_mali_fbdev_has_windowed(void *data)
{
   (void)data;
   return false;
}

const gfx_ctx_driver_t gfx_ctx_mali_fbdev = {
   gfx_ctx_mali_fbdev_init,
   gfx_ctx_mali_fbdev_destroy,
   gfx_ctx_mali_fbdev_bind_api,
   egl_set_swap_interval,
   gfx_ctx_mali_fbdev_set_video_mode,
   gfx_ctx_mali_fbdev_get_video_size,
   NULL, /* get_video_output_size */
   NULL, /* get_video_output_prev */
   NULL, /* get_video_output_next */
   NULL, /* get_metrics */
   NULL,
   gfx_ctx_mali_fbdev_update_window_title,
   gfx_ctx_mali_fbdev_check_window,
   gfx_ctx_mali_fbdev_set_resize,
   gfx_ctx_mali_fbdev_has_focus,
   gfx_ctx_mali_fbdev_suppress_screensaver,
   gfx_ctx_mali_fbdev_has_windowed,
   egl_swap_buffers,
   gfx_ctx_mali_fbdev_input_driver,
   egl_get_proc_address,
   NULL,
   NULL,
   NULL,
   "mali-fbdev",
   egl_bind_hw_render,
};

