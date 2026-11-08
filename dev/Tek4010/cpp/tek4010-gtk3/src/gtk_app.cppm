export module gtk_app;

// Export only the host runner; GTK types remain private to the implementation.
export int run_gtk_app(int argc, char* argv[]);
