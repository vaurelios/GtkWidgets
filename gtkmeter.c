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


#include <math.h>
#include <stdio.h>
#include <gtk/gtk.h>

#include "gtkmeter.h"


#define METERSCALE_MAX_FONT_SIZE 8

/* Forward declarations */

static void gtk_meter_class_init               (GtkMeterClass    *klass);
static void gtk_meter_init                     (GtkMeter         *meter);
static void gtk_meter_set_property             (GObject          *object,
                                                guint             prop_id,
                                                const GValue     *value,
                                                GParamSpec       *pspec);
static void gtk_meter_get_property             (GObject          *object,
                                                guint             prop_id,
                                                GValue           *value,
                                                GParamSpec       *pspec);
static void gtk_meter_destroy                  (GtkWidget        *widget);
static gboolean gtk_meter_draw                 (GtkWidget        *widget,
                                                cairo_t          *cr);
static void meterscale_draw_notch_label        (GtkMeter         *meterscale,
                                                float             db,
                                                int               mark,
                                                PangoRectangle   *last_label_rect);
static void draw_notch                         (GtkMeter         *meter,
                                                cairo_t          *cr,
                                                float             db,
                                                int               mark,
                                                int               length,
                                                int               width);
static void gtk_meter_adjustment_changed       (GtkAdjustment    *adjustment,
                                                gpointer          data);
static void gtk_meter_adjustment_value_changed (GtkAdjustment    *adjustment,
                                                gpointer          data);
static float iec_scale                         (float db);


G_DEFINE_TYPE_WITH_CODE (GtkMeter, gtk_meter, GTK_TYPE_DRAWING_AREA,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL))
                                               
/* Local data */

static GtkWidgetClass *parent_class = NULL;

  
struct _GtkMeterPrivate
{  
  GtkOrientation orientation;

  gboolean inverted;

  /* The adjustment object that stores the data for this meter */
  GtkAdjustment *adjustment;
    
  /* Deflection limits */
  gfloat lower;
  gfloat upper;
  gfloat iec_lower;
  gfloat iec_upper;

  /* update policy (GTK_UPDATE_[CONTINUOUS/DELAYED/DISCONTINUOUS]) */
  guint direction : 2;
  
  GdkWindow         *event_window;

  /* Button currently pressed or 0 if none */
  guint8 button;

  /* Amber dB and deflection points */
  gfloat amber_level;
  gfloat amber_frac;

  /* Peak deflection */
  gfloat peak;
  
  /* Peak deflection in reasonable units */
  gfloat peak_db;
  
  /* ID of update timer, or 0 if none */
  guint32 timer;

  /* Old values from adjustment stored so we know when something changes */
  gfloat old_value;
  gfloat old_lower;
  gfloat old_upper;
};

enum {
  PROP_0,
  
  PROP_ORIENTATION,
  PROP_ADJUSTMENT,
  
  N_PROPERTIES
};

static void
gtk_meter_class_init (GtkMeterClass *kclass)
{
  GObjectClass   *object_class;
  GtkWidgetClass *widget_class;

  object_class = G_OBJECT_CLASS (kclass);
  widget_class = GTK_WIDGET_CLASS (kclass);

  object_class->set_property = gtk_meter_set_property;
  object_class->get_property = gtk_meter_get_property;

  widget_class->draw = gtk_meter_draw;

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

 g_object_class_install_property (object_class,
                                  PROP_ADJUSTMENT,
                                  g_param_spec_object ("adjustment",
                                                       "Adjustment",
                                                       "The GtkAdjustment that contains the current value of this meter object",
                                                        GTK_TYPE_ADJUSTMENT,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_type_class_add_private (object_class, sizeof (GtkMeterPrivate));
}

static void
gtk_meter_init (GtkMeter *meter)
{
//  printf("in gtk_meter_init\n");

  meter->priv = G_TYPE_INSTANCE_GET_PRIVATE (meter,
                                             GTK_TYPE_METER,
                                             GtkMeterPrivate);

  gtk_widget_set_has_window (GTK_WIDGET (meter), FALSE);

  meter->priv->orientation = GTK_ORIENTATION_VERTICAL;
  meter->priv->inverted = FALSE;
  meter->priv->adjustment = NULL;
  meter->priv->button = 0;
  meter->priv->timer = 0;
  meter->priv->amber_level = -6.0f;
  meter->priv->amber_frac = 0.0f;
  meter->priv->iec_lower = 0.0f;
  meter->priv->iec_upper = 0.0f;
  meter->priv->old_value = 0.0;
  meter->priv->old_lower = 0.0;
  meter->priv->old_upper = 0.0;
}

static void
gtk_meter_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GtkMeter *meter;

  meter = GTK_METER (object);

  switch (prop_id)
  {
  case PROP_ORIENTATION:
    meter->priv->orientation = g_value_get_enum (value);
    gtk_widget_queue_resize (GTK_WIDGET (meter));
    break;
  case PROP_ADJUSTMENT:
    gtk_meter_set_adjustment (meter, g_value_get_object (value));
    break;
  }
}

static void
gtk_meter_get_property (GObject      *object,
                        guint         prop_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
  GtkMeter *meter = GTK_METER (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, meter->priv->orientation);
      break;
    case PROP_ADJUSTMENT:
      g_value_set_object (value, meter->priv->adjustment);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

GtkWidget*
gtk_meter_new (GtkOrientation  orientation,
               GtkAdjustment  *adjustment)
{
	GtkWidget *widget = NULL;
  GtkMeter  *meter  = NULL;

  g_return_val_if_fail (GTK_IS_ADJUSTMENT (adjustment), widget);

  widget = g_object_new (GTK_TYPE_METER, NULL);
  meter = GTK_METER (widget);

  gtk_meter_set_adjustment (meter, adjustment);
  meter->priv->orientation = orientation;
  meter->priv->lower = gtk_adjustment_get_lower (adjustment);
  meter->priv->upper = gtk_adjustment_get_upper (adjustment);
  meter->priv->iec_lower = iec_scale(meter->priv->lower);
  meter->priv->iec_upper = iec_scale(meter->priv->upper);

  return widget;
}

GtkAdjustment*
gtk_meter_get_adjustment (GtkMeter *meter)
{
  g_return_val_if_fail (GTK_IS_METER (meter), NULL);

  if (!meter->priv->adjustment) {
    GtkAdjustment *adj = NULL;

    adj = gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    gtk_meter_set_adjustment (meter, adj);
  }

  return meter->priv->adjustment;
}

void
gtk_meter_set_adjustment (GtkMeter      *meter,
                          GtkAdjustment *adjustment)
{
  g_return_if_fail (GTK_IS_METER (meter));

  if (meter->priv->adjustment != adjustment) {
    if (meter->priv->adjustment) {
      g_signal_handlers_disconnect_by_func (meter->priv->adjustment,
                                            gtk_meter_adjustment_changed,
                                            meter);
      g_signal_handlers_disconnect_by_func (meter->priv->adjustment,
                                            gtk_meter_adjustment_value_changed,
                                            meter);
      g_object_unref (meter->priv->adjustment);
    }
    
    meter->priv->adjustment = adjustment;
    g_object_ref_sink (adjustment);
    
    g_signal_connect (adjustment, "changed",
                      G_CALLBACK (gtk_meter_adjustment_changed),
                      meter);
    g_signal_connect (adjustment, "value-changed",
                      G_CALLBACK (gtk_meter_adjustment_value_changed),
                      meter);

    gtk_meter_adjustment_changed (adjustment, meter);
  }
  
  gtk_widget_queue_draw (GTK_WIDGET (meter));
} 

gboolean
gtk_meter_get_inverted (GtkMeter *meter)
{
  return meter->priv->inverted;
}

void
gtk_meter_set_inverted (GtkMeter *meter,
                        gboolean  inverted)
{
  meter->priv->inverted = inverted;

  gtk_widget_queue_draw (GTK_WIDGET (meter));
}

static gboolean
gtk_meter_draw (GtkWidget *widget,
                cairo_t   *cr)
{
  float    val, frac, peak_frac;
  int      g_h, a_h, r_h;
  int      width = 0, height = 0;
  int      dw = 0, dh = 0;
  cairo_pattern_t *pat;
  GtkMeter *meter = GTK_METER (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  if (meter->priv->orientation == GTK_ORIENTATION_VERTICAL) {
    dw = width - 2;
    dh = height - 2;
  } else {
    dw = height - 2;
    dh = width - 2;
  }

  /* Draw the background layer with gradient  */
  if (meter->priv->orientation == GTK_ORIENTATION_VERTICAL)
    pat = cairo_pattern_create_linear (width / 2, height, width / 2, 0);
  else
    pat = cairo_pattern_create_linear (0, height / 2, width, height / 2);

  if (meter->priv->inverted) {
    cairo_pattern_add_color_stop_rgba (pat, 1, 0, 0, 0, 1);
    cairo_pattern_add_color_stop_rgba (pat, 0, 0.6, 0.6, 0.6, 1);
  } else {
    cairo_pattern_add_color_stop_rgba (pat, 0, 0.0, 0.0, 0.0, 1);
    cairo_pattern_add_color_stop_rgba (pat, 1, 0.6, 0.6, 0.6, 1);
  }

  cairo_rectangle (cr, 0, 0, width, height);
  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  val = iec_scale(gtk_adjustment_get_value(meter->priv->adjustment));
  if (val > meter->priv->peak) {
    if (val > meter->priv->iec_upper) {
      meter->priv->peak = meter->priv->iec_upper;
    } else {
      meter->priv->peak = val;
    }
  }

  frac = (val - meter->priv->iec_lower) / (meter->priv->iec_upper - meter->priv->iec_lower);
  peak_frac = (meter->priv->peak - meter->priv->iec_lower) /
              (meter->priv->iec_upper - meter->priv->iec_lower);

  if (frac < meter->priv->amber_frac) {
    g_h = frac * dh;
    a_h = g_h;
    r_h = g_h;
  } else if (val <= 100.0f) {
    g_h = meter->priv->amber_frac * dh;
    a_h = frac * dh;
    r_h = a_h;
  } else {
    g_h = meter->priv->amber_frac * dh;
    a_h = dh * (100.0f - meter->priv->iec_lower) / (meter->priv->iec_upper - meter->priv->iec_lower);
    r_h = frac * dh;
  }

  if (a_h > dh) {
    a_h = dh;
  }
  if (r_h > dh) {
    r_h = dh;
  }

  /* generate colors */

  // Green bar
  if (meter->priv->orientation == GTK_ORIENTATION_VERTICAL && !meter->priv->inverted)
	  cairo_rectangle (cr, 2, dh - g_h, dw, g_h);
  else if (meter->priv->orientation == GTK_ORIENTATION_VERTICAL && meter->priv->inverted)
    cairo_rectangle (cr, 2, 2, dw, g_h);
  else if (meter->priv->orientation == GTK_ORIENTATION_HORIZONTAL && !meter->priv->inverted)
	  cairo_rectangle (cr, 2, 2, g_h, dw);
  else if (meter->priv->orientation == GTK_ORIENTATION_HORIZONTAL && meter->priv->inverted)
	  cairo_rectangle (cr, dh - g_h, 2, g_h, dw);
	cairo_set_source_rgba (cr, 0.1, 0.5, 0.2, 0.8);
	cairo_fill (cr);

  // amber bar
  if (a_h > g_h) {
	  if (meter->priv->orientation == GTK_ORIENTATION_VERTICAL && !meter->priv->inverted)
			cairo_rectangle (cr, 2, dh - a_h, dw, a_h - g_h);
    else if (meter->priv->orientation == GTK_ORIENTATION_VERTICAL && meter->priv->inverted)
      cairo_rectangle (cr, 2, g_h, dw, a_h - g_h);
		else if (meter->priv->orientation == GTK_ORIENTATION_HORIZONTAL && !meter->priv->inverted)
			cairo_rectangle (cr, g_h, 2, a_h - g_h, dw);
    else if (meter->priv->orientation == GTK_ORIENTATION_HORIZONTAL && meter->priv->inverted)
      cairo_rectangle (cr, dh - a_h, 2, a_h - g_h, dw);
    cairo_set_source_rgba (cr, 0.8,0.8,0.2, 0.8);
    cairo_fill (cr);
  }
  
  // red bar
  if (r_h > a_h) {
	  if (meter->priv->orientation == GTK_ORIENTATION_VERTICAL && !meter->priv->inverted)
		  cairo_rectangle (cr, 2, dh - r_h, dw, r_h - a_h);
    else if (meter->priv->orientation == GTK_ORIENTATION_VERTICAL && meter->priv->inverted)
      cairo_rectangle (cr, 2, a_h, dw, r_h - a_h);
    else if (meter->priv->orientation == GTK_ORIENTATION_HORIZONTAL && !meter->priv->inverted)
		  cairo_rectangle (cr, a_h, 2, r_h - a_h, dw);
    else if (meter->priv->orientation == GTK_ORIENTATION_HORIZONTAL && meter->priv->inverted)
      cairo_rectangle (cr, dh - r_h, 2, r_h - a_h, dw);
		cairo_set_source_rgba (cr, 1,0,0.1, 0.8);
		cairo_fill (cr);
  }


  // Create glassy layer effect
  
  cairo_set_source_rgba (cr, 0.8, 0.8, 0.8, 1.0);

  // left
  cairo_rectangle (cr, 0, 0, 2, height);
  cairo_fill (cr);

  // top
	cairo_rectangle (cr, 2, 0, width - 2, 2);
  cairo_fill (cr);
 
  // right border
	cairo_rectangle (cr, width - 2, 0, 2, height);
  cairo_fill (cr);

  // bottom border
  cairo_rectangle (cr, 2, height - 2, width - 2, 2);
  cairo_fill (cr);

 // left hand glass bubble effect
  if(meter->priv->orientation == GTK_ORIENTATION_VERTICAL)
	  pat = cairo_pattern_create_linear (2, dh / 2, dw / 2, dh / 2);
  else
  	pat = cairo_pattern_create_linear (dh / 2, 2, dh / 2, dw / 2);

  cairo_pattern_add_color_stop_rgba (pat, 0, 0.0, 0.2, 0.7, 0.2);
  cairo_pattern_add_color_stop_rgba (pat, 1, 1.0, 1.0, 1.0, 0.3);

  if(meter->priv->orientation == GTK_ORIENTATION_VERTICAL)
		cairo_rectangle (cr, 2, 2, dw / 2, dh - 2);
	else
		cairo_rectangle (cr, 2, 2, dh - 2, dw / 2 - 2);
  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  // right hand glass bubble effect
  if(meter->priv->orientation == GTK_ORIENTATION_VERTICAL)
	  pat = cairo_pattern_create_linear (dw / 2, 2, dw - 2, 2);
  else
  	pat = cairo_pattern_create_linear (2, dw / 2 + 2, 2, dw);

  cairo_pattern_add_color_stop_rgba (pat, 1, 0.0, 0.4, 1.0, 0.2);
  cairo_pattern_add_color_stop_rgba (pat, 0, 1.0, 1.0, 1.0, 0.3);

  if(meter->priv->orientation == GTK_ORIENTATION_VERTICAL)
	  cairo_rectangle (cr, dw / 2 + 2, 2, dw / 2 - 2, dh - 2);
  else
	  cairo_rectangle (cr,  0, dw / 2, dh, dw);
  cairo_set_source (cr, pat);
  cairo_fill (cr);
  cairo_pattern_destroy (pat);

  // draw_notch(meter, cr, 0.0f, 3, dh, dw);

  // for (val = 5.0f; val < meter->priv->upper; val += 5.0f)
  //   draw_notch(meter, cr, val, 2, dh, dw);

  // for (val = -5.0f; val > meter->priv->lower+5.0f; val -= 5.0f)
  //    draw_notch(meter, cr, val, 2, dh, dw);
  
  // for (val = -10.0f; val < meter->priv->upper; val += 1.0f)
  //   draw_notch(meter, cr, val, 1, dh, dw);

  return FALSE;
}

// static void draw_notch(GtkMeter *meter,
//                        cairo_t  *cr,
//                        float     db,
//                        int       mark,
//                        int       length,
//                        int       width)
// {
//   int pos;

//   if (meter->priv->sides & GTK_METERSCALE_LEFT ||
//       meter->priv->sides & GTK_METERSCALE_RIGHT)
//   {
//     pos = length - length * (iec_scale(db) - meter->priv->iec_lower) /
// 		                        (meter->priv->iec_upper - meter->priv->iec_lower);
//   } else {
// 	  pos = length * (iec_scale(db) - meter->priv->iec_lower) /
// 		               (meter->priv->iec_upper - meter->priv->iec_lower);
//   }

// 	cairo_set_source_rgba (cr, 0,0,0, 0.8);

//   if (meter->priv->sides & GTK_METERSCALE_LEFT) {
//     cairo_rectangle (cr, pos, width/2 - mark/2, 1, mark+1);	
// 		cairo_fill (cr);
//   }
//   if (meter->priv->sides & GTK_METERSCALE_RIGHT) {
// 	  cairo_rectangle (cr, length -pos, width/2 - mark/2, 1, mark+1);
// 		cairo_fill (cr);
//   }
//   if (meter->priv->sides & GTK_METERSCALE_TOP) {
// 		cairo_rectangle (cr, width/2 - mark/2, length - pos, mark +1, 1);
// 		cairo_fill (cr);	    
//   }
//   if (meter->priv->sides & GTK_METERSCALE_BOTTOM) {
// 		cairo_rectangle (cr, width/2 - mark/2, pos, mark +1, 1);
// 		cairo_fill (cr);	    
//   }
// }

static void
gtk_meter_adjustment_changed (GtkAdjustment *adjustment,
                              gpointer       data)
{
  GtkMeter *meter;

  g_return_if_fail (data != NULL);

  meter = GTK_METER (data);
  
  if ((meter->priv->old_lower != gtk_adjustment_get_lower(adjustment)) ||
      (meter->priv->old_upper != gtk_adjustment_get_upper(adjustment)))
  {
    meter->priv->iec_lower = iec_scale(gtk_adjustment_get_lower(adjustment));
    meter->priv->iec_upper = iec_scale(gtk_adjustment_get_upper(adjustment));

    gtk_meter_set_warn_point(meter, meter->priv->amber_level);

    meter->priv->old_value = gtk_adjustment_get_value(adjustment);
    meter->priv->old_lower = gtk_adjustment_get_lower(adjustment);
    meter->priv->old_upper = gtk_adjustment_get_upper(adjustment);
  }
  else if (meter->priv->old_value != gtk_adjustment_get_value(adjustment))
    meter->priv->old_value = gtk_adjustment_get_value(adjustment);

  gtk_widget_queue_draw (GTK_WIDGET (meter));
}

static void
gtk_meter_adjustment_value_changed (GtkAdjustment *adjustment,
                                    gpointer       data)
{
  GtkMeter *meter;

  g_return_if_fail (data != NULL);

  meter = GTK_METER (data);

  if (meter->priv->old_value != gtk_adjustment_get_value(adjustment)) {
    meter->priv->old_value = gtk_adjustment_get_value(adjustment);

    gtk_widget_queue_draw (GTK_WIDGET (data));
  }
}

static float iec_scale(float db)
{
  float def = 0.0f;		/* Meter deflection %age */

  if (db < -70.0f) {
    def = 0.0f;
  } else if (db < -60.0f) {
    def = (db + 70.0f) * 0.25f;
  } else if (db < -50.0f) {
    def = (db + 60.0f) * 0.5f + 2.5f;
  } else if (db < -40.0f) {
    def = (db + 50.0f) * 0.75f + 7.5;
  } else if (db < -30.0f) {
    def = (db + 40.0f) * 1.5f + 15.0f;
  } else if (db < -20.0f) {
    def = (db + 30.0f) * 2.0f + 30.0f;
  } else {
    def = (db + 20.0f) * 2.5f + 50.0f;
  }

  return def; 
}

float gtk_meter_get_peak(GtkMeter *meter)
{
  return meter->priv->peak_db;
}

void gtk_meter_reset_peak(GtkMeter *meter)
{
  meter->priv->peak = 0.0f;
}

void gtk_meter_set_warn_point(GtkMeter *meter, gfloat pt)
{
  meter->priv->amber_level = pt;

  if (meter->priv->inverted)
    meter->priv->amber_frac = 1.0f - (iec_scale(meter->priv->amber_level) - meter->priv->iec_lower) /
		                                 (meter->priv->iec_upper - meter->priv->iec_lower);
  else
    meter->priv->amber_frac = (iec_scale(meter->priv->amber_level) - meter->priv->iec_lower) /
		                          (meter->priv->iec_upper - meter->priv->iec_lower);

  gtk_widget_queue_draw (GTK_WIDGET (meter));
}
