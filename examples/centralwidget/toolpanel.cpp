#include "toolpanel.h"

#include <QSlider>
#include <QFormLayout>

ToolPanel::ToolPanel()
{
    layout = new QFormLayout();

    for (int i = 0; i < 10; ++i) {
        auto slider = new QSlider();
        slider->setOrientation(Qt::Horizontal);
        layout->addRow(QString("parameter %1").arg(i), slider);
    }

    setLayout(layout);
}

ToolPanel::~ToolPanel()
{

}
