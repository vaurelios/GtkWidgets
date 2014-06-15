#include <gtk/gtk.h>

#include "gtkmeter.h"

/* Forward declarations */
void cmbbox_changed         (GtkWidget *cmbbox,
                             gpointer  *meter);
void chkbnt_toggled         (GtkToggleButton *chkbtn,
                             gpointer         meter);

enum {
  COL_TEXT,
  COL_ORIENTATION,

  N_COLS
};

int
main(int argc, char **argv)
{
  GtkWindow       *window  = NULL;
  GtkWidget       *box     = NULL;
  GtkWidget       *box2    = NULL;
  GtkWidget       *scale   = NULL;
  GtkWidget       *meter   = NULL;
  GtkWidget       *chkbtn  = NULL;
  GtkWidget       *cmbbox  = NULL;
  GtkAdjustment   *adj     = NULL;
  GtkListStore    *orils   = NULL;
  GtkTreeIter      iter;
  
  gtk_init (&argc, &argv);

  adj = gtk_adjustment_new (0, -60, 6, 1, -60.0f, 0.0f);

  meter = gtk_meter_new (GTK_ORIENTATION_VERTICAL, adj);
  gtk_meter_set_warn_point (GTK_METER (meter), -5.0f);

  scale = gtk_scale_new (GTK_ORIENTATION_VERTICAL, adj);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 5);
  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (meter), TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), scale, FALSE, TRUE, 0);

  /* settings box */
  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 10);
  gtk_box_pack_start (GTK_BOX (box), box2, FALSE, FALSE, 0);

  // Inverted
  chkbtn = gtk_check_button_new_with_label ("Inverted");
  g_signal_connect (chkbtn, "toggled", G_CALLBACK (chkbnt_toggled), meter);

  // orientation
  orils = gtk_list_store_new (N_COLS, G_TYPE_STRING, GTK_TYPE_ORIENTATION);
  gtk_list_store_append (orils, &iter);
  gtk_list_store_set (orils, &iter,
                      COL_TEXT, "Vertical",
                      COL_ORIENTATION, GTK_ORIENTATION_VERTICAL,
                      -1);
  gtk_list_store_append (orils, &iter);
  gtk_list_store_set (orils, &iter,
                      COL_TEXT, "Horizontal",
                      COL_ORIENTATION, GTK_ORIENTATION_HORIZONTAL,
                      -1);
  cmbbox = gtk_combo_box_new_with_model_and_entry (GTK_TREE_MODEL (orils));
  gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (cmbbox), COL_TEXT);
  gtk_combo_box_set_active (GTK_COMBO_BOX (cmbbox), 0);
  g_signal_connect (cmbbox, "changed", G_CALLBACK (cmbbox_changed), meter);

  gtk_box_pack_start(GTK_BOX (box2), chkbtn, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX (box2), cmbbox, FALSE, FALSE, 0);

  window = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  gtk_container_add (GTK_CONTAINER (window), box);
  gtk_window_set_default_size (window, 300, 300);
  g_signal_connect (window, "delete-event", gtk_main_quit, NULL);
  gtk_widget_show_all (GTK_WIDGET (window));

  gtk_main ();
}

void
cmbbox_changed (GtkWidget *cmbbox,
                gpointer  *meter)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GtkOrientation orientation;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (cmbbox));
  gtk_combo_box_get_active_iter (GTK_COMBO_BOX (cmbbox), &iter);

  gtk_tree_model_get (model, &iter,
                      COL_ORIENTATION, &orientation,
                      -1);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (meter), orientation);
}

void
chkbnt_toggled (GtkToggleButton *chkbtn,
                gpointer         meter)
{
  gtk_meter_set_inverted (GTK_METER (meter), gtk_toggle_button_get_active (chkbtn));
}