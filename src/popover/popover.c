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

/**
 * We'll likely take this from a style property in future, but for now it
 * is both the width and height of a tail
 */
#define TAIL_DIMENSION 20
#define SHADOW_DIMENSION 4

struct _BudgiePopoverPrivate {
        gboolean grabbed;
        GtkWidget *add_area;
        GtkWidget *relative_to;
        GtkPositionType tail_position;
};

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

enum { PROP_RELATIVE_TO = 1, N_PROPS };

static GParamSpec *obj_properties[N_PROPS] = {
        NULL,
};

G_DEFINE_TYPE_WITH_PRIVATE(BudgiePopover, budgie_popover, GTK_TYPE_WINDOW)

static gboolean budgie_popover_draw(GtkWidget *widget, cairo_t *cr);
static void budgie_popover_map(GtkWidget *widget);
static void budgie_popover_unmap(GtkWidget *widget);
static void budgie_popover_grab_notify(GtkWidget *widget, gboolean was_grabbed, gpointer udata);
static gboolean budgie_popover_grab_broken(GtkWidget *widget, GdkEvent *event, gpointer udata);
static void budgie_popover_grab(BudgiePopover *self);
static void budgie_popover_ungrab(BudgiePopover *self);
static void budgie_popover_add(GtkContainer *container, GtkWidget *widget);
static gboolean budgie_popover_button_press(GtkWidget *widget, GdkEventButton *button,
                                            gpointer udata);
static gboolean budgie_popover_key_press(GtkWidget *widget, GdkEventKey *key, gpointer udata);
static void budgie_popover_set_property(GObject *object, guint id, const GValue *value,
                                        GParamSpec *spec);
static void budgie_popover_get_property(GObject *object, guint id, GValue *value, GParamSpec *spec);
static void budgie_popover_compute_positition(BudgiePopover *self, GdkRectangle *target,
                                              GtkPositionType *final_tail);
static void budgie_popover_compute_widget_geometry(GtkWidget *parent_widget, GdkRectangle *target);

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
        obj_class->set_property = budgie_popover_set_property;
        obj_class->get_property = budgie_popover_get_property;

        /* widget vtable hookup */
        wid_class->draw = budgie_popover_draw;
        wid_class->map = budgie_popover_map;
        wid_class->unmap = budgie_popover_unmap;

        /* container vtable */
        cont_class->add = budgie_popover_add;

        /*
         * BudgiePopover:relative-to
         *
         * Determines the GtkWidget that we'll appear next to
         */
        obj_properties[PROP_RELATIVE_TO] = g_param_spec_object("relative-to",
                                                               "Relative widget",
                                                               "Set the relative widget",
                                                               GTK_TYPE_WIDGET,
                                                               G_PARAM_READWRITE);

        g_object_class_install_properties(obj_class, N_PROPS, obj_properties);
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

        self->priv = budgie_popover_get_instance_private(self);
        self->priv->grabbed = FALSE;

        style = gtk_widget_get_style_context(GTK_WIDGET(self));
        gtk_style_context_add_class(style, "budgie-popover");

        /* Allow budgie-wm to know what we are */
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        gtk_window_set_wmclass(GTK_WINDOW(self), "budgie-popover", "budgie-popover");
        G_GNUC_END_IGNORE_DEPRECATIONS

        self->priv->add_area = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(self), self->priv->add_area);
        gtk_widget_show_all(self->priv->add_area);

        /* Setup window specific bits */
        gtk_window_set_position(win, GTK_WIN_POS_CENTER);
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
        g_object_set(self->priv->add_area,
                     "margin-top",
                     5,
                     "margin-bottom",
                     15,
                     "margin-start",
                     5,
                     "margin-end",
                     5,
                     NULL);
}

static void budgie_popover_map(GtkWidget *widget)
{
        GdkWindow *window = NULL;
        GdkRectangle coords = { 0 };
        BudgiePopover *self = NULL;
        GtkPositionType tail_position = GTK_POS_BOTTOM;

        self = BUDGIE_POPOVER(widget);

        /* Work out where we go on screen now */
        budgie_popover_compute_positition(self, &coords, &tail_position);

        g_message("Appearing at X, Y: %d %d", coords.x, coords.y);
        self->priv->tail_position = tail_position;

        /* Forcibly request focus */
        window = gtk_widget_get_window(widget);
        gdk_window_set_accept_focus(window, TRUE);
        gdk_window_focus(window, GDK_CURRENT_TIME);
        gdk_window_move(window, coords.x, coords.y);
        gtk_window_present(GTK_WINDOW(widget));

        budgie_popover_grab(BUDGIE_POPOVER(widget));
        budgie_popover_ungrab(BUDGIE_POPOVER(widget));
        budgie_popover_grab(BUDGIE_POPOVER(widget));

        GTK_WIDGET_CLASS(budgie_popover_parent_class)->map(widget);
}

static void budgie_popover_unmap(GtkWidget *widget)
{
        budgie_popover_ungrab(BUDGIE_POPOVER(widget));
        GTK_WIDGET_CLASS(budgie_popover_parent_class)->unmap(widget);
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

        if (self->priv->grabbed) {
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
                self->priv->grabbed = TRUE;
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

        if (!self->priv->grabbed) {
                return;
        }

        display = gtk_widget_get_display(GTK_WIDGET(self));
        seat = gdk_display_get_default_seat(display);

        gtk_grab_remove(GTK_WIDGET(self));
        gdk_seat_ungrab(seat);
        self->priv->grabbed = FALSE;
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
        self->priv->grabbed = FALSE;
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
        budgie_popover_ungrab(BUDGIE_POPOVER(widget));

        /* And being visible. ofc. */
        if (!gtk_widget_get_visible(widget)) {
                return;
        }

        self = BUDGIE_POPOVER(widget);
        budgie_popover_grab(self);
        budgie_popover_ungrab(BUDGIE_POPOVER(widget));
        budgie_popover_grab(self);
}

/**
 * Work out the geometry for the relative_to widget in absolute coordinates
 * on the screen.
 */
static void budgie_popover_compute_widget_geometry(GtkWidget *parent_widget, GdkRectangle *target)
{
        GtkAllocation alloc = { 0 };
        GtkWidget *toplevel = NULL;
        GdkWindow *toplevel_window = NULL;
        gint rx, ry = 0;
        gint x, y = 0;

        if (!parent_widget) {
                g_warning("compute_widget_geometry(): missing relative_widget");
                return;
        }

        toplevel = gtk_widget_get_toplevel(parent_widget);
        toplevel_window = gtk_widget_get_window(toplevel);
        gdk_window_get_position(toplevel_window, &x, &y);
        gtk_widget_translate_coordinates(parent_widget, toplevel, x, y, &rx, &ry);
        gtk_widget_get_allocation(parent_widget, &alloc);

        *target = (GdkRectangle){.x = rx, .y = ry, .width = alloc.width, .height = alloc.height };
}

/**
 * Work out exactly where the popover needs to appear on screen
 *
 * This will try to account for all potential positions, using a fairly
 * biased view of what the popover should do in each situation.
 *
 * Unlike a typical popover implementation, this relies on some information
 * from the toplevel window on what edge it happens to be on.
 */
static void budgie_popover_compute_positition(BudgiePopover *self, GdkRectangle *target,
                                              GtkPositionType *final_tail)
{
        GdkRectangle widget_rect = { 0 };
        GtkPositionType tail_position = GTK_POS_BOTTOM;
        gint our_width = 0, our_height = 0;
        GtkWidget *parent_window = NULL;
        int x = 0, y = 0, width = 0, height = 0;

        /* Find out where the widget is on screen */
        budgie_popover_compute_widget_geometry(self->priv->relative_to, &widget_rect);

        /* Work out our own size */
        gtk_window_get_size(GTK_WINDOW(self), &our_width, &our_height);

        /* Tail points out from the panel */
        parent_window = gtk_widget_get_toplevel(self->priv->relative_to);
        if (parent_window) {
                GtkStyleContext *context = gtk_widget_get_style_context(parent_window);
                if (gtk_style_context_has_class(context, "bottom")) {
                        tail_position = GTK_POS_TOP;
                } else if (gtk_style_context_has_class(context, "left")) {
                        tail_position = GTK_POS_RIGHT;
                } else if (gtk_style_context_has_class(context, "right")) {
                        tail_position = GTK_POS_LEFT;
                } else {
                        tail_position = GTK_POS_BOTTOM;
                }
        }

        /* Now work out where we live on screen */
        switch (tail_position) {
        case GTK_POS_BOTTOM:
                /* We need to appear above the widget */
                y = widget_rect.y - our_height;
                break;
        case GTK_POS_TOP:
                /* We need to appear below the widget */
                y = widget_rect.y + widget_rect.height + (TAIL_DIMENSION / 2);
                break;
        default:
                break;
        }

        /* Set the target rectangle */
        *target = (GdkRectangle){.x = x, .y = y, .width = width, .height = height };
        *final_tail = tail_position;
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

        cairo_set_line_width(cr, 1.3);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
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

static void budgie_popover_add(GtkContainer *container, GtkWidget *widget)
{
        BudgiePopover *self = NULL;

        self = BUDGIE_POPOVER(container);

        /* Only add internal area to self for real. Anything else goes to add_area */
        if (widget == self->priv->add_area) {
                GTK_CONTAINER_CLASS(budgie_popover_parent_class)->add(container, widget);
                return;
        }

        gtk_container_add(GTK_CONTAINER(self->priv->add_area), widget);
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

static void budgie_popover_set_property(GObject *object, guint id, const GValue *value,
                                        GParamSpec *spec)
{
        BudgiePopover *self = BUDGIE_POPOVER(object);

        switch (id) {
        case PROP_RELATIVE_TO:
                self->priv->relative_to = g_value_get_object(value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, spec);
                break;
        }
}

static void budgie_popover_get_property(GObject *object, guint id, GValue *value, GParamSpec *spec)
{
        BudgiePopover *self = BUDGIE_POPOVER(object);

        switch (id) {
        case PROP_RELATIVE_TO:
                g_value_set_object(value, self->priv->relative_to);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID(object, id, spec);
                break;
        }
}

/**
 * budgie_popover_new:
 * @relative_to: The widget to show the popover for

 * Construct a new BudgiePopover object
 *
 * Returns: (transfer full): A newly created #BudgiePopover
 */
GtkWidget *budgie_popover_new(GtkWidget *relative_to)
{
        /* Blame clang-format for weird wrapping */
        return g_object_new(BUDGIE_TYPE_POPOVER,
                            "relative-to",
                            relative_to,
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
                            GTK_WIN_POS_NONE,
                            NULL);
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
