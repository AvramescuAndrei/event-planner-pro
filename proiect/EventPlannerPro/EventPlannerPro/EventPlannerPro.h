#pragma once

#include <QtWidgets/QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include "TableItem.h"

class EventPlannerPro : public QMainWindow
{
    Q_OBJECT

public:
    EventPlannerPro(QWidget* parent = nullptr);
    ~EventPlannerPro();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QWidget* centralWidget;
    QHBoxLayout* mainLayout;
    QWidget* sidePanel;
    QVBoxLayout* sideLayout;

    QSpinBox* spinMinDist;
    QSpinBox* spinMaxGuests;

    QLineEdit* txtNumeInvitat;
    QPushButton* btnAdaugaInvitat;
    QPushButton* btnImport;

    QLabel* lblInAsteptare;
    QListWidget* listWaiting;

    QLabel* lblAsezati;
    QListWidget* listSeated;

    QLabel* lblControls;
    QPushButton* btnAddTable;
    QPushButton* btnSave;
    QPushButton* btnLoad;

    QGraphicsView* canvasView;
    QGraphicsScene* scene;

    bool projectModified;

    void initUI();
    QPointF getNextFreePosition();
    int getNextFreeTableID();
    bool checkUnsavedChanges();
    bool guestExists(const QString& name);

private slots:
    void onAddTable();
    void onAddGuest();
    void onImportGuests();

    void onSaveProject();
    void onLoadProject();
    void onMinDistChanged(int val);

    void onGuestSeated(QString name, int tableId);
    void onGuestUnseated(QString name);
    void onTableDeleted(TableItem* table);

    void onListContextMenu(const QPoint& pos);
};