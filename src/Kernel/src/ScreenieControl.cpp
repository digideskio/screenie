/* This file is part of the Screenie project.
   Screenie is a fancy screenshot composer.

   Copyright (C) 2008 Ariya Hidayat <ariya.hidayat@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QPoint>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QStringList>
#include <QtCore/QtAlgorithms>
#include <QtCore/QMimeData>
#include <QtCore/QUrl>
#include <QtGui/QColor>
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsItem>
#include <QtGui/QBrush>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtGui/QMainWindow>
#include <QtGui/QSlider>

#include "../../Utils/src/PaintTools.h"
#include "../../Utils/src/SizeFitter.h"
#include "../../Model/src/ScreenieScene.h"
#include "../../Model/src/ScreenieModelInterface.h"
#include "../../Model/src/ScreenieFilePathModel.h"
#include "../../Model/src/ScreeniePixmapModel.h"
#include "../../Model/src/ScreenieTemplateModel.h"
#include "TemplateOrganizer.h"
#include "Reflection.h"
#include "ScreenieGraphicsScene.h"
#include "ScreeniePixmapItem.h"

#include "ScreenieControl.h"

namespace
{
    bool zSort(const ScreeniePixmapItem *item1, const ScreeniePixmapItem *item2)
    {
        // closer items come first in the sorted list
        return item1->getScreenieModel().getDistance() > item2->getScreenieModel().getDistance();
    }
}; // anonymous

class ScreenieControlPrivate
{
public:
    ScreenieControlPrivate(ScreenieScene &theScreenieScene, ScreenieGraphicsScene &theScreenieGraphicsScene)
        : screenieScene(theScreenieScene),
          screenieGraphicsScene(theScreenieGraphicsScene),
          reflection(new Reflection()),
          templateOrganizer(theScreenieScene)
    {}

    ~ScreenieControlPrivate()
    {
        delete reflection;
    }

    ScreenieScene &screenieScene;
    ScreenieGraphicsScene &screenieGraphicsScene;
    QBrush checkerBoardBrush;
    QTimer qualityTimer;
    Reflection *reflection; /*! \todo The Reflection effect does not belong here. Add an "FX Manager" which keeps track of effects instead */
    DefaultScreenieModel defaultScreenieModel;
    TemplateOrganizer templateOrganizer;
};

// public

ScreenieControl::ScreenieControl(ScreenieScene &screenieScene, ScreenieGraphicsScene &screenieGraphicsScene)
    : QObject(),
      d(new ScreenieControlPrivate(screenieScene, screenieGraphicsScene))
{
    d->qualityTimer.setSingleShot(true);
    d->qualityTimer.setInterval(300);
    frenchConnection();
}

ScreenieControl::~ScreenieControl()
{
    delete d;
}

QList<ScreenieModelInterface *> ScreenieControl::getSelectedScreenieModels() const
{
    QList<ScreenieModelInterface *> result;
    foreach (QGraphicsItem *selectedItem, d->screenieGraphicsScene.selectedItems()) {
        if (selectedItem->type() == ScreeniePixmapItem::ScreeniePixmapType) {
            ScreeniePixmapItem *screeniePixmapItem = static_cast<ScreeniePixmapItem *>(selectedItem);
            result.append(&screeniePixmapItem->getScreenieModel());
        }
    }
    return result;
}

DefaultScreenieModel &ScreenieControl::getDefaultScreenieModel()
{
    return d->defaultScreenieModel;
}

void ScreenieControl::updateScene()
{
   d->screenieGraphicsScene.clear();
   handleBackgroundChanged();
   foreach (ScreenieModelInterface *screenieModel, d->screenieScene.getModels()) {
       handleModelAdded(*screenieModel);
   }
}

void ScreenieControl::updateModel(const QMimeData *mimeData, ScreenieModelInterface &screenieModel)
{
    // prefer image data over file paths (URLs)
    if (mimeData->hasImage()) {
        /*!\todo Convert to QImage instead, as not to loose any information (on screens with fewer capabilities!) */
        QPixmap pixmap = qvariant_cast<QPixmap>(mimeData->imageData());
        updatePixmapModel(pixmap, screenieModel);
    } else {
        QString filePath = mimeData->urls().first().toLocalFile();
        updateFilePathModel(filePath, screenieModel);
    }
}

ScreenieScene &ScreenieControl::getScreenieScene() const
{
    return d->screenieScene;
}
ScreenieGraphicsScene &ScreenieControl::getScreenieGraphicsScene() const
{
    return d->screenieGraphicsScene;
}

// public slots

void ScreenieControl::addImage(QString filePath, QPointF centerPosition)
{
    QStringList filePaths(filePath);
    addImages(filePaths, centerPosition);
}

void ScreenieControl::addImages(QStringList filePaths, QPointF centerPosition)
{
    QPointF position = centerPosition;
    foreach (QString filePath, filePaths) {
        ScreenieModelInterface *screenieModel = new ScreenieFilePathModel(filePath);
        applyDefaultValues(*screenieModel);
        QPointF itemPosition = calculateItemPosition(*screenieModel, position);
        position += QPointF(20.0, 20.0);
        screenieModel->setPosition(itemPosition);
        d->screenieScene.addModel(screenieModel);
    }
}

void ScreenieControl::addImage(QPixmap pixmap, QPointF centerPosition)
{
    QList<QPixmap> pixmaps;
    pixmaps.append(pixmap);
    addImages(pixmaps, centerPosition);
}

void ScreenieControl::addImages(QList<QPixmap> pixmaps, QPointF centerPosition)
{
    QPointF position = centerPosition;
    foreach (QPixmap pixmap, pixmaps) {
        ScreeniePixmapModel *screenieModel = new ScreeniePixmapModel(pixmap);
        applyDefaultValues(*screenieModel);
        QPointF itemPosition = calculateItemPosition(*screenieModel, position);
        position += QPointF(20.0, 20.0);
        screenieModel->setPosition(itemPosition);
        d->screenieScene.addModel(screenieModel);
    }
}

void ScreenieControl::addTemplate(QPointF centerPosition)
{
    /*!\todo Make the template size configurable in some UI dialog */
    QSize size(400, 400);
    ScreenieTemplateModel *screenieModel = new ScreenieTemplateModel(size);
    applyDefaultValues(*screenieModel);
    screenieModel->setPosition(centerPosition);
    d->screenieScene.addModel(screenieModel);
}

void ScreenieControl::removeAll()
{
    setRenderQuality(Low);
    QList<ScreenieModelInterface *> screenieModels = getSelectedScreenieModels();
    foreach (ScreenieModelInterface *screenieModel, screenieModels) {
        d->screenieScene.removeModel(screenieModel);
    }
    d->qualityTimer.start();
}

void ScreenieControl::selectAll()
{
    foreach(QGraphicsItem *item, d->screenieGraphicsScene.items()) {
        item->setSelected(true);
    }
}

void ScreenieControl::translate(qreal dx, qreal dy)
{
    bool decreaseQuality = dx != 0.0 && dy != 0.0;
    if (decreaseQuality) {
        setRenderQuality(Low);
        d->qualityTimer.start();
    }
    QList<ScreenieModelInterface *> screenieModels = getSelectedScreenieModels();
    foreach (ScreenieModelInterface *screenieModel, screenieModels) {
        screenieModel->translate(dx, dy);
    }

}

void ScreenieControl::setRotation(int angle)
{
    setRenderQuality(Low);
    QList<ScreenieModelInterface *> screenieModels = getSelectedScreenieModels();
    foreach (ScreenieModelInterface *screenieModel, screenieModels) {
        screenieModel->setRotation(angle);
    }
    d->qualityTimer.start();
}

void ScreenieControl::rotate(int angle)
{
    setRenderQuality(Low);
    QList<ScreenieModelInterface *> screenieModels = getSelectedScreenieModels();
    foreach (ScreenieModelInterface *screenieModel, screenieModels) {
        screenieModel->rotate(angle);
    }
    d->qualityTimer.start();
}

void ScreenieControl::setDistance(int distance)
{
    setRenderQuality(Low);
    QList<ScreenieModelInterface *> screenieModels = getSelectedScreenieModels();
    foreach (ScreenieModelInterface *screenieModel, screenieModels) {
        screenieModel->setDistance(distance);
    }
    d->qualityTimer.start();
}

void ScreenieControl::addDistance(int distance)
{
    setRenderQuality(Low);
    QList<ScreenieModelInterface *> screenieModels = getSelectedScreenieModels();
    foreach (ScreenieModelInterface *screenieModel, screenieModels) {
        screenieModel->addDistance(distance);
    }
    d->qualityTimer.start();
}

void ScreenieControl::setReflectionEnabled(bool enable)
{
    setRenderQuality(Low);
    QList<ScreenieModelInterface *> screenieModels = getSelectedScreenieModels();
    foreach (ScreenieModelInterface *screenieModel, screenieModels) {
        screenieModel->setReflectionEnabled(enable);
    }
    d->qualityTimer.start();
}

void ScreenieControl::setReflectionOffset(int reflectionOffset)
{
    setRenderQuality(Low);
    QList<ScreenieModelInterface *> screenieModels = getSelectedScreenieModels();
    foreach (ScreenieModelInterface *screenieModel, screenieModels) {
        screenieModel->setReflectionOffset(reflectionOffset);
    }
    d->qualityTimer.start();
}

void ScreenieControl::addReflectionOffset(int reflectionOffset)
{
    setRenderQuality(Low);
    QList<ScreenieModelInterface *> screenieModels = getSelectedScreenieModels();
    foreach (ScreenieModelInterface *screenieModel, screenieModels) {
        screenieModel->addReflectionOffset(reflectionOffset);
    }
    d->qualityTimer.start();
}

void ScreenieControl::setReflectionOpacity(int reflectionOpacity)
{
    setRenderQuality(Low);
    QList<ScreenieModelInterface *> screenieModels = getSelectedScreenieModels();
    foreach (ScreenieModelInterface *screenieModel, screenieModels) {
        screenieModel->setReflectionOpacity(reflectionOpacity);
    }
    d->qualityTimer.start();
}

void ScreenieControl::addReflectionOpacity(int reflectionOpacity)
{
    setRenderQuality(Low);
    QList<ScreenieModelInterface *> screenieModels = getSelectedScreenieModels();
    foreach (ScreenieModelInterface *screenieModel, screenieModels) {
        screenieModel->addReflectionOpacity(reflectionOpacity);
    }
    d->qualityTimer.start();
}

void ScreenieControl::setBackgroundEnabled(bool enable)
{
    d->screenieScene.setBackgroundEnabled(enable);
}

void ScreenieControl::setBackgroundColor(QColor color)
{
    d->screenieScene.setBackgroundColor(color);
}

void ScreenieControl::setRedBackgroundComponent(int red)
{
    QColor backgroundColor = d->screenieScene.getBackgroundColor();
    backgroundColor.setRed(red);
    d->screenieScene.setBackgroundColor(backgroundColor);
}

void ScreenieControl::setGreenBackgroundComponent(int green)
{
    QColor backgroundColor = d->screenieScene.getBackgroundColor();
    backgroundColor.setGreen(green);
    d->screenieScene.setBackgroundColor(backgroundColor);
}

void ScreenieControl::setBlueBackgroundComponent(int blue)
{
    QColor backgroundColor = d->screenieScene.getBackgroundColor();
    backgroundColor.setBlue(blue);
    d->screenieScene.setBackgroundColor(backgroundColor);
}

// private

void ScreenieControl::frenchConnection()
{
    connect(&d->screenieScene, SIGNAL(distanceChanged()),
            this, SLOT(handleDistanceChanged()));
    connect(&d->screenieScene, SIGNAL(modelAdded(ScreenieModelInterface &)),
            this, SLOT(handleModelAdded(ScreenieModelInterface &)));
    connect(&d->screenieScene, SIGNAL(modelRemoved(ScreenieModelInterface &)),
            this, SLOT(handleModelRemoved(ScreenieModelInterface &)));
    connect(&d->screenieScene, SIGNAL(backgroundChanged()),
            this, SLOT(handleBackgroundChanged()));
    connect(&d->screenieGraphicsScene, SIGNAL(pixmapsDropped(QList<QPixmap>, QPointF)),
            this, SLOT(handlePixmapsDrop(QList<QPixmap>, QPointF)));
    connect(&d->screenieGraphicsScene, SIGNAL(filePathsDropped(QStringList, QPointF)),
            this, SLOT(handleFilePathsDrop(QStringList, QPointF)));
    connect(&d->screenieGraphicsScene, SIGNAL(rotate(int)),
            this, SLOT(rotate(int)));
    connect(&d->screenieGraphicsScene, SIGNAL(addDistance(int)),
            this, SLOT(addDistance(int)));
    connect(&d->screenieGraphicsScene, SIGNAL(translate(qreal, qreal)),
            this, SLOT(translate(qreal, qreal)));
    connect(&d->qualityTimer, SIGNAL(timeout()),
            this, SLOT(restoreRenderQuality()));
}

QList<ScreeniePixmapItem *> ScreenieControl::getScreeniePixmapItems() const
{
    QList<ScreeniePixmapItem *> result;
    foreach (QGraphicsItem *item, d->screenieGraphicsScene.items()) {
        if (item->type() == ScreeniePixmapItem::ScreeniePixmapType) {
            ScreeniePixmapItem *screeniePixmapItem = static_cast<ScreeniePixmapItem *>(item);
            result.append(screeniePixmapItem);
        }
    }
    return result;
}

void ScreenieControl::setRenderQuality(RenderQuality renderQuality)
{
    QList<ScreeniePixmapItem *> items = getScreeniePixmapItems();
    switch (renderQuality) {
    case Low:
        foreach (ScreeniePixmapItem *item, items) {
            item->setTransformationMode(Qt::FastTransformation);
        }
        foreach (QGraphicsView *view, d->screenieGraphicsScene.views()) {
            view->setRenderHints(QPainter::NonCosmeticDefaultPen);
        }
        break;
    case High:
        foreach (ScreeniePixmapItem *item, items) {
            item->setTransformationMode(Qt::SmoothTransformation);
        }
        foreach (QGraphicsView *view, d->screenieGraphicsScene.views()) {
            view->setRenderHints(QPainter::Antialiasing|QPainter::SmoothPixmapTransform);
        }
    default:
        break;
    }
}

void ScreenieControl::applyDefaultValues(ScreenieModelInterface &screenieModelInterface)
{
    screenieModelInterface.setDistance(d->defaultScreenieModel.getDistance());
    screenieModelInterface.setRotation(d->defaultScreenieModel.getRotation());
    screenieModelInterface.setReflectionEnabled(d->defaultScreenieModel.isReflectionEnabled());
    screenieModelInterface.setReflectionOffset(d->defaultScreenieModel.getReflectionOffset());
    screenieModelInterface.setReflectionOpacity(d->defaultScreenieModel.getReflectionOpacity());
}

QPointF ScreenieControl::calculateItemPosition(const ScreenieModelInterface &screenieModel, const QPointF &centerPosition)
{
    QPointF result;
    QSize itemSize = screenieModel.getSize();
    result.setX(centerPosition.x() - itemSize.width()  / 2.0);
    result.setY(centerPosition.y() - itemSize.height() / 2.0);
    return result;
}

QPixmap ScreenieControl::scaleToTemplate(const ScreenieTemplateModel &templateModel, const QPixmap &pixmap)
{
    QPixmap result;
    const SizeFitter &sizeFitter = templateModel.getSizeFitter();
    QSize fittedSize;
    bool doResize = sizeFitter.fit(pixmap.size(), fittedSize);
    if (doResize) {
        result = pixmap.scaled(fittedSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    } else {
        result = pixmap;
    }
    return result;
}

QPointF ScreenieControl::calculateItemPosition(const QPointF &sourcePosition, const QSize &sourceSize, const QSize &targetSize)
{
    QPointF result;
    result.setX(sourcePosition.x() + sourceSize.width()  / 2.0 - targetSize.width()  / 2.0);
    result.setY(sourcePosition.y() + sourceSize.height() / 2.0 - targetSize.height() / 2.0);
    return result;
}

void ScreenieControl::updatePixmapModel(const QPixmap &pixmap, ScreenieModelInterface &screenieModel)
{
    QPixmap actualPixmap = pixmap;
    if (!actualPixmap.isNull()) {
        if (screenieModel.inherits(ScreeniePixmapModel::staticMetaObject.className())) {
            ScreeniePixmapModel &screeniePixmapModel = static_cast<ScreeniePixmapModel &>(screenieModel);
            QPointF itemPosition = calculateItemPosition(screeniePixmapModel.getPosition(), screeniePixmapModel.getSize(), actualPixmap.size());
            screeniePixmapModel.setPixmap(actualPixmap);
            screeniePixmapModel.setPosition(itemPosition);
        } else {
            // convert to ScreeniePixmapModel
            if (screenieModel.inherits(ScreenieTemplateModel::staticMetaObject.className())) {
                actualPixmap = scaleToTemplate(static_cast<ScreenieTemplateModel &>(screenieModel), actualPixmap);
            }
            ScreeniePixmapModel *screeniePixmapModel = new ScreeniePixmapModel(actualPixmap);
            QPointF itemPosition = calculateItemPosition(screenieModel.getPosition(), screenieModel.getSize(), actualPixmap.size());
            screeniePixmapModel->convert(screenieModel);
            screeniePixmapModel->setPosition(itemPosition);
            d->screenieScene.removeModel(&screenieModel);
            d->screenieScene.addModel(screeniePixmapModel);
        }
    }
}

void ScreenieControl::updateFilePathModel(const QString &filePath, ScreenieModelInterface &screenieModel)
{
    if (screenieModel.inherits(ScreenieFilePathModel::staticMetaObject.className())) {
        ScreenieFilePathModel &filePathModel = static_cast<ScreenieFilePathModel &>(screenieModel);
        QSize oldSize = filePathModel.getSize();
        filePathModel.setFilePath(filePath);
        QPointF itemPosition = calculateItemPosition(screenieModel.getPosition(), oldSize, screenieModel.getSize());
        filePathModel.setPosition(itemPosition);
    } else {
        SizeFitter sizeFitter;
        ScreenieFilePathModel *screenieFilePathModel;
        if (screenieModel.inherits(ScreenieTemplateModel::staticMetaObject.className())) {
            sizeFitter = static_cast<ScreenieTemplateModel &>(screenieModel).getSizeFitter();
            screenieFilePathModel = new ScreenieFilePathModel(filePath, &sizeFitter);
        } else {
            screenieFilePathModel = new ScreenieFilePathModel(filePath);
        }
        QSize size = screenieFilePathModel->getSize();
        QPointF itemPosition = calculateItemPosition(screenieModel.getPosition(), screenieModel.getSize(), size);
        screenieFilePathModel->convert(screenieModel);
        screenieFilePathModel->setPosition(itemPosition);
        d->screenieScene.removeModel(&screenieModel);
        d->screenieScene.addModel(screenieFilePathModel);
    }
}

// private slots

void ScreenieControl::handleFilePathsDrop(QStringList filePaths, QPointF centerPosition)
{
    ScreenieTemplateModel *templateModel;
    if (!d->screenieScene.hasTemplates()) {
        addImages(filePaths, centerPosition);
    } else {
        QList<ScreenieTemplateModel *> templateModels = d->templateOrganizer.getOrderedTemplates();
        int i = 0;
        int n = filePaths.count();
        QList<ScreenieTemplateModel *>::const_iterator it = templateModels.constBegin();
        while (i < n && it != templateModels.constEnd()) {
            templateModel = *it;
            QString filePath = filePaths.at(i);
            updateFilePathModel(filePath, *templateModel);
            i++;
            it++;
        }
    }
}

void ScreenieControl::handlePixmapsDrop(QList<QPixmap> pixmaps, QPointF centerPosition)
{
    ScreenieTemplateModel *templateModel;
    if (!d->screenieScene.hasTemplates()) {
        addImages(pixmaps, centerPosition);
    } else {
        QList<ScreenieTemplateModel *> templateModels = d->templateOrganizer.getOrderedTemplates();
        int i = 0;
        int n = pixmaps.count();
        QList<ScreenieTemplateModel *>::const_iterator it = templateModels.constBegin();
        while (i < n && it != templateModels.constEnd()) {
            templateModel = *it;
            QPixmap pixmap = pixmaps.at(i);
            updatePixmapModel(pixmap, *templateModel);
            i++;
            it++;
        }
    }
}

void ScreenieControl::handleDistanceChanged()
{
    QList<ScreeniePixmapItem *> screeniePixmapItems;
    foreach (QGraphicsItem *graphicsItem, d->screenieGraphicsScene.items()) {
        if (graphicsItem->type() == ScreeniePixmapItem::ScreeniePixmapType) {
            ScreeniePixmapItem *screeniePixmapItem = static_cast<ScreeniePixmapItem *>(graphicsItem);
            screeniePixmapItems.append(screeniePixmapItem);
        }
    }
    ::qSort(screeniePixmapItems.begin(), screeniePixmapItems.end(), zSort);
    int z = 0;
    foreach (QGraphicsItem *graphicsItem, screeniePixmapItems) {
        graphicsItem->setZValue(z++);
    }
}

void ScreenieControl::handleModelAdded(ScreenieModelInterface &screenieModel)
{
    ScreeniePixmapItem *screeniePixmapItem = new ScreeniePixmapItem(screenieModel, *this, *d->reflection);
    screeniePixmapItem->setPos(screenieModel.getPosition());
    d->screenieGraphicsScene.clearSelection();
    screeniePixmapItem->setSelected(true);
    d->screenieGraphicsScene.addItem(screeniePixmapItem);
    handleDistanceChanged();
}

void ScreenieControl::handleModelRemoved(ScreenieModelInterface &screenieModel)
{
    foreach (QGraphicsItem *graphicsItem, d->screenieGraphicsScene.items()) {
        if (graphicsItem->type() == ScreeniePixmapItem::ScreeniePixmapType) {
            ScreeniePixmapItem *screeniePixmapItem = static_cast<ScreeniePixmapItem *>(graphicsItem);
            if (&screenieModel == &screeniePixmapItem->getScreenieModel()) {
                d->screenieGraphicsScene.removeItem(screeniePixmapItem);
                delete screeniePixmapItem;
                break;
            }
        }
    }
}

void ScreenieControl::handleBackgroundChanged()
{
    QBrush backgroundBrush;
    if (d->screenieScene.isBackgroundEnabled()) {
        QColor backgroundColor = d->screenieScene.getBackgroundColor();
        backgroundBrush = QBrush(backgroundColor);
    } else {
        if (d->checkerBoardBrush.style() == Qt::NoBrush) {
            d->checkerBoardBrush = PaintTools::createCheckerPattern();
        }
        backgroundBrush = d->checkerBoardBrush;
    }
    d->screenieGraphicsScene.setBackgroundBrush(backgroundBrush);
}

void ScreenieControl::restoreRenderQuality()
{
    setRenderQuality(High);
}
