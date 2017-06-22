/*
 * This file is part of ui-tests
 *
 * Copyright © 2016-2017 Ikey Doherty <ikey@solus-project.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#include "util.h"
#include <stdlib.h>

BUDGIE_BEGIN_PEDANTIC
#include "popover-manager.h"
#include "popover.h"
BUDGIE_END_PEDANTIC

static void budgie_popover_demo_load_css(void)
{
        GdkScreen *screen = NULL;
        GtkCssProvider *css = NULL;

        screen = gdk_screen_get_default();
        css = gtk_css_provider_new();
        gtk_css_provider_load_from_path(css, "src/popover/styling.css", NULL);
        gtk_style_context_add_provider_for_screen(screen,
                                                  GTK_STYLE_PROVIDER(css),
                                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

static void button_click_cb(__budgie_unused__ GtkWidget *pop, gpointer udata)
{
        GtkWidget *popover = udata;

        gtk_widget_hide(popover);
}

static gboolean show_popover_cb(__budgie_unused__ GtkWidget *window,
                                __budgie_unused__ GdkEventButton *button, gpointer udata)
{
        GtkWidget *popover = udata;

        g_message("GOT CLICKED YO");
        gtk_widget_show_all(popover);

        return GDK_EVENT_STOP;
}

static GtkWidget *sudo_make_me_a_popover(GtkWidget *relative_to, const gchar *le_label)
{
        GtkWidget *popover = NULL;
        GtkWidget *entry, *button, *layout = NULL;

        popover = budgie_popover_new(relative_to);

        layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_container_set_border_width(GTK_CONTAINER(layout), 5);
        gtk_container_add(GTK_CONTAINER(popover), layout);

        /* Add content */
        entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Type here!");
        gtk_box_pack_start(GTK_BOX(layout), entry, TRUE, TRUE, 2);

        button = gtk_button_new_with_label(le_label);
        g_signal_connect(button, "clicked", G_CALLBACK(button_click_cb), popover);
        gtk_box_pack_end(GTK_BOX(layout), button, FALSE, FALSE, 2);

        /* Popovery methods */
        g_signal_connect(popover, "destroy", gtk_main_quit, NULL);

        return popover;
}

int main(int argc, char **argv)
{
        gtk_init(&argc, &argv);
        GtkWidget *popover = NULL;
        BudgiePopoverManager *manager = NULL;

        /* Hacky demo */
        budgie_popover_demo_load_css();

        GtkWidget *main_window = NULL;
        GtkWidget *button, *layout = NULL;

        manager = budgie_popover_manager_new();
        g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", FALSE, NULL);

        main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(main_window), "Popovers..");
        gtk_window_set_default_size(main_window, -1, -1);

        layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        gtk_widget_set_valign(layout, GTK_ALIGN_CENTER);
        gtk_container_add(GTK_CONTAINER(main_window), layout);

        /* Hook up the popover to the actionable button */
        button = gtk_toggle_button_new_with_label("Click me #1");
        popover = sudo_make_me_a_popover(button, "Popover #1");

        g_object_bind_property(popover, "visible", button, "active", G_BINDING_DEFAULT);
        budgie_popover_manager_register_popover(manager, button, BUDGIE_POPOVER(popover));

        gtk_box_pack_start(GTK_BOX(layout), button, FALSE, FALSE, 0);
        g_signal_connect(button, "button-press-event", G_CALLBACK(show_popover_cb), popover);

        button = gtk_toggle_button_new_with_label("Click me #2");
        popover = sudo_make_me_a_popover(button, "Popover #2");
        g_object_bind_property(popover, "visible", button, "active", G_BINDING_DEFAULT);
        gtk_box_pack_start(GTK_BOX(layout), button, FALSE, FALSE, 0);

        g_signal_connect(button, "button-press-event", G_CALLBACK(show_popover_cb), popover);
        budgie_popover_manager_register_popover(manager, button, BUDGIE_POPOVER(popover));

        g_signal_connect(main_window, "destroy", gtk_main_quit, NULL);

        gtk_widget_show_all(main_window);

        /* Run */
        gtk_main();

        g_object_unref(manager);

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
