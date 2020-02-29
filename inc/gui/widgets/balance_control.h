/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

/** \file
 */

#ifndef __GUI_WIDGETS_BALANCE_CONTROL_H__
#define __GUI_WIDGETS_BALANCE_CONTROL_H__

#include "utils/general.h"

#include <gtk/gtk.h>

#define BALANCE_CONTROL_WIDGET_TYPE \
  (balance_control_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  BalanceControlWidget, balance_control_widget,
  Z, BALANCE_CONTROL_WIDGET,
  GtkDrawingArea)

typedef struct _BalanceControlWidget
{
  GtkDrawingArea     parent_instance;
  GtkGestureDrag *   drag;

  /** Getter. */
  GenericFloatGetter getter;

  /** Setter. */
  GenericFloatSetter setter;

  /** Object to call get/set with. */
  void *             object;
  double             last_x;
  double             last_y;
  GtkWindow *        tooltip_win;
  GtkLabel *         tooltip_label;
  GdkRGBA            start_color;
  GdkRGBA            end_color;

  /** Currently hovered or not. */
  int                hovered;

  /** Currently being dragged or not. */
  int                dragged;

  /** BalanceControlgo layout for drawing text. */
  PangoLayout *      layout;
} BalanceControlWidget;

/**
 * Creates a new BalanceControl widget and binds it to the
 * given value.
 */
BalanceControlWidget *
balance_control_widget_new (
  GenericFloatGetter getter,
  GenericFloatSetter setter,
  void *             object,
  int                height);

#endif