#pragma once

#include <QPixmap>
#include <QSize>
#include <QString>

namespace RibbonUI {

namespace RibbonStyle {

enum ButtonState {
    NORMAL,
    HOVER,
    ACTIVE,
    DISABLED
};

class RibbonStyle {
public:
    virtual QPixmap drawTab(QSize minsize, ButtonState state, const QString &name, const QPixmap &icon = QPixmap(), QSize maxsize = QSize()) = 0;
    virtual QPixmap drawButton(QSize minsize, ButtonState state, const QString &name, const QPixmap &icon = QPixmap(), QSize maxsize = QSize()) = 0;


};

}

}