#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include "secondform.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void dbconnect();
    void selectAll();
    void addClientRecord();
    void editClientRecord();
    void deleteClientRecord();
    void addDocumentRecord();
    void editDocumentRecord();
    void deleteDocumentRecord();
    void onClientTableSelectionChanged();
    void onDocumentTableSelectionChanged();
    void saveChanges();
    void openSecondForm();

private:
    Ui::MainWindow *ui;
    QSqlDatabase dbconn;
    SecondForm *secondForm;
};

#endif // MAINWINDOW_H
