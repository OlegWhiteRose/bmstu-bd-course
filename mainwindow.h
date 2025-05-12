#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>

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
    void addRecord();     // Для кнопки Add
    void editRecord();    // Для кнопки Edit
    void deleteRecord();  // Для кнопки Del
    void onTableSelectionChanged(); // Новый слот для обработки выбора строки

private:
    Ui::MainWindow *ui;
    QSqlDatabase dbconn;
};

#endif // MAINWINDOW_H
