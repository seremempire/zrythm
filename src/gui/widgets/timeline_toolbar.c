/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "gui/accel.h"
#include "gui/widgets/timeline_toolbar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/snap_grid.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (TimelineToolbarWidget,
               timeline_toolbar_widget,
               GTK_TYPE_TOOLBAR)

void
timeline_toolbar_widget_setup (
  TimelineToolbarWidget * self)
{
  /* setup bot toolbar */
  snap_grid_widget_setup (
    self->snap_grid_timeline,
    &PROJECT->snap_grid_timeline);
}

static void
timeline_toolbar_widget_init (
  TimelineToolbarWidget * self)
{
  g_type_ensure (SNAP_GRID_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
timeline_toolbar_widget_class_init (
  TimelineToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "timeline_toolbar.ui");

  gtk_widget_class_set_css_name (
    klass, "timeline-toolbar");

  gtk_widget_class_bind_template_child (
    klass,
    TimelineToolbarWidget,
    snap_grid_timeline);
}