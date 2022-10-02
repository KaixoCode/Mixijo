#include "Gui/Frame.hpp"
#include "Gui/Mixer.hpp"
#include "Controller.hpp"

namespace Mixijo {

    Frame::Button::Button(int type) : type(type) {
        background = Color{ 120, 120, 120, 0 };
        background[Pressed] = Color{ 120, 120, 120, 150 };
        background[Hovering] = Color{ 120, 120, 120, 90 };
        icon = Color{ 250, 250, 250 };
        link(background);
        link(icon);
        event<Button>();
    }

    void Frame::Button::mouseRelease(const MouseRelease& e) {
        if (hitbox(e.pos) && callback) callback();
    }

    void Frame::Button::draw(DrawContext& p) const {
        p.strokeWeight(0);
        p.fill(background);
        p.rect(dimensions());
        p.fill({ 0, 0, 0, 0 });
        p.strokeWeight(1);
        p.stroke(icon);
        auto _center = dimensions().center();
        auto _size = 5;
        switch (type) {
        case 0:
            p.line({ _center.x() - _size, _center.y() - _size }, { _center.x() + _size, _center.y() + _size });
            p.line({ _center.x() - _size, _center.y() + _size }, { _center.x() + _size, _center.y() - _size });
            break;
        case 1:
            p.rect(Dimensions{ _center.x() - _size - 0.5, _center.y() - _size - 0.5, _size * 2, _size * 2 });
            break;
        case 2:
            p.line({ _center.x() - _size, _center.y() }, { _center.x() + _size, _center.y() });
            break;
        }
    }

    Frame::Frame(Window::Construct c)
        : Window(c)
    {
        title = Color{ 200, 200, 200 };
        mixer = emplace<Gui::Mixer>().as<Object>();
        box.use = false;

        close->callback = [&] {
            m_ShouldExit = true;
        };

        maximize->callback = [&] {
            if (IsMaximized(m_Handle)) ShowWindow(m_Handle, SW_RESTORE);
            else ShowWindow(m_Handle, SW_MAXIMIZE);
        };

        minimize->callback = [&] {
            ShowWindow(m_Handle, SW_MINIMIZE);
        };
    }

    void Frame::draw(DrawContext& p) const {
        p.strokeWeight(0);
        p.fill(border);
        p.rect(Dimensions{ 0, 0, width(), height() });
        Object::draw(p);
    }

    void Frame::update() {
        if (IsMaximized(m_Handle)) {
            mixer->dimensions({ 16, 40, width() - 32, height() - 56 });
            close->dimensions({ width() - 45 * 1 - 8, 8, 45, 29 });
            maximize->dimensions({ width() - 45 * 2 - 8, 8, 45, 29 });
            minimize->dimensions({ width() - 45 * 3 - 8, 8, 45, 29 });
        }
        else {
            mixer->dimensions({ 8, 32, width() - 16, height() - 40 });
            close->dimensions({ width() - 45 * 1, 0, 45, 29 });
            maximize->dimensions({ width() - 45 * 2, 0, 45, 29 });
            minimize->dimensions({ width() - 45 * 3, 0, 45, 29 });
        }
        Window::update();
    }

    void Frame::updateTheme() {
        Controller::theme.border.assign(border);
        Controller::theme.close.background.assign(close->background);
        Controller::theme.close.icon.assign(close->icon);
        Controller::theme.minimize.background.assign(minimize->background);
        Controller::theme.minimize.icon.assign(minimize->icon);
        Controller::theme.maximize.background.assign(maximize->background);
        Controller::theme.maximize.icon.assign(maximize->icon);
        mixer.as<Gui::Mixer>()->updateTheme();
    }
}