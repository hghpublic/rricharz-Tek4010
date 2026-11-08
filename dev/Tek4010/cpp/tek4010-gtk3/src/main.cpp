// Keep the executable entry point thin; the GTK host owns startup and shutdown.
import gtk_app;

int main(int argc, char* argv[])
{
    return run_gtk_app(argc, argv);
}
