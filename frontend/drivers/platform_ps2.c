/* RetroArch - A frontend for libretro.
 * Copyright (C) 2010-2018 - Francisco Javier Trujillo Mata - fjtrujy
 *
 * RetroArch is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 * * You should have received a copy of the GNU General Public License along with RetroArch.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>

#include "../frontend_driver.h"

#include <io_common.h>
#include <loadfile.h>
#include <unistd.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <libpwroff.h>
#include <audsrv.h>
#include <libpad.h>
#include <ps2_devices.h>


extern unsigned char poweroff_irx_start[];
extern unsigned int poweroff_irx_size;

extern unsigned char ps2dev9_irx_start[];
extern unsigned int ps2dev9_irx_size;

extern unsigned char ps2atad_irx_start[];
extern unsigned int ps2atad_irx_size;

extern unsigned char ps2hdd_irx_start[];
extern unsigned int ps2hdd_irx_size;

extern unsigned char ps2fs_irx_start[];
extern unsigned int ps2fs_irx_size;

extern unsigned char iomanX_irx_start[];
extern unsigned int iomanX_irx_size;

extern unsigned char fileXio_irx_start[];
extern unsigned int fileXio_irx_size;

extern unsigned char freesd_irx_start[];
extern unsigned int freesd_irx_size;

extern unsigned char audsrv_irx_start[];
extern unsigned int audsrv_irx_size;

extern unsigned char usbd_irx_start[];
extern unsigned int usbd_irx_size;

extern unsigned char usbhdfsd_irx_start[];
extern unsigned int usbhdfsd_irx_size;

extern unsigned char mcman_irx_start[];
extern unsigned int mcman_irx_size;

extern unsigned char mcserv_irx_start[];
extern unsigned int mcserv_irx_size;

char eboot_path[512];
char user_path[512];

static enum frontend_fork ps2_fork_mode = FRONTEND_FORK_NONE;

static void create_path_names(void)
{
   char cwd[FILENAME_MAX];
   int bootDeviceID;

   getcwd(cwd, sizeof(cwd));
   bootDeviceID=getBootDeviceID(cwd);
   strlcpy(cwd, rootDevicePath(bootDeviceID), sizeof(cwd));
   strcat(cwd, "RETROARCH");

   strlcpy(eboot_path, cwd, sizeof(eboot_path));
   strlcpy(g_defaults.dirs[DEFAULT_DIR_PORT], eboot_path, sizeof(g_defaults.dirs[DEFAULT_DIR_PORT]));
   strlcpy(user_path, eboot_path, sizeof(user_path));

   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_CORE], g_defaults.dirs[DEFAULT_DIR_PORT],
         "CORES", sizeof(g_defaults.dirs[DEFAULT_DIR_CORE]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_CORE_INFO], g_defaults.dirs[DEFAULT_DIR_PORT],
         "INFO", sizeof(g_defaults.dirs[DEFAULT_DIR_CORE_INFO]));

   /* user data */
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_CHEATS], user_path,
         "CHEATS", sizeof(g_defaults.dirs[DEFAULT_DIR_CHEATS]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_MENU_CONFIG], user_path,
         "CONFIG", sizeof(g_defaults.dirs[DEFAULT_DIR_MENU_CONFIG]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_CORE_ASSETS], user_path,
         "DOWNLOADS", sizeof(g_defaults.dirs[DEFAULT_DIR_CORE_ASSETS]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_PLAYLIST], user_path,
         "PLAYLISTS", sizeof(g_defaults.dirs[DEFAULT_DIR_PLAYLIST]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_REMAP], g_defaults.dirs[DEFAULT_DIR_MENU_CONFIG],
         "REMAPS", sizeof(g_defaults.dirs[DEFAULT_DIR_REMAP]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_SRAM], user_path,
         "SAVEFILES", sizeof(g_defaults.dirs[DEFAULT_DIR_SRAM]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_SAVESTATE], user_path,
         "SAVESTATES", sizeof(g_defaults.dirs[DEFAULT_DIR_SAVESTATE]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_SCREENSHOT], user_path,
         "SCREENSHOTS", sizeof(g_defaults.dirs[DEFAULT_DIR_SCREENSHOT]));
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_SYSTEM], user_path,
         "SYSTEM", sizeof(g_defaults.dirs[DEFAULT_DIR_SYSTEM]));

   /* cache dir */
   fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_CACHE], user_path,
         "TEMP", sizeof(g_defaults.dirs[DEFAULT_DIR_CACHE]));

   /* history and main config */
   strlcpy(g_defaults.dirs[DEFAULT_DIR_CONTENT_HISTORY],
         user_path, sizeof(g_defaults.dirs[DEFAULT_DIR_CONTENT_HISTORY]));
   fill_pathname_join(g_defaults.path.config, user_path,
         file_path_str(FILE_PATH_MAIN_CONFIG), sizeof(g_defaults.path.config));
}

static void poweroffCallback(void *arg)
{
#if 0
	/* Close all files and unmount all partitions. */
	close(fd);

	/* If you use PFS, close all files and unmount all partitions. */
	fileXioDevctl("pfs:", PDIOC_CLOSEALL, NULL, 0, NULL, 0)

	/* Shut down DEV9, if you used it. */
	while(fileXioDevctl("dev9x:", DDIOC_OFF, NULL, 0, NULL, 0) < 0){};
#endif

	printf("Shutdown!");
	poweroffShutdown();
}

static void frontend_ps2_get_environment_settings(int *argc, char *argv[],
      void *args, void *params_data)
{
   create_path_names();

#ifndef IS_SALAMANDER
   if (!string_is_empty(argv[1]))
   {
      static char path[PATH_MAX_LENGTH] = {0};
      struct rarch_main_wrap      *args =
         (struct rarch_main_wrap*)params_data;

      if (args)
      {
         strlcpy(path, argv[1], sizeof(path));

         args->touched        = true;
         args->no_content     = false;
         args->verbose        = false;
         args->config_path    = NULL;
         args->sram_path      = NULL;
         args->state_path     = NULL;
         args->content_path   = path;
         args->libretro_path  = NULL;

         RARCH_LOG("argv[0]: %s\n", argv[0]);
         RARCH_LOG("argv[1]: %s\n", argv[1]);
         RARCH_LOG("argv[2]: %s\n", argv[2]);

         RARCH_LOG("Auto-start game %s.\n", argv[1]);
      }
   }
#endif
   int i;
   for (i = 0; i < DEFAULT_DIR_LAST; i++)
   {
      const char *dir_path = g_defaults.dirs[i];
      if (!string_is_empty(dir_path))
         path_mkdir(dir_path);
   }
}

static void frontend_ps2_init(void *data)
{
   char cwd[FILENAME_MAX];
   int bootDeviceID;

   SifInitRpc(0);
#if !defined(DEBUG)
   /* Comment this line if you don't wanna debug the output */
   while(!SifIopReset(NULL, 0)){};
#endif

   while(!SifIopSync()){};
   SifInitRpc(0);
   sbv_patch_enable_lmb();

   /* Controllers */
   SifLoadModule("rom0:SIO2MAN", 0, NULL);
   SifLoadModule("rom0:PADMAN", 0, NULL);

   /* I/O Files */
   SifExecModuleBuffer(iomanX_irx_start, iomanX_irx_size, 0, NULL, NULL);
   SifExecModuleBuffer(fileXio_irx_start, fileXio_irx_size, 0, NULL, NULL);

   /* Memory Card */
   SifExecModuleBuffer(mcman_irx_start, mcman_irx_size, 0, NULL, NULL);
   SifExecModuleBuffer(mcserv_irx_start, mcserv_irx_size, 0, NULL, NULL);

   /* USB */
   SifExecModuleBuffer(usbd_irx_start, usbd_irx_size, 0, NULL, NULL);
   SifExecModuleBuffer(usbhdfsd_irx_start, usbhdfsd_irx_size, 0, NULL, NULL);

   /* Audio */
   SifExecModuleBuffer(freesd_irx_start, freesd_irx_size, 0, NULL, NULL);
   SifExecModuleBuffer(audsrv_irx_start, audsrv_irx_size, 0, NULL, NULL);

   /* Initializes audsrv library */
   if (audsrv_init()) {
      RARCH_ERR("audsrv library not initalizated\n");
   }

   /* Initializes pad library
      Must be init with 0 as parameter*/
   if (padInit(0) != 1) {
      RARCH_ERR("padInit library not initalizated\n");
   }

   /* Prepare device */
   getcwd(cwd, sizeof(cwd));
   bootDeviceID=getBootDeviceID(cwd);
   waitUntilDeviceIsReady(bootDeviceID);

#if defined(HAVE_FILE_LOGGER)
   retro_main_log_file_init("retroarch.log");
   verbosity_enable();
#endif
}

static void frontend_ps2_deinit(void *data)
{
   (void)data;
#ifndef IS_SALAMANDER
   verbosity_disable();
#ifdef HAVE_FILE_LOGGER
   command_event(CMD_EVENT_LOG_FILE_DEINIT, NULL);
#endif

#endif

   padEnd();
   audsrv_quit();

   fileXioUmount("pfs0:");
   fileXioExit();

   SifExitRpc();
}

static void frontend_ps2_exec(const char *path, bool should_load_game)
{
#if defined(IS_SALAMANDER)
   char argp[512] = {0};
   SceSize   args = 0;

   strlcpy(argp, eboot_path, sizeof(argp));
   args = strlen(argp) + 1;

#ifndef IS_SALAMANDER
   if (should_load_game && !path_is_empty(RARCH_PATH_CONTENT))
   {
      argp[args] = '\0';
      strlcat(argp + args, path_get(RARCH_PATH_CONTENT), sizeof(argp) - args);
      args += strlen(argp + args) + 1;
   }
#endif

   RARCH_LOG("Attempt to load executable: [%s].\n", path);
#if 0
   exitspawn_kernel(path, args, argp); /* I don't know what this is doing */
#endif
#endif
}

#ifndef IS_SALAMANDER
static bool frontend_ps2_set_fork(enum frontend_fork fork_mode)
{
   switch (fork_mode)
   {
      case FRONTEND_FORK_CORE:
         RARCH_LOG("FRONTEND_FORK_CORE\n");
         ps2_fork_mode  = fork_mode;
         break;
      case FRONTEND_FORK_CORE_WITH_ARGS:
         RARCH_LOG("FRONTEND_FORK_CORE_WITH_ARGS\n");
         ps2_fork_mode  = fork_mode;
         break;
      case FRONTEND_FORK_RESTART:
         RARCH_LOG("FRONTEND_FORK_RESTART\n");
         /* NOTE: We don't implement Salamander, so just turn
          * this into FRONTEND_FORK_CORE. */
         ps2_fork_mode  = FRONTEND_FORK_CORE;
         break;
      case FRONTEND_FORK_NONE:
      default:
         return false;
   }

   return true;
}
#endif

static void frontend_ps2_exitspawn(char *core_path, size_t core_path_size)
{
   bool should_load_game = false;
#ifndef IS_SALAMANDER
   if (ps2_fork_mode == FRONTEND_FORK_NONE)
      return;

   switch (ps2_fork_mode)
   {
      case FRONTEND_FORK_CORE_WITH_ARGS:
         should_load_game = true;
         break;
      case FRONTEND_FORK_NONE:
      default:
         break;
   }
#endif
   frontend_ps2_exec(core_path, should_load_game);
}

static void frontend_ps2_shutdown(bool unused)
{
   poweroffInit();
   /* Set callback function */
	poweroffSetCallback(&poweroffCallback, NULL);
}

static int frontend_ps2_get_rating(void)
{
    return 10;
}

enum frontend_architecture frontend_ps2_get_architecture(void)
{
    return FRONTEND_ARCH_MIPS;
}

static int frontend_ps2_parse_drive_list(void *data, bool load_content)
{
#ifndef IS_SALAMANDER
   file_list_t *list = (file_list_t*)data;
   enum msg_hash_enums enum_idx = load_content ?
      MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR :
      MSG_UNKNOWN;

   menu_entries_append_enum(list,
         rootDevicePath(BOOT_DEVICE_MC0),
         msg_hash_to_str(MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR),
         enum_idx,
         FILE_TYPE_DIRECTORY, 0, 0);
   menu_entries_append_enum(list,
         rootDevicePath(BOOT_DEVICE_MC1),
         msg_hash_to_str(MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR),
         enum_idx,
         FILE_TYPE_DIRECTORY, 0, 0);
   menu_entries_append_enum(list,
         rootDevicePath(BOOT_DEVICE_MASS),
         msg_hash_to_str(MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR),
         enum_idx,
         FILE_TYPE_DIRECTORY, 0, 0);
   menu_entries_append_enum(list,
         rootDevicePath(BOOT_DEVICE_HOST),
         msg_hash_to_str(MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR),
         enum_idx,
         FILE_TYPE_DIRECTORY, 0, 0);
#if defined(DEBUG)
   menu_entries_append_enum(list,
         "host:",
         msg_hash_to_str(MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR),
         enum_idx,
         FILE_TYPE_DIRECTORY, 0, 0);
#endif
#endif

   return 0;
}

frontend_ctx_driver_t frontend_ctx_ps2 = {
   frontend_ps2_get_environment_settings,                         /* environment_get */
   frontend_ps2_init,                         /* init */
   frontend_ps2_deinit,                         /* deinit */
   frontend_ps2_exitspawn,                         /* exitspawn */
   NULL,                         /* process_args */
   frontend_ps2_exec,                         /* exec */
   #ifdef IS_SALAMANDER
   NULL,                         /* set_fork */
#else
   frontend_ps2_set_fork,                         /* set_fork */
#endif
   frontend_ps2_shutdown,                         /* shutdown */
   NULL,                         /* get_name */
   NULL,                         /* get_os */
   frontend_ps2_get_rating,                         /* get_rating */
   NULL,                         /* load_content */
   frontend_ps2_get_architecture,                         /* get_architecture */
   NULL,                         /* get_powerstate */
   frontend_ps2_parse_drive_list,                         /* parse_drive_list */
   NULL,                         /* get_mem_total */
   NULL,                         /* get_mem_free */
   NULL,                         /* install_signal_handler */
   NULL,                         /* get_sighandler_state */
   NULL,                         /* set_sighandler_state */
   NULL,                         /* destroy_sighandler_state */
   NULL,                         /* attach_console */
   NULL,                         /* detach_console */
#ifdef HAVE_LAKKA
   NULL,                         /* get_lakka_version */
#endif
   NULL,                         /* watch_path_for_changes */
   NULL,                         /* check_for_path_changes */
   NULL,                         /* set_sustained_performance_mode */
   NULL,                         /* get_cpu_model_name */
   "null",
};
