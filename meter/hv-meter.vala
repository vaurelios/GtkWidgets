/* 
 * This file is part of HVMeter.
 *
 * HVMeter is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HVMeter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HVMeter.  If not, see <http://www.gnu.org/licenses/>.
 */


namespace HV
{
    public class Meter : Gtk.DrawingArea, Gtk.Orientable
    {
        /* Property-backing fields */
        private Gtk.Adjustment  _adjustment;
        private Gtk.Orientation _orientation;
        private bool            _inverted;
        private float           _warn_point;
        private float           _peak;

        /* Properties */
        public Gtk.Adjustment adjustment {
            get { return _adjustment; }
            set { _set_adjustment(value); }
        }

        public Gtk.Orientation orientation {
            get { return _orientation; }
            set { _orientation = value; queue_draw(); }
        }

        public bool inverted {
            get { return _inverted; }
            set { _inverted = value; queue_draw(); }
        }

        public float warn_point {
            get { return _warn_point; }
            set { _set_warn_point(value); queue_draw(); }
        }

        public float peak {
            get { return _peak; }
        }

        private float iec_lower;
        private float iec_upper;
        private float amber_frac;

        public Meter(Gtk.Orientation ori, Gtk.Adjustment? adj)
        {
            iec_lower    = 0.0f;
            iec_upper    = 0.0f;
            amber_frac   = 0.0f;
            _inverted    = false;
            adjustment   = adj;
            _orientation = ori;
            warn_point   = -6.0f;
            reset_peak();
        }

        public override bool draw(Cairo.Context cr)
        {
            int xlen, ylen;
            Cairo.Pattern pat;

            var width = get_allocated_width();
            var height = get_allocated_height();

            if (_orientation == Gtk.Orientation.HORIZONTAL)
            {
                xlen = height;
                ylen = width;
            }
            else
            {
                xlen = width;
                ylen = height;
            }

            if (_orientation == Gtk.Orientation.HORIZONTAL)
                pat = new Cairo.Pattern.linear(0, height / 2, width, height / 2);
            else
                pat = new Cairo.Pattern.linear(width / 2, 0, width / 2, height);

            if (_orientation == Gtk.Orientation.VERTICAL)
            {
                pat.add_color_stop_rgba((int) !_inverted, 0.0, 0.0, 0.0, 1);
                pat.add_color_stop_rgba((int)  _inverted, 0.6, 0.6, 0.6, 1);
            }
            else
            {
                pat.add_color_stop_rgba((int)  _inverted, 0.0, 0.0, 0.0, 1); 
                pat.add_color_stop_rgba((int) !_inverted, 0.6, 0.6, 0.6, 1);
            }

            cr.rectangle(0, 0, width, height);
            cr.set_source(pat);
            cr.fill();

            var val = iec_scale((float) _adjustment.value);
            var frac = (val - iec_lower) / (iec_upper - iec_lower);

            float g_h, a_h, r_h;
            if (frac < amber_frac)
            {
                g_h = frac * (ylen - 4);
                a_h = g_h;
                r_h = g_h;
            }
            else if (val <= 100.0f)
            {
                g_h = amber_frac * (ylen - 4);
                a_h = frac * (ylen - 4);
                r_h = a_h;
            }
            else
            {
                g_h = amber_frac * (ylen - 4);
                a_h = (100.0f - iec_lower) / (iec_upper - iec_lower) * (ylen - 4);
                r_h = frac * (ylen - 4);
            }

            if (a_h > (ylen - 4))
            {
                a_h = ylen - 4;
            }
            if (r_h > (ylen - 4))
            {
                r_h = ylen - 4;
            }
            
            /* generate colors */
            
            // Green bar
            if (_orientation == Gtk.Orientation.VERTICAL)
            {
                if (_inverted)
                    cr.rectangle(2, 2, xlen - 4, g_h);
                else
                    cr.rectangle(2, (ylen - 2) - g_h, xlen - 4, g_h);
            }
            else
            {
                if (_inverted)
                    cr.rectangle((ylen - 4) - g_h, 2, g_h + 2, xlen - 4);
                else
                    cr.rectangle(2, 2, g_h, xlen - 4);
            }
            cr.set_source_rgba(0.1, 0.5, 0.2, 0.8);
            cr.fill();

            // amber bar
            if (a_h > g_h) {
                if (_orientation == Gtk.Orientation.VERTICAL)
                {
                    if (_inverted)
                        cr.rectangle(2, g_h + 2, xlen - 4, a_h - (g_h - 2));
                    else
                        cr.rectangle(2, (ylen - 2) - a_h, xlen - 4, a_h - g_h);
                }
                else
                {
                    if (_inverted)
                        cr.rectangle((ylen - 4) - a_h, 2, a_h - g_h, xlen);
                    else
                        cr.rectangle(g_h + 2, 2, a_h - g_h, xlen - 4);
                }
                cr.set_source_rgba(0.8,0.8,0.2, 0.8);
                cr.fill();
            }

            // red bar
            if (r_h > a_h)
            {
                if (_orientation == Gtk.Orientation.VERTICAL)
                {
                    if (_inverted)
                        cr.rectangle(2, a_h + 4, xlen - 4, r_h - (a_h - 2));
                    else
                        cr.rectangle(2, (ylen - 2) - r_h, xlen - 4, r_h - a_h);
                }
                else
                {
                    if (_inverted)
                        cr.rectangle((ylen - 4) - r_h, 2, r_h - a_h, xlen - 4);
                    else
                        cr.rectangle(a_h + 2, 2, r_h - a_h, xlen - 4);
                }
                cr.set_source_rgba(1.0, 0.0, 0.1, 0.8);
                cr.fill();
            }

            /* Create glassy layer effect */

            cr.set_source_rgba(0.8, 0.8, 0.8, 1.0);

            // left
            cr.rectangle(0, 0, 2, height);
            cr.fill();

            // top
            cr.rectangle(2, 0, width - 4, 2);
            cr.fill();
 
            // right
            cr.rectangle(width - 2, 0, 2, height);
            cr.fill();

            // bottom
            cr.rectangle(2, height - 2, width - 4, 2);
            cr.fill();

            // left hand glass bubble effect
            if(_orientation == Gtk.Orientation.VERTICAL)
            {
                pat = new Cairo.Pattern.linear(0, 0, width / 2, 0);

                cr.rectangle(0, 0, width / 2, height);
            }
            else
            {
                pat = new Cairo.Pattern.linear(0, 0, 0, height / 2);

                cr.rectangle(0, 0, width, height / 2);
            }
 
            pat.add_color_stop_rgba(0, 0.0, 0.2, 0.7, 0.2);
            pat.add_color_stop_rgba(1, 1.0, 1.0, 1.0, 0.3);

            cr.set_source(pat);
            cr.fill();

            // right hand glass bubble effect
            if(_orientation == Gtk.Orientation.VERTICAL)
            {
                pat = new Cairo.Pattern.linear(width / 2, 0, width, 0);

                cr.rectangle(width / 2, 0, width / 2, height); 
            }
            else
            {
                pat = new Cairo.Pattern.linear(0, height / 2, 0, height);

                cr.rectangle(0, height / 2, width, height / 2); 
            }

            pat.add_color_stop_rgba(1, 0.0, 0.4, 1.0, 0.2);
            pat.add_color_stop_rgba(0, 1.0, 1.0, 1.0, 0.3);

            cr.set_source(pat);
            cr.fill();

            draw_peak(cr, xlen, ylen);
 
            /* Draw notches */
            draw_notch(cr, 0.0f, 4, xlen, ylen);

            for (var i = 5.0f; i < _adjustment.upper; i += 5.0f)
                draw_notch(cr, i, 2, xlen, ylen);

            for (var i = -5.0f; i > _adjustment.lower + 5.0f; i -= 5.0f)
                draw_notch(cr, i, 2, xlen, ylen);

            //for (var i = -5.0f; i <= 5.0f; val += 1.0f)
            //    draw_notch(cr, i, 1, xlen, ylen);

            return true;
        }

        private void draw_notch(Cairo.Context cr, float db, int mark, int xlen, int ylen)
        {
            float pos;

            if (_orientation == Gtk.Orientation.VERTICAL)
            {
                if (_inverted)
                    pos = ylen - (ylen * (iec_scale(db) - iec_lower) / (iec_upper - iec_lower));
                else
                    pos = ylen * (iec_scale(db) - iec_lower) / (iec_upper - iec_lower);
            }
            else
            {
                if (_inverted)
                    pos = ylen * (iec_scale(db) - iec_lower) / (iec_upper - iec_lower);
                else
                    pos = ylen - (ylen * (iec_scale(db) - iec_lower) / (iec_upper - iec_lower)); 
            }  

            cr.set_source_rgba(0, 0, 0, 0.8);

            if (_orientation == Gtk.Orientation.VERTICAL)
                cr.rectangle((xlen - 2) / 2 - mark / 2, ylen - pos + 1, mark, 1); 
            else
                cr.rectangle((ylen - 2) - pos, xlen / 2 - mark / 2, 1, mark); 
            cr.fill();
        }

        private void draw_peak(Cairo.Context cr, float xlen, float ylen)
        {
            var pos = (ylen - 2) * (iec_scale(_peak) - iec_lower) / (iec_upper - iec_lower);

            cr.set_source_rgba(0, 0, 1, 1);

            if (_orientation == Gtk.Orientation.VERTICAL)
            {
                if (_inverted)
                    cr.rectangle(2, pos, xlen - 4, 0.5);
                else
                    cr.rectangle(2, ylen - pos, xlen - 4, 0.5);
            }
            else
            {
                if (_inverted)
                    cr.rectangle(ylen - pos, 2, 0.5, xlen - 4);
                else
                    cr.rectangle(pos, 2, 0.5, xlen - 4);
            }
            cr.fill();
        }

        public void _set_adjustment(Gtk.Adjustment value)
        {
            Gtk.Adjustment adj = value;

            if (adj == null)
                adj = new Gtk.Adjustment(0, 0, 0, 0, 0, 0);

            if (_adjustment != adj)
            {
                if (_adjustment != null)
                {
                    _adjustment.changed.disconnect(adjustment_changed);
                    _adjustment.value_changed.disconnect(adjustment_value_changed);
                }

                _adjustment = adj;

                _adjustment.changed.connect(adjustment_changed);
                _adjustment.value_changed.connect(adjustment_value_changed);

                adjustment_changed();
            }
        }

        public void adjustment_changed()
        {
            iec_lower = iec_scale((float) _adjustment.lower);
            iec_upper = iec_scale((float) _adjustment.upper);

            queue_draw();
        }

        public void adjustment_value_changed()
        {
            if (_peak < _adjustment.value)
                _peak = (float) _adjustment.value;

            queue_draw();
        }

        public void _set_warn_point(float pt)
        {
            _warn_point = pt;

            if (_inverted)
                amber_frac = 1.0f - (iec_scale(_warn_point) - iec_lower) / (iec_upper - iec_lower);
            else
               amber_frac = (iec_scale(_warn_point) - iec_lower) / (iec_upper - iec_lower);
        }

        public void reset_peak()
        {
            _peak = (float) _adjustment.lower;

            queue_draw();
        }

        private float iec_scale(float db)
        {
            float def = 0.0f;       /* Meter deflection %age */

            if (db < -70.0f) {
                def = 0.0f;
            } else if (db < -60.0f) {
                def = (db + 70.0f) * 0.25f;
            } else if (db < -50.0f) {
                def = (db + 60.0f) * 0.5f + 2.5f;
            } else if (db < -40.0f) {
                def = (db + 50.0f) * 0.75f + 7.5f;
            } else if (db < -30.0f) {
                def = (db + 40.0f) * 1.5f + 15.0f;
            } else if (db < -20.0f) {
                def = (db + 30.0f) * 2.0f + 30.0f;
            } else {
                def = (db + 20.0f) * 2.5f + 50.0f;
            }

            return def;
        }
    }
}
