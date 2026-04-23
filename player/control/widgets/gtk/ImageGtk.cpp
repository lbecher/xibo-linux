#include "control/widgets/gtk/ImageGtk.hpp"

#include "common/fs/FilePath.hpp"
#include "common/fs/FileSystem.hpp"
#include "common/logger/Logging.hpp"
#include "common/types/Uri.hpp"

ImageGtk::ImageGtk() : WidgetGtk(handler_)
{
    handler_.set_can_shrink(false);
    handler_.set_content_fit(Gtk::ContentFit::FILL);
    handler_.set_halign(Gtk::Align::FILL);
    handler_.set_valign(Gtk::Align::FILL);

    set(Gdk::Pixbuf::create(Gdk::Colorspace::RGB, DefaultUseAlpha, BitsPerSample, DefaultWidth, DefaultHegiht));
}

int ImageGtk::width() const
{
    assert(pixbuf());
    return pixbuf()->get_width();
}

int ImageGtk::height() const
{
    assert(pixbuf());
    return pixbuf()->get_height();
}

void ImageGtk::setSize(int width, int height)
{
    check(width, height);
    Log::debug("[ImageGtk] setSize requested {}x{}, source pixbuf {}x{}",
               width,
               height,
               pixbuf()->get_width(),
               pixbuf()->get_height());
    set(pixbuf()->scale_simple(width, height, Gdk::InterpType::BILINEAR));
}

void ImageGtk::fillColor(const Color& color)
{
    assert(pixbuf());
    pixbuf()->fill(color.hex());
}

void ImageGtk::loadFrom(const Uri& uri, PreserveRatio preserveRatio)
{
    try
    {
        Log::debug("[ImageGtk] Loading '{}' with target {}x{} preserveRatio={}",
                   uri.path(),
                   width(),
                   height(),
                   static_cast<bool>(preserveRatio));
        set(Gdk::Pixbuf::create_from_file(uri.path(), width(), height(), static_cast<bool>(preserveRatio)));
    }
    catch (Glib::Error& e)
    {
        throw Error{"ImageGtk", static_cast<std::string>(e.what())};
    }
}

Gtk::Picture& ImageGtk::handler()
{
    return handler_;
}

Glib::RefPtr<const Gdk::Pixbuf> ImageGtk::pixbuf() const
{
    return pixbuf_;
}

Glib::RefPtr<Gdk::Pixbuf> ImageGtk::pixbuf()
{
    return pixbuf_;
}

void ImageGtk::check(int width, int height)
{
    if (width <= 0 || height <= 0) throw Error{"ImageGtk", "Size should be positive"};
}

void ImageGtk::set(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf)
{
    if (!pixbuf) throw Error{"ImageGtk", "Not enough memory to allocate image"};
    pixbuf_ = pixbuf;
    handler_.set_pixbuf(pixbuf_);
    handler_.set_size_request(pixbuf_->get_width(), pixbuf_->get_height());
    Log::debug("[ImageGtk] Applied pixbuf {}x{} to Gtk::Picture", pixbuf_->get_width(), pixbuf_->get_height());
}
