/* Copyright 2010 Robert Spanton
   This file is part of sr-ts.
   sr-ts is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   sr-ts is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with sr-ts.  If not, see <http://www.gnu.org/licenses/>. */
#include <gtk/gtk.h>
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>
#include <stdint.h>

enum {
	WINDOW_LIST_NAME_COLUMN,
	WINDOW_LIST_WNCK_WINDOW,
	WINDOW_LIST_N_COLUMNS,
};

typedef struct {
	GtkTreeView *treeview;
	GtkListStore *store;
	GtkTreeSelection *sel;
} window_list_t;

typedef struct {
	window_list_t winlist;

	GtkWindow *mainwin;

	WnckScreen *screen;
} ts_t;

static void cb_winlist_sel_changed( GtkTreeSelection *treesel, gpointer _ts );

/* Called when a window is opened (and for all windows on startup) */
static void cb_wnck_window_opened( WnckScreen *screen, WnckWindow *window,
				   gpointer _ts );

/* Called when a window is closed */
static void cb_wnck_window_closed( WnckScreen *screen, WnckWindow *window,
				   gpointer _ts );

static void build_treeview( ts_t *ts, GtkBuilder *builder )
{
	window_list_t *wl = &ts->winlist;
	GtkCellRenderer *rend;
	GtkTreeViewColumn *col;

	wl->treeview = GTK_TREE_VIEW( gtk_builder_get_object( builder, "treeview_windows" ) );
	wl->store = gtk_list_store_new( WINDOW_LIST_N_COLUMNS,
					G_TYPE_STRING,
					G_TYPE_POINTER );
	gtk_tree_view_set_model( wl->treeview,
				 GTK_TREE_MODEL( wl->store ) );

	/* Sort out the rendering */
	rend = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes( "Window", rend,
							"text", WINDOW_LIST_NAME_COLUMN,
							NULL );
	gtk_tree_view_append_column( wl->treeview, col );

	wl->sel = gtk_tree_view_get_selection( wl->treeview );
	gtk_tree_selection_set_mode( wl->sel, GTK_SELECTION_SINGLE );
	g_signal_connect( G_OBJECT(wl->sel), "changed",
			  G_CALLBACK( cb_winlist_sel_changed ), ts );
}

#define get(t, n) do {				\
	ts-> n = t(gtk_builder_get_object( builder, #n ));	\
	g_assert( ts-> n != NULL );				\
	} while (0)
static void build_interface( ts_t *ts )
{
	GtkBuilder *builder;
	builder = gtk_builder_new();
	g_assert( gtk_builder_add_from_file( builder, "sr-ts.glade", NULL ) );
	build_treeview( ts, builder );

	get( GTK_WINDOW, mainwin );

	gtk_builder_connect_signals( builder, ts );
	g_object_unref(builder);

	gtk_widget_show( GTK_WIDGET(ts->mainwin) );
}
#undef get

static void init_wnck( ts_t *ts )
{
	ts->screen = wnck_screen_get(0);
	g_assert( ts->screen != NULL );

	g_signal_connect( G_OBJECT(ts->screen), "window-opened",
			  G_CALLBACK( cb_wnck_window_opened ), ts );

	g_signal_connect( G_OBJECT(ts->screen), "window-closed",
			  G_CALLBACK( cb_wnck_window_closed ), ts );
}

int main( int argc, char **argv )
{
	ts_t ts;
	gtk_init( &argc, &argv );
	build_interface(&ts);
	init_wnck(&ts);



	gtk_main();
	return 0;
}

/* Find the list entry that maps to the given window.
   Returns FALSE if not found. */
static gboolean find_list_entry( ts_t *ts, WnckWindow *window, GtkTreeIter *i )
{
	window_list_t *wl = &ts->winlist;
	g_assert( i != NULL );

	if( !gtk_tree_model_get_iter_first( GTK_TREE_MODEL( wl->store ), i ) )
		return FALSE;

	do {
		WnckWindow *cmpwin;
		gtk_tree_model_get( GTK_TREE_MODEL(wl->store), i,
				    WINDOW_LIST_WNCK_WINDOW, &cmpwin, -1 );

		if( window == cmpwin )
			return TRUE;
	}
	while( gtk_tree_model_iter_next( GTK_TREE_MODEL(wl->store), i ) );

	return FALSE;
}

static void cb_winlist_sel_changed( GtkTreeSelection *treesel, gpointer _ts )
{
	g_warning( "window list selection changed, but nothing done..." );
}

static void cb_window_name_changed( WnckWindow *window, gpointer _ts )
{
	ts_t *ts = _ts;
	const gchar *name = wnck_window_get_name(window);
	GtkTreeIter i;

	if( !find_list_entry( ts, window, &i ) ) {
		g_warning( "Couldn't find window in list store" );
		return;
	}

	gtk_list_store_set( ts->winlist.store, &i,
			    WINDOW_LIST_NAME_COLUMN, name, -1 );
}

static void cb_wnck_window_opened( WnckScreen *screen, WnckWindow *window,
				   gpointer _ts )
{
	ts_t *ts = _ts;
	const gchar *name = wnck_window_get_name(window);
	GtkTreeIter i;

	/* Add the window to the store */
	gtk_list_store_append( ts->winlist.store, &i );
	gtk_list_store_set( ts->winlist.store, &i,
			    WINDOW_LIST_NAME_COLUMN, name,
			    WINDOW_LIST_WNCK_WINDOW, window,
			    -1 );

	g_signal_connect( G_OBJECT(window), "name-changed",
			  G_CALLBACK( cb_window_name_changed ), ts );
}

static void cb_wnck_window_closed( WnckScreen *screen, WnckWindow *window,
				   gpointer _ts )
{
	g_warning( "window closed. nothing done!" );
}

G_MODULE_EXPORT void cb_winlist_row_activated( GtkTreeView *treeview,
					       GtkTreePath *path,
					       GtkTreeViewColumn *col,
					       gpointer _ts )
{
	ts_t *ts = _ts;
	window_list_t *wl = &ts->winlist;
	GtkTreeIter i;
	WnckWindow *win;

	g_assert( gtk_tree_model_get_iter( GTK_TREE_MODEL(wl->store), &i, path ) );

	gtk_tree_model_get( GTK_TREE_MODEL(wl->store), &i,
			    WINDOW_LIST_WNCK_WINDOW, &win, -1 );

	/* TODO: Send a sane time through */
	wnck_window_activate(win, 0);
}
