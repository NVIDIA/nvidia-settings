/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 */

#ifndef __CTK_EVENT_H__
#define __CTK_EVENT_H__

#include <gtk/gtk.h>

#include "NvCtrlAttributes.h"

G_BEGIN_DECLS

#define CTK_TYPE_EVENT (ctk_event_get_type())

#define CTK_EVENT(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CTK_TYPE_EVENT, CtkEvent))

#define CTK_EVENT_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CTK_TYPE_EVENT, CtkEventClass))

#define CTK_IS_EVENT(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CTK_TYPE_EVENT))

#define CTK_IS_EVENT_CLASS(class) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CTK_TYPE_EVENT))

#define CTK_EVENT_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CTK_TYPE_EVENT, CtkEventClass))


typedef struct _CtkEvent       CtkEvent;
typedef struct _CtkEventClass  CtkEventClass;
typedef struct _CtkEventStruct CtkEventStruct;

struct _CtkEvent
{
    GObject               parent;
    NvCtrlAttributeHandle  *handle;
};

struct _CtkEventClass
{
    GtkWidgetClass parent_class;
};

struct _CtkEventStruct
{
    gint attribute;
    gint value;
    guint display_mask;
    gboolean is_availability_changed;
    gboolean availability;
};

GType       ctk_event_get_type  (void) G_GNUC_CONST;
GObject*    ctk_event_new       (NvCtrlAttributeHandle*);

void ctk_event_emit(CtkEvent *ctk_event,
                    unsigned int mask, int attrib, int value);
void ctk_event_emit_string(CtkEvent *ctk_event,
                    unsigned int mask, int attrib);

#define CTK_EVENT_NAME(x) ("CTK_EVENT_" #x)





G_END_DECLS

#endif /* __CTK_EVENT_H__ */
