/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_ROUTE_TARGET_SELECTOR_POPOVER_H__
#define __GUI_WIDGETS_ROUTE_TARGET_SELECTOR_POPOVER_H__


#include <gtk/gtk.h>

#define ROUTE_TARGET_SELECTOR_POPOVER_WIDGET_TYPE \
  (route_target_selector_popover_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  RouteTargetSelectorPopoverWidget,
  route_target_selector_popover_widget,
  Z, ROUTE_TARGET_SELECTOR_POPOVER_WIDGET,
  GtkPopover)

typedef enum RouteTargetSelectorType
{
  ROUTE_TARGET_TYPE_MASTER,
  ROUTE_TARGET_TYPE_GROUP,
  ROUTE_TARGET_TYPE_BUS,
} RouteTargetSelectorType;

typedef struct _RouteTargetSelectorWidget
  RouteTargetSelectorWidget;

typedef struct _RouteTargetSelectorPopoverWidget
{
  GtkPopover              parent_instance;

  /** The owner button. */
  RouteTargetSelectorWidget * owner;

  GtkBox *                type_treeview_box;
  GtkTreeView *           type_treeview;
  GtkTreeModel *          type_model;
  GtkBox *                route_treeview_box;
  GtkTreeView *           route_treeview;
  GtkTreeModel *          route_model;

  GtkLabel *              info;
  RouteTargetSelectorType type;
} RouteTargetSelectorPopoverWidget;

/**
 * Creates the popover.
 */
RouteTargetSelectorPopoverWidget *
route_target_selector_popover_widget_new (
  RouteTargetSelectorWidget * owner);

#endif
