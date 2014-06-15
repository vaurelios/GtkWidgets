#!/bin/env python3


from gi.repository import GObject
from gi.repository import Gtk
import cairo


class GtkMeter(Gtk.DrawingArea, Gtk.Orientable):
    adjustment = GObject.Property(type=Gtk.Adjustment,
                                  default=None,
                                  nick="Adjustment",
                                  blurb="The GtkAdjustment that contains the current value of this range object")
    peak = GObject.Property(type=float, default=0.0)
    lower = GObject.Property(type=float, default=-60.0)
    upper = GObject.Property(type=float, default=6.0)
    iec_lower = GObject.Property(type=float, default=0.0)
    iec_upper = GObject.Property(type=float, default=0.0)
    amber_level = GObject.Property(type=float, default=0.0)
    amber_frac = GObject.Property(type=float, default=0.0)

    def get_orientation(self):
        return self._orientation

    def set_orientation(self, orientation):
        self._orientation = orientation

        self.queue_resize()
    orientation = GObject.Property(get_orientation, set_orientation, Gtk.Orientation, Gtk.Orientation.VERTICAL)

    def get_inverted(self):
        return self._inverted

    def set_inverted(self, inverted):
        self._inverted = inverted

        self.queue_draw()
    inverted = GObject.Property(get_inverted, set_inverted, bool, False)


    def __init__(self, adjustment, orientation, min, max):
        if (not isinstance(adjustment, Gtk.Adjustment)): return
        
        super().__init__()

        self.adjustment = adjustment
        self.set_orientation(orientation)
        self.set_inverted(False)
        self.lower = min
        self.upper = max
        self.iec_lower = self.iec_scale(min)
        self.iec_upper = self.iec_scale(max)

        # old vars #
        self.old_value = 0.0
        self.old_lower = 0.0
        self.old_upper = 0.0

        # size request #
        if (self._orientation == Gtk.Orientation.VERTICAL):
            self.set_size_request (20, 10)
        else:
            self.set_size_request (10, 20)

        # Callbacks #

        ## Meter callbacks ##
        self.connect('draw', self.draw)

        ## Adjustment callbacks ##
        self.adjustment.connect('value-changed', self.adjustment_value_changed)

    def draw(self, meter, cr):
        width = meter.get_allocated_width()
        height = meter.get_allocated_height()

        if (self._orientation == Gtk.Orientation.VERTICAL):
            dw = width - 2
            dh = height - 2
        else:
            dw = height - 2
            dh = width - 2

        if (self._orientation == Gtk.Orientation.VERTICAL):
            pat = cairo.LinearGradient(width / 2, height, width / 2, 0)
        else:
            pat = cairo.LinearGradient(0, height / 2, width, height / 2)

        if (self._inverted):
            pat.add_color_stop_rgba(1, 0, 0, 0, 1)
            pat.add_color_stop_rgba(0, 0.6, 0.6, 0.6, 1)
        else:
            pat.add_color_stop_rgba(0, 0, 0, 0, 1)
            pat.add_color_stop_rgba(1, 0.6, 0.6, 0.6, 1)

        cr.rectangle(0, 0, width, height)
        cr.set_source(pat)
        cr.fill()

        val = self.iec_scale(self.adjustment.get_value())

        frac = (val - self.iec_lower) / (self.iec_upper - self.iec_lower)
        peak_frac = (self.peak - self.iec_lower) / (self.iec_upper - self.iec_lower)

        if (val > self.peak):
            if (val > self.iec_upper):
                self.peak = self.iec_upper
            else:
                self.peak = val

        if (frac < self.amber_frac):
            g_h = frac * dh
            a_h = g_h
            r_h = g_h
        elif (val <= 100.0):
            g_h = self.amber_frac * dh
            a_h = frac * dh
            r_h = a_h
        else:
            g_h = self.amber_frac * dh
            a_h = dh * (100.0 - self.iec_lower) / (self.iec_upper - self.iec_lower)
            r_h = frac * dh

        if (a_h > dh):
            a_h = dh
        if (r_h > dh):
            r_h = dh


        # Generate bars #

        ## Green bar ##
        if (self._orientation == Gtk.Orientation.VERTICAL and self._inverted):
            cr.rectangle(2, 2, dw, g_h)
        elif (self._orientation == Gtk.Orientation.VERTICAL and not self._inverted):
            cr.rectangle(2, dh - g_h, dw, g_h)
        elif (self._orientation == Gtk.Orientation.HORIZONTAL and self._inverted):
            cr.rectangle(dh - g_h, 2, g_h, dw)
        elif (self._orientation == Gtk.Orientation.HORIZONTAL and not self._inverted):
            cr.rectangle(2, 2, g_h, dw)
        cr.set_source_rgba(0.1, 0.5, 0.2, 0.8)
        cr.fill()

        ## Amber bar ##
        if (a_h > g_h):
            if (self._orientation == Gtk.Orientation.VERTICAL and self._inverted):
                cr.rectangle(2, g_h, dw, a_h - g_h)
            elif (self._orientation == Gtk.Orientation.VERTICAL and not self._inverted):
                cr.rectangle(2, dh - a_h, dw, a_h - g_h)
            elif (self._orientation == Gtk.Orientation.HORIZONTAL and self._inverted):
                cr.rectangle(dh - a_h, 2, a_h - g_h, dw)
            elif (self._orientation == Gtk.Orientation.HORIZONTAL and not self._inverted):
                cr.rectangle(g_h, 2, a_h - g_h, dw)
            cr.set_source_rgba(0.8, 0.8, 0.2, 0.8)
            cr.fill()

        ## Red bar ##
        if (r_h > a_h):
            if (self._orientation == Gtk.Orientation.VERTICAL and self._inverted):
                cr.rectangle(2, a_h, dw, r_h - a_h)
            elif (self._orientation == Gtk.Orientation.VERTICAL and not self._inverted):
                cr.rectangle(2, dh - r_h, dw, r_h - a_h)
            elif (self._orientation == Gtk.Orientation.HORIZONTAL and self._inverted):
                cr.rectangle(dh - r_h, 2, r_h - a_h, dw)
            elif (self._orientation == Gtk.Orientation.HORIZONTAL and not self._inverted):
                cr.rectangle(a_h, 2,  r_h - a_h , dw)
            cr.set_source_rgba(1, 0, 0.1, 0.8)
            cr.fill()

        # Create glassy layer effect #

        cr.set_source_rgba(0.8, 0.8, 0.8, 1.0)

        ## left ##
        cr.rectangle(0, 0, 2, height)
        cr.fill()

        ## top ##
        cr.rectangle(0, 0, width, 2)
        cr.fill()

        ## right ##
        cr.rectangle(width - 2, 0, 2, height)
        cr.fill()

        ## bottom ##
        cr.rectangle(0, height - 2, width, 2)
        cr.fill()

        # left hand glass bubble effect #
        if (self._orientation == Gtk.Orientation.VERTICAL):
            pat = cairo.LinearGradient(2, dh / 2, dw / 2, dh / 2)
        else:
            pat = cairo.LinearGradient(dh / 2, 2, dh / 2, dw / 2)

        pat.add_color_stop_rgba (0, 0, 0.2, 0.7, 0.2)
        pat.add_color_stop_rgba (1, 1.0, 1.0, 1.0, 0.3)
        
        if (self._orientation == Gtk.Orientation.VERTICAL):
            cr.rectangle(2, 2, dw / 2, dh - 2)
        else:
            cr.rectangle(2, 2, dh - 2, dw / 2 - 2)
        cr.set_source(pat)
        cr.fill()

        # right hand glass bubble effect #
        if (self._orientation == Gtk.Orientation.VERTICAL):
            pat = cairo.LinearGradient(dw / 2, 2, dw - 2, 2)
        else:
            pat = cairo.LinearGradient(2, dw / 2 + 2, 2, dw)

        pat.add_color_stop_rgba (1, 0.0, 0.4, 1.0, 0.2)
        pat.add_color_stop_rgba (0, 1.0, 1.0, 1.0, 0.3)

        if (self._orientation == Gtk.Orientation.VERTICAL):
            cr.rectangle(dw / 2 + 2, 2, dw / 2 - 2, dh - 2)
        else:
            cr.rectangle(0, dw / 2, dh, dw)
        cr.set_source(pat)
        cr.fill()

        # draw notches #
        # self.draw_notch(cr, 0.0, 4, dw, dh)

        # for val in range(5, int(self.upper), 5):
        #     self.draw_notch(cr, val, 2, dw, dh)

        # for val in range(int(self.lower), 0, 5):
        #     self.draw_notch(cr, val, 2, dw, dh)

        # for val in range(-10, int(self.upper)):
        #     self.draw_notch(cr, val, 1, dw, dh)

        return False

    def draw_notch(self, cr, db, mark, dw, dh):
        if (self._orientation == Gtk.Orientation.HORIZONTAL and self._inverted):
            pos = dh * (self.iec_scale(db) - self.iec_lower) / (self.iec_upper - self.iec_lower)
        elif (self._orientation == Gtk.Orientation.HORIZONTAL and not self._inverted):
            pos = dh - dh * (self.iec_scale(db) - self.iec_lower) / (self.iec_upper - self.iec_lower)
        elif (self._orientation == Gtk.Orientation.VERTICAL and not self._inverted):
            pos = dh * (self.iec_scale(db) - self.iec_lower) / (self.iec_upper - self.iec_lower)
        elif (self._orientation == Gtk.Orientation.VERTICAL and self._inverted):
            pos = dh - dh * (self.iec_scale(db) - self.iec_lower) / (self.iec_upper - self.iec_lower)

        cr.set_source_rgba (0, 0, 0, 0.8)

        if (self._orientation == Gtk.Orientation.HORIZONTAL):
            cr.rectangle(dh - pos, dw / 2 - mark / 2, 1, mark + 1)
            cr.fill()
        else:
            cr.rectangle(dw / 2 - mark / 2, dh - pos, mark + 1, 1)
            cr.fill()

    def adjustment_changed(self, adj):
        if (self.old_lower != adj.get_lower() or self.old_upper != adj.get_upper()):
            self.iec_lower = self.iec_scale(adj.get_lower())
            self.iec_upper = self.iec_scale(adj.get_upper())

            self.set_warn_point(self.amber_level)

            self.old_lower = adj.get_lower()
            self.old_upper = adj.get_upper()

    def adjustment_value_changed(self, adj):
        if (self.old_value != adj.get_value()):
            self.old_value = adj.get_value()
            self.queue_draw()

    def iec_scale(self, db):
        if (db < -70.0):
            defr = 0.0
        elif (db < -60.0):
            defr = (db + 70.0) * 0.25
        elif (db < -50.0):
            defr = (db + 60.0) * 0.5 + 2.5
        elif (db < -40.0):
            defr = (db + 50.0) * 0.75 + 7.5
        elif (db < -30.0):
            defr = (db + 40.0) * 1.5 + 15.0
        elif (db < -20.0):
            defr = (db + 30.0) * 2.0 + 30.0
        else:
            defr = (db + 20.0) * 2.5 + 50.0

        return defr

    def set_warn_point(self, pt):
        self.amber_level = pt

        if (self._inverted):
            self.amber_frac = 1.0 - (self.iec_scale(self.amber_level) - self.iec_lower) / (self.iec_upper - self.iec_lower)
        else:
            self.amber_frac = (self.iec_scale(self.amber_level) - self.iec_lower) / (self.iec_upper - self.iec_lower)

        self.queue_draw()