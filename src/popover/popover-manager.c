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

#define _GNU_SOURCE

#include "util.h"

BUDGIE_BEGIN_PEDANTIC
#include "popover-manager.h"
#include <gtk/gtk.h>
BUDGIE_END_PEDANTIC

struct _BudgiePopoverManagerClass {
        GObjectClass parent_class;
};

struct _BudgiePopoverManager {
        GObject parent;
};

G_DEFINE_TYPE(BudgiePopoverManager, budgie_popover_manager, G_TYPE_OBJECT)

/**
 * budgie_popover_manager_new:
 *
 * Construct a new BudgiePopoverManager object
 */
BudgiePopoverManager *budgie_popover_manager_new()
{
        return g_object_new(BUDGIE_TYPE_POPOVER_MANAGER, NULL);
}

/**
 * budgie_popover_manager_dispose:
 *
 * Clean up a BudgiePopoverManager instance
 */
static void budgie_popover_manager_dispose(GObject *obj)
{
        G_OBJECT_CLASS(budgie_popover_manager_parent_class)->dispose(obj);
}

/**
 * budgie_popover_manager_class_init:
 *
 * Handle class initialisation
 */
static void budgie_popover_manager_class_init(BudgiePopoverManagerClass *klazz)
{
        GObjectClass *obj_class = G_OBJECT_CLASS(klazz);

        /* gobject vtable hookup */
        obj_class->dispose = budgie_popover_manager_dispose;
}

/**
 * budgie_popover_manager_init:
 *
 * Handle construction of the BudgiePopoverManager
 */
static void budgie_popover_manager_init(__budgie_unused__ BudgiePopoverManager *self)
{
}

void budgie_popover_manager_register_popover(BudgiePopoverManager *manager,
                                             GtkWidget *parent_widget, BudgiePopover *popover)
{
        g_assert(manager != NULL);
        g_return_if_fail(parent_widget != NULL && popover != NULL);
        g_warning("register_popover(): not yet implemented");
}

void budgie_popover_manager_unregister_popover(BudgiePopoverManager *manager,
                                               GtkWidget *parent_widget)
{
        g_assert(manager != NULL);
        g_return_if_fail(parent_widget != NULL);
        g_warning("unregister_popover(): not yet implemented");
}

void budgie_popover_manager_show_popover(BudgiePopover *manager, GtkWidget *parent_widget)
{
        g_assert(manager != NULL);
        g_return_if_fail(parent_widget != NULL);
        g_warning("show_popover(): not yet implemented");
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
