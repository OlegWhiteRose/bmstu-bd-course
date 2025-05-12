#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Подключаем кнопку "Connect" к слоту dbconnect
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::dbconnect);

    // Подключаем кнопку "Select All" к слоту selectAll
    connect(ui->actionSelectAll, &QAction::triggered, this, &MainWindow::selectAll);

    // Подключаем кнопки Add, Edit, Del
    connect(ui->btnAdd, &QPushButton::clicked, this, &MainWindow::addRecord);
    connect(ui->btnEdit, &QPushButton::clicked, this, &MainWindow::editRecord);
    connect(ui->btnDel, &QPushButton::clicked, this, &MainWindow::deleteRecord);

    // Подключаем сигнал выбора строки в таблице
    connect(ui->dataTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onTableSelectionChanged);

    // Настраиваем свойства таблицы
    ui->dataTable->setColumnCount(3);
    ui->dataTable->setHorizontalHeaderLabels({"Имя", "Фамилия", "Телефон"});
    ui->dataTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->dataTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->dataTable->horizontalHeader()->setStretchLastSection(true);
    ui->dataTable->setSortingEnabled(true);
    ui->dataTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

MainWindow::~MainWindow()
{
    if (dbconn.isOpen())
        dbconn.close();
    delete ui;
}

void MainWindow::dbconnect()
{
    if (!dbconn.isOpen())
    {
        ui->labelSqlDrivers->setText("Драйверы SQL: " + QSqlDatabase::drivers().join(","));
        dbconn = QSqlDatabase::addDatabase("QPSQL");
        dbconn.setDatabaseName("bank");
        dbconn.setHostName("localhost");
        dbconn.setPort(5433);
        dbconn.setUserName("postgres");
        dbconn.setPassword("1234");

        if (dbconn.open())
        {
            ui->labelRows->setText("Подключение успешно открыто...");
        }
        else
        {
            ui->labelRows->setText("Ошибка подключения: " + dbconn.lastError().text());
        }
    }
    else
    {
        ui->labelRows->setText("Подключение уже открыто...");
    }
}

void MainWindow::selectAll()
{
    if (!dbconn.isOpen())
    {
        dbconnect();
        if (!dbconn.isOpen())
        {
            QMessageBox::critical(this, "Ошибка", dbconn.lastError().text());
            return;
        }
    }

    QSqlQuery query(dbconn);
    if (!query.exec("SELECT first_name, last_name, phone FROM Client"))
    {
        QMessageBox::critical(this, "Ошибка", query.lastError().text());
        return;
    }

    // Очищаем таблицу
    ui->dataTable->clearContents();
    ui->dataTable->setRowCount(0);

    int row = 0;
    while (query.next())
    {
        ui->dataTable->insertRow(row);
        QString firstName = query.value("first_name").toString();
        QString lastName = query.value("last_name").toString();
        QString phone = query.value("phone").toString();

        // Отладочный вывод
        qDebug() << "Row" << row << "First Name:" << firstName << "Last Name:" << lastName << "Phone:" << phone;

        ui->dataTable->setItem(row, 0, new QTableWidgetItem(firstName));
        ui->dataTable->setItem(row, 1, new QTableWidgetItem(lastName));
        ui->dataTable->setItem(row, 2, new QTableWidgetItem(phone));
        row++;
    }

    ui->labelRows->setText(QString("Прочитано %1 строк").arg(row));
    ui->dataTable->resizeColumnsToContents();
}

void MainWindow::addRecord()
{
    if (!dbconn.isOpen())
    {
        dbconnect();
        if (!dbconn.isOpen())
        {
            QMessageBox::critical(this, "Ошибка", "Не удалось подключиться к базе данных!");
            return;
        }
    }

    QString firstName = ui->firstNameEdit->text().trimmed();
    QString lastName = ui->lastNameEdit->text().trimmed();
    QString phone = ui->phoneEdit->text().trimmed();

    if (firstName.isEmpty() || lastName.isEmpty() || phone.isEmpty())
    {
        QMessageBox::warning(this, "Предупреждение", "Все поля должны быть заполнены!");
        return;
    }

    QSqlQuery query(dbconn);
    QString sqlstr = "INSERT INTO Client (first_name, last_name, phone) VALUES (?, ?, ?)";
    query.prepare(sqlstr);
    query.addBindValue(firstName);
    query.addBindValue(lastName);
    query.addBindValue(phone);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось добавить запись: " + query.lastError().text());
        return;
    }

    selectAll();
    ui->firstNameEdit->clear();
    ui->lastNameEdit->clear();
    ui->phoneEdit->clear();
}

void MainWindow::editRecord()
{
    if (!dbconn.isOpen())
    {
        dbconnect();
        if (!dbconn.isOpen())
        {
            QMessageBox::critical(this, "Ошибка", "Не удалось подключиться к базе данных!");
            return;
        }
    }

    int currentRow = ui->dataTable->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this, "Предупреждение", "Выберите запись для редактирования!");
        return;
    }

    QString firstName = ui->firstNameEdit->text().trimmed();
    QString lastName = ui->lastNameEdit->text().trimmed();
    QString phone = ui->phoneEdit->text().trimmed();

    if (firstName.isEmpty() || lastName.isEmpty() || phone.isEmpty())
    {
        QMessageBox::warning(this, "Предупреждение", "Все поля должны быть заполнены!");
        return;
    }

    QTableWidgetItem *item0 = ui->dataTable->item(currentRow, 0);
    QTableWidgetItem *item1 = ui->dataTable->item(currentRow, 1);
    QTableWidgetItem *item2 = ui->dataTable->item(currentRow, 2);
    if (!item0 || !item1 || !item2)
    {
        QMessageBox::warning(this, "Ошибка", "Некорректные данные в таблице!");
        return;
    }
    QString originalFirstName = item0->text();
    QString originalLastName = item1->text();
    QString originalPhone = item2->text();

    QSqlQuery query(dbconn);
    QString sqlstr = "UPDATE Client SET first_name = ?, last_name = ?, phone = ? WHERE first_name = ? AND last_name = ? AND phone = ?";
    query.prepare(sqlstr);
    query.addBindValue(firstName);
    query.addBindValue(lastName);
    query.addBindValue(phone);
    query.addBindValue(originalFirstName);
    query.addBindValue(originalLastName);
    query.addBindValue(originalPhone);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось обновить запись: " + query.lastError().text());
        return;
    }

    selectAll();
    ui->firstNameEdit->clear();
    ui->lastNameEdit->clear();
    ui->phoneEdit->clear();
}

void MainWindow::deleteRecord()
{
    if (!dbconn.isOpen())
    {
        dbconnect();
        if (!dbconn.isOpen())
        {
            QMessageBox::critical(this, "Ошибка", "Не удалось подключиться к базе данных!");
            return;
        }
    }

    int currentRow = ui->dataTable->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this, "Предупреждение", "Выберите запись для удаления!");
        return;
    }

    QTableWidgetItem *item0 = ui->dataTable->item(currentRow, 0);
    QTableWidgetItem *item1 = ui->dataTable->item(currentRow, 1);
    QTableWidgetItem *item2 = ui->dataTable->item(currentRow, 2);
    if (!item0 || !item1 || !item2)
    {
        QMessageBox::warning(this, "Ошибка", "Некорректные данные в таблице!");
        return;
    }
    QString firstName = item0->text();
    QString lastName = item1->text();
    QString phone = item2->text();

    QSqlQuery query(dbconn);
    QString sqlstr = "DELETE FROM Client WHERE first_name = ? AND last_name = ? AND phone = ?";
    query.prepare(sqlstr);
    query.addBindValue(firstName);
    query.addBindValue(lastName);
    query.addBindValue(phone);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось удалить запись: " + query.lastError().text());
        return;
    }

    selectAll();
    ui->firstNameEdit->clear();
    ui->lastNameEdit->clear();
    ui->phoneEdit->clear();
}

void MainWindow::onTableSelectionChanged()
{
    int currentRow = ui->dataTable->currentRow();
    if (currentRow >= 0)
    {
        QTableWidgetItem *firstNameItem = ui->dataTable->item(currentRow, 0);
        QTableWidgetItem *lastNameItem = ui->dataTable->item(currentRow, 1);
        QTableWidgetItem *phoneItem = ui->dataTable->item(currentRow, 2);

        ui->firstNameEdit->setText(firstNameItem ? firstNameItem->text() : "");
        ui->lastNameEdit->setText(lastNameItem ? lastNameItem->text() : "");
        ui->phoneEdit->setText(phoneItem ? phoneItem->text() : "");
    }
}