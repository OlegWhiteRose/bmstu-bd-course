#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    secondForm(nullptr)
{
    ui->setupUi(this);

    // Подключаем кнопку "Connect" к слоту dbconnect
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::dbconnect);

    // Подключаем кнопку "Select All" к слоту selectAll
    connect(ui->actionSelectAll, &QAction::triggered, this, &MainWindow::selectAll);

    // Подключаем кнопки для таблицы Client
    connect(ui->btnAddClient, &QPushButton::clicked, this, &MainWindow::addClientRecord);
    connect(ui->btnEditClient, &QPushButton::clicked, this, &MainWindow::editClientRecord);
    connect(ui->btnDelClient, &QPushButton::clicked, this, &MainWindow::deleteClientRecord);

    // Подключаем кнопки для таблицы Document
    connect(ui->btnAddDocument, &QPushButton::clicked, this, &MainWindow::addDocumentRecord);
    connect(ui->btnEditDocument, &QPushButton::clicked, this, &MainWindow::editDocumentRecord);
    connect(ui->btnDelDocument, &QPushButton::clicked, this, &MainWindow::deleteDocumentRecord);

    // Подключаем сигналы выбора строк
    connect(ui->clientTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onClientTableSelectionChanged);
    connect(ui->documentTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::onDocumentTableSelectionChanged);

    // Подключаем кнопки Save и перехода
    connect(ui->btnSave, &QPushButton::clicked, this, &MainWindow::saveChanges);
    connect(ui->btnToForm2, &QPushButton::clicked, this, &MainWindow::openSecondForm);

    // Настраиваем таблицу Client
    ui->clientTable->setColumnCount(4);
    ui->clientTable->setHorizontalHeaderLabels({"ID", "Имя", "Фамилия", "Телефон"});
    ui->clientTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->clientTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->clientTable->horizontalHeader()->setStretchLastSection(true);
    ui->clientTable->setSortingEnabled(true);
    ui->clientTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Настраиваем таблицу Document
    ui->documentTable->setColumnCount(7);
    ui->documentTable->setHorizontalHeaderLabels({"ID", "Номер паспорта", "Дата рождения", "Пол", "Дата соглашения", "Секретное слово", "Статус"});
    ui->documentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->documentTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->documentTable->horizontalHeader()->setStretchLastSection(true);
    ui->documentTable->setSortingEnabled(true);
    ui->documentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

MainWindow::~MainWindow()
{
    if (dbconn.isOpen())
        dbconn.close();
    delete secondForm;
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

    // Загружаем таблицу Client
    QSqlQuery clientQuery(dbconn);
    if (!clientQuery.exec("SELECT id, first_name, last_name, phone FROM Client"))
    {
        QMessageBox::critical(this, "Ошибка", clientQuery.lastError().text());
        return;
    }

    ui->clientTable->clearContents();
    ui->clientTable->setRowCount(0);

    int row = 0;
    while (clientQuery.next())
    {
        ui->clientTable->insertRow(row);
        ui->clientTable->setItem(row, 0, new QTableWidgetItem(clientQuery.value("id").toString()));
        ui->clientTable->setItem(row, 1, new QTableWidgetItem(clientQuery.value("first_name").toString()));
        ui->clientTable->setItem(row, 2, new QTableWidgetItem(clientQuery.value("last_name").toString()));
        ui->clientTable->setItem(row, 3, new QTableWidgetItem(clientQuery.value("phone").toString()));
        row++;
    }

    ui->labelRows->setText(QString("Прочитано %1 клиентов").arg(row));
    ui->clientTable->resizeColumnsToContents();

    // Загружаем таблицу Document для выбранного клиента
    onClientTableSelectionChanged();
}

void MainWindow::addClientRecord()
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

    QString firstName = ui->clientFirstNameEdit->text().trimmed();
    QString lastName = ui->clientLastNameEdit->text().trimmed();
    QString phone = ui->clientPhoneEdit->text().trimmed();

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
        QMessageBox::critical(this, "Ошибка", "Не удалось добавить клиента: " + query.lastError().text());
        return;
    }

    selectAll();
    ui->clientFirstNameEdit->clear();
    ui->clientLastNameEdit->clear();
    ui->clientPhoneEdit->clear();
}

void MainWindow::editClientRecord()
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

    int currentRow = ui->clientTable->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this, "Предупреждение", "Выберите клиента для редактирования!");
        return;
    }

    QString firstName = ui->clientFirstNameEdit->text().trimmed();
    QString lastName = ui->clientLastNameEdit->text().trimmed();
    QString phone = ui->clientPhoneEdit->text().trimmed();

    if (firstName.isEmpty() || lastName.isEmpty() || phone.isEmpty())
    {
        QMessageBox::warning(this, "Предупреждение", "Все поля должны быть заполнены!");
        return;
    }

    QTableWidgetItem *item0 = ui->clientTable->item(currentRow, 0);
    if (!item0)
    {
        QMessageBox::warning(this, "Ошибка", "Некорректные данные в таблице!");
        return;
    }
    int clientId = item0->text().toInt();

    QSqlQuery query(dbconn);
    QString sqlstr = "UPDATE Client SET first_name = ?, last_name = ?, phone = ? WHERE id = ?";
    query.prepare(sqlstr);
    query.addBindValue(firstName);
    query.addBindValue(lastName);
    query.addBindValue(phone);
    query.addBindValue(clientId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось обновить клиента: " + query.lastError().text());
        return;
    }

    selectAll();
    ui->clientFirstNameEdit->clear();
    ui->clientLastNameEdit->clear();
    ui->clientPhoneEdit->clear();
}

void MainWindow::deleteClientRecord()
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

    int currentRow = ui->clientTable->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this, "Предупреждение", "Выберите клиента для удаления!");
        return;
    }

    QTableWidgetItem *item0 = ui->clientTable->item(currentRow, 0);
    if (!item0)
    {
        QMessageBox::warning(this, "Ошибка", "Некорректные данные в таблице!");
        return;
    }
    int clientId = item0->text().toInt();

    QSqlQuery query(dbconn);
    QString sqlstr = "DELETE FROM Client WHERE id = ?";
    query.prepare(sqlstr);
    query.addBindValue(clientId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось удалить клиента: " + query.lastError().text());
        return;
    }

    selectAll();
    ui->clientFirstNameEdit->clear();
    ui->clientLastNameEdit->clear();
    ui->clientPhoneEdit->clear();
}

void MainWindow::addDocumentRecord()
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

    int currentClientRow = ui->clientTable->currentRow();
    if (currentClientRow < 0)
    {
        QMessageBox::warning(this, "Предупреждение", "Выберите клиента для добавления документа!");
        return;
    }

    QTableWidgetItem *clientItem = ui->clientTable->item(currentClientRow, 0);
    if (!clientItem)
    {
        QMessageBox::warning(this, "Ошибка", "Некорректные данные клиента!");
        return;
    }
    int clientId = clientItem->text().toInt();

    QString passportNumber = ui->docPassportEdit->text().trimmed();
    QString birthDate = ui->docBirthDateEdit->text().trimmed();
    QString gender = ui->docGenderEdit->text().trimmed();
    QString securityWord = ui->docSecurityWordEdit->text().trimmed();
    QString agreementStatus = ui->docStatusEdit->text().trimmed();

    if (passportNumber.isEmpty() || birthDate.isEmpty() || gender.isEmpty() || securityWord.isEmpty() || agreementStatus.isEmpty())
    {
        QMessageBox::warning(this, "Предупреждение", "Все поля должны быть заполнены!");
        return;
    }

    QSqlQuery query(dbconn);
    QString sqlstr = "INSERT INTO Document (passport_number, birth_date, gender, client_id, agreement_date, security_word, agreement_status) "
                     "VALUES (?, ?, ?, ?, CURRENT_TIMESTAMP, ?, ?)";
    query.prepare(sqlstr);
    query.addBindValue(passportNumber);
    query.addBindValue(birthDate);
    query.addBindValue(gender);
    query.addBindValue(clientId);
    query.addBindValue(securityWord);
    query.addBindValue(agreementStatus);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось добавить документ: " + query.lastError().text());
        return;
    }

    onClientTableSelectionChanged();
    ui->docPassportEdit->clear();
    ui->docBirthDateEdit->clear();
    ui->docGenderEdit->clear();
    ui->docSecurityWordEdit->clear();
    ui->docStatusEdit->clear();
}

void MainWindow::editDocumentRecord()
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

    int currentRow = ui->documentTable->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this, "Предупреждение", "Выберите документ для редактирования!");
        return;
    }

    QString passportNumber = ui->docPassportEdit->text().trimmed();
    QString birthDate = ui->docBirthDateEdit->text().trimmed();
    QString gender = ui->docGenderEdit->text().trimmed();
    QString securityWord = ui->docSecurityWordEdit->text().trimmed();
    QString agreementStatus = ui->docStatusEdit->text().trimmed();

    if (passportNumber.isEmpty() || birthDate.isEmpty() || gender.isEmpty() || securityWord.isEmpty() || agreementStatus.isEmpty())
    {
        QMessageBox::warning(this, "Предупреждение", "Все поля должны быть заполнены!");
        return;
    }

    QTableWidgetItem *item0 = ui->documentTable->item(currentRow, 0);
    if (!item0)
    {
        QMessageBox::warning(this, "Ошибка", "Некорректные данные в таблице!");
        return;
    }
    int documentId = item0->text().toInt();

    QSqlQuery query(dbconn);
    QString sqlstr = "UPDATE Document SET passport_number = ?, birth_date = ?, gender = ?, security_word = ?, agreement_status = ? WHERE id = ?";
    query.prepare(sqlstr);
    query.addBindValue(passportNumber);
    query.addBindValue(birthDate);
    query.addBindValue(gender);
    query.addBindValue(securityWord);
    query.addBindValue(agreementStatus);
    query.addBindValue(documentId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось обновить документ: " + query.lastError().text());
        return;
    }

    onClientTableSelectionChanged();
    ui->docPassportEdit->clear();
    ui->docBirthDateEdit->clear();
    ui->docGenderEdit->clear();
    ui->docSecurityWordEdit->clear();
    ui->docStatusEdit->clear();
}

void MainWindow::deleteDocumentRecord()
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

    int currentRow = ui->documentTable->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this, "Предупреждение", "Выберите документ для удаления!");
        return;
    }

    QTableWidgetItem *item0 = ui->documentTable->item(currentRow, 0);
    if (!item0)
    {
        QMessageBox::warning(this, "Ошибка", "Некорректные данные в таблице!");
        return;
    }
    int documentId = item0->text().toInt();

    QSqlQuery query(dbconn);
    QString sqlstr = "DELETE FROM Document WHERE id = ?";
    query.prepare(sqlstr);
    query.addBindValue(documentId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось удалить документ: " + query.lastError().text());
        return;
    }

    onClientTableSelectionChanged();
    ui->docPassportEdit->clear();
    ui->docBirthDateEdit->clear();
    ui->docGenderEdit->clear();
    ui->docSecurityWordEdit->clear();
    ui->docStatusEdit->clear();
}

void MainWindow::onClientTableSelectionChanged()
{
    int currentRow = ui->clientTable->currentRow();
    if (currentRow >= 0)
    {
        QTableWidgetItem *firstNameItem = ui->clientTable->item(currentRow, 1);
        QTableWidgetItem *lastNameItem = ui->clientTable->item(currentRow, 2);
        QTableWidgetItem *phoneItem = ui->clientTable->item(currentRow, 3);

        ui->clientFirstNameEdit->setText(firstNameItem ? firstNameItem->text() : "");
        ui->clientLastNameEdit->setText(lastNameItem ? lastNameItem->text() : "");
        ui->clientPhoneEdit->setText(phoneItem ? phoneItem->text() : "");

        // Загружаем связанные документы
        QTableWidgetItem *clientIdItem = ui->clientTable->item(currentRow, 0);
        if (!clientIdItem)
            return;
        int clientId = clientIdItem->text().toInt();

        QSqlQuery query(dbconn);
        QString sqlstr = "SELECT id, passport_number, birth_date, gender, agreement_date, security_word, agreement_status "
                         "FROM Document WHERE client_id = ?";
        query.prepare(sqlstr);
        query.addBindValue(clientId);

        if (!query.exec())
        {
            QMessageBox::critical(this, "Ошибка", query.lastError().text());
            return;
        }

        ui->documentTable->clearContents();
        ui->documentTable->setRowCount(0);

        int docRow = 0;
        while (query.next())
        {
            ui->documentTable->insertRow(docRow);
            ui->documentTable->setItem(docRow, 0, new QTableWidgetItem(query.value("id").toString()));
            ui->documentTable->setItem(docRow, 1, new QTableWidgetItem(query.value("passport_number").toString()));
            ui->documentTable->setItem(docRow, 2, new QTableWidgetItem(query.value("birth_date").toString()));
            ui->documentTable->setItem(docRow, 3, new QTableWidgetItem(query.value("gender").toString()));
            ui->documentTable->setItem(docRow, 4, new QTableWidgetItem(query.value("agreement_date").toString()));
            ui->documentTable->setItem(docRow, 5, new QTableWidgetItem(query.value("security_word").toString()));
            ui->documentTable->setItem(docRow, 6, new QTableWidgetItem(query.value("agreement_status").toString()));
            docRow++;
        }

        ui->documentTable->resizeColumnsToContents();
    }
}

void MainWindow::onDocumentTableSelectionChanged()
{
    int currentRow = ui->documentTable->currentRow();
    if (currentRow >= 0)
    {
        QTableWidgetItem *passportItem = ui->documentTable->item(currentRow, 1);
        QTableWidgetItem *birthDateItem = ui->documentTable->item(currentRow, 2);
        QTableWidgetItem *genderItem = ui->documentTable->item(currentRow, 3);
        QTableWidgetItem *securityWordItem = ui->documentTable->item(currentRow, 5);
        QTableWidgetItem *statusItem = ui->documentTable->item(currentRow, 6);

        ui->docPassportEdit->setText(passportItem ? passportItem->text() : "");
        ui->docBirthDateEdit->setText(birthDateItem ? birthDateItem->text() : "");
        ui->docGenderEdit->setText(genderItem ? genderItem->text() : "");
        ui->docSecurityWordEdit->setText(securityWordItem ? securityWordItem->text() : "");
        ui->docStatusEdit->setText(statusItem ? statusItem->text() : "");
    }
}

void MainWindow::saveChanges()
{
    selectAll();
    QMessageBox::information(this, "Успех", "Изменения сохранены!");
}

void MainWindow::openSecondForm()
{
    int currentRow = ui->clientTable->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this, "Предупреждение", "Выберите клиента для просмотра транзакций!");
        return;
    }

    QTableWidgetItem *clientIdItem = ui->clientTable->item(currentRow, 0);
    if (!clientIdItem)
    {
        QMessageBox::warning(this, "Ошибка", "Некорректные данные клиента!");
        return;
    }
    int clientId = clientIdItem->text().toInt();

    if (!secondForm)
    {
        secondForm = new SecondForm(this);
    }
    secondForm->setClientId(clientId);
    secondForm->loadTransactions();
    secondForm->show();
    this->hide();
}