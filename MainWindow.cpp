#include "MainWindow.hpp"

#include <QApplication>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSslError>
#include <QMenuBar>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>
#include <QStyleFactory>
#include <QVBoxLayout>
#include <functional>
#include <iostream>
#include <sstream>

// ── stdout 捕获辅助 ──
static QString captureStdout(const std::function<void()> &fn) {
    std::stringstream buf;
    auto old = std::cout.rdbuf(buf.rdbuf());
    try {
        fn();
    } catch (...) {
        std::cout.rdbuf(old);
        throw;
    }
    std::cout.rdbuf(old);
    return QString::fromUtf8(buf.str().c_str());
}

// ===============================================================
//  InputDialog — 录入/编辑表单
// ===============================================================
InputDialog::InputDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("录入联系人");
    setMinimumWidth(400);

    auto *mainLayout = new QVBoxLayout(this);
    auto *form = new QFormLayout();

    typeCombo = new QComboBox(this);
    typeCombo->addItems({"同学", "同事", "朋友", "亲戚"});
    form->addRow("类  别：", typeCombo);

    nameEdit = new QLineEdit(this);
    nameEdit->setPlaceholderText("请输入姓名");
    form->addRow("姓  名：", nameEdit);

    birthEdit = new QDateEdit(this);
    birthEdit->setCalendarPopup(true);
    birthEdit->setDisplayFormat("yyyy-MM-dd");
    birthEdit->setDate(QDate(2000, 1, 1));
    form->addRow("生  日：", birthEdit);

    phoneEdit = new QLineEdit(this);
    phoneEdit->setPlaceholderText("请输入电话号码");
    form->addRow("电  话：", phoneEdit);

    emailEdit = new QLineEdit(this);
    emailEdit->setPlaceholderText("请输入邮箱地址");
    form->addRow("Email：", emailEdit);

    const char *placeholders[] = {
        "请输入学校名称", "请输入共事单位", "请输入认识地点", "请输入称呼"};
    extraStack = new QStackedWidget(this);
    for (int i = 0; i < 4; ++i) {
        extraEdits[i] = new QLineEdit(this);
        extraEdits[i]->setPlaceholderText(placeholders[i]);
        extraStack->addWidget(extraEdits[i]);
    }
    extraLabel = new QLabel("学校名称：", this);
    form->addRow(extraLabel, extraStack);

    mainLayout->addLayout(form);

    auto *btnLayout = new QHBoxLayout();
    auto *okBtn = new QPushButton("确 认", this);
    auto *cancelBtn = new QPushButton("取 消", this);
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &InputDialog::onTypeChanged);
    connect(okBtn, &QPushButton::clicked, this, [this]() {
        if (nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "输入错误", "姓名不能为空！");
            return;
        }
        accept();
    });
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void InputDialog::onTypeChanged(int index) {
    static const char *labels[] = {"学校名称：", "共事单位：", "认识地点：", "称    呼："};
    extraLabel->setText(labels[index]);
    extraStack->setCurrentIndex(index);
}

int InputDialog::getTypeIndex() const { return typeCombo->currentIndex(); }
QString InputDialog::getPersonName() const { return nameEdit->text().trimmed(); }
QDate InputDialog::getBirthDate() const { return birthEdit->date(); }
QString InputDialog::getPhone() const { return phoneEdit->text().trimmed(); }
QString InputDialog::getEmail() const { return emailEdit->text().trimmed(); }
QString InputDialog::getExtra() const {
    return extraEdits[typeCombo->currentIndex()]->text().trimmed();
}

void InputDialog::setTypeIndex(int idx) { typeCombo->setCurrentIndex(idx); }
void InputDialog::setPersonName(const QString &s) { nameEdit->setText(s); }
void InputDialog::setBirthDate(const QDate &d) { birthEdit->setDate(d); }
void InputDialog::setPhone(const QString &s) { phoneEdit->setText(s); }
void InputDialog::setEmail(const QString &s) { emailEdit->setText(s); }
void InputDialog::setExtra(const QString &s) {
    extraEdits[typeCombo->currentIndex()]->setText(s);
}

void InputDialog::setEditMode(bool readOnly) {
    if (readOnly) {
        nameEdit->setReadOnly(true);
        birthEdit->setReadOnly(true);
        nameEdit->setStyleSheet("background-color: #2d2d2d; color: #888888;");
        birthEdit->setStyleSheet("background-color: #2d2d2d; color: #888888;");
    } else {
        nameEdit->setReadOnly(false);
        birthEdit->setReadOnly(false);
        nameEdit->setStyleSheet("");
        birthEdit->setStyleSheet("");
    }
}

// ===============================================================
//  LLMAssistant — 大语言模型意图识别引擎
// ===============================================================
LLMAssistant::LLMAssistant(QObject *parent)
    : QObject(parent),
      m_nam(new QNetworkAccessManager(this)) {}

QString LLMAssistant::buildSystemPrompt() const {
    return QStringLiteral(
        "你是一个通信录管理助手的意图识别引擎，将用户的中文指令转为JSON命令。\n\n"
        "通信录包含四类联系人：同学、同事、朋友、亲戚。\n"
        "每个人的记录包括：姓名、生日（YYYY-MM-DD）、电话、Email、以及各自额外的字段。\n\n"
        "可用 action：\n"
        "  search — 按姓名搜索。参数: name\n"
        "  searchByPhone — 按电话号码搜索。参数: phone\n"
        "  searchByEmail — 按邮箱搜索。参数: email\n"
        "  searchBySurname — 按姓氏搜索。参数: surname\n"
        "  searchByYear — 按出生年份搜索。参数: year\n"
        "  add — 添加联系人。参数: type, name, birth, phone, email, extra\n"
        "  delete — 按姓名删除。参数: name\n"
        "  edit — 修改联系人。参数: oldName, type, name, birth, phone, email, extra\n"
        "  sortByName — 按姓名排序。无需额外参数\n"
        "  sortByBirth — 按生日排序。无需额外参数\n"
        "  statMonth — 统计某月生日的人。参数: month（必需，1-12）, type（可选，筛选类别）\n"
        "  findSameBirthday — 查找生日相同的人。参数: type（可选，筛选类别）\n"
        "  scanBirthdays — 扫描未来5天生日并生成祝福文件。参数: sender（可选，默认Admin）\n"
        "  listAll — 列出所有联系人。无需额外参数\n"
        "  listByType — 列出某类联系人。参数: type\n"
        "  typeCount — 统计各类别联系人数量。无需额外参数\n"
        "  save — 保存数据到文件。无需额外参数\n"
        "  reply — 不执行操作，仅回复用户。参数: text\n\n"
        "规则：\n"
        "1. 意图明确 → 输出JSON命令。组合条件用可选参数表达。\n"
        "2. 问候/闲聊/意图不明 → {\"action\":\"reply\",\"text\":\"...\"}\n"
        "3. 只输出一个JSON对象，无多余文字。\n\n"
        "示例：\n"
        "用户：查一下张三\n"
        "输出：{\"action\":\"search\",\"name\":\"张三\"}\n\n"
        "用户：电话号码为13500001111的人\n"
        "输出：{\"action\":\"searchByPhone\",\"phone\":\"13500001111\"}\n\n"
        "用户：帮我添加一个同学叫李四，生日2000年1月1日，电话13800138000\n"
        "输出：{\"action\":\"add\",\"type\":\"同学\",\"name\":\"李四\","
        "\"birth\":\"2000-01-01\",\"phone\":\"13800138000\",\"email\":\"\",\"extra\":\"\"}\n\n"
        "用户：查一下五月份生日的同学\n"
        "输出：{\"action\":\"statMonth\",\"month\":5,\"type\":\"同学\"}\n\n"
        "用户：找出生日相同的同学\n"
        "输出：{\"action\":\"findSameBirthday\",\"type\":\"同学\"}\n\n"
        "用户：列出所有朋友\n"
        "输出：{\"action\":\"listByType\",\"type\":\"朋友\"}\n\n"
        "用户：把全体联系人列出来\n"
        "输出：{\"action\":\"listAll\"}\n\n"
        "用户：查一下1999年出生的人\n"
        "输出：{\"action\":\"searchByYear\",\"year\":1999}\n\n"
        "用户：姓李的人有哪些\n"
        "输出：{\"action\":\"searchBySurname\",\"surname\":\"李\"}\n\n"
        "用户：搜索邮箱abc@example.com\n"
        "输出：{\"action\":\"searchByEmail\",\"email\":\"abc@example.com\"}\n\n"
        "用户：各类别分别有多少人\n"
        "输出：{\"action\":\"typeCount\"}\n\n"
        "用户：你好\n"
        "输出：{\"action\":\"reply\",\"text\":\"你好！我是通信录助手，"
        "可以帮你查找、添加、删除联系人，或进行排序、统计。试试说：查一下张三\"}"
    );
}

void LLMAssistant::sendQuery(const QString &input) {
    QUrl url(LLM_API_URL);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", ("Bearer " + QString(LLM_API_KEY)).toUtf8());

    QJsonObject body;
    body["model"] = QStringLiteral("deepseek-chat");
    body["temperature"] = 0.1;

    QJsonArray messages;
    QJsonObject sysMsg;
    sysMsg["role"] = QStringLiteral("system");
    sysMsg["content"] = buildSystemPrompt();
    messages.append(sysMsg);

    QJsonObject userMsg;
    userMsg["role"] = QStringLiteral("user");
    userMsg["content"] = input;
    messages.append(userMsg);

    body["messages"] = messages;

    QByteArray data = QJsonDocument(body).toJson(QJsonDocument::Compact);
    emit statusMessage(QStringLiteral("正在分析意图..."));

    QNetworkReply *reply = m_nam->post(req, data);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
    connect(reply, &QNetworkReply::sslErrors, this, [this, reply](const QList<QSslError> &errors) {
        QStringList details;
        for (const auto &e : errors)
            details << e.errorString();
        emit errorOccurred(QStringLiteral("SSL 错误：%1").arg(details.join("；")));
        reply->ignoreSslErrors();
    });
}

void LLMAssistant::onReplyFinished(QNetworkReply *reply) {
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(QStringLiteral("网络请求失败：%1").arg(reply->errorString()));
        return;
    }

    QByteArray raw = reply->readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(raw, &err);

    if (err.error != QJsonParseError::NoError) {
        emit errorOccurred(QStringLiteral("JSON 解析失败：%1").arg(err.errorString()));
        return;
    }

    QJsonObject root = doc.object();

    // 检查 API 层错误
    if (root.contains("error")) {
        QJsonObject errObj = root["error"].toObject();
        emit errorOccurred(QStringLiteral("API 错误：%1").arg(
            errObj["message"].toString()));
        return;
    }

    // 提取 choices[0].message.content
    QJsonArray choices = root["choices"].toArray();
    if (choices.isEmpty()) {
        emit errorOccurred(QStringLiteral("API 返回为空"));
        return;
    }

    QString content = choices[0].toObject()["message"].toObject()["content"].toString().trimmed();

    // 解析 content 中的 JSON 命令
    QJsonDocument cmdDoc = QJsonDocument::fromJson(content.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        emit errorOccurred(QStringLiteral("LLM 返回的 JSON 格式错误：%1\n内容：%2")
                               .arg(err.errorString(), content));
        return;
    }

    QJsonObject cmd = cmdDoc.object();
    if (!cmd.contains("action")) {
        emit errorOccurred(QStringLiteral("LLM 返回缺少 action 字段"));
        return;
    }

    emit commandReady(cmd);
}

// ===============================================================
//  MainWindow
// ===============================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle("个人通信录管理系统");

    // ── 菜单栏（任务书要求8：菜单形式） ──
    auto *menuBar = new QMenuBar(this);

    auto *fileMenu = menuBar->addMenu("文件(&F)");
    fileMenu->addAction("保存数据(&S)", this, &MainWindow::onSave);
    fileMenu->addSeparator();
    fileMenu->addAction("退出(&Q)", qApp, &QApplication::quit);

    auto *contactMenu = menuBar->addMenu("联系人(&C)");
    contactMenu->addAction("添加(&A)...", this, &MainWindow::onAdd);
    contactMenu->addAction("编辑(&E)...", this, &MainWindow::onEdit);
    contactMenu->addAction("删除(&D)", this, &MainWindow::onDelete);
    contactMenu->addAction("查询(&Q)...", this, &MainWindow::onSearch);

    auto *sortMenu = menuBar->addMenu("排序(&S)");
    sortMenu->addAction("按姓名", this, &MainWindow::onSortByName);
    sortMenu->addAction("按生日", this, &MainWindow::onSortByBirth);

    auto *statMenu = menuBar->addMenu("统计(&T)");
    statMenu->addAction("统计本月生日...", this, &MainWindow::onMonthStat);
    statMenu->addAction("扫描5天内生日", this, &MainWindow::onBirthScan);

    auto *viewMenu = menuBar->addMenu("视图(&V)");
    viewMenu->addAction("全体人员", this, [this] {
        QString msg = captureStdout([&] { manager.showAllBrief(); });
        appendLog(msg.trimmed());
    });
    static const char *typeNames[] = {"同学", "同事", "朋友", "亲戚"};
    for (int i = 0; i < 4; ++i) {
        int typeIdx = i + 1;
        viewMenu->addAction(typeNames[i], this, [this, typeIdx] {
            QString msg = captureStdout([&] { manager.showByType(typeIdx); });
            appendLog(msg.trimmed());
        });
    }

    setMenuBar(menuBar);

    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *hLayout = new QHBoxLayout(central);
    hLayout->setContentsMargins(8, 8, 8, 8);
    hLayout->setSpacing(8);

    // ── 左侧控制面板 ──
    auto *leftWidget = new QWidget(this);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setSpacing(6);
    leftLayout->setContentsMargins(4, 4, 4, 4);

    auto makeBtn = [&](const QString &text, void (MainWindow::*slot)()) {
        auto *btn = new QPushButton(text, this);
        btn->setMinimumHeight(36);
        connect(btn, &QPushButton::clicked, this, slot);
        leftLayout->addWidget(btn);
    };

    makeBtn("添加联系人", &MainWindow::onAdd);
    makeBtn("编辑选中联系人", &MainWindow::onEdit);
    makeBtn("删除选中联系人", &MainWindow::onDelete);
    makeBtn("查询联系人", &MainWindow::onSearch);

    leftLayout->addSpacing(8);

    makeBtn("按姓名排序", &MainWindow::onSortByName);
    makeBtn("按生日排序", &MainWindow::onSortByBirth);

    leftLayout->addSpacing(8);

    // 月份统计行
    auto *statRow = new QHBoxLayout();
    statRow->addWidget(new QLabel("月份：", this));
    monthSpin = new QSpinBox(this);
    monthSpin->setRange(1, 12);
    monthSpin->setValue(1);
    statRow->addWidget(monthSpin);
    auto *statBtn = new QPushButton("统计本月生日", this);
    connect(statBtn, &QPushButton::clicked, this, &MainWindow::onMonthStat);
    statRow->addWidget(statBtn);
    leftLayout->addLayout(statRow);

    makeBtn("扫描 5 天内生日并生成祝福邮件", &MainWindow::onBirthScan);

    leftLayout->addSpacing(12);

    auto *saveBtn = new QPushButton("保 存 数 据", this);
    saveBtn->setMinimumHeight(42);
    saveBtn->setStyleSheet(
        "QPushButton { background-color: #0e639c; font-weight: bold; }"
        "QPushButton:hover { background-color: #1177bb; }");
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::onSave);
    leftLayout->addWidget(saveBtn);

    leftLayout->addStretch();

    // ── 右侧数据视图 ──
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(4);

    table = new QTableWidget(0, 6, this);
    table->setHorizontalHeaderLabels({"姓名", "生日", "电话", "Email", "类别", "特殊信息"});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    rightLayout->addWidget(table, 3);

    log = new QTextEdit(this);
    log->setReadOnly(true);
    log->document()->setMaximumBlockCount(1000);
    rightLayout->addWidget(log, 1);

    // ── AI 交互输入行 ──
    auto *aiRow = new QHBoxLayout();
    aiInput = new QLineEdit(this);
    aiInput->setPlaceholderText("输入自然语言指令（如：查一下张三）…");
    aiSendBtn = new QPushButton("发送", this);
    aiSendBtn->setMinimumHeight(32);
    aiSendBtn->setFixedWidth(60);
    auto *clearLogBtn = new QPushButton("清空日志", this);
    clearLogBtn->setFixedWidth(70);
    clearLogBtn->setMinimumHeight(32);
    connect(clearLogBtn, &QPushButton::clicked, this, [this] {
        log->clear();
    });
    aiRow->addWidget(aiInput, 1);
    aiRow->addWidget(aiSendBtn);
    aiRow->addWidget(clearLogBtn);
    rightLayout->addLayout(aiRow);

    // ── LLM 初始化 ──
    m_llm = new LLMAssistant(this);
    connect(m_llm, &LLMAssistant::commandReady, this, &MainWindow::onAiCommand);
    connect(m_llm, &LLMAssistant::statusMessage, this, &MainWindow::appendLog);
    connect(m_llm, &LLMAssistant::errorOccurred, this, [this](const QString &err) {
        appendLog("[AI 错误] " + err);
    });
    connect(aiSendBtn, &QPushButton::clicked, this, &MainWindow::onAiSend);
    connect(aiInput, &QLineEdit::returnPressed, this, &MainWindow::onAiSend);

    hLayout->addWidget(leftWidget, 0);
    hLayout->addWidget(rightWidget, 1);

    // 启动时加载数据
    try {
        QString msg = captureStdout([&] { manager.loadFromFile(); });
        if (!msg.trimmed().isEmpty())
            appendLog(msg.trimmed());
    } catch (const std::exception &e) {
        appendLog(QString("数据加载：%1").arg(e.what()));
    }
    refreshTable();
}

MainWindow::~MainWindow() = default;

void MainWindow::appendLog(const QString &msg) {
    log->append(msg);
}

void MainWindow::refreshTable() {
    table->setRowCount(0);
    const auto &contacts = manager.getContacts();

    static const char *typeNames[] = {"同学", "同事", "朋友", "亲戚"};
    static const char *extraLabels[] = {"学校", "单位", "地点", "称呼"};

    for (size_t i = 0; i < contacts.size(); ++i) {
        const auto &p = contacts[i];
        int row = table->rowCount();
        table->insertRow(row);

        int typeIdx = p->getType() - 1;

        table->setItem(row, 0, new QTableWidgetItem(QString::fromUtf8(p->getName().c_str())));
        table->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8(p->getBirthDate().toString().c_str())));
        table->setItem(row, 2, new QTableWidgetItem(QString::fromUtf8(p->getPhone().c_str())));
        table->setItem(row, 3, new QTableWidgetItem(QString::fromUtf8(p->getEmail().c_str())));
        table->setItem(row, 4, new QTableWidgetItem((typeIdx >= 0 && typeIdx < 4) ? typeNames[typeIdx] : "未知"));

        // 特殊信息：从 formatForFile() 末尾提取最后一个字段
        std::string ff = p->formatForFile();
        auto lastComma = ff.rfind(',');
        std::string extra = (lastComma != std::string::npos)
                                ? ff.substr(lastComma + 1)
                                : "";
        QString displayExtra = (typeIdx >= 0 && typeIdx < 4)
                                   ? QString("%1：%2").arg(extraLabels[typeIdx],
                                                           QString::fromUtf8(extra.c_str()))
                                   : QString::fromUtf8(extra.c_str());
        table->setItem(row, 5, new QTableWidgetItem(displayExtra));
    }
}

// ======================== 槽函数 ========================

void MainWindow::onAdd() {
    InputDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    int typeIdx = dlg.getTypeIndex();
    std::string name = dlg.getPersonName().toUtf8().toStdString();
    QDate qd = dlg.getBirthDate();
    Date birth = {qd.year(), qd.month(), qd.day()};
    std::string phone = dlg.getPhone().toUtf8().toStdString();
    std::string email = dlg.getEmail().toUtf8().toStdString();
    std::string extra = dlg.getExtra().toUtf8().toStdString();

    switch (typeIdx) {
    case 0:
        manager.addPerson(std::make_unique<Classmate>(name, birth, phone, email, extra));
        break;
    case 1:
        manager.addPerson(std::make_unique<Colleague>(name, birth, phone, email, extra));
        break;
    case 2:
        manager.addPerson(std::make_unique<Friend>(name, birth, phone, email, extra));
        break;
    case 3:
        manager.addPerson(std::make_unique<Relative>(name, birth, phone, email, extra));
        break;
    }
    refreshTable();
    appendLog("==> 联系人录入成功：" + QString::fromUtf8(name.c_str()));
}

void MainWindow::onEdit() {
    int row = table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "提示", "请先选中一个联系人。");
        return;
    }

    const auto &contacts = manager.getContacts();
    if (static_cast<size_t>(row) >= contacts.size())
        return;

    const auto &p = contacts[row];
    std::string originalName = p->getName(); // 记录原名，用于删旧

    InputDialog dlg(this);
    dlg.setWindowTitle("修改联系人");
    dlg.setTypeIndex(p->getType() - 1);
    dlg.setPersonName(QString::fromUtf8(p->getName().c_str()));
    dlg.setBirthDate(QDate(p->getBirthDate().year, p->getBirthDate().month, p->getBirthDate().day));
    dlg.setPhone(QString::fromUtf8(p->getPhone().c_str()));
    dlg.setEmail(QString::fromUtf8(p->getEmail().c_str()));
    {
        std::string ff = p->formatForFile();
        auto lastComma = ff.rfind(',');
        std::string extra = (lastComma != std::string::npos) ? ff.substr(lastComma + 1) : "";
        dlg.setExtra(QString::fromUtf8(extra.c_str()));
    }
    dlg.setEditMode(true);  // 姓名和生日禁止修改（任务书要求）

    if (dlg.exec() != QDialog::Accepted)
        return;

    // 删旧添新
    manager.deletePerson(originalName);

    int typeIdx = dlg.getTypeIndex();
    std::string newName = dlg.getPersonName().toUtf8().toStdString();
    QDate qd = dlg.getBirthDate();
    Date birth = {qd.year(), qd.month(), qd.day()};
    std::string phone = dlg.getPhone().toUtf8().toStdString();
    std::string email = dlg.getEmail().toUtf8().toStdString();
    std::string extra = dlg.getExtra().toUtf8().toStdString();

    switch (typeIdx) {
    case 0:
        manager.addPerson(std::make_unique<Classmate>(newName, birth, phone, email, extra));
        break;
    case 1:
        manager.addPerson(std::make_unique<Colleague>(newName, birth, phone, email, extra));
        break;
    case 2:
        manager.addPerson(std::make_unique<Friend>(newName, birth, phone, email, extra));
        break;
    case 3:
        manager.addPerson(std::make_unique<Relative>(newName, birth, phone, email, extra));
        break;
    }
    refreshTable();
    appendLog("==> 联系人已更新。");
}

void MainWindow::onDelete() {
    int row = table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, "提示", "请先选中一个联系人。");
        return;
    }

    const auto &contacts = manager.getContacts();
    if (static_cast<size_t>(row) >= contacts.size())
        return;

    std::string target = contacts[row]->getName();
    QString displayName = QString::fromUtf8(target.c_str());

    if (QMessageBox::question(this, "确认删除",
                              QString("确定要删除「%1」吗？").arg(displayName),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QString msg = captureStdout([&] { manager.deletePerson(target); });
    refreshTable();
    appendLog(msg.trimmed());
}

void MainWindow::onSearch() {
    bool ok = false;
    QString name = QInputDialog::getText(this, "查询联系人",
                                         "请输入姓名：", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty())
        return;

    Person *p = manager.searchByName(name.toUtf8().toStdString());
    if (p) {
        QString msg = captureStdout([p] { p->display(); });
        appendLog(msg.trimmed());
    } else {
        appendLog(QString("==> 未找到名为「%1」的联系人。").arg(name));
    }
}

void MainWindow::onSortByName() {
    QString msg = captureStdout([&] { manager.sortByName(); });
    refreshTable();
    appendLog(msg.trimmed());
}

void MainWindow::onSortByBirth() {
    QString msg = captureStdout([&] { manager.sortByBirthDate(); });
    refreshTable();
    appendLog(msg.trimmed());
}

void MainWindow::onMonthStat() {
    QString msg = captureStdout([&] { manager.countByBirthMonth(monthSpin->value()); });
    appendLog(msg.trimmed());
}

void MainWindow::onBirthScan() {
    QString msg = captureStdout([&] { manager.checkBirthdaysAndSendEmails("Admin"); });
    refreshTable();
    appendLog(msg.trimmed());
}

void MainWindow::onSave() {
    try {
        QString msg = captureStdout([&] { manager.saveToFile(); });
        appendLog(msg.trimmed());
    } catch (const std::exception &e) {
        QString err = QString::fromUtf8(e.what());
        appendLog("[保存失败] " + err);
        QMessageBox::warning(this, "保存失败", err);
    }
}

// ===============================================================
//  AI 交互
// ===============================================================
void MainWindow::onAiSend() {
    QString text = aiInput->text().trimmed();
    if (text.isEmpty())
        return;

    appendLog(QString("[用户] %1").arg(text));
    aiInput->clear();

    QString apiKey = QString(LLM_API_KEY);
    if (apiKey.isEmpty() || apiKey == "在此填入你的 API Key") {
        appendLog("[AI 错误] 请先在 MainWindow.hpp 中配置 LLM_API_KEY");
        return;
    }

    m_llm->sendQuery(text);
}

void MainWindow::onAiCommand(const QJsonObject &cmd) {
    QString action = cmd["action"].toString();

    // ── 类型名称 → 类型索引映射 ──
    auto typeNameToIndex = [](const QString &name) -> int {
        static const char *names[] = {"同学", "同事", "朋友", "亲戚"};
        for (int i = 0; i < 4; ++i)
            if (name == names[i]) return i + 1;
        return -1;
    };

    if (action == "search") {
        std::string name = cmd["name"].toString().toUtf8().toStdString();
        if (name.empty()) {
            appendLog("==> 请提供要搜索的姓名");
            return;
        }
        Person *p = manager.searchByName(name);
        if (p) {
            QString msg = captureStdout([p] { p->display(); });
            appendLog(msg.trimmed());
        } else {
            appendLog(QString("==> 未找到名为「%1」的联系人。").arg(cmd["name"].toString()));
        }
    }

    else if (action == "searchByPhone") {
        std::string phone = cmd["phone"].toString().toUtf8().toStdString();
        if (phone.empty()) {
            appendLog("==> 请提供要搜索的电话号码");
            return;
        }
        Person *p = manager.searchByPhone(phone);
        if (p) {
            QString msg = captureStdout([p] { p->display(); });
            appendLog(msg.trimmed());
        } else {
            appendLog(QString("==> 未找到电话号码为「%1」的联系人。").arg(cmd["phone"].toString()));
        }
    }

    else if (action == "searchByEmail") {
        std::string email = cmd["email"].toString().toUtf8().toStdString();
        if (email.empty()) {
            appendLog("==> 请提供要搜索的邮箱");
            return;
        }
        Person *p = manager.searchByEmail(email);
        if (p) {
            QString msg = captureStdout([p] { p->display(); });
            appendLog(msg.trimmed());
        } else {
            appendLog(QString("==> 未找到邮箱为「%1」的联系人。").arg(cmd["email"].toString()));
        }
    }

    else if (action == "searchBySurname") {
        std::string surname = cmd["surname"].toString().toUtf8().toStdString();
        if (surname.empty()) {
            appendLog("==> 请提供要搜索的姓氏");
            return;
        }
        auto results = manager.searchBySurname(surname);
        if (results.empty()) {
            appendLog(QString("==> 没有找到姓「%1」的联系人。").arg(cmd["surname"].toString()));
        } else {
            appendLog(QString("==> 姓「%1」的联系人（共 %2 人）：").arg(cmd["surname"].toString()).arg(results.size()));
            for (const auto &person : results) {
                QString msg = captureStdout([person] { person->display(); });
                appendLog(msg.trimmed());
            }
        }
    }

    else if (action == "searchByYear") {
        int year = cmd["year"].toInt();
        if (year <= 0) {
            appendLog("==> 请提供有效的出生年份");
            return;
        }
        auto results = manager.searchByBirthYear(year);
        if (results.empty()) {
            appendLog(QString("==> 没有找到 %1 年出生的联系人。").arg(year));
        } else {
            appendLog(QString("==> %1 年出生的联系人（共 %2 人）：").arg(year).arg(results.size()));
            for (const auto &person : results) {
                QString msg = captureStdout([person] { person->display(); });
                appendLog(msg.trimmed());
            }
        }
    }

    else if (action == "add") {
        QString typeStr = cmd["type"].toString();
        std::string name = cmd["name"].toString().toUtf8().toStdString();
        if (name.empty()) {
            appendLog("==> 添加联系人失败：缺少姓名");
            return;
        }
        QString birth = cmd["birth"].toString();
        QStringList parts = birth.split('-');
        Date d = {2000, 1, 1};
        if (parts.size() == 3)
            d = {parts[0].toInt(), parts[1].toInt(), parts[2].toInt()};
        std::string phone = cmd["phone"].toString().toUtf8().toStdString();
        std::string email = cmd["email"].toString().toUtf8().toStdString();
        std::string extra = cmd["extra"].toString().toUtf8().toStdString();

        static const struct { const char *key; int idx; } typeMap[] = {
            {"同学", 0}, {"同事", 1}, {"朋友", 2}, {"亲戚", 3}};
        int typeIdx = 0;
        for (auto &m : typeMap) {
            if (typeStr == m.key) { typeIdx = m.idx; break; }
        }

        switch (typeIdx) {
        case 0: manager.addPerson(std::make_unique<Classmate>(name, d, phone, email, extra)); break;
        case 1: manager.addPerson(std::make_unique<Colleague>(name, d, phone, email, extra)); break;
        case 2: manager.addPerson(std::make_unique<Friend>(name, d, phone, email, extra)); break;
        case 3: manager.addPerson(std::make_unique<Relative>(name, d, phone, email, extra)); break;
        }
        refreshTable();
        appendLog(QString("==> AI 已添加联系人：%1").arg(cmd["name"].toString()));
    }

    else if (action == "delete") {
        std::string name = cmd["name"].toString().toUtf8().toStdString();
        if (name.empty()) {
            appendLog("==> 删除失败：缺少姓名");
            return;
        }
        QString msg = captureStdout([&] { manager.deletePerson(name); });
        refreshTable();
        appendLog(msg.trimmed());
    }

    else if (action == "edit") {
        std::string oldName = cmd["oldName"].toString().toUtf8().toStdString();
        if (oldName.empty()) {
            appendLog("==> 修改失败：缺少原名");
            return;
        }
        manager.deletePerson(oldName);

        QString typeStr = cmd["type"].toString();
        std::string name = cmd["name"].toString().toUtf8().toStdString();
        QString birth = cmd["birth"].toString();
        QStringList parts = birth.split('-');
        Date d = {2000, 1, 1};
        if (parts.size() == 3)
            d = {parts[0].toInt(), parts[1].toInt(), parts[2].toInt()};
        std::string phone = cmd["phone"].toString().toUtf8().toStdString();
        std::string email = cmd["email"].toString().toUtf8().toStdString();
        std::string extra = cmd["extra"].toString().toUtf8().toStdString();

        static const struct { const char *key; int idx; } typeMap[] = {
            {"同学", 0}, {"同事", 1}, {"朋友", 2}, {"亲戚", 3}};
        int typeIdx = 0;
        for (auto &m : typeMap) {
            if (typeStr == m.key) { typeIdx = m.idx; break; }
        }

        switch (typeIdx) {
        case 0: manager.addPerson(std::make_unique<Classmate>(name, d, phone, email, extra)); break;
        case 1: manager.addPerson(std::make_unique<Colleague>(name, d, phone, email, extra)); break;
        case 2: manager.addPerson(std::make_unique<Friend>(name, d, phone, email, extra)); break;
        case 3: manager.addPerson(std::make_unique<Relative>(name, d, phone, email, extra)); break;
        }
        refreshTable();
        appendLog(QString("==> AI 已更新联系人：%1").arg(cmd["name"].toString()));
    }

    else if (action == "sortByName") {
        QString msg = captureStdout([&] { manager.sortByName(); });
        refreshTable();
        appendLog(msg.trimmed());
    }

    else if (action == "sortByBirth") {
        QString msg = captureStdout([&] { manager.sortByBirthDate(); });
        refreshTable();
        appendLog(msg.trimmed());
    }

    else if (action == "statMonth") {
        int month = cmd["month"].toInt();
        if (month < 1 || month > 12) {
            appendLog("==> 月份无效，请输入 1-12");
            return;
        }

        QString typeFilter = cmd["type"].toString();
        if (typeFilter.isEmpty()) {
            // 只按月份统计
            QString msg = captureStdout([&] { manager.countByBirthMonth(month); });
            appendLog(msg.trimmed());
        } else {
            // 月份 + 类别联合过滤
            int typeIdx = typeNameToIndex(typeFilter);
            if (typeIdx < 0) {
                appendLog(QString("==> 无效的联系人类别：%1").arg(typeFilter));
                return;
            }
            int count = 0;
            appendLog(QString("==> %1 月份过生日的%2：").arg(month).arg(typeFilter));
            for (const auto &p : manager.getContacts()) {
                if (p->getBirthDate().month == month && p->getType() == typeIdx) {
                    QString line = captureStdout([&p] { p->display(); });
                    appendLog(line.trimmed());
                    count++;
                }
            }
            appendLog(QString("==> 共 %1 人。").arg(count));
        }
    }

    else if (action == "scanBirthdays") {
        QString sender = cmd["sender"].toString("Admin");
        QString msg = captureStdout(
            [&] { manager.checkBirthdaysAndSendEmails(sender.toUtf8().toStdString()); });
        refreshTable();
        appendLog(msg.trimmed());
    }

    else if (action == "listAll" || action == "list") {
        QString msg = captureStdout([&] { manager.showAllBrief(); });
        appendLog(msg.trimmed());
    }

    else if (action == "listByType") {
        QString typeFilter = cmd["type"].toString();
        int typeIdx = typeNameToIndex(typeFilter);
        if (typeIdx < 0) {
            appendLog(QString("==> 无效的联系人类别：%1").arg(typeFilter));
            return;
        }
        QString msg = captureStdout([&] { manager.showByType(typeIdx); });
        appendLog(msg.trimmed());
    }

    else if (action == "typeCount") {
        QString msg = captureStdout([&] { manager.countByType(); });
        appendLog(msg.trimmed());
    }

    else if (action == "findSameBirthday") {
        QString typeFilter = cmd["type"].toString();
        if (typeFilter.isEmpty()) {
            QString msg = captureStdout([&] { manager.findSameBirthday(0); });
            appendLog(msg.trimmed());
        } else {
            int typeIdx = typeNameToIndex(typeFilter);
            if (typeIdx < 0) {
                appendLog(QString("==> 无效的联系人类别：%1").arg(typeFilter));
                return;
            }
            QString msg = captureStdout([&] { manager.findSameBirthday(typeIdx); });
            appendLog(msg.trimmed());
        }
    }

    else if (action == "save") {
        onSave();
    }

    else if (action == "reply") {
        appendLog(QString("[AI] %1").arg(cmd["text"].toString()));
    }

    else {
        appendLog(QString("==> 无法识别的操作：%1").arg(action));
    }
}

// ===============================================================
//  main() 入口
// ===============================================================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));

    // ── 现代深色 QSS 主题 ──
    app.setStyleSheet(R"(
        QMainWindow, QDialog, QWidget {
            background-color: #1e1e1e;
            color: #d4d4d4;
        }
        QPushButton {
            background-color: #2d2d2d;
            color: #d4d4d4;
            border: 1px solid #3c3c3c;
            padding: 6px 16px;
            border-radius: 6px;
        }
        QPushButton:hover {
            background-color: #3c3c3c;
            border-color: #007acc;
        }
        QPushButton:pressed {
            background-color: #505050;
        }
        QTableWidget {
            background-color: #252526;
            color: #d4d4d4;
            gridline-color: #3c3c3c;
            border: 1px solid #3c3c3c;
            alternate-background-color: #2d2d2d;
        }
        QHeaderView::section {
            background-color: #2d2d2d;
            color: #d4d4d4;
            border: 1px solid #3c3c3c;
            padding: 4px;
        }
        QPlainTextEdit {
            background-color: #1e1e1e;
            color: #cccccc;
            border: 1px solid #3c3c3c;
        }
        QLineEdit, QComboBox, QDateEdit, QSpinBox {
            background-color: #3c3c3c;
            color: #d4d4d4;
            border: 1px solid #555555;
            padding: 4px 8px;
            border-radius: 4px;
        }
        QLineEdit:focus, QComboBox:focus {
            border-color: #007acc;
        }
        QComboBox::drop-down { border: none; }
        QComboBox QAbstractItemView {
            background-color: #3c3c3c;
            color: #d4d4d4;
            selection-background-color: #007acc;
        }
        QLabel, QGroupBox { color: #d4d4d4; }
        QGroupBox {
            border: 1px solid #3c3c3c;
            border-radius: 6px;
            margin-top: 8px;
            padding-top: 16px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 4px;
        }
        QScrollBar:vertical {
            background-color: #1e1e1e;
            width: 10px;
        }
        QScrollBar::handle:vertical {
            background-color: #555555;
            border-radius: 5px;
            min-height: 20px;
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar:horizontal {
            background-color: #1e1e1e;
            height: 10px;
        }
        QScrollBar::handle:horizontal {
            background-color: #555555;
            border-radius: 5px;
            min-width: 20px;
        }
        QScrollBar::add-line:horizontal,
        QScrollBar::sub-line:horizontal { width: 0px; }
        QTableWidget::item:selected {
            background-color: #094771;
        }
        QDialog QPushButton {
            min-width: 80px;
        }
    )");

    MainWindow w;
    w.resize(1200, 750);
    w.show();

    return app.exec();
}
