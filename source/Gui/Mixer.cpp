#include "Gui/Mixer.hpp"
#include "Controller.hpp"

namespace Mixijo::Gui {

    Mixer::Mixer() {
        box.use = false;

        link(background);

        event<Mixer>();
        updateTheme();
    }

    void Mixer::mouseClick(const MousePress& e) {
        bool _notHovering = true;
        for (auto& _obj : objects()) {
            auto _channel = _obj.as<Channel>();
            if (_channel->get(Hovering)) _notHovering = false;
            if (_channel->get(Hovering) && !_channel->route->get(Hovering)) {
                _channel->set(Selected);
                Controller::selectedChannel = _channel->id;
                Controller::selectedInput = _channel->input;
                break;
            }
        }
        if (_notHovering) Controller::selectedChannel = -1;
        for (auto& _obj : objects()) {
            auto _channel = _obj.as<Channel>();
            if (_channel->get(Selected) && (Controller::selectedChannel != _channel->id
                || Controller::selectedInput != _channel->input)) {
                _channel->set(Selected, false);
            }
        }
    }

    void Mixer::draw(DrawContext& p) const  {
        p.strokeWeight(0);
        p.fill(background);
        p.rect(dimensions());
        p.fill(divider);
        p.rect(Dimensions{ dividerX, y(), 2, height() });
        Object::draw(p);
    }

    void Mixer::update()  {
        auto& _objects = objects();
        constexpr auto _padding = 4;
        constexpr auto _outerPadding = 8;
        float _x = x() + _outerPadding;
        float _y = y() + _outerPadding;
        float _h = height() - _outerPadding * 2;
        float _w = (width() - _outerPadding - 4 * _padding - 2) / _objects.size();
        bool _outputs = true;
        for (auto& _obj : _objects) {
            auto _channel = _obj.as<Channel>();
            if (_channel->input && _outputs) {
                _outputs = false;
                _x += _padding;
                dividerX = _x;
                _x += 2;
                _x += _padding;
                _x += _padding;
            }
            _channel->x(_x);
            _channel->y(_y);
            _channel->height(_h);
            _channel->width(_w - _padding);
            _x += _w;
        }
        Object::update();
    }

    void Mixer::updateTheme() {
        Controller::theme.background.assign(background);
        Controller::theme.divider.assign(divider);
        for (auto _obj : objects()) {
            auto _channel = _obj.as<Gui::Channel>();
            _channel->updateTheme();
        }
    }
}