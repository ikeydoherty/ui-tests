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

#define _GNU_SOURCE

#include "util.h"

BUDGIE_BEGIN_PEDANTIC
#include "popover.h"
#include <gtk/gtk.h>
BUDGIE_END_PEDANTIC

struct _BudgiePopoverClass {
        GtkWindowClass parent_class;
};

struct _BudgiePopover {
        GtkWindow parent;
};

G_DEFINE_TYPE(BudgiePopover, budgie_popover, GTK_TYPE_WINDOW)

/**
 * budgie_popover_new:
 *
 * Construct a new BudgiePopover object
 */
GtkWidget *budgie_popover_new()
{
        return g_object_new(BUDGIE_TYPE_POPOVER, "type", GTK_WINDOW_POPUP, NULL);
}

/**
 * budgie_popover_dispose:
 *
 * Clean up a BudgiePopover instance
 */
static void budgie_popover_dispose(GObject *obj)
{
        G_OBJECT_CLASS(budgie_popover_parent_class)->dispose(obj);
}

/**
 * budgie_popover_class_init:
 *
 * Handle class initialisation
 */
static void budgie_popover_class_init(BudgiePopoverClass *klazz)
{
        GObjectClass *obj_class = G_OBJECT_CLASS(klazz);

        /* gobject vtable hookup */
        obj_class->dispose = budgie_popover_dispose;
}

/**
 * budgie_popover_init:
 *
 * Handle construction of the BudgiePopover
 */
static void budgie_popover_init(BudgiePopover *self)
{
        GtkWindow *win = GTK_WINDOW(self);

        /* Setup window specific bits */
        gtk_window_set_type_hint(win, GDK_WINDOW_TYPE_HINT_POPUP_MENU);
        gtk_window_set_skip_pager_hint(win, TRUE);
        gtk_window_set_skip_taskbar_hint(win, TRUE);
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
