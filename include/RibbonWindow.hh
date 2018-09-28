#pragma once

#include <CustomWindow.hh>

namespace RibbonUI {

class Window : public CustomWindow::CustomWindow {
public:
	Window(QWidget* parent, Qt::WindowFlags flags);
};

}