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

static void button_click_cb(GtkWidget *pop, gpointer udata)
{
        gtk_widget_hide(pop);
        gtk_widget_destroy(pop);
}

int main(int argc, char **argv)
{
        gtk_init(&argc, &argv);
        GtkWidget *window = NULL;
        GtkWidget *entry, *button, *layout = NULL;

        /* Create popover */
        window = budgie_popover_new();

        layout = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_container_add(GTK_CONTAINER(window), layout);

        /* Add content */
        entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Type here!");
        gtk_box_pack_start(GTK_BOX(layout), entry, TRUE, TRUE, 2);

        button = gtk_button_new_with_label("Click me!");
        g_signal_connect_swapped(button, "clicked", G_CALLBACK(button_click_cb), window);
        gtk_box_pack_end(GTK_BOX(layout), button, FALSE, FALSE, 2);

        /* Render popover */
        g_signal_connect(window, "destroy", gtk_main_quit, NULL);
        gtk_widget_show_all(window);

        /* Run */
        gtk_main();
        gtk_widget_destroy(window);

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
