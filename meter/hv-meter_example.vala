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


int main(string[] args)
{
    Gtk.init(ref args);

    var window = new Gtk.Window(Gtk.WindowType.TOPLEVEL);

    var hbox = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 10);
    window.add(hbox);

    var adj = new Gtk.Adjustment(0, -60, 6, 0.1, 0, 0);

    var meter = new HV.Meter(Gtk.Orientation.VERTICAL, adj);
    hbox.pack_start(meter, true, true, 0);

    var scale = new Gtk.Scale(Gtk.Orientation.VERTICAL, adj);
    scale.inverted = true;
    hbox.pack_start(scale, false, true, 0);

    var vbox = new Gtk.Box(Gtk.Orientation.VERTICAL, 10);
    hbox.pack_start(vbox, false, true, 0);

    var ls = new Gtk.ListStore(2, typeof(string), typeof(Gtk.Orientation));
    Gtk.TreeIter it;

    ls.append(out it);
    ls.set(it, 0, "Vertical", 1, Gtk.Orientation.VERTICAL);
    ls.append(out it);
    ls.set(it, 0, "Horizontal", 1, Gtk.Orientation.HORIZONTAL); 

    var cmb = new Gtk.ComboBox.with_model(ls);
    cmb.changed.connect(() => {
        Gtk.TreeIter iter;
        Gtk.Orientation ori;

        cmb.get_active_iter(out iter);
        ls.get(iter, 1, out ori);

        meter.set_orientation(ori);
    });
    cmb.active = 0;
    vbox.pack_start(cmb, false, true, 0);

    var renderer = new Gtk.CellRendererText();
    cmb.pack_start(renderer, true);
    cmb.add_attribute(renderer, "text", 0); 

    var chk = new Gtk.CheckButton.with_label("Inverted");
    chk.toggled.connect(() => {
        scale.inverted = !chk.active;
        meter.inverted = chk.active;
    });
    vbox.pack_start(chk, false, true, 0);

    var lower = new Gtk.Entry();
    adj.bind_property("lower", lower, "text", BindingFlags.SYNC_CREATE | BindingFlags.BIDIRECTIONAL, (binding, srcval, ref targetval) => {
        targetval.set_string(srcval.get_double().to_string());

        return true;
    }, (binding, srcval, ref targetval) => {
        targetval.set_double(double.parse(srcval.get_string()));

        return true;
    });

    var upper = new Gtk.Entry();
    adj.bind_property("upper", upper, "text", BindingFlags.SYNC_CREATE | BindingFlags.BIDIRECTIONAL, (binding, srcval, ref targetval) => {
        targetval.set_string(srcval.get_double().to_string());

        return true;
    }, (binding, srcval, ref targetval) => {
        targetval.set_double(double.parse(srcval.get_string()));

        return true;
    });

    var hbox_range = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 10);
    hbox_range.pack_start(lower, false, false, 0);
    hbox_range.pack_start(upper, false, false, 0);
    vbox.pack_start(hbox_range, false, true, 0);

    var wp = new Gtk.Entry();
    meter.bind_property("warn-point", wp, "text", BindingFlags.SYNC_CREATE | BindingFlags.BIDIRECTIONAL, (binding, srcval, ref targetval) => {
        targetval.set_string(srcval.get_float().to_string());

        return true;
    }, (binding, srcval, ref targetval) => {
        targetval.set_float((float) double.parse(srcval.get_string()));

        return true;
    });

    var pulse = new Gtk.ToggleButton.with_label("Pulse");
    pulse.toggled.connect(() => {
        if (pulse.active)
            GLib.Timeout.add(100, () => {
                var rand = new Rand();

                adj.value = rand.int_range((int) adj.lower, (int) adj.upper);

                if (!pulse.active)
                    return false;
                return true;
            });
    });

    var hbox_p_wp = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 10);
    hbox_p_wp.pack_start(wp, true, true, 0);
    hbox_p_wp.pack_start(pulse, true, true, 0);
    vbox.pack_start(hbox_p_wp, false, true, 0);

    window.destroy.connect(Gtk.main_quit);

    window.show_all();

    Gtk.main();

    return 0;
}
