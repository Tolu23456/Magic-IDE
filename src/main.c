#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <stdio.h>
#include <stdlib.h>

static void destroy_window_cb(GtkWidget* widget, GtkWidget* window);
static gboolean close_web_view_cb(WebKitWebView* web_view, GtkWidget* window);

int main(int argc, char* argv[]) {
    GtkWidget *main_window;
    GtkWidget *web_view;

    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create main window
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "Magic IDE");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 1200, 800);
    g_signal_connect(main_window, "destroy", G_CALLBACK(destroy_window_cb), NULL);

    // Create WebKit WebView
    web_view = webkit_web_view_new();
    g_signal_connect(web_view, "close", G_CALLBACK(close_web_view_cb), main_window);

    // Load initial HTML
    webkit_web_view_load_html(WEBKIT_WEB_VIEW(web_view),
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
            "<title>Magic IDE</title>"
            "<style>"
                "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }"
                "h1 { color: #333; }"
                ".editor { width: 100%; height: 400px; border: 1px solid #ccc; }"
            "</style>"
        "</head>"
        "<body>"
            "<h1>Welcome to Magic IDE</h1>"
            "<p>A modern, lightweight code editor built with C and web technologies.</p>"
            "<textarea class='editor' placeholder='Start coding here...'></textarea>"
        "</body>"
        "</html>", NULL);

    // Add web view to window
    gtk_container_add(GTK_CONTAINER(main_window), web_view);
    gtk_widget_show_all(main_window);

    // Run GTK main loop
    gtk_main();

    return 0;
}

static void destroy_window_cb(GtkWidget* widget, GtkWidget* window) {
    gtk_main_quit();
}

static gboolean close_web_view_cb(WebKitWebView* web_view, GtkWidget* window) {
    gtk_widget_destroy(window);
    return TRUE;
}