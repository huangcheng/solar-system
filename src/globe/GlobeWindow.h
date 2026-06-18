#pragma once
#include <QWidget>

class GlobeWindow : public QWidget {
    Q_OBJECT
public:
    explicit GlobeWindow(QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *) override;
};
