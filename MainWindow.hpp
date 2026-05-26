#ifndef MANAGER_MAINWINDOW_HPP
#define MANAGER_MAINWINDOW_HPP

#include <QDialog>
#include <QComboBox>
#include <QDateEdit>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTextEdit>

#include "Core.hpp"

// ── LLM API 配置 ──
#ifndef LLM_API_KEY
#define LLM_API_KEY "sk-7fb9d6c623194cca84b725c2211a424a"
#endif
#ifndef LLM_API_URL
#define LLM_API_URL "https://api.deepseek.com/v1/chat/completions"
#endif

// ── LLM 自然语言交互助手 ──
class LLMAssistant : public QObject {
    Q_OBJECT
public:
    explicit LLMAssistant(QObject *parent = nullptr);

    void sendQuery(const QString &input);

signals:
    void commandReady(const QJsonObject &cmd);
    void statusMessage(const QString &msg);
    void errorOccurred(const QString &err);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QString buildSystemPrompt() const;
    QNetworkAccessManager *m_nam;
};

// ── 录入/编辑表单对话框 ──
class InputDialog : public QDialog {
    Q_OBJECT
public:
    explicit InputDialog(QWidget *parent = nullptr);

    int     getTypeIndex() const;
    QString getPersonName() const;
    QDate   getBirthDate()  const;
    QString getPhone()      const;
    QString getEmail()      const;
    QString getExtra()      const;

    void setTypeIndex(int idx);
    void setPersonName(const QString &s);
    void setBirthDate(const QDate &d);
    void setPhone(const QString &s);
    void setEmail(const QString &s);
    void setExtra(const QString &s);
    void setEditMode(bool readOnly);

private slots:
    void onTypeChanged(int index);

private:
    QComboBox      *typeCombo;
    QLineEdit      *nameEdit;
    QDateEdit      *birthEdit;
    QLineEdit      *phoneEdit;
    QLineEdit      *emailEdit;
    QStackedWidget *extraStack;
    QLineEdit      *extraEdits[4];
    QLabel         *extraLabel;
};

// ── 主窗口 ──
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onSearch();
    void onSortByName();
    void onSortByBirth();
    void onMonthStat();
    void onBirthScan();
    void onSave();
    void onAiSend();
    void onAiCommand(const QJsonObject &cmd);

private:
    void refreshTable();
    void appendLog(const QString &msg);

    AddressBookManager manager;
    QTableWidget *table;
    QTextEdit *log;
    QSpinBox     *monthSpin;
    QLineEdit    *aiInput;
    QPushButton  *aiSendBtn;
    LLMAssistant *m_llm;
};

#endif // MANAGER_MAINWINDOW_HPP
