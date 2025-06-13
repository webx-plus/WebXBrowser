#include <gtk/gtk.h>

#include <webkit2/webkit2.h>

#include <string.h>

typedef struct {
  GtkWidget * entry;
  WebKitWebView * view;
  GtkWidget * tab_label;
}
TabData;

static gchar * get_seg(const gchar * uri) {
  if (!uri) return g_strdup("");
  const gchar * p = g_strstr_len(uri, -1, "url=");
  if (!p) return g_strdup("");
  p += 4;
  const gchar * end = strchr(p, '&');
  size_t len = end ? (size_t)(end - p) : strlen(p);
  return g_strndup(p, len);
}

static void set_tab(TabData * tab,
  const gchar * uri) {
  gchar * seg = get_seg(uri);
  if (seg && * seg) {
    const gchar * name = seg;
    if (g_str_has_prefix(seg, "buss://"))
      name = seg + strlen("buss://");
    gtk_label_set_text(GTK_LABEL(tab -> tab_label), name);
  } else {
    gtk_label_set_text(GTK_LABEL(tab -> tab_label), "New Tab");
  }
  g_free(seg);
}

static void nav(GtkEntry * entry, gpointer user_data) {
  TabData * tab = (TabData * ) user_data;
  const gchar * url = gtk_entry_get_text(entry);
  if (!url || ! * url) return;

  const gchar * prefix = "buss://";
  gchar * raw = g_str_has_prefix(url, prefix) ? g_strdup(url + strlen(prefix)) : g_strdup(url);
  gchar * enc = g_uri_escape_string(raw, NULL, TRUE);
  gchar * full = g_strdup_printf("https://inventionpro.github.io/Webx-viewer/embed?url=%s&bussinga=true", enc);

  webkit_web_view_load_uri(tab -> view, full);

  gchar * disp = g_strdup_printf("buss://%s", raw);
  gtk_entry_set_text(entry, disp);
  set_tab(tab, full);

  g_free(raw);
  g_free(enc);
  g_free(full);
  g_free(disp);
}

static void handle_load(WebKitWebView * view, WebKitLoadEvent evt, gpointer user_data) {
  if (evt != WEBKIT_LOAD_FINISHED) return;

  TabData * tab = (TabData * ) user_data;
  GtkEntry * entry = GTK_ENTRY(tab -> entry);
  if (!GTK_IS_ENTRY(entry) || gtk_widget_has_focus(GTK_WIDGET(entry))) return;

  const gchar * uri = webkit_web_view_get_uri(view);
  gchar * seg = get_seg(uri);
  if (seg && * seg) {
    gchar * disp = g_strdup_printf("buss://%s", seg);
    gtk_entry_set_text(entry, disp);
    set_tab(tab, uri);
    g_free(disp);
  } else {
    gtk_entry_set_text(entry, "");
    gtk_label_set_text(GTK_LABEL(tab -> tab_label), "New Tab");
  }
  g_free(seg);
}

static void go_back(GtkButton * btn, gpointer data) {
  WebKitWebView * view = WEBKIT_WEB_VIEW(data);
  if (webkit_web_view_can_go_back(view)) webkit_web_view_go_back(view);
}

static void go_fwd(GtkButton * btn, gpointer data) {
  WebKitWebView * view = WEBKIT_WEB_VIEW(data);
  if (webkit_web_view_can_go_forward(view)) webkit_web_view_go_forward(view);
}

static void close_tab(GtkButton * btn, gpointer notebook_ptr) {
  GtkNotebook * notebook = GTK_NOTEBOOK(notebook_ptr);
  GtkWidget * tab_content = g_object_get_data(G_OBJECT(btn), "page");
  int page = gtk_notebook_page_num(notebook, tab_content);
  if (page != -1) gtk_notebook_remove_page(notebook, page);
}

static void new_tab(GtkButton * btn, gpointer user_data);

static GtkWidget * create_tab(GtkNotebook * notebook) {
  GtkWidget * vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget * hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  GtkWidget * entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter URL");
  GtkWidget * btn_back = gtk_button_new_with_label("⟵");
  GtkWidget * btn_fwd = gtk_button_new_with_label("⟶");

  GtkWidget * btn_new_tab = gtk_button_new_with_label("+");
  gtk_widget_set_tooltip_text(btn_new_tab, "New Tab");
  gtk_button_set_relief(GTK_BUTTON(btn_new_tab), GTK_RELIEF_NONE);
  gtk_widget_set_size_request(btn_new_tab, 20, 20);
  g_signal_connect(btn_new_tab, "clicked", G_CALLBACK(new_tab), notebook);

  gtk_box_pack_start(GTK_BOX(hbox), btn_back, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), btn_fwd, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), btn_new_tab, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  GtkWidget * webview = webkit_web_view_new();
  gtk_box_pack_start(GTK_BOX(vbox), webview, TRUE, TRUE, 0);

  GtkWidget * tab_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  GtkWidget * label = gtk_label_new("New Tab");
  GtkWidget * close_btn = gtk_button_new_with_label("✕");
  gtk_button_set_relief(GTK_BUTTON(close_btn), GTK_RELIEF_NONE);
  gtk_widget_set_name(close_btn, "tab-close-button");

  gtk_box_pack_start(GTK_BOX(tab_hbox), label, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(tab_hbox), close_btn, FALSE, FALSE, 0);
  gtk_widget_show_all(tab_hbox);

  g_object_set_data(G_OBJECT(close_btn), "page", vbox);
  g_signal_connect(close_btn, "clicked", G_CALLBACK(close_tab), notebook);

  int page = gtk_notebook_append_page(notebook, vbox, tab_hbox);
  gtk_notebook_set_tab_reorderable(notebook, vbox, TRUE);
  gtk_notebook_set_tab_detachable(notebook, vbox, TRUE);
  gtk_notebook_set_current_page(notebook, page);

  TabData * tab = g_new(TabData, 1);
  tab -> entry = entry;
  tab -> view = WEBKIT_WEB_VIEW(webview);
  tab -> tab_label = label;

  g_signal_connect(entry, "activate", G_CALLBACK(nav), tab);
  g_signal_connect(webview, "load-changed", G_CALLBACK(handle_load), tab);
  g_signal_connect(btn_back, "clicked", G_CALLBACK(go_back), tab -> view);
  g_signal_connect(btn_fwd, "clicked", G_CALLBACK(go_fwd), tab -> view);

  webkit_web_view_load_uri(tab -> view,
    "https://inventionpro.github.io/Webx-viewer/embed?url=about.frontdoor&bussinga=true");

  gtk_widget_show_all(vbox);
  return vbox;
}

static void new_tab(GtkButton * btn, gpointer notebook_ptr) {
  GtkNotebook * notebook = GTK_NOTEBOOK(notebook_ptr);
  GtkWidget * tab = create_tab(notebook);
  int page = gtk_notebook_page_num(notebook, tab);
  gtk_notebook_set_current_page(notebook, page);
}

int main(int argc, char * argv[]) {
  gtk_init( & argc, & argv);

  GtkWidget * win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(win), 1000, 700);
  gtk_window_set_title(GTK_WINDOW(win), "WebXBrowser");

  GtkWidget * notebook = gtk_notebook_new();
  gtk_container_add(GTK_CONTAINER(win), notebook);
  create_tab(GTK_NOTEBOOK(notebook));

  g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);
  gtk_widget_show_all(win);
  gtk_main();
  return 0;
}
