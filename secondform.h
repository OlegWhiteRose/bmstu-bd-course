#ifndef SECONDFORM_H
#define SECONDFORM_H

#include <QWidget>
#include <QSqlDatabase>

namespace Ui {
class SecondForm;
}

class SecondForm : public QWidget
{
    Q_OBJECT

public:
    explicit SecondForm(QWidget *parent = nullptr);
    ~SecondForm();
    void setClientId(int clientId);
    void loadTransactions();

signals:
    void returnToMainWindow();  // Добавили сигнал

private slots:
    void onBackButtonClicked();
    void onFilterButtonClicked();
    void onAddTransactionClicked();
    void onEditTransactionClicked();
    void onDeleteTransactionClicked();
    void onTransactionTableSelectionChanged();

private:
    Ui::SecondForm *ui;
    QSqlDatabase dbconn;
    int currentClientId;
};

#endif // SECONDFORM_H