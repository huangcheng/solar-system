#pragma once
#include <QWidget>
#include "render/GlobeView.h"

class GlobeWindow : public QWidget {
    Q_OBJECT
public:
    explicit GlobeWindow(QWidget *parent = nullptr);
    GlobeView *view() { return m_view; }
protected:
    void paintEvent(QPaintEvent *) override {}
private:
    GlobeView *m_view;
};
