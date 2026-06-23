#ifndef HELLO_CONTACT_MAINWINDOW_HPP
#define HELLO_CONTACT_MAINWINDOW_HPP

#include <QDialog>
#include <QComboBox>
#include <QDateEdit>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QPainter>
#include <QPushButton>
#include <QShortcut>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QTextEdit>

#include "hello_contact/core.hpp"

// ── LLM API 配置（通过环境变量注入，切勿硬编码） ──
#ifndef LLM_API_KEY
#define LLM_API_KEY ""
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
    void onHeaderClicked(int section);
    void onTableContextMenu(const QPoint &pos);
    void onThemeToggle();

private:
    void refreshTable();
    void updateStatusBar();
    void appendLog(const QString &msg);
    void saveUndoSnapshot(const QString &action, const QString &name,
                          const QString &snapshot = "");

    AddressBookManager manager;
    QTableWidget *table;
    QTextEdit *log;
    QLineEdit    *aiInput;
    QPushButton  *aiSendBtn;
    QLineEdit    *quickSearch = nullptr;
    QComboBox    *monthCombo = nullptr;
    QPushButton  *m_sortNameBtn = nullptr;
    QPushButton  *m_sortBirthBtn = nullptr;
    LLMAssistant *m_llm;

    // 排序
    bool m_sortNameAsc  = true;
    bool m_sortBirthAsc = true;

    // 主题
    bool    m_darkTheme = true;
    QString m_darkQSS;
    QString m_lightQSS;

    // 撤销
    enum class LastAction { None, Add, Delete, Edit };
    LastAction m_lastAction   = LastAction::None;
    QString    m_lastTarget;
    QString    m_undoSnapshot;

    // 用户设置
    QString m_senderName = "Admin";
    QString m_emailPath = "data";
};

// ── 数据可视化图表对话框 ──
class ChartWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChartWidget(const std::vector<std::unique_ptr<Person>> &contacts,
                         QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *event) override;
private:
    const std::vector<std::unique_ptr<Person>> &m_contacts;
};

class ChartDialog : public QDialog {
    Q_OBJECT
public:
    explicit ChartDialog(const std::vector<std::unique_ptr<Person>> &contacts,
                         QWidget *parent = nullptr);
};

#endif // HELLO_CONTACT_MAINWINDOW_HPP
