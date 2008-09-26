#ifndef _GIMMIX_SYSTRAY_H
#define _GIMMIX_SYSTRAY_H

#include <gtk/gtk.h>
#include "gimmix.h"
#include "eggtrayicon.h"
#include "gimmix-tooltip.h"
#include "sexy-tooltip.h"

/* create system tray icon */
void gimmix_create_systray_icon (void);

/* Disable system tray icon */
void gimmix_disable_systray_icon (void);

/* Enable system tray icon */
void gimmix_enable_systray_icon (void);

/* update notification tooltip */
void gimmix_update_systray_tooltip (mpd_Song *s);

/* destroy system tray icon */
void gimmix_destroy_systray_icon (void);

/* set default image of systray tooltip */
void gimmix_systray_tooltip_set_default_image (void);

#endif
