#include <QScrollBar>
#include <QTimer>
#include <QGraphicsItem>
#include <QGraphicsBlurEffect>
#include <QFile>
#include <QTextStream>

#include "Gui/Settings.h"
#include "EngineInterface/SimulationAccess.h"
#include "EngineInterface/SimulationController.h"
#include "EngineInterface/SimulationContext.h"
#include "EngineInterface/SpaceProperties.h"

#include "PixelUniverseView.h"
#include "VectorUniverseView.h"
#include "ItemUniverseView.h"
#include "QApplicationHelper.h"
#include "StartupController.h"

#include "SimulationViewWidget.h"
#include "ui_SimulationViewWidget.h"


SimulationViewWidget::SimulationViewWidget(QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::SimulationViewWidget)
{
    ui->setupUi(this);

    _pixelUniverse = new PixelUniverseView(ui->simulationView, this);
    _vectorUniverse = new VectorUniverseView(ui->simulationView, this);
    _itemUniverse = new ItemUniverseView(ui->simulationView, this);
    connect(_pixelUniverse, &PixelUniverseView::startContinuousZoomIn, this, &SimulationViewWidget::continuousZoomIn);
    connect(_pixelUniverse, &PixelUniverseView::startContinuousZoomOut, this, &SimulationViewWidget::continuousZoomOut);
    connect(_pixelUniverse, &PixelUniverseView::endContinuousZoom, this, &SimulationViewWidget::endContinuousZoom);
    connect(_vectorUniverse, &VectorUniverseView::startContinuousZoomIn, this, &SimulationViewWidget::continuousZoomIn);
    connect(
        _vectorUniverse, &VectorUniverseView::startContinuousZoomOut, this, &SimulationViewWidget::continuousZoomOut);
    connect(_vectorUniverse, &VectorUniverseView::endContinuousZoom, this, &SimulationViewWidget::endContinuousZoom);
    connect(_itemUniverse, &ItemUniverseView::startContinuousZoomIn, this, &SimulationViewWidget::continuousZoomIn);
    connect(_itemUniverse, &ItemUniverseView::startContinuousZoomOut, this, &SimulationViewWidget::continuousZoomOut);
    connect(_itemUniverse, &ItemUniverseView::endContinuousZoom, this, &SimulationViewWidget::endContinuousZoom);

    ui->simulationView->horizontalScrollBar()->setStyleSheet(Const::ScrollbarStyleSheet);
    ui->simulationView->verticalScrollBar()->setStyleSheet(Const::ScrollbarStyleSheet);

    ui->simulationView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->simulationView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto startupScene = new QGraphicsScene(this);
    startupScene->setBackgroundBrush(QBrush(Const::UniverseColor));
    ui->simulationView->setScene(startupScene);
}

SimulationViewWidget::~SimulationViewWidget()
{
    delete ui;
}

void SimulationViewWidget::init(
    Notifier* notifier,
    SimulationController* controller,
    SimulationAccess* access,
    DataRepository* repository)
{
    auto const InitialZoomFactor = 4.0;

	_controller = controller;

    _pixelUniverse->init(notifier, controller, access, repository);
    _vectorUniverse->init(notifier, controller, access, repository);
    _itemUniverse->init(notifier, controller, repository);

    _vectorUniverse->activate(InitialZoomFactor);

    auto const size = _controller->getContext()->getSpaceProperties()->getSize();
    _vectorUniverse->centerTo({ static_cast<float>(size.x) / 2, static_cast<float>(size.y) / 2 });

    _vectorUniverse->connectView();
    _vectorUniverse->refresh();

    Q_EMIT zoomFactorChanged(InitialZoomFactor);
}

void SimulationViewWidget::connectView()
{
    getActiveUniverseView()->connectView();
}

void SimulationViewWidget::disconnectView()
{
    getActiveUniverseView()->disconnectView();
}

void SimulationViewWidget::refresh()
{
    getActiveUniverseView()->refresh();
}

ActiveView SimulationViewWidget::getActiveView() const
{
    if (_pixelUniverse->isActivated()) {
        return ActiveView::PixelScene;
    }
    if (_vectorUniverse->isActivated()) {
        return ActiveView::VectorScene;
    }
    if (_itemUniverse->isActivated()) {
        return ActiveView::ItemScene;
    }

    THROW_NOT_IMPLEMENTED();
}

void SimulationViewWidget::setActiveScene (ActiveView activeScene)
{
    auto scrollPosX = ui->simulationView->horizontalScrollBar()->value();
    auto scrollPosY = ui->simulationView->verticalScrollBar()->value();

    auto zoom = getZoomFactor();

    auto view = getView(activeScene);
    view->activate(zoom);

    ui->simulationView->horizontalScrollBar()->setValue(scrollPosX); //workaround since UniverseView::centerTo has bad precision
    ui->simulationView->verticalScrollBar()->setValue(scrollPosY);
}

double SimulationViewWidget::getZoomFactor()
{
    return getActiveUniverseView()->getZoomFactor();
}

void SimulationViewWidget::setZoomFactor(double factor)
{
    auto activeView = getActiveUniverseView();
    auto screenCenterPos = activeView->getCenterPositionOfScreen();
    activeView->setZoomFactor(factor);
    activeView->centerTo(screenCenterPos);

    Q_EMIT zoomFactorChanged(factor);
}

void SimulationViewWidget::setZoomFactor(double zoomFactor, QVector2D const& worldPos)
{
    auto origZoomFactor = getZoomFactor();
    auto activeView = getActiveUniverseView();
    auto worldPosOfScreenCenter = activeView->getCenterPositionOfScreen();
    activeView->setZoomFactor(zoomFactor);
    QVector2D mu(
        worldPosOfScreenCenter.x() * (zoomFactor / origZoomFactor - 1.0),
        worldPosOfScreenCenter.y() * (zoomFactor / origZoomFactor - 1.0));
    QVector2D correction(
        mu.x() * (worldPosOfScreenCenter.x() - worldPos.x()) / worldPosOfScreenCenter.x(),
        mu.y() * (worldPosOfScreenCenter.y() - worldPos.y()) / worldPosOfScreenCenter.y());
    activeView->centerTo(worldPosOfScreenCenter - correction);

    Q_EMIT zoomFactorChanged(zoomFactor);
}

QVector2D SimulationViewWidget::getViewCenterWithIncrement ()
{
	auto screenCenterPos = getActiveUniverseView()->getCenterPositionOfScreen();

    QVector2D posIncrement(_posIncrement, -_posIncrement);
    _posIncrement = _posIncrement + 1.0;
    if (_posIncrement > 9.0) {
        _posIncrement = 0.0;
    }
    return screenCenterPos + posIncrement;
}

void SimulationViewWidget::toggleCenterSelection(bool value)
{
    auto activeUniverseView = getActiveUniverseView();
    auto itemUniverseView = dynamic_cast<ItemUniverseView*>(activeUniverseView);
    CHECK(itemUniverseView);

    itemUniverseView->toggleCenterSelection(value);
}

UniverseView * SimulationViewWidget::getActiveUniverseView() const
{
    if (_pixelUniverse->isActivated()) {
        return _pixelUniverse;
    }
    if (_vectorUniverse->isActivated()) {
        return _vectorUniverse;
    }
    if (_itemUniverse->isActivated()) {
        return _itemUniverse;
    }

    THROW_NOT_IMPLEMENTED();
}

UniverseView * SimulationViewWidget::getView(ActiveView activeView) const
{
    if (ActiveView::PixelScene == activeView) {
        return _pixelUniverse;
    }
    if (ActiveView::VectorScene == activeView) {
        return _vectorUniverse;
    }
    if (ActiveView::ItemScene== activeView) {
        return _itemUniverse;
    }

    THROW_NOT_IMPLEMENTED();
}



