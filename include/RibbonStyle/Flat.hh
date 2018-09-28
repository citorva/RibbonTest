#pragma once

#include <RibbonStyle/RibbonStyle.hh>

#include <QColor>

namespace RibbonUI {

namespace RibbonStyle {

class FlatStyle {
public:
    QPixmap drawTab(QSize minsize, ButtonState state, const QString &name, const QPixmap &icon = QPixmap(), QSize maxsize = QSize());
    QPixmap drawButton(QSize minsize, ButtonState state, const QString &name, const QPixmap &icon = QPixmap(), QSize maxsize = QSize());

    void setMainColor(const QColor &color);
    QColor mainColor(void) const;
    void setHightlightColor(const QColor &color);
    QColor hightlightColor(void) const;
};

}

}