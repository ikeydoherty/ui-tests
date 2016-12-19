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

static gboolean budgie_popover_draw(GtkWidget *widget, cairo_t *cr);
static void budgie_popover_load_css(void);

/**
 * Used for storing BudgieTail calculations
 */
typedef struct BudgieTail {
        double start_x;
        double start_y;
        double end_x;
        double end_y;
        double x;
        double y;
} BudgieTail;

/**
 * We'll likely take this from a style property in future, but for now it
 * is both the width and height of a tail
 */
#define TAIL_DIMENSION 20
#define SHADOW_DIMENSION 4

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
        GtkWidgetClass *wid_class = GTK_WIDGET_CLASS(klazz);

        /* gobject vtable hookup */
        obj_class->dispose = budgie_popover_dispose;

        /* widget vtable hookup */
        wid_class->draw = budgie_popover_draw;
}

/**
 * budgie_popover_init:
 *
 * Handle construction of the BudgiePopover
 */
static void budgie_popover_init(BudgiePopover *self)
{
        GtkWindow *win = GTK_WINDOW(self);
        GdkScreen *screen = NULL;
        GdkVisual *visual = NULL;
        GtkStyleContext *style = NULL;

        style = gtk_widget_get_style_context(GTK_WIDGET(self));
        gtk_style_context_add_class(style, "budgie-popover");

        /* Hacky demo */
        budgie_popover_load_css();

        /* Setup window specific bits */
        gtk_window_set_type_hint(win, GDK_WINDOW_TYPE_HINT_POPUP_MENU);
        gtk_window_set_skip_pager_hint(win, TRUE);
        gtk_window_set_skip_taskbar_hint(win, TRUE);
        gtk_window_set_position(win, GTK_WIN_POS_CENTER);

        /* Set up RGBA ability */
        screen = gtk_widget_get_screen(GTK_WIDGET(self));
        visual = gdk_screen_get_rgba_visual(screen);
        if (visual) {
                gtk_widget_set_visual(GTK_WIDGET(self), visual);
        }
        /* We do all rendering */
        gtk_widget_set_app_paintable(GTK_WIDGET(self), TRUE);

        /* TESTING: To let us develop the tail render code */
        gtk_container_set_border_width(GTK_CONTAINER(self), 40);
}

static void budgie_popover_compute_tail(GtkWidget *widget, BudgieTail *tail)
{
        GtkAllocation alloc = { 0 };
        BudgieTail t = { 0 };

        if (!tail) {
                return;
        }

        gtk_widget_get_allocation(widget, &alloc);

        /* Right now just assume we're centered and at the bottom. */
        t.x = (alloc.x + alloc.width / 2) - SHADOW_DIMENSION;
        t.y = (alloc.y + alloc.height) - SHADOW_DIMENSION;

        t.start_x = t.x - (TAIL_DIMENSION / 2);
        t.end_x = t.start_x + TAIL_DIMENSION;

        t.start_y = t.y - (TAIL_DIMENSION / 2);
        t.end_y = t.start_y;
        *tail = t;
}

/**
 * Draw the actual tail itself.
 */
static void budgie_popover_draw_tail(BudgieTail *tail, cairo_t *cr)
{
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
        cairo_move_to(cr, tail->start_x, tail->start_y);
        cairo_line_to(cr, tail->x, tail->y);
        cairo_line_to(cr, tail->end_x, tail->end_y);
        cairo_stroke_preserve(cr);
}

/**
 * Override the drawing to provide a tail region
 */
static gboolean budgie_popover_draw(GtkWidget *widget, cairo_t *cr)
{
        GtkStyleContext *style = NULL;
        GtkAllocation alloc = { 0 };
        GtkWidget *child = NULL;
        BudgieTail tail = { 0 };
        gint original_height;
        GdkRGBA border_color = { 0 };
        GtkBorder border = { 0 };
        GtkStateFlags fl;

        budgie_popover_compute_tail(widget, &tail);
        fl = GTK_STATE_FLAG_VISITED;

        style = gtk_widget_get_style_context(widget);
        gtk_widget_get_allocation(widget, &alloc);

        /* Warning: Using deprecated API */
        gtk_style_context_get_border_color(style, fl, &border_color);
        gtk_style_context_get_border(style, fl, &border);

        /* Set up the offset */
        original_height = alloc.height;
        alloc.height -= (alloc.height - (int)tail.start_y) - SHADOW_DIMENSION;

        /* Fix overhang */
        // alloc.height += 1;
        gtk_render_background(style,
                              cr,
                              alloc.x + SHADOW_DIMENSION,
                              alloc.y + SHADOW_DIMENSION,
                              alloc.width - SHADOW_DIMENSION * 2,
                              alloc.height - SHADOW_DIMENSION * 2);
        gtk_render_frame_gap(style,
                             cr,
                             alloc.x + SHADOW_DIMENSION,
                             alloc.y + SHADOW_DIMENSION,
                             alloc.width - SHADOW_DIMENSION * 2,
                             alloc.height - SHADOW_DIMENSION * 2,
                             GTK_POS_BOTTOM,
                             tail.start_x - SHADOW_DIMENSION,
                             tail.end_x - SHADOW_DIMENSION);

        child = gtk_bin_get_child(GTK_BIN(widget));
        if (child) {
                gtk_container_propagate_draw(GTK_CONTAINER(widget), child, cr);
        }

        cairo_set_line_width(cr, border.bottom);
        cairo_set_source_rgba(cr,
                              border_color.red,
                              border_color.green,
                              border_color.blue,
                              border_color.alpha);
        budgie_popover_draw_tail(&tail, cr);
        cairo_clip(cr);
        cairo_move_to(cr, 0, 0);
        gtk_render_background(style, cr, alloc.x, alloc.y, alloc.width, original_height);

        return GDK_EVENT_STOP;
}

static void budgie_popover_load_css()
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
