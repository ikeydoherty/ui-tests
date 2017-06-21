/*
 * This file is part of ui-tests
 *
 * Copyright © 2016 Ikey Doherty <ikey@solus-project.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include "util.h"
#include <stdlib.h>

BUDGIE_BEGIN_PEDANTIC
#include "popover.h"
BUDGIE_END_PEDANTIC

static void button_click_cb(__budgie_unused__ GtkWidget *pop, gpointer udata)
{
        GtkWidget *popover = udata;

        gtk_widget_hide(popover);
        gtk_widget_destroy(popover);
}

static void show_popover_cb(__budgie_unused__ GtkWidget *window, gpointer udata)
{
        GtkWidget *popover = udata;

        g_message("GOT CLICKED YO");
        gtk_widget_show_all(popover);
}

static gboolean enter(GtkWidget *widget, __budgie_unused__ GdkEventCrossing *event,
                      __budgie_unused__ gpointer udata)
{
        g_message("Inside: %s", gtk_widget_get_name(widget));
        return GDK_EVENT_PROPAGATE;
}

int main(int argc, char **argv)
{
        gtk_init(&argc, &argv);
        GtkWidget *window = NULL;
        GtkWidget *entry, *button, *layout = NULL;
        GtkWidget *main_window = NULL;

        g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", FALSE, NULL);

        /* Create popover */
        window = budgie_popover_new();

        main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_widget_set_size_request(main_window, 400, 400);

        layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_set_valign(layout, GTK_ALIGN_CENTER);
        gtk_container_add(GTK_CONTAINER(main_window), layout);

        button = gtk_button_new_with_label("Click me #1");

        gtk_box_pack_start(GTK_BOX(layout), button, FALSE, FALSE, 0);
        g_signal_connect(button, "clicked", G_CALLBACK(show_popover_cb), window);

        button = gtk_button_new_with_label("Click me #2");
        gtk_box_pack_start(GTK_BOX(layout), button, FALSE, FALSE, 0);
        g_signal_connect(button, "clicked", G_CALLBACK(show_popover_cb), window);

        layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_container_add(GTK_CONTAINER(window), layout);

        /* Add content */
        entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Type here!");
        gtk_box_pack_start(GTK_BOX(layout), entry, TRUE, TRUE, 2);

        button = gtk_button_new_with_label("Click me!");
        gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
        g_signal_connect(button, "clicked", G_CALLBACK(button_click_cb), window);
        gtk_box_pack_end(GTK_BOX(layout), button, FALSE, FALSE, 2);

        /* Render popover */
        g_signal_connect(window, "destroy", gtk_main_quit, NULL);
        g_signal_connect(main_window, "destroy", gtk_main_quit, NULL);

        /* Popovery methods */
        g_signal_connect_after(window, "enter-notify-event", G_CALLBACK(enter), window);
        g_signal_connect_after(window, "button-press-event", gtk_main_quit, window);

        gtk_widget_show_all(main_window);

        /* Run */
        gtk_main();

        return EXIT_SUCCESS;
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 expandtab:
 * :indentSize=8:tabSize=8:noTabs=true:
 */
