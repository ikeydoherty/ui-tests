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
#include "budgie-enums.h"
#include "popover.h"
#include <gtk/gtk.h>
BUDGIE_END_PEDANTIC

/**
 * We'll likely take this from a style property in future, but for now it
 * is both the width and height of a tail
 */
#define TAIL_DIMENSION 16
#define TAIL_HEIGHT TAIL_DIMENSION / 2
#define SHADOW_DIMENSION 4

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
        double x_offset;
        double y_offset;
        GtkPositionType position;
} BudgieTail;

struct _BudgiePopoverPrivate {
        GtkWidget *add_area;
        GtkWidget *relative_to;
        BudgieTail tail;
        BudgiePopoverPositionPolicy policy;
        gboolean grabbed;
};

enum { PROP_RELATIVE_TO = 1, PROP_POLICY, N_PROPS };

static GParamSpec *obj_properties[N_PROPS] = {
        NULL,
};

G_DEFINE_TYPE_WITH_PRIVATE(BudgiePopover, budgie_popover, GTK_TYPE_WINDOW)

static gboolean budgie_popover_draw(GtkWidget *widget, cairo_t *cr);
static void budgie_popover_map(GtkWidget *widget);
static void budgie_popover_unmap(GtkWidget *widget);
static gboolean budgie_popover_configure(GtkWidget *widget, GdkEventConfigure *event);
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
static void budgie_popover_compute_positition(BudgiePopover *self, GdkRectangle *target);
static void budgie_popover_compute_widget_geometry(GtkWidget *parent_widget, GdkRectangle *target);
static void budgie_popover_compute_tail(BudgiePopover *self);

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
        wid_class->configure_event = budgie_popover_configure;
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

        /**
         * BudgiePopover:position-policy:
         *
         * Control the behaviour used to place the popover on screen.
         */
        obj_properties[PROP_POLICY] = g_param_spec_enum("position-policy",
                                                        "Positioning policy",
                                                        "Get/set the popover position policy",
                                                        BUDGIE_TYPE_POPOVER_POSITION_POLICY,
                                                        BUDGIE_POPOVER_POSITION_AUTOMATIC,
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

        /* Set initial placement up for default bottom position */
        self->priv->tail.position = GTK_POS_BOTTOM;

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

        self = BUDGIE_POPOVER(widget);

        /* Work out where we go on screen now */
        budgie_popover_compute_positition(self, &coords);
        gtk_widget_queue_draw(widget);

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
 * We did a thing, so update our position to match our size
 */
static gboolean budgie_popover_configure(GtkWidget *widget, GdkEventConfigure *event)
{
        GdkWindow *window = NULL;
        GdkRectangle coords = { 0 };
        BudgiePopover *self = NULL;

        self = BUDGIE_POPOVER(widget);
        window = gtk_widget_get_window(widget);
        if (!window) {
                goto done;
        }

        /* Work out where we go on screen now */
        budgie_popover_compute_positition(self, &coords);

        gdk_window_move(window, coords.x, coords.y);
        gtk_widget_queue_draw(widget);

done:
        return GTK_WIDGET_CLASS(budgie_popover_parent_class)->configure_event(widget, event);
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
 * Use the appropriate function to find out the monitor's resolution for the
 * given @widget.
 */
static void budgie_popover_get_screen_for_widget(GtkWidget *widget, GdkRectangle *rectangle)
{
        GdkScreen *screen = NULL;
        GdkWindow *assoc_window = NULL;
        GdkDisplay *display = NULL;

        assoc_window = gtk_widget_get_parent_window(widget);
        screen = gtk_widget_get_screen(widget);
        display = gdk_screen_get_display(screen);

#if GTK_CHECK_VERSION(3, 22, 0)
        GdkMonitor *monitor = gdk_display_get_monitor_at_window(display, assoc_window);
        gdk_monitor_get_geometry(monitor, rectangle);
#else
        gint monitor = gdk_screen_get_monitor_at_window(screen, assoc_window);
        gdk_screen_get_monitor_geometry(screen, monitor, rectangle);
#endif
}

/**
 * Select the position of the popover tail (and extend outwards from it)
 * based on the hints provided by the toplevel
 */
static GtkPositionType budgie_popover_select_position_toplevel(BudgiePopover *self)
{
        GtkWidget *parent_window = NULL;

        /* Tail points out from the panel */
        parent_window = gtk_widget_get_toplevel(self->priv->relative_to);
        if (!parent_window) {
                return GTK_POS_BOTTOM;
        }

        GtkStyleContext *context = gtk_widget_get_style_context(parent_window);
        if (gtk_style_context_has_class(context, "top")) {
                return GTK_POS_TOP;
        } else if (gtk_style_context_has_class(context, "left")) {
                return GTK_POS_LEFT;
        } else if (gtk_style_context_has_class(context, "right")) {
                return GTK_POS_RIGHT;
        }

        return GTK_POS_BOTTOM;
}

/**
 * Select the position based on the amount of space available in the given
 * regions.
 *
 * Typically we'll always try to display underneath first, and failing that
 * we'll try to appear above.  If we're still estate-limited, we'll then try
 * the right hand side, before finally falling back to the left hand side for display.
 *
 * The side options will also utilise Y-offsets and bounding to ensure there
 * is always some way to fit the popover sanely on screen.
 */
static GtkPositionType budgie_popover_select_position_automatic(gint our_height,
                                                                GdkRectangle screen_rect,
                                                                GdkRectangle widget_rect)
{
        /* Try to show the popover underneath */
        if (widget_rect.y + widget_rect.height + TAIL_HEIGHT + SHADOW_DIMENSION + our_height <=
            screen_rect.y + screen_rect.height) {
                return GTK_POS_TOP;
        }

        /* Now try to show the popover above the widget */
        if (widget_rect.y - TAIL_HEIGHT - SHADOW_DIMENSION - our_height >= screen_rect.y) {
                return GTK_POS_BOTTOM;
        }

        /* Work out which has more room, left or right. */
        double room_right = screen_rect.x + screen_rect.width - (widget_rect.x + widget_rect.width);
        double room_left = widget_rect.x - screen_rect.x;

        if (room_left > room_right) {
                return GTK_POS_RIGHT;
        }

        return GTK_POS_LEFT;
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
static void budgie_popover_compute_positition(BudgiePopover *self, GdkRectangle *target)
{
        GdkRectangle widget_rect = { 0 };
        GtkPositionType tail_position = GTK_POS_BOTTOM;
        gint our_width = 0, our_height = 0;
        GtkStyleContext *style = NULL;
        int x = 0, y = 0, width = 0, height = 0;
        static const gchar *position_classes[] = { "top", "left", "right", "bottom" };
        const gchar *style_class = NULL;
        GdkRectangle display_geom = { 0 };

        /* Find out where the widget is on screen */
        budgie_popover_compute_widget_geometry(self->priv->relative_to, &widget_rect);

        /* Work out our own size */
        gtk_window_get_size(GTK_WINDOW(self), &our_width, &our_height);

        /* Work out the real screen geometry involved here */
        budgie_popover_get_screen_for_widget(self->priv->relative_to, &display_geom);

        if (self->priv->policy == BUDGIE_POPOVER_POSITION_TOPLEVEL_HINT) {
                tail_position = budgie_popover_select_position_toplevel(self);
        } else {
                tail_position =
                    budgie_popover_select_position_automatic(our_height, display_geom, widget_rect);
        }

        /* Now work out where we live on screen */
        switch (tail_position) {
        case GTK_POS_BOTTOM:
                /* We need to appear above the widget */
                y = widget_rect.y - our_height;
                x = (widget_rect.x + (widget_rect.width / 2)) - (our_width / 2);
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
                style_class = "bottom";
                break;
        case GTK_POS_TOP:
                /* We need to appear below the widget */
                y = widget_rect.y + widget_rect.height + (TAIL_DIMENSION / 2);
                x = (widget_rect.x + (widget_rect.width / 2)) - (our_width / 2);
                g_object_set(self->priv->add_area,
                             "margin-top",
                             10,
                             "margin-bottom",
                             10,
                             "margin-start",
                             5,
                             "margin-end",
                             5,
                             NULL);
                style_class = "top";
                break;
        case GTK_POS_LEFT:
                /* We need to appear to the right of the widget */
                y = (widget_rect.y + (widget_rect.height / 2)) - (our_height / 2);
                y += TAIL_DIMENSION / 4;
                x = widget_rect.x + widget_rect.width;
                g_object_set(self->priv->add_area,
                             "margin-top",
                             5,
                             "margin-bottom",
                             10,
                             "margin-start",
                             15,
                             "margin-end",
                             5,
                             NULL);
                style_class = "left";
                break;
        case GTK_POS_RIGHT:
                y = (widget_rect.y + (widget_rect.height / 2)) - (our_height / 2);
                y += TAIL_DIMENSION / 4;
                x = widget_rect.x - our_width;
                g_object_set(self->priv->add_area,
                             "margin-top",
                             5,
                             "margin-bottom",
                             10,
                             "margin-start",
                             5,
                             "margin-end",
                             15,
                             NULL);
                style_class = "right";
                break;
        default:
                break;
        }

        /* Update tail knowledge */
        self->priv->tail.position = tail_position;
        budgie_popover_compute_tail(self);

        /* Allow themers to know what kind of popover this is, and set the
         * CSS class in accordance with the direction that the popover is
         * pointing in.
         */
        style = gtk_widget_get_style_context(GTK_WIDGET(self));
        for (guint i = 0; i < G_N_ELEMENTS(position_classes); i++) {
                gtk_style_context_remove_class(style, position_classes[i]);
        }

        gtk_style_context_add_class(style, style_class);

        static int pad_num = 1;

        /* Bound X to display width */
        if (x < display_geom.x) {
                self->priv->tail.x_offset += (x - (display_geom.x + pad_num));
                x -= (int)(self->priv->tail.x_offset);
        } else if ((x + our_width) >= display_geom.x + display_geom.width) {
                self->priv->tail.x_offset -=
                    ((display_geom.x + display_geom.width) - (our_width + pad_num)) - x;
                x -= (int)(self->priv->tail.x_offset);
        }

        double display_tail_x = x + self->priv->tail.x + self->priv->tail.x_offset;
        double display_tail_y = y + self->priv->tail.y + self->priv->tail.y_offset;
        static double required_offset_x = TAIL_DIMENSION * 1.25;
        static double required_offset_y = TAIL_DIMENSION * 1.75;

        /* Prevent the tail pointer spilling outside the X bounds */
        if (display_tail_x <= display_geom.x + required_offset_x) {
                self->priv->tail.x_offset += (display_geom.x + required_offset_x) - display_tail_x;
        } else if (display_tail_x >= ((display_geom.x + display_geom.width) - required_offset_x)) {
                self->priv->tail.x_offset -=
                    (display_tail_x + required_offset_x) - (display_geom.x + display_geom.width);
        }

        /* Prevent the tail pointer spilling outside the Y bounds */
        if (display_tail_y <= display_geom.y + required_offset_y) {
                self->priv->tail.y_offset += (display_geom.y + required_offset_y) - display_tail_y;
        } else if (display_tail_y >= ((display_geom.y + display_geom.height) - required_offset_y)) {
                self->priv->tail.y_offset -=
                    (display_tail_y + required_offset_y) - (display_geom.y + display_geom.height);
        }

        /* Bound Y to display height */
        if (y < display_geom.y) {
                self->priv->tail.y_offset += (y - (display_geom.y + pad_num));
                y -= (int)(self->priv->tail.y_offset);
        } else if ((y + our_height) >= display_geom.y + display_geom.height) {
                self->priv->tail.y_offset -=
                    ((display_geom.y + display_geom.height) - (our_height + pad_num)) - y;
                y -= (int)(self->priv->tail.y_offset);
        }

        /* Set the target rectangle */
        *target = (GdkRectangle){.x = x, .y = y, .width = width, .height = height };
}

static void budgie_popover_compute_tail(BudgiePopover *self)
{
        GtkAllocation alloc = { 0 };
        BudgieTail t = { 0 };

        gtk_widget_get_allocation(GTK_WIDGET(self), &alloc);

        t.position = self->priv->tail.position;

        switch (self->priv->tail.position) {
        case GTK_POS_LEFT:
                t.x = alloc.x;
                t.y = alloc.y + (alloc.height / 2);
                t.start_y = t.y - TAIL_HEIGHT;
                t.end_y = t.y + TAIL_HEIGHT;
                t.start_x = t.end_x = t.x + TAIL_HEIGHT + SHADOW_DIMENSION;
                break;
        case GTK_POS_RIGHT:
                t.x = alloc.width;
                t.y = alloc.y + (alloc.height / 2);
                t.start_y = t.y - TAIL_HEIGHT;
                t.end_y = t.y + TAIL_HEIGHT;
                t.start_x = t.end_x = t.x - TAIL_HEIGHT - SHADOW_DIMENSION;
                break;
        case GTK_POS_TOP:
                t.x = (alloc.x + alloc.width / 2);
                t.y = alloc.y;
                t.start_x = t.x - TAIL_HEIGHT;
                t.end_x = t.start_x + TAIL_DIMENSION;
                t.start_y = t.y + TAIL_HEIGHT;
                t.end_y = t.start_y;
                break;
        case GTK_POS_BOTTOM:
        default:
                t.x = (alloc.x + alloc.width / 2);
                t.y = (alloc.y + alloc.height) - SHADOW_DIMENSION;
                t.start_x = t.x - TAIL_HEIGHT;
                t.end_x = t.start_x + TAIL_DIMENSION;
                t.start_y = t.y - TAIL_HEIGHT;
                t.end_y = t.start_y;
                break;
        }

        self->priv->tail = t;
}

/**
 * Draw the actual tail itself.
 */
static void budgie_popover_draw_tail(BudgiePopover *self, cairo_t *cr)
{
        BudgieTail *tail = &(self->priv->tail);

        /* Draw "through" the previous box-shadow */
        cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
        cairo_move_to(cr, tail->start_x + tail->x_offset, tail->start_y + tail->y_offset);
        cairo_line_to(cr, tail->x + tail->x_offset, tail->y + tail->y_offset);
        cairo_line_to(cr, tail->end_x + tail->x_offset, tail->end_y + tail->y_offset);
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
        GdkRGBA border_color = { 0 };
        GtkBorder border = { 0 };
        GtkStateFlags fl;
        BudgiePopover *self = NULL;
        BudgieTail *tail = NULL;
        GtkAllocation body_alloc = { 0 };

        self = BUDGIE_POPOVER(widget);
        tail = &(self->priv->tail);
        fl = GTK_STATE_FLAG_VISITED;

        cairo_set_antialias(cr, CAIRO_ANTIALIAS_SUBPIXEL);

        style = gtk_widget_get_style_context(widget);
        gtk_widget_get_allocation(widget, &alloc);
        body_alloc = alloc;

        /* Set up the offset */

        gdouble gap_start = 0, gap_end = 0;

        body_alloc.x += SHADOW_DIMENSION;
        body_alloc.width -= SHADOW_DIMENSION * 2;
        body_alloc.y += SHADOW_DIMENSION;
        body_alloc.height -= SHADOW_DIMENSION * 2;

        switch (self->priv->tail.position) {
        case GTK_POS_LEFT:
                body_alloc.height -= SHADOW_DIMENSION;
                body_alloc.width -= TAIL_HEIGHT;
                body_alloc.x += TAIL_HEIGHT;
                gap_start = tail->start_y + tail->y_offset;
                gap_end = tail->end_y + tail->y_offset;
                break;
        case GTK_POS_RIGHT:
                body_alloc.height -= SHADOW_DIMENSION;
                body_alloc.width -= TAIL_HEIGHT;
                gap_start = tail->start_y + tail->y_offset;
                gap_end = tail->end_y + tail->y_offset;
                break;
        case GTK_POS_TOP:
                body_alloc.height -= SHADOW_DIMENSION * 2;
                body_alloc.y += TAIL_HEIGHT;
                body_alloc.y -= SHADOW_DIMENSION;
                gap_start = tail->start_x + tail->x_offset;
                gap_end = tail->end_x + tail->x_offset;
                break;
        case GTK_POS_BOTTOM:
        default:
                body_alloc.height -= TAIL_HEIGHT;
                gap_start = tail->start_x + tail->x_offset;
                gap_end = tail->end_x + tail->x_offset;
                break;
        }

        gtk_style_context_set_state(style, GTK_STATE_FLAG_BACKDROP);
        /* Warning: Using deprecated API */
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        gtk_style_context_get_border_color(style, fl, &border_color);
        G_GNUC_END_IGNORE_DEPRECATIONS
        gtk_style_context_get_border(style, fl, &border);
        gtk_render_background(style,
                              cr,
                              body_alloc.x,
                              body_alloc.y,
                              body_alloc.width,
                              body_alloc.height);

        gtk_render_frame_gap(style,
                             cr,
                             body_alloc.x,
                             body_alloc.y,
                             body_alloc.width,
                             body_alloc.height,
                             self->priv->tail.position,
                             gap_start,
                             gap_end);
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
        budgie_popover_draw_tail(self, cr);
        cairo_clip(cr);
        cairo_move_to(cr, 0, 0);
        gtk_render_background(style, cr, alloc.x, alloc.y, alloc.width, alloc.height);

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

static gboolean budgie_popover_hide_self(gpointer v)
{
        gtk_widget_hide(GTK_WIDGET(v));
        return G_SOURCE_REMOVE;
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
        g_idle_add(budgie_popover_hide_self, widget);
        return GDK_EVENT_PROPAGATE;
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

/**
 * Our associated widget has died, so we must unref ourselves now.
 */
static void budgie_popover_disconnect(__budgie_unused__ GtkWidget *relative_to, BudgiePopover *self)
{
        self->priv->relative_to = NULL;
        gtk_widget_destroy(GTK_WIDGET(self));
}

static void budgie_popover_set_property(GObject *object, guint id, const GValue *value,
                                        GParamSpec *spec)
{
        BudgiePopover *self = BUDGIE_POPOVER(object);

        switch (id) {
        case PROP_RELATIVE_TO:
                if (self->priv->relative_to) {
                        g_signal_handlers_disconnect_by_data(self->priv->relative_to, self);
                }
                self->priv->relative_to = g_value_get_object(value);
                if (self->priv->relative_to) {
                        g_signal_connect(self->priv->relative_to,
                                         "destroy",
                                         G_CALLBACK(budgie_popover_disconnect),
                                         self);
                        budgie_popover_compute_tail(self);
                }
                break;
        case PROP_POLICY:
                self->priv->policy = g_value_get_enum(value);
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
        case PROP_POLICY:
                g_value_set_enum(value, self->priv->policy);
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

/**
 * budgie_popover_set_position_policy:
 *
 * Set the positioning policy employed by the popover
 *
 * @policy: New policy to use for positioning the popover
 */
void budgie_popover_set_position_policy(BudgiePopover *self, BudgiePopoverPositionPolicy policy)
{
        g_return_if_fail(self != NULL);
        g_object_set(self, "position-policy", policy, NULL);
}

/**
 * budgie_popover_get_position_policy:
 *
 * Retrieve the currently active positioning policy for this popover
 *
 * Returns: The #BudgiePopoverPositionPolicy currently in use
 */
BudgiePopoverPositionPolicy budgie_popover_get_position_policy(BudgiePopover *self)
{
        g_return_val_if_fail(self != NULL, 0);
        return self->priv->policy;
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
