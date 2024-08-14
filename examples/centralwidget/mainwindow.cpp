#include "mainwindow.h"

#include "ui_mainwindow.h"

#include <QWidgetAction>
#include <QLabel>
#include <QCalendarWidget>
#include <QTreeView>
#include <QFileSystemModel>
#include <QTableWidget>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QPushButton>
#include <QInputDialog>
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QToolBar>

#include "DockAreaWidget.h"
#include "DockAreaTitleBar.h"
#include "DockAreaTabBar.h"
#include "FloatingDockContainer.h"
#include "DockComponentsFactory.h"
#include "centralwidget.h"
#include "toolpanel.h"

using namespace ads;


CMainWindow::CMainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::CMainWindow)
{
    ui->setupUi(this);
    CDockManager::setConfigFlag(CDockManager::OpaqueSplitterResize, true);
    CDockManager::setConfigFlag(CDockManager::XmlCompressionEnabled, false);
    CDockManager::setConfigFlag(CDockManager::FocusHighlighting, true);
    CDockManager::setConfigFlag(CDockManager::DragPreviewIsDynamic, false);
    DockManager = new CDockManager(this);

    // Set central widget
    QPlainTextEdit* w = new QPlainTextEdit();
	w->setPlaceholderText("This is the central editor. Enter your text here.");
    CentralWidget* widget = new CentralWidget();
    CDockWidget* CentralDockWidget = new CDockWidget("CentralWidget");
    CentralDockWidget->setWidget(widget);
    auto* CentralDockArea = DockManager->setCentralWidget(CentralDockWidget);
    CentralDockArea->setAllowedAreas(DockWidgetArea::OuterDockAreas);

    // create other dock widgets
    ToolPanel* table = new ToolPanel();
    CDockWidget* TableDockWidget = new CDockWidget("Table 1");
    TableDockWidget->setWidget(table);
    TableDockWidget->setMinimumSizeHintMode(CDockWidget::MinimumSizeHintFromDockWidgetMinimumSize);
    TableDockWidget->resize(250, 150);
    // TableDockWidget->setMinimumSize(200,150);
    auto TableArea = DockManager->addDockWidget(DockWidgetArea::LeftDockWidgetArea, TableDockWidget);
    ui->menuView->addAction(TableDockWidget->toggleViewAction());

    table = new ToolPanel();
    TableDockWidget = new CDockWidget("Table 2");
    TableDockWidget->setWidget(table);
    TableDockWidget->setMinimumSizeHintMode(CDockWidget::MinimumSizeHintFromDockWidgetMinimumSize);
    TableDockWidget->resize(250, 150);
    // TableDockWidget->setMinimumSize(200,150);
    DockManager->addDockWidget(DockWidgetArea::BottomDockWidgetArea, TableDockWidget, TableArea);
    ui->menuView->addAction(TableDockWidget->toggleViewAction());

    ToolPanel* propertiesTable = new ToolPanel();
    CDockWidget* PropertiesDockWidget = new CDockWidget("Properties");
    PropertiesDockWidget->setWidget(propertiesTable);
    PropertiesDockWidget->setMinimumSizeHintMode(CDockWidget::MinimumSizeHintFromDockWidgetMinimumSize);
    PropertiesDockWidget->resize(250, 150);
    // PropertiesDockWidget->setMinimumSize(200,150);
    auto area = DockManager->addDockWidget(DockWidgetArea::RightDockWidgetArea, PropertiesDockWidget, CentralDockArea);
    ui->menuView->addAction(PropertiesDockWidget->toggleViewAction());
    
    ToolPanel* panel = new ToolPanel();
    CDockWidget* panelWidget = new CDockWidget("Panel");
    panelWidget->setWidget(panel);
    panelWidget->setMinimumSizeHintMode(CDockWidget::MinimumSizeHintFromDockWidgetMinimumSize);
    panelWidget->resize(250, 150);
    // PropertiesDockWidget->setMinimumSize(200,150);
    DockManager->addDockWidget(DockWidgetArea::BottomDockWidgetArea, panelWidget, area);
    ui->menuView->addAction(PropertiesDockWidget->toggleViewAction());
    
    createPerspectiveUi();
}

CMainWindow::~CMainWindow()
{
    delete ui;
}


void CMainWindow::createPerspectiveUi()
{
	SavePerspectiveAction = new QAction("Create Perspective", this);
	connect(SavePerspectiveAction, SIGNAL(triggered()), SLOT(savePerspective()));
	PerspectiveListAction = new QWidgetAction(this);
	PerspectiveComboBox = new QComboBox(this);
	PerspectiveComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	PerspectiveComboBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	connect(PerspectiveComboBox, SIGNAL(currentTextChanged(const QString&)),
		DockManager, SLOT(openPerspective(const QString&)));
	PerspectiveListAction->setDefaultWidget(PerspectiveComboBox);
	ui->toolBar->addSeparator();
	ui->toolBar->addAction(PerspectiveListAction);
	ui->toolBar->addAction(SavePerspectiveAction);
}


void CMainWindow::savePerspective()
{
	QString PerspectiveName = QInputDialog::getText(this, "Save Perspective", "Enter unique name:");
	if (PerspectiveName.isEmpty())
	{
		return;
	}

	DockManager->addPerspective(PerspectiveName);
	QSignalBlocker Blocker(PerspectiveComboBox);
	PerspectiveComboBox->clear();
	PerspectiveComboBox->addItems(DockManager->perspectiveNames());
	PerspectiveComboBox->setCurrentText(PerspectiveName);
}

//============================================================================
void CMainWindow::closeEvent(QCloseEvent* event)
{
    // Delete dock manager here to delete all floating widgets. This ensures
    // that all top level windows of the dock manager are properly closed
    DockManager->deleteLater();
	QMainWindow::closeEvent(event);
}
