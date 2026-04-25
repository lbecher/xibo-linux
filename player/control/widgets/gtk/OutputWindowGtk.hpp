#pragma once

#include "control/widgets/OutputWindow.hpp"
#include "control/widgets/gtk/WidgetGtk.hpp"

#include <memory>

class OutputWindowGtk : public WidgetGtk<Xibo::OutputWindow>
{
public:
    OutputWindowGtk(Gtk::Widget* handler);
    explicit OutputWindowGtk(std::unique_ptr<Gtk::Widget>&& handler);

    Gtk::Widget& handler() override;

private:
    std::unique_ptr<Gtk::Widget> ownedHandler_;
    Gtk::Widget* handler_ = nullptr;
};
