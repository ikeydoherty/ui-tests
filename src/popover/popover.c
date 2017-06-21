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
#include "popover.h"
#include <gtk/gtk.h>
BUDGIE_END_PEDANTIC

struct _BudgiePopoverClass {
        GtkWindowClass parent_class;
};

struct _BudgiePopover {
        GtkWindow parent;

        gboolean grabbed;
        GtkWidget *add_area;
};

G_DEFINE_TYPE(BudgiePopover, budgie_popover, GTK_TYPE_WINDOW)

static gboolean budgie_popover_draw(GtkWidget *widget, cairo_t *cr);
static gboolean budgie_popover_map(GtkWidget *widget, gpointer udata);
static gboolean budgie_popover_unmap(GtkWidget *widget, gpointer udata);
static void budgie_popover_grab_notify(GtkWidget *widget, gboolean was_grabbed, gpointer udata);
static gboolean budgie_popover_grab_broken(GtkWidget *widget, GdkEvent *event, gpointer udata);
static void budgie_popover_grab(BudgiePopover *self);
static void budgie_popover_ungrab(BudgiePopover *self);
static void budgie_popover_load_css(void);
static void budgie_popover_add(GtkContainer *container, GtkWidget *widget);
static gboolean budgie_popover_button_press(GtkWidget *widget, GdkEventButton *button,
                                            gpointer udata);
static gboolean budgie_popover_key_press(GtkWidget *widget, GdkEventKey *key, gpointer udata);

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
        /* Blame clang-format for weird wrapping */
        return g_object_new(BUDGIE_TYPE_POPOVER,
                            "decorated",
                            FALSE,
                            "deletable",
                            FALSE,
                            "focus-on-map",
                            TRUE,
                            "gravity",
                            GDK_GRAVITY_NORTH_WEST,
                            "modal",
                            FALSE,
                            "resizable",
                            FALSE,
                            "skip-pager-hint",
                            TRUE,
                            "skip-taskbar-hint",
                            TRUE,
                            "type",
                            GTK_WINDOW_POPUP,
                            "type-hint",
                            GDK_WINDOW_TYPE_HINT_POPUP_MENU,
                            "window-position",
                            GTK_WIN_POS_CENTER,
                            NULL);
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
        GtkContainerClass *cont_class = GTK_CONTAINER_CLASS(klazz);

        /* gobject vtable hookup */
        obj_class->dispose = budgie_popover_dispose;

        /* widget vtable hookup */
        wid_class->draw = budgie_popover_draw;

        cont_class->add = budgie_popover_add;
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

        self->grabbed = FALSE;

        style = gtk_widget_get_style_context(GTK_WIDGET(self));
        gtk_style_context_add_class(style, "budgie-popover");

        self->add_area = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(self), self->add_area);
        gtk_widget_show_all(self->add_area);

        /* Hacky demo */
        budgie_popover_load_css();

        /* Setup window specific bits */
        gtk_window_set_position(win, GTK_WIN_POS_CENTER);
        g_signal_connect(win, "map-event", G_CALLBACK(budgie_popover_map), NULL);
        g_signal_connect(win, "unmap-event", G_CALLBACK(budgie_popover_unmap), NULL);
        g_signal_connect(win, "grab-notify", G_CALLBACK(budgie_popover_grab_notify), NULL);
        g_signal_connect(win, "grab-broken-event", G_CALLBACK(budgie_popover_grab_broken), NULL);
        g_signal_connect(win, "button-press-event", G_CALLBACK(budgie_popover_button_press), NULL);
        g_signal_connect(win, "key-press-event", G_CALLBACK(budgie_popover_key_press), NULL);

        /* Set up RGBA ability */
        screen = gtk_widget_get_screen(GTK_WIDGET(self));
        visual = gdk_screen_get_rgba_visual(screen);
        if (visual) {
                gtk_widget_set_visual(GTK_WIDGET(self), visual);
        }
        /* We do all rendering */
        gtk_widget_set_app_paintable(GTK_WIDGET(self), TRUE);

        /* TESTING: To let us develop the tail render code */
        g_object_set(self->add_area,
                     "margin-top",
                     10,
                     "margin-bottom",
                     20,
                     "margin-start",
                     5,
                     "margin-end",
                     5,
                     NULL);
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
        /* Draw "through" the previous box-shadow */
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_SUBPIXEL);
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
        gtk_style_context_set_state(style, GTK_STATE_FLAG_BACKDROP);
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
        gtk_style_context_set_state(style, fl);

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

static gboolean budgie_popover_map(GtkWidget *widget, __budgie_unused__ gpointer udata)
{
        GdkWindow *window = NULL;

        /* Forcibly request focus */
        window = gtk_widget_get_window(widget);
        gdk_window_set_accept_focus(window, TRUE);
        gdk_window_focus(window, GDK_CURRENT_TIME);
        gtk_window_present(GTK_WINDOW(widget));

        budgie_popover_grab(BUDGIE_POPOVER(widget));
        budgie_popover_ungrab(BUDGIE_POPOVER(widget));
        budgie_popover_grab(BUDGIE_POPOVER(widget));

        return GDK_EVENT_PROPAGATE;
}

static gboolean budgie_popover_unmap(GtkWidget *widget, __budgie_unused__ gpointer udata)
{
        budgie_popover_ungrab(BUDGIE_POPOVER(widget));
        return GDK_EVENT_PROPAGATE;
}

/**
 * Grab the input events using the GdkSeat
 */
static void budgie_popover_grab(BudgiePopover *self)
{
        GdkDisplay *display = NULL;
        GdkSeat *seat = NULL;
        GdkWindow *window = NULL;
        GdkSeatCapabilities caps = 0;
        GdkGrabStatus st;

        g_message("regrab");

        if (self->grabbed) {
                return;
        }

        window = gtk_widget_get_window(GTK_WIDGET(self));

        if (!window) {
                g_warning("Attempting to grab BudgiePopover when not realized");
                return;
        }

        display = gtk_widget_get_display(GTK_WIDGET(self));
        seat = gdk_display_get_default_seat(display);

        caps = GDK_SEAT_CAPABILITY_ALL;

        st = gdk_seat_grab(seat, window, caps, TRUE, NULL, NULL, NULL, NULL);
        if (st == GDK_GRAB_SUCCESS) {
                self->grabbed = TRUE;
                gtk_grab_add(GTK_WIDGET(self));
        }
}

/**
 * Ungrab a previous grab by this widget
 */
static void budgie_popover_ungrab(BudgiePopover *self)
{
        GdkDisplay *display = NULL;
        GdkSeat *seat = NULL;

        if (!self->grabbed) {
                return;
        }

        display = gtk_widget_get_display(GTK_WIDGET(self));
        seat = gdk_display_get_default_seat(display);

        gtk_grab_remove(GTK_WIDGET(self));
        gdk_seat_ungrab(seat);
        self->grabbed = FALSE;
}

/**
 * Grab was broken, most likely due to a window within our application
 */
static gboolean budgie_popover_grab_broken(GtkWidget *widget, __budgie_unused__ GdkEvent *event,
                                           __budgie_unused__ gpointer udata)
{
        BudgiePopover *self = NULL;

        g_message("Broke");

        self = BUDGIE_POPOVER(widget);
        self->grabbed = FALSE;
        return GDK_EVENT_PROPAGATE;
}

/**
 * Grab changed _within_ the application
 *
 * If our grab was broken, i.e. due to some popup menu, and we're still visible,
 * we'll now try and grab focus once more.
 */
static void budgie_popover_grab_notify(GtkWidget *widget, gboolean was_grabbed,
                                       __budgie_unused__ gpointer udata)
{
        BudgiePopover *self = NULL;

        /* Only interested in unshadowed */
        if (!was_grabbed) {
                return;
        }

        g_message("Derp notify");
        // budgie_popover_ungrab(BUDGIE_POPOVER(widget));

        /* And being visible. ofc. */
        if (!gtk_widget_get_visible(widget)) {
                return;
        }

        self = BUDGIE_POPOVER(widget);
        budgie_popover_grab(self);
}

static void budgie_popover_add(GtkContainer *container, GtkWidget *widget)
{
        BudgiePopover *self = NULL;

        self = BUDGIE_POPOVER(container);

        /* Only add internal area to self for real. Anything else goes to add_area */
        if (widget == self->add_area) {
                GTK_CONTAINER_CLASS(budgie_popover_parent_class)->add(container, widget);
                return;
        }

        gtk_container_add(GTK_CONTAINER(self->add_area), widget);
}

/**
 * If the mouse button is pressed outside of our window, that's our cue to close.
 */
static gboolean budgie_popover_button_press(GtkWidget *widget, GdkEventButton *button,
                                            __budgie_unused__ gpointer udata)
{
        gint x, y = 0;
        gint w, h = 0;
        gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
        gtk_window_get_size(GTK_WINDOW(widget), &w, &h);

        gint root_x = (gint)button->x_root;
        gint root_y = (gint)button->y_root;

        /* Inside our window? Continue as normal. */
        if ((root_x >= x && root_x <= x + w) && (root_y >= y && root_y <= y + h)) {
                return GDK_EVENT_PROPAGATE;
        }

        /* Happened outside, we're done. */
        gtk_widget_hide(widget);
        return GDK_EVENT_STOP;
}

/**
 * If the Escape key is pressed, then we also need to close.
 */
static gboolean budgie_popover_key_press(GtkWidget *widget, GdkEventKey *key,
                                         __budgie_unused__ gpointer udata)
{
        if (key->keyval == GDK_KEY_Escape) {
                gtk_widget_hide(widget);
                return GDK_EVENT_STOP;
        }
        return GDK_EVENT_PROPAGATE;
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
