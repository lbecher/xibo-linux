#include <gtkmm/application.h>
#include <glib.h>
#include <spdlog/sinks/stdout_sinks.h>

#include "MainWindowController.hpp"

#include "common/logger/Logging.hpp"
#include "config/AppConfig.hpp"

int main(int argc, char** argv)
{
    g_setenv("GDK_BACKEND", "wayland,x11", false);

    auto app = Gtk::Application::create("org.gtkmm.xibo.options");
    auto ui = Gtk::Builder::create_from_file(AppConfig::uiFile().string());

    std::vector<spdlog::sink_ptr> sinks{std::make_shared<spdlog::sinks::stdout_sink_mt>()};
    Log::create(sinks);

    auto* mainWindow = ui->get_widget<Gtk::Window>(Resources::Ui::MainWindow);

    MainWindowController controller{mainWindow, ui};

    app->signal_activate().connect([app, mainWindow]() {
        app->add_window(*mainWindow);
        mainWindow->present();
    });

    return app->run(argc, argv);
}
