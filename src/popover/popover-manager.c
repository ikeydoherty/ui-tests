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
        GHashTable *popovers;
};

G_DEFINE_TYPE(BudgiePopoverManager, budgie_popover_manager, G_TYPE_OBJECT)

static void budgie_popover_manager_link_signals(BudgiePopoverManager *manager,
                                                GtkWidget *parent_widget, BudgiePopover *popover);
static void budgie_popover_manager_unlink_signals(BudgiePopoverManager *manager,
                                                  GtkWidget *parent_widget, BudgiePopover *popover);

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
        BudgiePopoverManager *self = NULL;

        self = BUDGIE_POPOVER_MANAGER(obj);
        g_clear_pointer(&self->popovers, g_hash_table_unref);

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
static void budgie_popover_manager_init(BudgiePopoverManager *self)
{
        /* We don't re-ref anything as we just effectively hold floating references
         * to the WhateverTheyAres
         */
        self->popovers = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);
}

void budgie_popover_manager_register_popover(BudgiePopoverManager *self, GtkWidget *parent_widget,
                                             BudgiePopover *popover)
{
        g_assert(self != NULL);
        g_return_if_fail(parent_widget != NULL && popover != NULL);

        if (g_hash_table_contains(self->popovers, parent_widget)) {
                g_warning("register_popover(): Widget %p is already registered",
                          (gpointer)parent_widget);
                return;
        }

        /* Stick it into the map and hook it up */
        budgie_popover_manager_link_signals(self, parent_widget, popover);
        g_hash_table_insert(self->popovers, parent_widget, popover);
}

void budgie_popover_manager_unregister_popover(BudgiePopoverManager *self, GtkWidget *parent_widget)
{
        g_assert(self != NULL);
        g_return_if_fail(parent_widget != NULL);
        BudgiePopover *popover = NULL;

        popover = g_hash_table_lookup(self->popovers, parent_widget);
        if (!popover) {
                g_warning("unregister_popover(): Widget %p is unknown", (gpointer)parent_widget);
                return;
        }

        budgie_popover_manager_unlink_signals(self, parent_widget, popover);
        g_hash_table_remove(self->popovers, parent_widget);
}

void budgie_popover_manager_show_popover(BudgiePopover *self, GtkWidget *parent_widget)
{
        g_assert(self != NULL);
        g_return_if_fail(parent_widget != NULL);
        g_warning("show_popover(): not yet implemented");
}

/**
 * Hook up the various signals we need to manage this popover correctly
 */
static void budgie_popover_manager_link_signals(BudgiePopoverManager *self,
                                                GtkWidget *parent_widget, BudgiePopover *popover)
{
        g_warning("link_signals(): not yet implemented");
}

/**
 * Disconnect any prior signals for this popover so we stop receiving events for it
 */
static void budgie_popover_manager_unlink_signals(BudgiePopoverManager *self,
                                                  GtkWidget *parent_widget, BudgiePopover *popover)
{
        g_warning("unlink_signals(): not yet implemented");
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
