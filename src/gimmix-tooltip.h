#ifndef _GIMMIX_TOOLTIP_H
#define _GIMMIX_TOOLTIP_H

#include <gtk/gtk.h>

/* GimmixTooltip structure */
typedef struct {
	GtkWidget *window;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *icon;
	GtkWidget *progressbar;
} GimmixTooltip;

/* Gimmix Tooltip Functions */

/* Create a new tooltip */
GimmixTooltip *gimmix_tooltip_new (void);

/* Sets the tooltip text (label1) */
void gimmix_tooltip_set_text1 (GimmixTooltip *tooltip, const gchar *text, gboolean formatting);

/* Sets the tooltip text (label2) */
void gimmix_tooltip_set_text2 (GimmixTooltip *tooltip, const gchar *text, gboolean formatting);

/* Sets the icon for the tooltip */
void gimmix_tooltip_set_icon (GimmixTooltip *tooltip, GdkPixbuf *pixbuf);

/* Attach tooltip to a widget */
// void gimmix_tooltip_attach_to_widget (GimmixTooltip *tooltip, GtkWidget *widget);

/* Show the tooltip */
void gimmix_tooltip_show (GimmixTooltip *tooltip);

/* Hide the tooltip */
void gimmix_tooltip_hide (GimmixTooltip *tooltip);

/* Destroy the tooltip object and free the memory */
void gimmix_tooltip_destroy (GimmixTooltip *tooltip);


#endif

