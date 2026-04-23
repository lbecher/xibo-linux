#pragma once

#include "control/widgets/FixedContainer.hpp"
#include "control/widgets/gtk/WidgetGtk.hpp"

#include <gtkmm/fixed.h>

template <typename Interface>
class BaseFixedContainerGtk : public FixedContainer<WidgetGtk<Interface>>
{
public:
    BaseFixedContainerGtk() : FixedContainer<WidgetGtk<Interface>>{handler_}
    {
    }

    Gtk::Fixed& handler() override
    {
        return handler_;
    }

private:
    void addToHandler(const std::shared_ptr<Xibo::Widget>& child, int left, int top, int /*zorder*/) override
    {
        handler_.put(this->handlerFor(child), left, top);
    }

    void removeFromHandler(const std::shared_ptr<Xibo::Widget>& child) override
    {
        handler_.remove(this->handlerFor(child));
    }

    void reorderInHandler(const std::shared_ptr<Xibo::Widget>& child, int zorder) override
    {
        auto& childHandler = this->handlerFor(child);

        if (zorder == 0)
        {
            if (this->children().size() > 1)
            {
                childHandler.insert_before(handler_, this->handlerFor(this->children()[1].widget));
            }
            return;
        }

        childHandler.insert_after(handler_, this->handlerFor(this->children()[zorder - 1].widget));
    }

private:
    Gtk::Fixed handler_;
};

using FixedContainerGtk = BaseFixedContainerGtk<Xibo::FixedContainer>;
