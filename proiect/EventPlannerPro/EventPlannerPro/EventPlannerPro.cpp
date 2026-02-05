#include "EventPlannerPro.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtMath> 
#include <QCloseEvent>
#include <QMenu>

EventPlannerPro::EventPlannerPro(QWidget* parent)
    : QMainWindow(parent), projectModified(false)
{
    setWindowTitle("EventPlanner Pro");
    resize(1200, 800);
    initUI();
}

EventPlannerPro::~EventPlannerPro() {}

void EventPlannerPro::initUI()
{
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    mainLayout = new QHBoxLayout(centralWidget);

    sidePanel = new QWidget(this);
    sidePanel->setFixedWidth(300);
    sideLayout = new QVBoxLayout(sidePanel);

    sideLayout->addWidget(new QLabel("Distanta Minima (Mese):"));
    spinMinDist = new QSpinBox(this);
    spinMinDist->setRange(50, 500);
    spinMinDist->setValue(130);
    connect(spinMinDist, QOverload<int>::of(&QSpinBox::valueChanged), this, &EventPlannerPro::onMinDistChanged);
    sideLayout->addWidget(spinMinDist);

    sideLayout->addWidget(new QLabel("Max Persoane / Masa:"));
    spinMaxGuests = new QSpinBox(this);
    spinMaxGuests->setRange(1, 20);
    spinMaxGuests->setValue(12);
    sideLayout->addWidget(spinMaxGuests);

    sideLayout->addSpacing(10);
    sideLayout->addWidget(new QLabel("Adaugare Invitat"));

    txtNumeInvitat = new QLineEdit(this);
    txtNumeInvitat->setPlaceholderText("Nume Invitat...");
    sideLayout->addWidget(txtNumeInvitat);

    QHBoxLayout* addLayout = new QHBoxLayout();
    btnAdaugaInvitat = new QPushButton("Adauga", this);
    connect(btnAdaugaInvitat, &QPushButton::clicked, this, &EventPlannerPro::onAddGuest);

    btnImport = new QPushButton("Importare Lista", this);
    connect(btnImport, &QPushButton::clicked, this, &EventPlannerPro::onImportGuests);

    addLayout->addWidget(btnAdaugaInvitat);
    addLayout->addWidget(btnImport);
    sideLayout->addLayout(addLayout);

    lblInAsteptare = new QLabel("In Asteptare:", this);
    listWaiting = new QListWidget(this);
    listWaiting->setDragEnabled(true);
    listWaiting->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listWaiting, &QListWidget::customContextMenuRequested, this, &EventPlannerPro::onListContextMenu);

    sideLayout->addWidget(lblInAsteptare);
    sideLayout->addWidget(listWaiting);

    lblAsezati = new QLabel("Asezati:", this);
    listSeated = new QListWidget(this);
    listSeated->setSelectionMode(QAbstractItemView::NoSelection);
    listSeated->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listSeated, &QListWidget::customContextMenuRequested, this, &EventPlannerPro::onListContextMenu);

    sideLayout->addWidget(lblAsezati);
    sideLayout->addWidget(listSeated);

    btnAddTable = new QPushButton("Adauga Masa Noua", this);
    connect(btnAddTable, &QPushButton::clicked, this, &EventPlannerPro::onAddTable);

    sideLayout->addSpacing(20);
    sideLayout->addWidget(btnAddTable);
    sideLayout->addStretch();

    btnSave = new QPushButton("Salveaza Proiect", this);
    connect(btnSave, &QPushButton::clicked, this, &EventPlannerPro::onSaveProject);

    btnLoad = new QPushButton("Incarca Proiect", this);
    connect(btnLoad, &QPushButton::clicked, this, &EventPlannerPro::onLoadProject);

    sideLayout->addWidget(btnSave);
    sideLayout->addWidget(btnLoad);

    scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, 2000, 2000);
    canvasView = new QGraphicsView(scene, this);
    canvasView->setRenderHint(QPainter::Antialiasing);

    canvasView->setMouseTracking(true);
    canvasView->ensureVisible(0, 0, 1, 1);

    mainLayout->addWidget(sidePanel);
    mainLayout->addWidget(canvasView);
}

void EventPlannerPro::closeEvent(QCloseEvent* event)
{
    if (checkUnsavedChanges()) {
        event->accept();
    }
    else {
        event->ignore();
    }
}

bool EventPlannerPro::checkUnsavedChanges()
{
    if (!projectModified) return true;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Salvare Proiect",
        "Doresti sa salvezi acest aranjament?",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    if (reply == QMessageBox::Yes) {
        onSaveProject();
        return !projectModified;
    }
    else if (reply == QMessageBox::No) {
        return true;
    }
    else {
        return false;
    }
}

// Functie noua pentru verificarea duplicatelor
bool EventPlannerPro::guestExists(const QString& name)
{
    if (!listWaiting->findItems(name, Qt::MatchExactly).isEmpty()) return true;

    for (int i = 0; i < listSeated->count(); ++i) {
        QString text = listSeated->item(i)->text();
        int idx = text.lastIndexOf(" (MASA ");
        if (idx != -1) {
            QString seatedName = text.left(idx);
            if (seatedName == name) return true;
        }
    }
    return false;
}

void EventPlannerPro::onMinDistChanged(int val)
{
    TableItem::GlobalMinDist = val;
    projectModified = true;
}

void EventPlannerPro::onSaveProject()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Salveaza Proiect", "", "JSON Files (*.json)");
    if (fileName.isEmpty()) return;

    QJsonObject rootObj;
    rootObj["min_dist"] = spinMinDist->value();
    rootObj["max_guests_per_table"] = spinMaxGuests->value();

    QJsonArray waitingArray;
    for (int i = 0; i < listWaiting->count(); ++i) {
        waitingArray.append(listWaiting->item(i)->text());
    }
    rootObj["waiting_list"] = waitingArray;

    QJsonArray tablesArray;
    QList<QGraphicsItem*> items = scene->items();
    for (QGraphicsItem* item : items) {
        TableItem* table = dynamic_cast<TableItem*>(item);
        if (table) {
            QJsonObject tableObj;
            tableObj["id"] = table->getId();
            tableObj["x"] = table->x();
            tableObj["y"] = table->y();
            tableObj["chairs_count"] = table->getMaxChairs();

            tableObj["guests"] = QJsonArray::fromStringList(table->getGuestsData());

            tablesArray.append(tableObj);
        }
    }
    rootObj["tables"] = tablesArray;

    QJsonDocument doc(rootObj);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        projectModified = false;
        QMessageBox::information(this, "Succes", "Proiect salvat cu succes!");
    }
}

void EventPlannerPro::onLoadProject()
{
    if (!checkUnsavedChanges()) return;

    QString fileName = QFileDialog::getOpenFileName(this, "Incarca Proiect", "", "JSON Files (*.json)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Eroare", "Nu am putut deschide fisierul!");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) return;

    QJsonObject rootObj = doc.object();

    if (rootObj.contains("min_dist")) {
        spinMinDist->setValue(rootObj["min_dist"].toInt());
    }
    if (rootObj.contains("max_guests_per_table")) {
        spinMaxGuests->setValue(rootObj["max_guests_per_table"].toInt());
    }

    scene->clear();
    listWaiting->clear();
    listSeated->clear();

    QJsonArray waitingArray = rootObj["waiting_list"].toArray();
    for (auto val : waitingArray) {
        listWaiting->addItem(val.toString());
    }

    QJsonArray tablesArray = rootObj["tables"].toArray();
    for (auto val : tablesArray) {
        QJsonObject tableObj = val.toObject();

        int id = tableObj["id"].toInt();
        int chairsCount = tableObj["chairs_count"].toInt();
        double x = tableObj["x"].toDouble();
        double y = tableObj["y"].toDouble();

        TableItem* table = new TableItem(id, chairsCount, 0, 0, 100, 100);
        table->setPos(x, y);

        connect(table, &TableItem::guestSeated, this, &EventPlannerPro::onGuestSeated);
        connect(table, &TableItem::guestUnseated, this, &EventPlannerPro::onGuestUnseated);
        connect(table, &TableItem::tableDeleted, this, &EventPlannerPro::onTableDeleted);

        QStringList guestList;
        for (auto g : tableObj["guests"].toArray()) guestList << g.toString();
        table->setGuestsData(guestList);

        for (const QString& g : guestList) {
            if (!g.isEmpty()) {
                QString entry = g + " (MASA " + QString::number(id) + ")";
                listSeated->addItem(entry);
            }
        }

        scene->addItem(table);
    }
    projectModified = false;
}

QPointF EventPlannerPro::getNextFreePosition()
{
    int step = 100 + TableItem::GlobalMinDist;
    int x = 50;
    int y = 50;
    int maxWidth = 1000;

    while (true) {
        QRectF rectCheck(x, y, 100, 100);
        bool collision = false;
        QList<QGraphicsItem*> items = scene->items();

        QPointF myCenter(x + 50, y + 50);

        for (auto item : items) {
            if (dynamic_cast<TableItem*>(item)) {
                QPointF otherPos = item->pos();
                QPointF otherCenter = otherPos + QPointF(50, 50);
                qreal dist = qSqrt(qPow(myCenter.x() - otherCenter.x(), 2) +
                    qPow(myCenter.y() - otherCenter.y(), 2));

                if (dist < TableItem::GlobalMinDist) {
                    collision = true;
                    break;
                }
            }
        }

        if (!collision) return QPointF(x, y);

        x += step;
        if (x > maxWidth) {
            x = 50;
            y += step;
        }
        if (y > 2000) return QPointF(50, 50);
    }
}

int EventPlannerPro::getNextFreeTableID()
{
    int id = 1;
    while (true) {
        bool exists = false;
        QList<QGraphicsItem*> items = scene->items();
        for (auto item : items) {
            TableItem* tbl = dynamic_cast<TableItem*>(item);
            if (tbl && tbl->getId() == id) {
                exists = true;
                break;
            }
        }
        if (!exists) return id;
        id++;
    }
}

void EventPlannerPro::onAddGuest()
{
    QString txt = txtNumeInvitat->text().trimmed();
    if (!txt.isEmpty()) {
        if (guestExists(txt)) {
            QMessageBox::warning(this, "Eroare", "Invitatul exista deja!");
            return;
        }
        listWaiting->addItem(txt);
        txtNumeInvitat->clear();
        projectModified = true;
    }
}

void EventPlannerPro::onImportGuests()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Selecteaza Fisier Invitati", "", "Text Files (*.txt)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Eroare", "Nu am putut deschide fisierul!");
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty() && !guestExists(line)) {
            listWaiting->addItem(line);
        }
    }
    file.close();
    projectModified = true;
}

void EventPlannerPro::onAddTable()
{
    bool ok;
    int nr = QInputDialog::getInt(this, "Masa Noua", "Nr Scaune:", 4, 1, 12, 1, &ok);
    if (!ok) return;

    int maxAllowed = spinMaxGuests->value();
    if (nr > maxAllowed) {
        QString msg = QString("Din cauza distantarii sociale, nr maxim de oameni la o masa este: %1").arg(maxAllowed);
        QMessageBox::warning(this, "Limitare", msg);
        return;
    }

    int newId = getNextFreeTableID();
    QPointF pos = getNextFreePosition();

    TableItem* table = new TableItem(newId, nr, 0, 0, 100, 100);
    table->setPos(pos);

    connect(table, &TableItem::guestSeated, this, &EventPlannerPro::onGuestSeated);
    connect(table, &TableItem::guestUnseated, this, &EventPlannerPro::onGuestUnseated);
    connect(table, &TableItem::tableDeleted, this, &EventPlannerPro::onTableDeleted);

    scene->addItem(table);
    projectModified = true;
}

void EventPlannerPro::onGuestSeated(QString name, int tableId)
{
    QList<QListWidgetItem*> items = listWaiting->findItems(name, Qt::MatchExactly);

    if (items.isEmpty()) {
        QListWidgetItem* current = listWaiting->currentItem();
        if (current && current->text() == name) {
            delete listWaiting->takeItem(listWaiting->row(current));
        }
    }
    else {
        for (auto item : items) delete listWaiting->takeItem(listWaiting->row(item));
    }

    QString tableStr = QString::number(tableId);
    QString entry = name + " (MASA " + tableStr + ")";

    listSeated->addItem(entry);
    projectModified = true;
}

void EventPlannerPro::onGuestUnseated(QString name)
{
    for (int i = 0; i < listSeated->count(); ++i) {
        QListWidgetItem* item = listSeated->item(i);
        QString text = item->text();

        if (text.startsWith(name + " (MASA")) {
            delete listSeated->takeItem(i);
            break;
        }
    }

    listWaiting->addItem(name);
    projectModified = true;
}

void EventPlannerPro::onTableDeleted(TableItem* table)
{
    projectModified = true;
}

void EventPlannerPro::onListContextMenu(const QPoint& pos)
{
    QListWidget* listWidget = qobject_cast<QListWidget*>(sender());
    if (!listWidget) return;

    QListWidgetItem* item = listWidget->itemAt(pos);
    if (!item) return;

    QMenu menu;
    QAction* deleteAction = menu.addAction("Sterge invitat");
    QAction* selected = menu.exec(listWidget->mapToGlobal(pos));

    if (selected == deleteAction) {
        QString text = item->text();

        if (listWidget == listWaiting) {
            delete listWidget->takeItem(listWidget->row(item));
            projectModified = true;
        }
        else if (listWidget == listSeated) {
            int openParen = text.lastIndexOf(" (MASA ");
            if (openParen != -1) {
                QString name = text.left(openParen);
                QString idPart = text.mid(openParen + 7);
                idPart.chop(1);
                int tableId = idPart.toInt();

                QList<QGraphicsItem*> items = scene->items();
                for (QGraphicsItem* sceneItem : items) {
                    TableItem* table = dynamic_cast<TableItem*>(sceneItem);
                    if (table && table->getId() == tableId) {
                        table->removeGuestByName(name);
                        break;
                    }
                }
            }
        }
    }
}