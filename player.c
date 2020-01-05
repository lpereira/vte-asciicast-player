/*
 * Stupid simple asciicast player
 * Copyright (c) 2020 Leandro Pereira
 *
 * Released under the terms of GNU GPL version 3.
 */

#include <gtk/gtk.h>
#include <vte/vte.h>
#include <json-glib/json-glib.h>

struct command {
    GtkWidget *term;
    gchar *text;
};

static gpointer bundle_command(GtkWidget *term, const gchar *text)
{
    struct command *c = g_malloc(sizeof(*c));

    c->term = term;
    c->text = g_strdup(text);

    return c;
}

static gboolean send_command_to_vte(gpointer data)
{
    struct command *c = data;

    vte_terminal_feed(VTE_TERMINAL(c->term), c->text, -1);

    g_free(c->text);
    g_free(c);

    return FALSE;
}

int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *term;
    JsonParser *parser;
    gchar *contents;
    gsize contents_len;
    gint line = 0;

    gtk_init(&argc, &argv);

    if (argc < 2) {
        g_error("Usage: %s /path/to/asciicast/file", argv[0]);
        return 1;
    }

    if (!g_file_get_contents(argv[1], &contents, &contents_len, NULL)) {
        g_error("Could not read %s", argv[1]);
        return 1;
    }

    parser = json_parser_new();

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "asciinema player");

    term = vte_terminal_new();
    gtk_container_add(GTK_CONTAINER(window), term);

    for (gchar *p = g_utf8_strchr(contents, contents_len, '\n'); p && contents_len;
                p = g_utf8_strchr(contents, contents_len, '\n')) {
        JsonNode *root;

        *p = '\0';

        if (!json_parser_load_from_data(parser, contents, p - contents, NULL)) {
            g_error("Could not parse JSON line");
            return 1;
        }

        root = json_parser_get_root(parser);

        if (line == 0) {
            JsonObject *header;

            if (json_node_get_node_type(root) != JSON_NODE_OBJECT) {
                g_error("First line isn't an object");
                return 1;
            }

            header = json_node_get_object(root); 
            if (json_object_get_int_member(header, "version") != 2) {
                g_error("Unsupported version");
                return 1;
            }

            glong terminal_width = 80;
            glong terminal_height = 24;

            if (json_object_has_member(header, "width"))
                terminal_width = (glong)json_object_get_int_member(header, "width");
            if (json_object_has_member(header, "height"))
                terminal_height = (glong)json_object_get_int_member(header, "height");

            vte_terminal_set_size(VTE_TERMINAL(term), terminal_width, terminal_height);
        } else {
            JsonArray *command;

            if (json_node_get_node_type(root) != JSON_NODE_ARRAY) {
                g_error("Line %d isn't an array", line);
                return 1;
            }

            command = json_node_get_array(root);
            if (json_array_get_length(command) != 3) {
                g_error("Expecting an array of 3 elements in line %d", line);
                return 1;
            }
            if (!g_str_equal(json_array_get_string_element(command, 1), "o")) {
                g_error("Only `o' commands are supported");
                return 1;
            }

            gdouble timestamp = json_array_get_double_element(command, 0);
            const gchar *arg = json_array_get_string_element(command, 2);

            g_timeout_add(timestamp * 1000.0, send_command_to_vte, bundle_command(term, arg));
        }

        contents += p - contents + 1;
        contents_len -= p - contents + 1;
        line++;
    }

    g_object_unref(parser);

    gtk_widget_show_all(window);

    gtk_main();
}
