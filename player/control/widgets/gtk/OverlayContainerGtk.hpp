#include "control/widgets/OverlayContainer.hpp"
#include "control/widgets/gtk/FixedContainerGtk.hpp"

class OverlayContainerGtk : public OverlayContainer<BaseFixedContainerGtk<Xibo::OverlayContainer>>
{
protected:
    void setMainChildImpl(const std::shared_ptr<Xibo::Widget>& mainChild) override
    {
        auto& childHandler = handlerFor(mainChild);
        handler().put(childHandler, 0, 0);
        childHandler.set_valign(Gtk::Align::CENTER);
        childHandler.set_halign(Gtk::Align::CENTER);

        // Keep the main child as background and overlays (regions container) above it.
        if (this->children().size() > 1)
        {
            childHandler.insert_before(handler(), handlerFor(this->children().front().widget));
        }
    }

    void removeMainChildImpl(const std::shared_ptr<Xibo::Widget>& mainChild) override
    {
        handler().remove(handlerFor(mainChild));
    }
};
