#!/bin/env python3


from gi.repository import Gtk

from gtkmeter import GtkMeter


def chkbtn_toggled(chkbtn, data):
    data.set_inverted(chkbtn.get_active())

def orientation_changed(cmbbox, data):
    model = cmbbox.get_model()
    it = cmbbox.get_active_iter()

    data.set_orientation(*model.get(it, 1))


win = Gtk.Window.new(Gtk.WindowType.TOPLEVEL)
#win.set_size_request(300, 300)

adj = Gtk.Adjustment(0, -60, 6, 0.1, 0, 0)
meter = GtkMeter(adj, Gtk.Orientation.VERTICAL, -60.0, 6.0)
meter.set_warn_point(-5.0)

scale = Gtk.Scale(orientation=Gtk.Orientation.VERTICAL, adjustment=adj)

box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=0)
box.pack_start(meter, True, True, 0)
box.pack_start(scale, False, True, 0)

# Settings box #
box2 = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
box.pack_start(box2, False, True, 0)

## Inverted ##
chkbtn = Gtk.CheckButton.new_with_label("Inverted")
chkbtn.connect("toggled", chkbtn_toggled, meter)

## Orientation ##
orils = Gtk.ListStore(str, Gtk.Orientation)
it = orils.append()
orils.set(it,
          0, "Vertical",
          1, Gtk.Orientation.VERTICAL)
it = orils.append()
orils.set(it,
          0, "Horizontal",
          1, Gtk.Orientation.HORIZONTAL)
cmbbox = Gtk.ComboBox.new_with_model_and_entry (orils)
cmbbox.set_entry_text_column (0)
cmbbox.set_active(0)
cmbbox.connect("changed", orientation_changed, meter)

box2.pack_start(chkbtn, False, False, 0)
box2.pack_start(cmbbox, False, False, 0)

win.add(box)
win.show_all()
win.connect("delete-event", Gtk.main_quit)

Gtk.main()