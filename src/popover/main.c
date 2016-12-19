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

int main(int argc, char **argv)
{
        gtk_init(&argc, &argv);
        GtkWidget *window = NULL;

        window = budgie_popover_new();
        g_signal_connect(window, "destroy", gtk_main_quit, NULL);
        gtk_widget_show_all(window);

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
