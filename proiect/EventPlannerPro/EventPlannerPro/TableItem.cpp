#include "TableItem.h"
#include <QtMath>
#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMimeData>
#include <QMenu>
#include <QMessageBox>
#include <QFont>
#include <QDataStream> 
#include <QMap>
#include <QCursor>

int TableItem::GlobalMinDist = 130;

TableItem::TableItem(int id, int nrScaune, qreal x, qreal y, qreal w, qreal h)
    : QObject(), QGraphicsEllipseItem(x, y, w, h), tableId(id), maxChairs(nrScaune)
{
    setBrush(QBrush(Qt::lightGray));
    setPen(QPen(Qt::black));

    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setAcceptDrops(true);

    textId = new QGraphicsTextItem(QString::number(tableId), this);
    QFont font = textId->font();
    font.setBold(true);
    font.setPointSize(14);
    textId->setFont(font);

    QRectF textRect = textId->boundingRect();
    textId->setPos((w - textRect.width()) / 2, (h - textRect.height()) / 2);

    arrangeChairs();
}

void TableItem::arrangeChairs()
{
    qreal cx = rect().width() / 2;
    qreal cy = rect().height() / 2;
    qreal r = (rect().width() / 2) + 15;

    for (int i = 0; i < maxChairs; ++i)
    {
        double angle = 2 * M_PI * i / maxChairs;
        double sx = cx + r * cos(angle) - 10;
        double sy = cy + r * sin(angle) - 10;

        ChairItem* chair = new ChairItem(this);
        chair->setPos(sx, sy);

        connect(chair, &ChairItem::guestRemoved, this, &TableItem::onChairGuestRemoved);
        chairs.append(chair);
    }
}

QStringList TableItem::getGuestsData() const {
    QStringList data;
    for (auto chair : chairs) {
        data << chair->getGuestName();
    }
    return data;
}

void TableItem::setGuestsData(const QStringList& guests) {
    for (int i = 0; i < qMin(guests.size(), chairs.size()); ++i) {
        if (!guests[i].isEmpty()) {
            chairs[i]->setGuest(guests[i]);
        }
    }
}

void TableItem::removeGuestByName(const QString& name)
{
    for (ChairItem* chair : chairs) {
        if (chair->isOccupied() && chair->getGuestName() == name) {
            chair->removeGuest();
            break;
        }
    }
}

void TableItem::dragEnterEvent(QGraphicsSceneDragDropEvent* event)
{
    event->acceptProposedAction();
}

void TableItem::dropEvent(QGraphicsSceneDragDropEvent* event)
{
    QString name;
    if (event->mimeData()->hasText()) {
        name = event->mimeData()->text();
    }
    if (name.isEmpty() && event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        QByteArray encoded = event->mimeData()->data("application/x-qabstractitemmodeldatalist");
        QDataStream stream(&encoded, QIODevice::ReadOnly);
        while (!stream.atEnd()) {
            int row, col;
            QMap<int, QVariant> roleDataMap;
            stream >> row >> col >> roleDataMap;
            if (roleDataMap.contains(0)) {
                name = roleDataMap[0].toString();
                break;
            }
        }
    }

    if (name.trimmed().isEmpty()) return;

    bool seated = false;
    for (ChairItem* chair : chairs) {
        if (!chair->isOccupied()) {
            chair->setGuest(name);
            emit guestSeated(name, tableId);
            seated = true;
            break;
        }
    }

    if (!seated) {
        QMessageBox::warning(nullptr, "Masa Plina", "Aceasta masa nu mai are locuri libere!");
    }
    event->acceptProposedAction();
}

void TableItem::onChairGuestRemoved(QString name)
{
    emit guestUnseated(name);
    checkEmptyTable();
}

void TableItem::checkEmptyTable()
{
    bool isFullEmpty = true;
    for (ChairItem* chair : chairs) {
        if (chair->isOccupied()) {
            isFullEmpty = false;
            break;
        }
    }

    if (isFullEmpty) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr, "Stergere Masa",
            "Masa " + QString::number(tableId) + " este goala. O stergeti?",
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            emit tableDeleted(this);
            this->deleteLater();
        }
    }
}

void TableItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    QGraphicsItem* item = scene()->itemAt(event->scenePos(), QTransform());
    if (item == textId) item = this;
    if (item != this) {
        QGraphicsEllipseItem::contextMenuEvent(event);
        return;
    }

    QMenu menu;

    QAction* title = menu.addAction("Masa " + QString::number(tableId));
    title->setEnabled(false);
    menu.addSeparator();

    bool hasGuests = false;
    for (ChairItem* chair : chairs) {
        if (chair->isOccupied()) {
            QAction* guestInfo = menu.addAction(" - " + chair->getGuestName());
            guestInfo->setEnabled(false);
            hasGuests = true;
        }
    }

    if (!hasGuests) {
        QAction* emptyInfo = menu.addAction("(Masa goala)");
        emptyInfo->setEnabled(false);
    }

    menu.addSeparator();

    QAction* deleteAction = menu.addAction("STERGE MASA");
    QAction* selected = menu.exec(event->screenPos());

    if (selected == deleteAction) {
        for (ChairItem* chair : chairs) {
            if (chair->isOccupied()) {
                emit guestUnseated(chair->getGuestName());
            }
        }
        emit tableDeleted(this);
        this->deleteLater();
    }
}

void TableItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    originalPos = pos();
    QGraphicsEllipseItem::mousePressEvent(event);
}

void TableItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsEllipseItem::mouseReleaseEvent(event);

    bool collision = false;
    QPointF myCenter = pos() + QPointF(50, 50);
    QList<QGraphicsItem*> items = scene()->items();

    for (QGraphicsItem* item : items) {
        if (item == this) continue;
        if (item->type() == this->type()) {
            QPointF otherPos = item->pos();
            QPointF otherCenter = otherPos + QPointF(50, 50);
            qreal dist = qSqrt(qPow(myCenter.x() - otherCenter.x(), 2) +
                qPow(myCenter.y() - otherCenter.y(), 2));

            if (dist < GlobalMinDist) {
                collision = true;
                break;
            }
        }
    }

    if (collision) {
        setPos(originalPos);
    }

    setOpacity(1.0);
    setCursor(Qt::OpenHandCursor);
}

QVariant TableItem::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionChange && scene()) {
        QPointF newPos = value.toPointF();
        QPointF newCenter = newPos + QPointF(50, 50);
        QList<QGraphicsItem*> items = scene()->items();

        bool collision = false;

        for (QGraphicsItem* item : items) {
            if (item == this) continue;
            if (item->type() == this->type()) {
                QPointF otherPos = item->pos();
                QPointF otherCenter = otherPos + QPointF(50, 50);
                qreal dist = qSqrt(qPow(newCenter.x() - otherCenter.x(), 2) +
                    qPow(newCenter.y() - otherCenter.y(), 2));

                if (dist < GlobalMinDist) {
                    collision = true;
                    break;
                }
            }
        }

        if (collision) {
            setOpacity(0.5);
            setCursor(Qt::ForbiddenCursor);
        }
        else {
            setOpacity(1.0);
            setCursor(Qt::OpenHandCursor);
        }
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}