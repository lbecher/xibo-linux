#include "MainWindowController.hpp"

#include <gtkmm/filechooserdialog.h>
#include <gtkmm/messagedialog.h>

#include "cms/xmds/XmdsRequestSender.hpp"
#include "common/fs/FileSystem.hpp"
#include "common/logger/Logging.hpp"
#include "common/system/System.hpp"
#include "config/AppConfig.hpp"

MainWindowController::MainWindowController(Gtk::Window* window, const Glib::RefPtr<Gtk::Builder>& ui) :
    ui_(ui),
    mainWindow_(window)
{
    cmsSettings_.fromFile(AppConfig::cmsSettingsPath());
    playerSettings_.fromFile(AppConfig::playerSettingsPath());

    initUi();
    updateControls(cmsSettings_);
    connectSignals();
}

void MainWindowController::initUi()
{
    cmsAddressField_ = ui_->get_widget<Gtk::Entry>(Resources::Ui::CmsAddressEntry);
    keyField_ = ui_->get_widget<Gtk::Entry>(Resources::Ui::KeyEntry);
    resourcesPathField_ = ui_->get_widget<Gtk::Entry>(Resources::Ui::ResourcesPathEntry);
    browseResourcesPath_ = ui_->get_widget<Gtk::Button>(Resources::Ui::BrowseResourcesButton);

    usernameField_ = ui_->get_widget<Gtk::Entry>(Resources::Ui::UsernameEntry);
    passwordField_ = ui_->get_widget<Gtk::Entry>(Resources::Ui::PasswordEntry);
    splashScreenPath_ = ui_->get_widget<Gtk::Entry>(Resources::Ui::SplashScreenPathEntry);
    browseSplashScreenPath_ = ui_->get_widget<Gtk::Button>(Resources::Ui::BrowseSplashScreenButton);
    domainField_ = ui_->get_widget<Gtk::Entry>(Resources::Ui::DomainEntry);
    displayIdField_ = ui_->get_widget<Gtk::Entry>(Resources::Ui::DisplayIdEntry);

    connectionStatus_ = ui_->get_widget<Gtk::Label>(Resources::Ui::ConnectionStatusLabel);
    saveSettings_ = ui_->get_widget<Gtk::Button>(Resources::Ui::SaveButton);
    exit_ = ui_->get_widget<Gtk::Button>(Resources::Ui::ExitButton);
}

void MainWindowController::updateControls(const CmsSettings& settings)
{
    cmsAddressField_->set_text(Glib::ustring{settings.address()});
    keyField_->set_text(Glib::ustring{settings.key()});
    FilePath path = settings.resourcesPath();
    resourcesPathField_->set_text(Glib::ustring{path.string()});
    usernameField_->set_text(Glib::ustring{settings.username()});
    passwordField_->set_text(Glib::ustring{settings.password()});
    domainField_->set_text(Glib::ustring{settings.domain()});
    displayIdField_->set_text(Glib::ustring{settings.displayId()});
}

void MainWindowController::connectSignals()
{
    saveSettings_->signal_clicked().connect(std::bind(&MainWindowController::onSaveSettingsClicked, this));
    browseResourcesPath_->signal_clicked().connect(
        std::bind(&MainWindowController::onBrowseResourcesPathClicked, this));
    exit_->signal_clicked().connect(std::bind(&Gtk::Window::close, mainWindow_));
}

void MainWindowController::onSaveSettingsClicked()
{
    auto displayId = getDisplayId();
    auto connectionResult = connectToCms(cmsAddressField_->get_text(), keyField_->get_text(), displayId);

    connectionStatus_->set_text(connectionResult);
    displayIdField_->set_text(displayId);

    updateSettings();
}

std::string MainWindowController::getDisplayId()
{
    std::string displayId = displayIdField_->get_text();
    auto keyHash = static_cast<Md5Hash>(System::hardwareKey());

    return displayId.empty() ? static_cast<std::string>(keyHash) : displayId;
}

std::string MainWindowController::connectToCms(const std::string& cmsAddress,
                                               const std::string& key,
                                               const std::string& displayId)
{
    try
    {
        XmdsRequestSender xmdsRequester{cmsAddress, key, displayId};

                
        auto connectionResult =
            xmdsRequester
                .registerDisplay(std::stoi(AppConfig::codeVersion()), AppConfig::releaseVersion(), playerSettings_.displayName())
                .then([](auto future) {
                    auto [error, result] = future.get();

                    if (!error)
                        return result.status.message;
                    else
                        return error.message();
                });

        return connectionResult.get();
    }
    catch (std::exception& e)
    {
        return e.what();
    }
}

void MainWindowController::updateSettings()
{
    cmsSettings_.address().setValue(cmsAddressField_->get_text());
    cmsSettings_.key().setValue(keyField_->get_text());
    std::string path = resourcesPathField_->get_text();
    cmsSettings_.resourcesPath().setValue(path.empty() ? createDefaultResourceDir() : path);
    cmsSettings_.displayId().setValue(displayIdField_->get_text());

    cmsSettings_.updateProxy(domainField_->get_text(), usernameField_->get_text(), passwordField_->get_text());

    cmsSettings_.saveTo(AppConfig::cmsSettingsPath());
}

std::string MainWindowController::createDefaultResourceDir()
{
    auto defaultDir = AppConfig::configDirectory() / "resources";
    if (!FileSystem::exists(defaultDir))
    {
        bool result = FileSystem::createDirectory(defaultDir);
        if (!result)
        {
            Log::error("Unable to create resources directory");
        }
    }
    return defaultDir.string();
}

void MainWindowController::onBrowseResourcesPathClicked()
{
    auto* dialog =
        new Gtk::FileChooserDialog{*mainWindow_, Resources::ChooseResourcesFolder, Gtk::FileChooser::Action::SELECT_FOLDER};

    dialog->add_button("Cancel", static_cast<int>(Gtk::ResponseType::CANCEL));
    dialog->add_button("Select", static_cast<int>(Gtk::ResponseType::OK));

    dialog->signal_response().connect([this, dialog](int response) {
        if (response == static_cast<int>(Gtk::ResponseType::OK))
        {
            if (auto file = dialog->get_file())
            {
                resourcesPathField_->set_text(file->get_path());
            }
        }

        dialog->hide();
        delete dialog;
    });
    dialog->present();
}
