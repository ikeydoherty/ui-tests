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

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _BudgiePopover BudgiePopover;
typedef struct _BudgiePopoverClass BudgiePopoverClass;

#define BUDGIE_TYPE_POPOVER budgie_popover_get_type()
#define BUDGIE_POPOVER(o)                                                                    \
        (G_TYPE_CHECK_INSTANCE_CAST((o), BUDGIE_TYPE_POPOVER, BudgiePopover))
#define BUDGIE_IS_POPOVER(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), BUDGIE_TYPE_POPOVER))
#define BUDGIE_POPOVER_CLASS(o)                                                              \
        (G_TYPE_CHECK_CLASS_CAST((o), BUDGIE_TYPE_POPOVER, BudgiePopoverClass))
#define BUDGIE_IS_POPOVER_CLASS(o) (G_TYPE_CHECK_CLASS_TYPE((o), BUDGIE_TYPE_POPOVER))
#define BUDGIE_POPOVER_GET_CLASS(o)                                                          \
        (G_TYPE_INSTANCE_GET_CLASS((o), BUDGIE_TYPE_POPOVER, BudgiePopoverClass))

BudgiePopover *budgie_popover_new(void);

GType budgie_popover_get_type(void);

G_END_DECLS

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
