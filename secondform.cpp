#include "secondform.h"
#include "ui_secondform.h"
#include "mainwindow.h"  // Добавили include
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDebug>


SecondForm::SecondForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SecondForm),
    currentClientId(-1)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window); 

    // Подключаем кнопки
    connect(ui->btnBack, &QPushButton::clicked, this, &SecondForm::onBackButtonClicked);
    connect(ui->btnFilter, &QPushButton::clicked, this, &SecondForm::onFilterButtonClicked);
    connect(ui->btnAddTransaction, &QPushButton::clicked, this, &SecondForm::onAddTransactionClicked);
    connect(ui->btnEditTransaction, &QPushButton::clicked, this, &SecondForm::onEditTransactionClicked);
    connect(ui->btnDeleteTransaction, &QPushButton::clicked, this, &SecondForm::onDeleteTransactionClicked);
    connect(ui->transactionTable, &QTableWidget::itemSelectionChanged, this, &SecondForm::onTransactionTableSelectionChanged);

    // Настраиваем таблицу транзакций
    ui->transactionTable->setColumnCount(5);
    ui->transactionTable->setHorizontalHeaderLabels({"ID", "ID Вклада", "Сумма", "Дата", "Тип"});
    ui->transactionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->transactionTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->transactionTable->horizontalHeader()->setStretchLastSection(true);
    ui->transactionTable->setSortingEnabled(true);
    ui->transactionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

SecondForm::~SecondForm()
{
    delete ui;
}

void SecondForm::setClientId(int clientId)
{
    currentClientId = clientId;
}

void SecondForm::loadTransactions()
{
    if (currentClientId == -1)
    {
        QMessageBox::warning(this, "Ошибка", "Клиент не выбран!");
        return;
    }

    dbconn = QSqlDatabase::database();
    if (!dbconn.isOpen())
    {
        QMessageBox::critical(this, "Ошибка", "База данных не подключена!");
        return;
    }

    QString dateFilter = ui->filterDateEdit->text().trimmed();
    QString amountFilter = ui->filterAmountEdit->text().trimmed();
    QString typeFilter = ui->filterTypeCombo->currentText();

    // Исправленный SQL-запрос с именованными параметрами
    QString sqlstr = "SELECT t.id, t.deposit_id, t.amount, t.date, t.type "
                     "FROM Transaction t "
                     "JOIN Deposit d ON t.deposit_id = d.id "
                     "WHERE d.client_id = :client_id";  // Заменили ? на :client_id

    // Добавляем фильтры
    QStringList conditions;
    if (!dateFilter.isEmpty())
        conditions << "t.date::date = :date";
    if (!amountFilter.isEmpty())
        conditions << "t.amount >= :amount";
    if (typeFilter != "Все")
        conditions << "t.type = :type";

    if (!conditions.isEmpty())
        sqlstr += " AND " + conditions.join(" AND ");

    QSqlQuery query(dbconn);
    query.prepare(sqlstr);
    
    // Корректная привязка параметров
    query.bindValue(":client_id", currentClientId); // Теперь параметр именованный
    
    if (!dateFilter.isEmpty())
        query.bindValue(":date", dateFilter);
    if (!amountFilter.isEmpty())
        query.bindValue(":amount", amountFilter.toDouble());
    if (typeFilter != "Все")
        query.bindValue(":type", typeFilter);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка", query.lastError().text());
        return;
    }

    ui->transactionTable->clearContents();
    ui->transactionTable->setRowCount(0);

    int row = 0;
    while (query.next())
    {
        ui->transactionTable->insertRow(row);
        ui->transactionTable->setItem(row, 0, new QTableWidgetItem(query.value("id").toString()));
        ui->transactionTable->setItem(row, 1, new QTableWidgetItem(query.value("deposit_id").toString()));
        ui->transactionTable->setItem(row, 2, new QTableWidgetItem(query.value("amount").toString()));
        ui->transactionTable->setItem(row, 3, new QTableWidgetItem(query.value("date").toString()));
        ui->transactionTable->setItem(row, 4, new QTableWidgetItem(query.value("type").toString()));
        row++;
    }

    ui->transactionTable->resizeColumnsToContents();
}

// Измененный метод возврата
void SecondForm::onBackButtonClicked()
{
    this->hide();
    emit returnToMainWindow();  // Используем сигнал вместо parentWidget()
}
void SecondForm::onFilterButtonClicked()
{
    loadTransactions();
}

void SecondForm::onAddTransactionClicked()
{
    if (currentClientId == -1)
    {
        QMessageBox::warning(this, "Ошибка", "Клиент не выбран!");
        return;
    }

    QString depositIdStr = ui->depositIdEdit->text().trimmed();
    QString amountStr = ui->amountEdit->text().trimmed();
    QString date = ui->dateEdit->text().trimmed();
    QString type = ui->typeCombo->currentText();

    if (depositIdStr.isEmpty() || amountStr.isEmpty() || date.isEmpty() || type.isEmpty())
    {
        QMessageBox::warning(this, "Предупреждение", "Все поля должны быть заполнены!");
        return;
    }

    int depositId = depositIdStr.toInt();
    double amount = amountStr.toDouble();

    if (amount <= 0)
    {
        QMessageBox::warning(this, "Предупреждение", "Сумма должна быть больше 0!");
        return;
    }

    // Проверяем, принадлежит ли deposit_id текущему клиенту
    QSqlQuery checkQuery(dbconn);
    checkQuery.prepare("SELECT client_id FROM Deposit WHERE id = ?");
    checkQuery.addBindValue(depositId);
    if (!checkQuery.exec() || !checkQuery.next())
    {
        QMessageBox::critical(this, "Ошибка", "Вклад не найден: " + checkQuery.lastError().text());
        return;
    }

    if (checkQuery.value("client_id").toInt() != currentClientId)
    {
        QMessageBox::warning(this, "Предупреждение", "Этот вклад не принадлежит текущему клиенту!");
        return;
    }

    QSqlQuery query(dbconn);
    QString sqlstr = "INSERT INTO Transaction (deposit_id, amount, date, type) VALUES (?, ?, ?, ?)";
    query.prepare(sqlstr);
    query.addBindValue(depositId);
    query.addBindValue(amount);
    query.addBindValue(date);
    query.addBindValue(type);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось добавить транзакцию: " + query.lastError().text());
        return;
    }

    loadTransactions();
    ui->depositIdEdit->clear();
    ui->amountEdit->clear();
    ui->dateEdit->clear();
}

void SecondForm::onEditTransactionClicked()
{
    int currentRow = ui->transactionTable->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this, "Предупреждение", "Выберите транзакцию для редактирования!");
        return;
    }

    QString depositIdStr = ui->depositIdEdit->text().trimmed();
    QString amountStr = ui->amountEdit->text().trimmed();
    QString date = ui->dateEdit->text().trimmed();
    QString type = ui->typeCombo->currentText();

    if (depositIdStr.isEmpty() || amountStr.isEmpty() || date.isEmpty() || type.isEmpty())
    {
        QMessageBox::warning(this, "Предупреждение", "Все поля должны быть заполнены!");
        return;
    }

    int depositId = depositIdStr.toInt();
    double amount = amountStr.toDouble();

    if (amount <= 0)
    {
        QMessageBox::warning(this, "Предупреждение", "Сумма должна быть больше 0!");
        return;
    }

    QTableWidgetItem *item0 = ui->transactionTable->item(currentRow, 0);
    if (!item0)
    {
        QMessageBox::warning(this, "Ошибка", "Некорректные данные в таблице!");
        return;
    }
    int transactionId = item0->text().toInt();

    // Проверяем, принадлежит ли deposit_id текущему клиенту
    QSqlQuery checkQuery(dbconn);
    checkQuery.prepare("SELECT client_id FROM Deposit WHERE id = ?");
    checkQuery.addBindValue(depositId);
    if (!checkQuery.exec() || !checkQuery.next())
    {
        QMessageBox::critical(this, "Ошибка", "Вклад не найден: " + checkQuery.lastError().text());
        return;
    }

    if (checkQuery.value("client_id").toInt() != currentClientId)
    {
        QMessageBox::warning(this, "Предупреждение", "Этот вклад не принадлежит текущему клиенту!");
        return;
    }

    QSqlQuery query(dbconn);
    QString sqlstr = "UPDATE Transaction SET deposit_id = ?, amount = ?, date = ?, type = ? WHERE id = ?";
    query.prepare(sqlstr);
    query.addBindValue(depositId);
    query.addBindValue(amount);
    query.addBindValue(date);
    query.addBindValue(type);
    query.addBindValue(transactionId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось обновить транзакцию: " + query.lastError().text());
        return;
    }

    loadTransactions();
    ui->depositIdEdit->clear();
    ui->amountEdit->clear();
    ui->dateEdit->clear();
}

void SecondForm::onDeleteTransactionClicked()
{
    int currentRow = ui->transactionTable->currentRow();
    if (currentRow < 0)
    {
        QMessageBox::warning(this, "Предупреждение", "Выберите транзакцию для удаления!");
        return;
    }

    QTableWidgetItem *item0 = ui->transactionTable->item(currentRow, 0);
    if (!item0)
    {
        QMessageBox::warning(this, "Ошибка", "Некорректные данные в таблице!");
        return;
    }
    int transactionId = item0->text().toInt();

    QSqlQuery query(dbconn);
    QString sqlstr = "DELETE FROM Transaction WHERE id = ?";
    query.prepare(sqlstr);
    query.addBindValue(transactionId);

    if (!query.exec())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось удалить транзакцию: " + query.lastError().text());
        return;
    }

    loadTransactions();
    ui->depositIdEdit->clear();
    ui->amountEdit->clear();
    ui->dateEdit->clear();
}

void SecondForm::onTransactionTableSelectionChanged()
{
    int currentRow = ui->transactionTable->currentRow();
    if (currentRow >= 0)
    {
        QTableWidgetItem *depositIdItem = ui->transactionTable->item(currentRow, 1);
        QTableWidgetItem *amountItem = ui->transactionTable->item(currentRow, 2);
        QTableWidgetItem *dateItem = ui->transactionTable->item(currentRow, 3);
        QTableWidgetItem *typeItem = ui->transactionTable->item(currentRow, 4);

        ui->depositIdEdit->setText(depositIdItem ? depositIdItem->text() : "");
        ui->amountEdit->setText(amountItem ? amountItem->text() : "");
        ui->dateEdit->setText(dateItem ? dateItem->text() : "");
        ui->typeCombo->setCurrentText(typeItem ? typeItem->text() : "");
    }
}