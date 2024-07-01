#pragma once

#include <QWidget>

class QFormLayout;

class ToolPanel : public QWidget
{
public:
    ToolPanel();
    virtual ~ToolPanel();

private:
    QFormLayout* layout;
};
