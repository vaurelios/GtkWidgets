/*
 * This file is part of G3JAMin.
 *
 * Copyright (C) 2013 Patrick Shirkey
 * Copyright (C) 2014 Victor A. Santos <victoraur.santos@gmail.com>
 *
 * G3JAMin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * G3JAMin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __GTK_METER_H__
#define __GTK_METER_H__

#include <gdk/gdk.h>


G_BEGIN_DECLS

#define GTK_TYPE_METER            (gtk_meter_get_type ())
#define GTK_METER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_METER, GtkMeter))
#define GTK_METER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_METER, GtkMeterClass))
#define GTK_IS_METER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_METER))
#define GTK_IS_METER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_METER))
#define GTK_METER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_METER, GtkMeterClass))


typedef struct _GtkMeter            GtkMeter;
typedef struct _GtkMeterPrivate     GtkMeterPrivate;
typedef struct _GtkMeterClass       GtkMeterClass;


struct _GtkMeter
{
  GtkWidget widget;
  
  GtkMeterPrivate *priv;
  
};

struct _GtkMeterClass
{
  GtkDrawingAreaClass parent_class;
};


GType          gtk_meter_get_type               (void) G_GNUC_CONST;
GtkWidget*     gtk_meter_new                    (GtkOrientation  orientation,
                                                 GtkAdjustment  *adjustment);
GtkAdjustment* gtk_meter_get_adjustment         (GtkMeter       *meter);
void           gtk_meter_set_adjustment         (GtkMeter       *meter,
                                                 GtkAdjustment  *adjustment);
gboolean       gtk_meter_get_inverted           (GtkMeter       *meter);
void           gtk_meter_set_inverted           (GtkMeter       *meter,
                                                 gboolean        inverted);
float          gtk_meter_get_peak               (GtkMeter       *meter);
void	         gtk_meter_reset_peak             (GtkMeter       *meter);
void           gtk_meter_set_warn_point         (GtkMeter       *meter,
                                                 gfloat          pt);

G_END_DECLS

#endif /* __GTK_METER */
