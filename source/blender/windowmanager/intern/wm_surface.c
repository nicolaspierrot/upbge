/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/** \file
 * \ingroup wm
 */

#include "BKE_context.h"

#include "BLF_api.h"

#include "BLI_listbase.h"
#include "BLI_threads.h"

#include "DNA_scene_types.h"

#include "GHOST_C-api.h"

#include "GPU_batch_presets.h"
#include "GPU_context.h"
#include "GPU_framebuffer.h"
#include "GPU_immediate.h"

#include "MEM_guardedalloc.h"

#include "WM_api.h"
#include "WM_types.h"
#include "wm.h"

#include "wm_surface.h"

static ListBase global_surface_list = {NULL, NULL};
static wmSurface *g_drawable = NULL;

void wm_surfaces_iter(bContext *C, void (*cb)(bContext *C, wmSurface *))
{
  LISTBASE_FOREACH (wmSurface *, surf, &global_surface_list) {
    cb(C, surf);
  }
}

void wm_surface_clear_drawable(bContext *C)
{
  if (g_drawable) {
    bool is_game_xr_session = false;
    if (C != NULL) {
      Scene *scene = CTX_data_scene(C);
      if (scene->flag & SCE_IS_GAME_XR_SESSION) {
        is_game_xr_session = true;
      }
    }
    if (!is_game_xr_session) {
      WM_opengl_context_release(g_drawable->ghost_ctx);
      GPU_context_active_set(NULL);

      if (g_drawable->deactivate) {
        g_drawable->deactivate();
      }
    }

    g_drawable = NULL;
  }
}

void wm_surface_set_drawable(wmSurface *surface, bContext *C, bool activate)
{
  BLI_assert(ELEM(g_drawable, NULL, surface));

  bool is_game_xr_session = false;
  if (C != NULL) {
    Scene *scene = CTX_data_scene(C);
    if (scene->flag & SCE_IS_GAME_XR_SESSION) {
      is_game_xr_session = true;
    }
  }

  g_drawable = surface;
  if (activate) {
    if (surface->activate && !is_game_xr_session) {
      surface->activate();
    }
    WM_opengl_context_activate(surface->ghost_ctx);
  }

  GPU_context_active_set(surface->gpu_ctx);
}

void wm_surface_make_drawable(wmSurface *surface, bContext *C)
{
  BLI_assert(GPU_framebuffer_active_get() == GPU_framebuffer_back_get());

  if (surface != g_drawable) {
    wm_surface_clear_drawable(C);
    wm_surface_set_drawable(surface, C, true);
  }
}

void wm_surface_reset_drawable(bContext *C)
{
  BLI_assert(BLI_thread_is_main());
  BLI_assert(GPU_framebuffer_active_get() == GPU_framebuffer_back_get());

  if (g_drawable) {
    wm_surface_clear_drawable(C);
    wm_surface_set_drawable(g_drawable, C, true);
  }
}

void wm_surface_add(wmSurface *surface)
{
  BLI_addtail(&global_surface_list, surface);
}

void wm_surface_remove(wmSurface *surface)
{
  BLI_remlink(&global_surface_list, surface);
  surface->free_data(surface);
  MEM_freeN(surface);
}

void wm_surfaces_free(void)
{
  wm_surface_clear_drawable(NULL);

  LISTBASE_FOREACH_MUTABLE (wmSurface *, surf, &global_surface_list) {
    wm_surface_remove(surf);
  }

  BLI_assert(BLI_listbase_is_empty(&global_surface_list));
}
