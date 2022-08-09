#include "Gui/Channel.hpp"
#include "Controller.hpp"

namespace Mixijo::Gui {

    Channel::Channel(int gid, bool in, std::string_view name)
        : id(gid), input(in), name(name), route(emplace<RouteButton>())
    {
        link(background);
        link(meter);
        link(meterBackground);
        link(border);
        link(meterLine1);
        link(meterLine2);
        link(meterText);
        link(title);
        link(value);
        link(borderWidth);

        borderWidth = 0;
        border = Color{ 0, 0, 0 };
        meter = Color{ 0, 255, 0 };
        meterBackground = Color{ 5, 5, 5 };
        meterLine1 = Color{ 50, 50, 50 };
        meterLine2 = Color{ 90, 90, 90 };
        meterText = Color{ 200, 200, 200 };
        title = Color{ 200, 200, 200 };
        value = Color{ 200, 200, 200 };
        background.transition(100);
        background = Color{ 25, 25, 25 };
        background[Selected] = Color{ 30, 30, 30 };

        route->callback = [&] {
            if (Controller::selectedChannel == -1) return;
            if (input && !Controller::selectedInput) {
                auto& _val = Controller::processor.inputs[id].output_levels[Controller::selectedChannel];
                if (_val) _val = 0;
                else _val = 1;
                Controller::saveRouting();
            } else if (!input && Controller::selectedInput) {
                auto& _val = Controller::processor.inputs[Controller::selectedChannel].output_levels[id];
                if (_val) _val = 0;
                else _val = 1;
                Controller::saveRouting();
            }
        };

        box.use = false;
        event<Channel>();
        updateTheme();
    }

    void Channel::mousePress(const MousePress& e) {
        pressGain = std::pow(input
            ? Controller::processor.inputs[id].gain
            : Controller::processor.outputs[id].gain, 0.25);
    }
    
    void Channel::mouseClick(const MouseClick& e) {
        auto& _val = input
            ? Controller::processor.inputs[id].gain
            : Controller::processor.outputs[id].gain;
        if (counter > 0) _val = 1;
        if (!route->get(Hovering)) counter = 20;
    }

    void Channel::mouseDrag(const MouseDrag& e) {
        int _padding = 2;
        int _outerPadding = 12;
        Dimensions<int> _bars{
            x() + _outerPadding,
            y() + _outerPadding + 35,
            width() - 2 * _outerPadding - 19,
            height() - 2 * _outerPadding - 50 - 35
        };

        auto& _val = input
            ? Controller::processor.inputs[id].gain
            : Controller::processor.outputs[id].gain;
        _val = std::pow(std::clamp(pressGain + 1.412536 * (e.source.y() - e.pos.y()) / _bars.height(), 0., 1.412536), 4);
    }

    void Channel::draw(DrawContext& p) const {
        p.strokeWeight(borderWidth);
        p.stroke(border);
        p.fill(background);
        p.rect(dimensions());
        p.fill(title);
        p.font(Font::Default);
        p.fontSize(14);
        p.textAlign(Align::Center);
        p.text(name, dimensions().inset(12).topCenter());
        int _padding = 2;
        int _outerPadding = 12;
        Dimensions<int> _bars{
            x() + _outerPadding,
            y() + _outerPadding + 35,
            width() - 2 * _outerPadding - 19,
            height() - 2 * _outerPadding - 50 - 35
        };
        auto lin2y = [&](float lin) {
            return _bars.height() * std::clamp(std::pow(lin, 0.25) / 1.412536, 0., 1.);
        };

        auto db2y = [&](float db) {
            return lin2y(db2lin(db));
        };

        float _bottom = _bars.y() + _bars.height();
        int _x = _bars.x();
        p.strokeWeight(0);
        for (std::size_t i = 0; i < smoothed.size(); ++i) {
            int _w = (_bars.width() + _padding) * 1. / smoothed.size();
            float _h = std::floor(lin2y(smoothed[i])) - 0.3;
            float _y = _bottom - _h;
            p.fill(meterBackground);
            p.rect(Dimensions{ _x, _bars.y(), _w - _padding, _bars.height() });
            p.fill(meter);
            p.rect(Dimensions{ _x, _y, _w - _padding, _h });
            _x += _w;
        }
        p.fill(background);
        p.rect(Dimensions{ _bars.x(), _bars.bottom() - 1, _bars.width(), 2});
        const static std::array dB1{ 12.,                6.,               0.,                  -6.,                   -12.,                     -18.,                     -24.,                   -36.,                   -48.,                   -72. };
        const static std::array dB2{ 12.,       9.,      6.,      3.,      0.,       -3.,       -6.,       -9.,        -12.,        -15.,        -18.,        -21.,        -24.,       -30.,       -36.,       -42.,       -48.,       -60.,       -72.,       -96. };
        const static std::array dB3{ 12., 10.5, 9., 7.5, 6., 4.5, 3., 1.5, 0., -1.5, -3., -4.5, -6., -7.5, -9., -10.5, -12., -13.5, -15., -16.5, -18., -19.5, -21., -22.5, -24., -27., -30., -33., -36., -39., -42., -45., -48., -54., -60., -66., -72., -84., -96., -108. };
        const static std::array begins{ dB1.data(), dB2.data(), dB3.data() };
        const static std::array sizes{ dB1.size(), dB2.size(), dB3.size() };

        int _type = _bars.height() < 250 ? 0 : _bars.height() < 500 ? 1 : 2;

        auto _begin = begins[_type];
        auto _size = sizes[_type];
        bool _b = true;

        p.fontSize(14);
        p.textAlign(Align::Right | Align::CenterY);
        p.strokeWeight(1.5);
        for (int i = 0; i < _size; ++i) {
            auto _dB = _begin[i];
            float _mdb = 0.5 + std::floor(_bars.height() + _bars.y() - db2y(_dB));
            if (_b) {
                p.fill(meterText);
                p.text(NUMBERS[std::abs(_dB)], { _x + 25, _mdb });
                p.stroke(meterLine1);
            }
            else p.stroke(meterLine2);
            p.line({ _x, _mdb }, { _x + 5.f, _mdb });
            _b ^= true;
        }

        p.stroke(meterLine1);
        p.line({ _x, _bars.bottom() - 1.5 }, { _x + 5.f, _bars.bottom() - 1.5 });
        p.fill(meterText);
        p.text("inf", { _x + 25, _bars.bottom() - 1 });
        p.strokeWeight(0);

        auto _0y = _bars.height() + _bars.y() - lin2y(1);
        p.fill(background);
        p.rect(Dimensions{ _bars.x(), _0y, _bars.width(), 1 });

        auto _gain = input
            ? Controller::processor.inputs[id].gain
            : Controller::processor.outputs[id].gain;
        auto _sy = _bars.height() + _bars.y() - lin2y(_gain) - 2;
        p.fill(slider);
        p.rect(Dimensions{ _bars.x() - 3, _sy, _bars.width() + 6, 3 });
        p.triangle(
            { _bars.x(), _sy + 2 }, 
            { _bars.x() - 8, _sy + 9 }, 
            { _bars.x() - 8, _sy - 5 }
        );
        p.triangle(
            { _bars.x() + _bars.width(), _sy + 2 }, 
            { _bars.x() + _bars.width() + 8, _sy + 9 }, 
            { _bars.x() + _bars.width() + 8, _sy - 5 }
        );

        p.fill(value);
        p.textAlign(Align::Left | Align::Top);
        p.text(gain, _bars.inset(4, -8).bottomLeft());

        Object::draw(p);
    }

    void Channel::update() {
        counter--;
        auto ane = get(Selected);
        auto& _channel = input
            ? static_cast<Mixijo::Channel&>(Controller::processor.inputs[id])
            : static_cast<Mixijo::Channel&>(Controller::processor.outputs[id]);

        auto _db = input
            ? lin2db(Controller::processor.inputs[id].gain)
            : lin2db(Controller::processor.outputs[id].gain);
        if (_db < -120) gain = "-inf dB";
        else gain = std::format("{:.1f}", _db) + "dB";

        smoothed.resize(_channel.peaks.size());

        for (std::size_t i = 0; i < smoothed.size(); ++i) {
            smoothed[i] = smoothed[i] * 0.8 + 0.2 * _channel.peaks[i];
            _channel.peaks[i] = 0;
        }

        route->dimensions({ x() + 5, y() + height() - 30, width() - 10, 25 });

        if (Controller::selectedChannel != -1) {
            route->set(Disabled, Controller::selectedInput == input);
            if (input && !Controller::selectedInput) {
                auto& _val = Controller::processor.inputs[id].output_levels[Controller::selectedChannel];
                route->set(Selected, _val);
            }
            else if (!input && Controller::selectedInput) {
                auto& _val = Controller::processor.inputs[Controller::selectedChannel].output_levels[id];
                route->set(Selected, _val);
            }
        }
        else {
            route->set(Disabled, true);
        }

        Object::update();
    }
    
    void Channel::updateTheme() {
        Controller::theme.channel.background.assign(background);
        Controller::theme.channel.slider.assign(slider);
        Controller::theme.channel.meter.assign(meter);
        Controller::theme.channel.meterBackground.assign(meterBackground);
        Controller::theme.channel.border.assign(border);
        Controller::theme.channel.meterLine1.assign(meterLine1);
        Controller::theme.channel.meterLine2.assign(meterLine2);
        Controller::theme.channel.meterText.assign(meterText);
        Controller::theme.channel.title.assign(title);
        Controller::theme.channel.value.assign(value);
        Controller::theme.channel.borderWidth.assign(borderWidth);
        route->updateTheme();
    }

}