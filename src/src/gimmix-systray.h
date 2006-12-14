#ifndef _GIMMIX_SYSTRAY_H
#define _GIMMIX_SYSTRAY_H

#include <gtk/gtk.h>
#include "gimmix.h"
#include "eggtrayicon.h"

/* create system tray icon */
void gimmix_create_systray_icon (void);

/* Disable system tray icon */
void gimmix_disable_systray_icon (void);

/* Enable system tray icon */
void gimmix_enable_systray_icon (void);

/* update notification tooltip */
void gimmix_update_systray_tooltip (SongInfo *s);

/* destroy system tray icon */
void gimmix_destroy_systray_icon (void);

#endif
