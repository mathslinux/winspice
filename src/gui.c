#include "gui.h"
#include "session.h"

static gboolean gui_has_init = FALSE;

static void start_click(GtkButton *btn, gpointer data)
{
    Session *session = (Session *)data;
    GUI *gui = session->gui;
    /// TODO: set sensitive if and only if the server starts successfully
    session_start(session);
    gtk_widget_set_sensitive(gui->start_button, FALSE);
    gtk_label_set_text(GTK_LABEL(gui->status_label), "Waiting for client to connect ......");
}

static void disconnect_click(GtkButton *btn, gpointer data)
{
    Session *session = (Session *)data;
    GUI *gui = session->gui;
    gtk_label_set_text(GTK_LABEL(gui->status_label), "Waiting for client to connect ......");
    gtk_widget_set_sensitive(gui->disconnect_button, FALSE);
}

static void create_icon_widget(GUI *gui, Session *session)
{
    char *icon_path;
    char *app_dir;
    GtkWidget *image;

    app_dir = g_path_get_dirname(session->app_path);
    icon_path = g_build_filename(app_dir, "icon.png", NULL);

    image = gtk_image_new_from_file(icon_path);
    gtk_box_pack_start(GTK_BOX(gui->main_box), image, TRUE, FALSE, 0);

    g_free(icon_path);
    g_free(app_dir);
}

static void create_arguments_widget(GUI *gui, Session *session)
{
    char buf[128];
    GList *it;

    gui->arguments_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(gui->arguments_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(gui->arguments_grid), 10);
    gtk_box_pack_start(GTK_BOX(gui->main_box), gui->arguments_grid, TRUE, FALSE, 0);

    gtk_container_set_border_width(GTK_CONTAINER(gui->arguments_grid), 10);

    /// server port
    gui->port_label = gtk_label_new("Server Port: ");
    gtk_label_set_xalign(GTK_LABEL(gui->port_label), 1);
    gtk_grid_attach(GTK_GRID(gui->arguments_grid), gui->port_label, 0, 0, 1, 1);

    gui->port_entry = gtk_entry_new();
    snprintf(buf, sizeof(buf), "%d", session->options->port);
    gtk_entry_set_text((GtkEntry *)gui->port_entry, buf);
    gtk_grid_attach(GTK_GRID(gui->arguments_grid), gui->port_entry, 1, 0, 1, 1);

    /// password
    gui->password_label = gtk_label_new("Password: ");
    gtk_label_set_xalign(GTK_LABEL(gui->password_label), 1);
    gtk_grid_attach(GTK_GRID(gui->arguments_grid), gui->password_label, 0, 1, 1, 1);

    gui->password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(gui->password_entry), FALSE);
    gtk_grid_attach(GTK_GRID(gui->arguments_grid), gui->password_entry, 1, 1, 1, 1);

    /// image compression
    gui->compression_label = gtk_label_new("image compression: ");
    gtk_label_set_xalign(GTK_LABEL(gui->compression_label), 1);
    gtk_grid_attach(GTK_GRID(gui->arguments_grid), gui->compression_label, 0, 2, 1, 1);

    gui->compression_entry = gtk_combo_box_text_new();
    for (it = g_list_first(session->options->compression_name_list); it != NULL; it = g_list_next(it)) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(gui->compression_entry), it->data);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui->compression_entry), 0);
    gtk_grid_attach(GTK_GRID(gui->arguments_grid), gui->compression_entry, 1, 2, 1, 1);
}

static void create_start_widget(GUI *gui, Session *session)
{
    gui->button_box = gtk_button_box_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_set_spacing(GTK_BOX(gui->button_box), 10);
    gtk_box_pack_start(GTK_BOX(gui->main_box), gui->button_box, TRUE, FALSE, 0);

    /// status
    gui->status_label = gtk_label_new("Waiting for server to start ......");
    gtk_container_add(GTK_CONTAINER(gui->button_box), gui->status_label);

    /// disconnect
    gui->disconnect_button = gtk_button_new_with_label("Disconnet");
    g_signal_connect(G_OBJECT(gui->disconnect_button), "clicked", G_CALLBACK(disconnect_click), session);
    gtk_widget_set_sensitive(gui->disconnect_button, FALSE);
    gtk_container_add(GTK_CONTAINER(gui->button_box), gui->disconnect_button);

    /// start button
    gui->start_button = gtk_button_new_with_label("Start");
    g_signal_connect(G_OBJECT(gui->start_button), "clicked", G_CALLBACK(start_click), session);
    gtk_container_add(GTK_CONTAINER(gui->button_box), gui->start_button);
}

#if 0
int main(int argc, char *argv[])
{
    GUI gui;
    Options *options = options_new();

    gtk_init(&argc, &argv);

    /// top window
    gui.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(gui.window), "Window Spice Server");
    gtk_window_set_resizable(GTK_WINDOW(gui.window), FALSE);

    /// main panel
    gui.main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(gui.main_box), 10);
    gtk_container_add(GTK_CONTAINER(gui.window), gui.main_box);

    create_icon_widget(&gui, options);
    create_arguments_widget(&gui, options);
    create_start_widget(&gui, options);

    gtk_widget_show_all(gui.window);
    gtk_main();

    return 0;
}
#endif

GUI *gui_new(Session *session)
{
    GUI *gui;

    if (!gui_has_init) {
        gtk_init(NULL, NULL);
        gui_has_init = TRUE;
    }

    gui = (GUI *)g_malloc0(sizeof(GUI));
    session->gui = gui;

    /// top window
    gui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(gui->window), "Window Spice Server");
    gtk_window_set_resizable(GTK_WINDOW(gui->window), FALSE);

    /// main panel
    gui->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(gui->main_box), 10);
    gtk_container_add(GTK_CONTAINER(gui->window), gui->main_box);

    create_icon_widget(gui, session);
    create_arguments_widget(gui, session);
    create_start_widget(gui, session);

    gtk_widget_show_all(gui->window);

    return gui;
}

void gui_run(GUI *gui)
{
    gtk_main();
}

void gui_destroy(GUI *gui)
{
    /// TODO: destroy all child widget
    if (gui) {
        g_free(gui);
    }
}
