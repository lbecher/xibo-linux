#include "OutputWindowGtk.hpp"

OutputWindowGtk::OutputWindowGtk(Gtk::Widget* handler) : WidgetGtk(*handler), handler_(handler) {}

OutputWindowGtk::OutputWindowGtk(std::unique_ptr<Gtk::Widget>&& handler) :
    WidgetGtk(*handler),
    ownedHandler_(std::move(handler)),
    handler_(ownedHandler_.get())
{
}

Gtk::Widget& OutputWindowGtk::handler()
{
    return *handler_;
}
