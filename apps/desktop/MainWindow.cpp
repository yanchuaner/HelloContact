#include "MainWindow.hpp"

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
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
#include <QShortcut>
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
        "  edit — 修改联系人（仅电话/邮箱/extra可改，姓名和生日程序强制锁定）。参数: oldName, phone, email, extra\n"
        "  sortByName — 按姓名排序。无需额外参数\n"
        "  sortByBirth — 按生日排序。无需额外参数\n"
        "  statMonth — 统计某月生日的人。参数: month（必需，1-12）, type（可选，筛选类别）\n"
        "  findSameBirthday — 查找生日相同的人。参数: type（可选，筛选类别）\n"
        "  scanBirthdays — 扫描未来5天生日并生成祝福文件。参数: sender（可选，默认使用设置的发件人姓名）\n"
        "  listAll — 列出所有联系人。无需额外参数\n"
        "  listByType — 列出某类联系人。参数: type\n"
        "  typeCount — 统计各类别联系人数量。无需额外参数\n"
        "  filter — 组合筛选。可选参数: type, surname, year, month（任意组合）\n"
        "  save — 保存数据到文件。无需额外参数\n"
        "  reply — 不执行操作，仅回复用户。参数: text\n\n"
        "规则：\n"
        "1. 意图明确 → 输出JSON命令。组合条件优先使用filter（不要拆分多个action）。\n"
        "2. 问候/闲聊/意图不明 → {\"action\":\"reply\",\"text\":\"...\"}\n"
        "3. 只输出一个JSON对象，无多余文字。\n"
        "4. 月份理解：年初=1~2月, 年中=6~7月, 年底=11~12月, 上半年=1~6月, 下半年=7~12月。\n"
        "5. 模糊指代：若用户说\"最近过生日的\"，用scanBirthdays。\n\n"
        "示例：\n"
        "用户：查一下张三\n"
        "输出：{\"action\":\"search\",\"name\":\"张三\"}\n\n"
        "用户：2007年出生的同学\n"
        "输出：{\"action\":\"filter\",\"year\":2007,\"type\":\"同学\"}\n\n"
        "用户：姓李的2007年出生的\n"
        "输出：{\"action\":\"filter\",\"surname\":\"李\",\"year\":2007}\n\n"
        "用户：年底过生日的同事\n"
        "输出：{\"action\":\"filter\",\"month\":12,\"type\":\"同事\"}\n\n"
        "用户：12月过生日的人\n"
        "输出：{\"action\":\"filter\",\"month\":12}\n\n"
        "用户：上半年出生的朋友\n"
        "输出：{\"action\":\"filter\",\"type\":\"朋友\"}\n"
        "（注：上半年/下半年用于辅助AI理解，filter只传type即可，结果列表后可人工判断）\n\n"
        "用户：帮我添加一个同学叫李四，生日2000年1月1日，电话13800138000\n"
        "输出：{\"action\":\"add\",\"type\":\"同学\",\"name\":\"李四\","
        "\"birth\":\"2000-01-01\",\"phone\":\"13800138000\",\"email\":\"\",\"extra\":\"\"}\n\n"
        "用户：查一下五月份生日的同学\n"
        "输出：{\"action\":\"filter\",\"month\":5,\"type\":\"同学\"}\n\n"
        "用户：找出生日相同的同学\n"
        "输出：{\"action\":\"findSameBirthday\",\"type\":\"同学\"}\n\n"
        "用户：列出所有朋友\n"
        "输出：{\"action\":\"listByType\",\"type\":\"朋友\"}\n\n"
        "用户：把全体联系人列出来\n"
        "输出：{\"action\":\"listAll\"}\n\n"
        "用户：查一下1999年出生的人\n"
        "输出：{\"action\":\"filter\",\"year\":1999}\n\n"
        "用户：姓李的人有哪些\n"
        "输出：{\"action\":\"searchBySurname\",\"surname\":\"李\"}\n\n"
        "用户：搜索邮箱abc@example.com\n"
        "输出：{\"action\":\"searchByEmail\",\"email\":\"abc@example.com\"}\n\n"
        "用户：各类别分别有多少人\n"
        "输出：{\"action\":\"typeCount\"}\n\n"
        "用户：你好\n"
        "输出：{\"action\":\"reply\",\"text\":\"你好！我是通信录助手，"
        "可以帮你查找、添加、删除联系人，或进行排序、统计。试试说：查一下张三，或2007年出生的同学\"}"
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
    setWindowTitle("个人通信录管理系统 — UI版");
    setMinimumSize(900, 600);

    // 菜单栏
    auto *menuBar = new QMenuBar(this);
    auto *fileMenu = menuBar->addMenu("文件(&F)");
    fileMenu->addAction("设置发件人姓名...", this, [this] {
        bool ok = false;
        QString name = QInputDialog::getText(this, "设置发件人姓名",
            "生日贺信署名：", QLineEdit::Normal, m_senderName, &ok);
        if (ok && !name.trimmed().isEmpty()) {
            m_senderName = name.trimmed();
            statusBar()->showMessage("发件人已设为：" + m_senderName, 3000);
        }
    });
    fileMenu->addAction("设置贺信保存路径...", this, [this] {
        QString dir = QFileDialog::getExistingDirectory(this, "选择贺信保存目录", m_emailPath);
        if (!dir.isEmpty()) {
            m_emailPath = dir;
            statusBar()->showMessage("贺信保存路径：" + m_emailPath, 3000);
        }
    });
    fileMenu->addSeparator();
    fileMenu->addAction("退出(&Q)", qApp, &QApplication::quit);

    auto *viewMenu = menuBar->addMenu("视图(&V)");
    viewMenu->addAction("切换主题(&T)", this, &MainWindow::onThemeToggle);
    viewMenu->addAction("数据统计图表(&C)", this, [this] {
        ChartDialog dlg(manager.getContacts(), this);
        dlg.resize(700, 420);
        dlg.exec();
    });
    setMenuBar(menuBar);

    // 快捷键
    auto *shortcutAdd  = new QShortcut(QKeySequence("Ctrl+N"), this);
    auto *shortcutDel  = new QShortcut(QKeySequence("Delete"), this);
    auto *shortcutSave = new QShortcut(QKeySequence("Ctrl+S"), this);
    auto *shortcutUndo = new QShortcut(QKeySequence("Ctrl+Z"), this);
    auto *shortcutRefresh = new QShortcut(QKeySequence("F5"), this);
    connect(shortcutAdd,  &QShortcut::activated, this, &MainWindow::onAdd);
    connect(shortcutDel,  &QShortcut::activated, this, &MainWindow::onDelete);
    connect(shortcutSave, &QShortcut::activated, this, &MainWindow::onSave);
    connect(shortcutUndo, &QShortcut::activated, this, [this] {
        if (m_lastAction == LastAction::None) {
            statusBar()->showMessage("没有可撤销的操作", 3000);
            return;
        }
        if (m_lastAction == LastAction::Add) {
            manager.deletePerson(m_lastTarget.toUtf8().toStdString());
            appendLog("==> 已撤销：添加 " + m_lastTarget);
        } else if (m_lastAction == LastAction::Delete) {
            // m_undoSnapshot: "type|name|year|month|day|phone|email|extra"
            auto parts = m_undoSnapshot.split('|');
            if (parts.size() >= 8) {
                Date d = {parts[2].toInt(), parts[3].toInt(), parts[4].toInt()};
                int t = parts[0].toInt();
                auto name = parts[1].toUtf8().toStdString();
                auto phone = parts[5].toUtf8().toStdString();
                auto email = parts[6].toUtf8().toStdString();
                auto extra = parts[7].toUtf8().toStdString();
                switch (t) {
                case 1: manager.addPerson(std::make_unique<Classmate>(name, d, phone, email, extra)); break;
                case 2: manager.addPerson(std::make_unique<Colleague>(name, d, phone, email, extra)); break;
                case 3: manager.addPerson(std::make_unique<Friend>(name, d, phone, email, extra)); break;
                case 4: manager.addPerson(std::make_unique<Relative>(name, d, phone, email, extra)); break;
                }
                appendLog("==> 已撤销：删除 " + m_lastTarget);
            }
        } else if (m_lastAction == LastAction::Edit) {
            // Same snapshot format as delete
            auto parts = m_undoSnapshot.split('|');
            if (parts.size() >= 8) {
                manager.deletePerson(m_lastTarget.toUtf8().toStdString());
                Date d = {parts[2].toInt(), parts[3].toInt(), parts[4].toInt()};
                int t = parts[0].toInt();
                auto name = parts[1].toUtf8().toStdString();
                auto phone = parts[5].toUtf8().toStdString();
                auto email = parts[6].toUtf8().toStdString();
                auto extra = parts[7].toUtf8().toStdString();
                switch (t) {
                case 1: manager.addPerson(std::make_unique<Classmate>(name, d, phone, email, extra)); break;
                case 2: manager.addPerson(std::make_unique<Colleague>(name, d, phone, email, extra)); break;
                case 3: manager.addPerson(std::make_unique<Friend>(name, d, phone, email, extra)); break;
                case 4: manager.addPerson(std::make_unique<Relative>(name, d, phone, email, extra)); break;
                }
                appendLog("==> 已撤销：修改 " + m_lastTarget);
            }
        }
        m_lastAction = LastAction::None;
        m_lastTarget.clear();
        m_undoSnapshot.clear();
        refreshTable();
    });
    connect(shortcutRefresh, &QShortcut::activated, this, [this] { refreshTable(); });

    // 整体布局
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *hLayout = new QHBoxLayout(central);
    hLayout->setContentsMargins(8, 8, 8, 8);
    hLayout->setSpacing(8);

    // 左侧面板：分组按钮
    auto *leftWidget = new QWidget(this);
    leftWidget->setFixedWidth(200);
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setSpacing(4);
    leftLayout->setContentsMargins(4, 4, 4, 4);

    auto mkBtn = [&](const QString &text, void (MainWindow::*slot)()) {
        auto *btn = new QPushButton(text, this);
        btn->setMinimumHeight(34);
        connect(btn, &QPushButton::clicked, this, slot);
        leftLayout->addWidget(btn);
    };

    auto mkTitle = [&](const QString &text) {
        auto *lbl = new QLabel(text, this);
        lbl->setStyleSheet("font-weight: bold; color: #007acc; font-size: 13px;"
                           "padding-top: 8px; padding-bottom: 2px;"
                           "border-bottom: 1px solid #3c3c3c; margin-bottom: 2px;");
        leftLayout->addWidget(lbl);
    };

    // ---- 编辑 ----
    mkTitle("📝 编辑");
    mkBtn("＋ 添加联系人", &MainWindow::onAdd);
    {
        auto *btn = new QPushButton("✕ 删除选中联系人", this);
        btn->setObjectName("dangerBtn");
        btn->setMinimumHeight(34);
        btn->setStyleSheet(
            "#dangerBtn { border: 1px solid #c04040; color: #e06060; }"
            "#dangerBtn:hover { background-color: #3d2020; }");
        connect(btn, &QPushButton::clicked, this, &MainWindow::onDelete);
        leftLayout->addWidget(btn);
    }
    mkBtn("✎ 修改选中联系人", &MainWindow::onEdit);

    // ---- 查询 ----
    mkTitle("🔍 查询");
    mkBtn("⌕ 按姓名查询", &MainWindow::onSearch);

    // ---- 排序 ----
    mkTitle("↕ 排序");
    {
        m_sortNameBtn = new QPushButton("↑ 按姓名排序", this);
        m_sortNameBtn->setMinimumHeight(34);
        connect(m_sortNameBtn, &QPushButton::clicked, this, &MainWindow::onSortByName);
        leftLayout->addWidget(m_sortNameBtn);
    }
    {
        m_sortBirthBtn = new QPushButton("↑ 按生日排序", this);
        m_sortBirthBtn->setMinimumHeight(34);
        connect(m_sortBirthBtn, &QPushButton::clicked, this, &MainWindow::onSortByBirth);
        leftLayout->addWidget(m_sortBirthBtn);
    }

    // ---- 统计 ----
    mkTitle("📊 统计");

    auto *statRow = new QHBoxLayout();
    monthCombo = new QComboBox(this);
    for (int m = 1; m <= 12; ++m)
        monthCombo->addItem(QString("%1月").arg(m));
    monthCombo->setCurrentIndex(QDate::currentDate().month() - 1);
    monthCombo->setMinimumWidth(65);
    statRow->addWidget(monthCombo);
    auto *statBtn = new QPushButton("统计", this);
    statBtn->setMinimumHeight(32);
    statBtn->setMinimumWidth(48);
    connect(statBtn, &QPushButton::clicked, this, &MainWindow::onMonthStat);
    statRow->addWidget(statBtn);
    leftLayout->addLayout(statRow);

    mkBtn("🎂 扫描5天生日 → 贺信", &MainWindow::onBirthScan);

    // ---- 查看 ----
    mkTitle("📋 查看");
    {
        auto *btn = new QPushButton("☰ 列出全体人员", this);
        btn->setMinimumHeight(34);
        connect(btn, &QPushButton::clicked, this, [this] {
            QString msg = captureStdout([&] { manager.showAllBrief(); });
            appendLog(msg.trimmed());
        });
        leftLayout->addWidget(btn);
    }

    auto *typeRow = new QHBoxLayout();
    auto *typeComboForView = new QComboBox(this);
    typeComboForView->addItems({"同学", "同事", "朋友", "亲戚"});
    typeComboForView->setMinimumWidth(80);
    typeRow->addWidget(typeComboForView);
    auto *viewTypeBtn = new QPushButton("查看", this);
    viewTypeBtn->setMinimumHeight(32);
    viewTypeBtn->setFixedWidth(58);
    connect(viewTypeBtn, &QPushButton::clicked, this, [this, typeComboForView] {
        int t = typeComboForView->currentIndex() + 1;
        QString msg = captureStdout([&] { manager.showByType(t); });
        appendLog(msg.trimmed());
    });
    typeRow->addWidget(viewTypeBtn);
    leftLayout->addLayout(typeRow);

    // ---- 系统 ----
    mkTitle("⚙ 系统");
    auto *saveBtn = new QPushButton("💾 保存数据", this);
    saveBtn->setMinimumHeight(34);
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::onSave);
    leftLayout->addWidget(saveBtn);
    auto *exitBtn = new QPushButton("🚪 退出程序", this);
    exitBtn->setMinimumHeight(34);
    connect(exitBtn, &QPushButton::clicked, qApp, &QApplication::quit);
    leftLayout->addWidget(exitBtn);

    leftLayout->addStretch();

    // 右侧面板
    auto *rightWidget = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(6);

    // 快捷搜索框
    quickSearch = new QLineEdit(this);
    quickSearch->setPlaceholderText("快速过滤姓名...  (Ctrl+F 跳转此处)");
    quickSearch->setClearButtonEnabled(true);
    rightLayout->addWidget(quickSearch);

    table = new QTableWidget(0, 6, this);
    table->setHorizontalHeaderLabels({"姓名", "生日", "电话", "Email", "类别", "详细信息"});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    // 交互增强：表头排序、右键菜单、双击编辑
    connect(table->horizontalHeader(), &QHeaderView::sectionClicked, this, &MainWindow::onHeaderClicked);
    table->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(table, &QTableWidget::customContextMenuRequested, this, &MainWindow::onTableContextMenu);
    connect(table, &QTableWidget::cellDoubleClicked, this, [this](int, int) { onEdit(); });
    rightLayout->addWidget(table, 4);
    // Ctrl+F → 焦点到搜索框
    auto *shortcutFind = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(shortcutFind, &QShortcut::activated, this, [this] { quickSearch->setFocus(); quickSearch->selectAll(); });

    // 快捷搜索实时过滤
    connect(quickSearch, &QLineEdit::textChanged, this, [this](const QString &text) {
        for (int row = 0; row < table->rowCount(); ++row) {
            auto *item = table->item(row, 0);
            bool match = item && item->text().contains(text, Qt::CaseInsensitive);
            table->setRowHidden(row, !match);
        }
    });

    // AI 输入行
    auto *aiRow = new QHBoxLayout();
    aiInput = new QLineEdit(this);
    aiInput->setPlaceholderText("AI: 输入指令（查一下张三 / 五月生日 / 列出所有朋友）");
    aiSendBtn = new QPushButton("发送", this);
    aiSendBtn->setMinimumHeight(28);
    aiSendBtn->setFixedWidth(60);
    auto *clearLogBtn = new QPushButton("清空日志", this);
    clearLogBtn->setFixedWidth(90);
    clearLogBtn->setMinimumHeight(28);
    connect(clearLogBtn, &QPushButton::clicked, this, [this] { log->clear(); });
    aiRow->addWidget(aiInput, 1);
    aiRow->addWidget(aiSendBtn);
    aiRow->addWidget(clearLogBtn);
    rightLayout->addLayout(aiRow);

    // 日志面板
    log = new QTextEdit(this);
    log->setReadOnly(true);
    log->document()->setMaximumBlockCount(1000);
    rightLayout->addWidget(log, 1);

    // LLM
    m_llm = new LLMAssistant(this);
    connect(m_llm, &LLMAssistant::commandReady, this, &MainWindow::onAiCommand);
    connect(m_llm, &LLMAssistant::statusMessage, this, &MainWindow::appendLog);
    connect(m_llm, &LLMAssistant::errorOccurred, this, [this](const QString &err) {
        appendLog("<span style='color:#e06060'>[AI 错误]</span> " + err);
    });
    connect(aiSendBtn, &QPushButton::clicked, this, &MainWindow::onAiSend);
    connect(aiInput, &QLineEdit::returnPressed, this, &MainWindow::onAiSend);

    hLayout->addWidget(leftWidget);
    hLayout->addWidget(rightWidget, 1);

    // 状态栏
    statusBar()->showMessage("正在加载数据...");

    try {
        QString msg = captureStdout([&] { manager.loadFromFile(); });
        if (!msg.trimmed().isEmpty())
            appendLog(msg.trimmed());
    } catch (const std::exception &e) {
        appendLog(QString("加载异常：%1").arg(e.what()));
    }
    refreshTable();

    // 保存主题 QSS
    m_darkQSS = qApp->styleSheet();
    m_lightQSS = R"(
        QMainWindow, QDialog, QWidget { background-color: #f5f5f5; color: #1e1e1e; }
        QPushButton { background-color: #e0e0e0; color: #1e1e1e; border: 1px solid #cccccc;
                      padding: 6px 16px; border-radius: 6px; }
        QPushButton:hover { background-color: #d0d0d0; border-color: #007acc; }
        QPushButton:pressed { background-color: #c0c0c0; }
        QTableWidget { background-color: #ffffff; color: #1e1e1e; gridline-color: #e8e8e8;
                       border: 1px solid #d0d0d0; alternate-background-color: #f8f8fa;
                       selection-background-color: #cce5ff; selection-color: #1e1e1e;
                       border-radius: 6px; }
        QTableWidget::item { padding: 6px 10px; border-bottom: 1px solid #f0f0f0; }
        QTableWidget::item:hover { background-color: #e8f0fa; }
        QTableWidget::item:selected { background-color: #cce5ff; color: #1e1e1e;
                                       border-left: 3px solid #007acc; }
        QHeaderView::section { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
            stop:0 #f0f0f0, stop:1 #e0e0e0); color: #333333; border: none;
            border-right: 1px solid #d0d0d0; border-bottom: 2px solid #007acc;
            padding: 8px 10px; font-weight: bold; font-size: 13px; }
        QPlainTextEdit { background-color: #ffffff; color: #1e1e1e; border: 1px solid #cccccc; }
        QLineEdit, QComboBox, QDateEdit, QSpinBox { background-color: #ffffff; color: #1e1e1e;
            border: 1px solid #cccccc; padding: 6px 10px; border-radius: 4px; }
        QLineEdit:focus, QComboBox:focus { border-color: #007acc; }
        QTextEdit { background-color: #fafafa; color: #333333; border: 1px solid #dddddd;
                    border-radius: 4px; font-family: "Consolas", "Courier New", monospace;
                    font-size: 12px; }
        QComboBox::drop-down { border: none; width: 20px; }
        QComboBox QAbstractItemView { background-color: #ffffff; color: #1e1e1e;
            selection-background-color: #007acc; }
        QLabel, QGroupBox { color: #1e1e1e; }
        QGroupBox { border: 1px solid #cccccc; border-radius: 6px; margin-top: 8px; padding-top: 16px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }
        QScrollBar:vertical { background-color: #f0f0f0; width: 10px; }
        QScrollBar::handle:vertical { background-color: #cccccc; border-radius: 5px; min-height: 20px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
        QScrollBar:horizontal { background-color: #f0f0f0; height: 10px; }
        QScrollBar::handle:horizontal { background-color: #cccccc; border-radius: 5px; min-width: 20px; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }
        QTableWidget::item:selected { background-color: #007acc; color: white; }
        QDialog QPushButton { min-width: 80px; }
        #dangerBtn { border: 1px solid #d04040; color: #c02020; }
        #dangerBtn:hover { background-color: #fde8e8; }
    )";
}

MainWindow::~MainWindow() = default;

void MainWindow::appendLog(const QString &msg) {
    log->append(msg);
}

void MainWindow::onHeaderClicked(int section) {
    if (section == 0) {
        // 姓名列：上次若是升序则本次降序，交替切换
        manager.sortByName();
        if (!m_sortNameAsc) {
            auto &contacts = const_cast<std::vector<std::unique_ptr<Person>>&>(manager.getContacts());
            std::reverse(contacts.begin(), contacts.end());
        }
        // 箭头反映当前排序方向（翻转前读取）
        table->horizontalHeaderItem(0)->setText(m_sortNameAsc ? "姓名 ▲" : "姓名 ▼");
        table->horizontalHeaderItem(1)->setText("生日");
        m_sortNameBtn->setText(m_sortNameAsc ? "↑ 按姓名排序" : "↓ 按姓名排序");
        m_sortBirthBtn->setText("↑ 按生日排序");
        m_sortNameAsc = !m_sortNameAsc;
    } else if (section == 1) {
        // 生日列
        manager.sortByBirthDate();
        if (!m_sortBirthAsc) {
            auto &contacts = const_cast<std::vector<std::unique_ptr<Person>>&>(manager.getContacts());
            std::reverse(contacts.begin(), contacts.end());
        }
        table->horizontalHeaderItem(0)->setText("姓名");
        table->horizontalHeaderItem(1)->setText(m_sortBirthAsc ? "生日 ▲" : "生日 ▼");
        m_sortBirthBtn->setText(m_sortBirthAsc ? "↑ 按生日排序" : "↓ 按生日排序");
        m_sortNameBtn->setText("↑ 按姓名排序");
        m_sortBirthAsc = !m_sortBirthAsc;
    }
    refreshTable();
}

void MainWindow::onTableContextMenu(const QPoint &pos) {
    int row = table->rowAt(pos.y());
    if (row < 0) return;
    table->selectRow(row);

    auto *menu = new QMenu(this);
    auto *actView = menu->addAction("查看详情");
    auto *actEdit = menu->addAction("修改...");
    auto *actDel  = menu->addAction("删除");
    menu->addSeparator();
    auto *actCopy = menu->addAction("复制姓名");

    connect(actView, &QAction::triggered, this, [this] { onSearch(); });
    connect(actEdit, &QAction::triggered, this, &MainWindow::onEdit);
    connect(actDel,  &QAction::triggered, this, &MainWindow::onDelete);
    connect(actCopy, &QAction::triggered, this, [this] {
        int r = table->currentRow();
        if (r >= 0 && table->item(r, 0)) {
            QApplication::clipboard()->setText(table->item(r, 0)->text());
            statusBar()->showMessage("已复制到剪贴板", 2000);
        }
    });

    menu->popup(table->viewport()->mapToGlobal(pos));
}

void MainWindow::onThemeToggle() {
    m_darkTheme = !m_darkTheme;
    qApp->setStyleSheet(m_darkTheme ? m_darkQSS : m_lightQSS);
    refreshTable();
    statusBar()->showMessage(m_darkTheme ? "已切换至暗色主题" : "已切换至亮色主题", 2000);
}

void MainWindow::updateStatusBar() {
    const auto &contacts = manager.getContacts();
    int counts[4] = {0};
    for (const auto &p : contacts)
        if (p->getType() >= 1 && p->getType() <= 4)
            counts[p->getType() - 1]++;
    statusBar()->showMessage(
        QString("共 %1 人  │  🔵同学 %2  🟠同事 %3  🟢朋友 %4  🟣亲戚 %5")
            .arg(contacts.size())
            .arg(counts[0]).arg(counts[1]).arg(counts[2]).arg(counts[3]));
}

void MainWindow::saveUndoSnapshot(const QString &action, const QString &name,
                                  const QString &snapshot) {
    if (action == "Add") {
        m_lastAction = LastAction::Add;
        m_lastTarget = name;
        m_undoSnapshot.clear();
    } else if (action == "Delete") {
        m_lastAction = LastAction::Delete;
        m_lastTarget = name;
        m_undoSnapshot = snapshot;
    } else if (action == "Edit") {
        m_lastAction = LastAction::Edit;
        m_lastTarget = name;
        m_undoSnapshot = snapshot;
    }
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

        // 近5天生日的行高亮（淡金色背景）
        std::string weekday;
        if (p->getBirthDate().isBirthdayWithin5Days(weekday)) {
            QColor highlight(0x2d, 0x2a, 0x10); // 暗色主题下的淡金
            if (!m_darkTheme)
                highlight = QColor(0xff, 0xf3, 0xcd); // 亮色主题下的淡金
            for (int col = 0; col < 6; ++col)
                if (auto *it = table->item(row, col))
                    it->setBackground(highlight);
        }
    }

    updateStatusBar();

    // 刷新后重新应用快捷搜索过滤
    QString filterText = quickSearch->text();
    if (!filterText.isEmpty()) {
        for (int row = 0; row < table->rowCount(); ++row) {
            auto *item = table->item(row, 0);
            bool match = item && item->text().contains(filterText, Qt::CaseInsensitive);
            table->setRowHidden(row, !match);
        }
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
    saveUndoSnapshot("Add", QString::fromUtf8(name.c_str()));
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

    // 撤销快照：保存旧联系人完整信息
    std::string ff_old = p->formatForFile();
    saveUndoSnapshot("Edit", QString::fromUtf8(originalName.c_str()),
                     QString::fromUtf8(ff_old.c_str()));

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

    // 撤销快照
    std::string ff = contacts[row]->formatForFile();
    saveUndoSnapshot("Delete", displayName, QString::fromUtf8(ff.c_str()));

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
    manager.sortByName();
    if (!m_sortNameAsc) {
        auto &contacts = const_cast<std::vector<std::unique_ptr<Person>>&>(manager.getContacts());
        std::reverse(contacts.begin(), contacts.end());
    }
    table->horizontalHeaderItem(0)->setText(m_sortNameAsc ? "姓名 ▲" : "姓名 ▼");
    table->horizontalHeaderItem(1)->setText("生日");
    m_sortNameAsc = !m_sortNameAsc;
    m_sortNameBtn->setText(m_sortNameAsc ? "↑ 按姓名排序" : "↓ 按姓名排序");
    m_sortBirthBtn->setText("↑ 按生日排序");
    refreshTable();
    appendLog(m_sortNameAsc ? "==> 已按姓名升序排列" : "==> 已按姓名降序排列");
}

void MainWindow::onSortByBirth() {
    manager.sortByBirthDate();
    if (!m_sortBirthAsc) {
        auto &contacts = const_cast<std::vector<std::unique_ptr<Person>>&>(manager.getContacts());
        std::reverse(contacts.begin(), contacts.end());
    }
    table->horizontalHeaderItem(0)->setText("姓名");
    table->horizontalHeaderItem(1)->setText(m_sortBirthAsc ? "生日 ▲" : "生日 ▼");
    m_sortBirthAsc = !m_sortBirthAsc;
    m_sortBirthBtn->setText(m_sortBirthAsc ? "↑ 按生日排序" : "↓ 按生日排序");
    m_sortNameBtn->setText("↑ 按姓名排序");
    refreshTable();
    appendLog(m_sortBirthAsc ? "==> 已按生日升序排列" : "==> 已按生日降序排列");
}

void MainWindow::onMonthStat() {
    int month = monthCombo->currentIndex() + 1;
    QString msg = captureStdout([&] { manager.countByBirthMonth(month); });
    appendLog(msg.trimmed());
}

void MainWindow::onBirthScan() {
    QString msg = captureStdout([&] { manager.checkBirthdaysAndSendEmails(
        m_senderName.toUtf8().toStdString(),
        m_emailPath.toUtf8().toStdString()); });
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

    appendLog(QString("<span style='color:#5ea6e0'><b>[用户]</b> %1</span>").arg(text));
    aiInput->clear();

    QString apiKey = QString(LLM_API_KEY);
    if (apiKey.isEmpty() || apiKey == "在此填入你的 API Key") {
        appendLog("<span style='color:#e06060'><b>[AI 错误]</b> 请先在 MainWindow.hpp 中配置 LLM_API_KEY</span>");
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
        saveUndoSnapshot("Add", cmd["name"].toString());
    }

    else if (action == "delete") {
        std::string name = cmd["name"].toString().toUtf8().toStdString();
        if (name.empty()) {
            appendLog("==> 删除失败：缺少姓名");
            return;
        }
        // 撤销快照
        Person *toDel = manager.searchByName(name);
        if (toDel) {
            std::string ff = toDel->formatForFile();
            saveUndoSnapshot("Delete", cmd["name"].toString(),
                             QString::fromUtf8(ff.c_str()));
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

        // 任务书要求：姓名和出生日期不可修改 → 先从原联系人取出原始值
        Person *old = manager.searchByName(oldName);
        if (!old) {
            appendLog(QString("==> 未找到「%1」，修改失败。").arg(cmd["oldName"].toString()));
            return;
        }
        std::string lockedName = old->getName();
        Date lockedBirth = old->getBirthDate();
        int lockedType = old->getType();

        // 撤销快照
        std::string ff_old = old->formatForFile();
        saveUndoSnapshot("Edit", cmd["oldName"].toString(),
                         QString::fromUtf8(ff_old.c_str()));

        manager.deletePerson(oldName);

        // 电话和邮箱使用新值，姓名/生日/类别 强制使用原始值
        std::string phone = cmd["phone"].toString().toUtf8().toStdString();
        std::string email = cmd["email"].toString().toUtf8().toStdString();
        std::string extra = cmd["extra"].toString().toUtf8().toStdString();

        switch (lockedType) {
        case 1: manager.addPerson(std::make_unique<Classmate>(lockedName, lockedBirth, phone, email, extra)); break;
        case 2: manager.addPerson(std::make_unique<Colleague>(lockedName, lockedBirth, phone, email, extra)); break;
        case 3: manager.addPerson(std::make_unique<Friend>(lockedName, lockedBirth, phone, email, extra)); break;
        case 4: manager.addPerson(std::make_unique<Relative>(lockedName, lockedBirth, phone, email, extra)); break;
        }
        refreshTable();
        appendLog(QString("==> AI 已更新联系人：%1（姓名和生日已锁定）").arg(QString::fromUtf8(lockedName.c_str())));
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
        QString sender = cmd["sender"].toString(m_senderName);
        QString msg = captureStdout(
            [&] { manager.checkBirthdaysAndSendEmails(
                sender.toUtf8().toStdString(),
                m_emailPath.toUtf8().toStdString()); });
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

    else if (action == "filter") {
        // 组合筛选：type, year, month, name, surname 任意组合
        QString typeFilter = cmd["type"].toString();
        int year = cmd["year"].toInt(0);
        int month = cmd["month"].toInt(0);
        QString nameFilter = cmd["name"].toString();
        QString surnameFilter = cmd["surname"].toString();

        auto match = [&](const std::unique_ptr<Person> &p) -> bool {
            if (!typeFilter.isEmpty()) {
                int idx = typeNameToIndex(typeFilter);
                if (idx < 0 || p->getType() != idx) return false;
            }
            if (year > 0 && p->getBirthDate().year != year) return false;
            if (month >= 1 && month <= 12 && p->getBirthDate().month != month)
                return false;
            if (!nameFilter.isEmpty() && p->getName() != nameFilter.toUtf8().toStdString())
                return false;
            if (!surnameFilter.isEmpty()) {
                auto surname = surnameFilter.toUtf8().toStdString();
                auto name = p->getName();
                if (name.size() < surname.size() ||
                    name.compare(0, surname.size(), surname) != 0)
                    return false;
            }
            return true;
        };

        // 构建条件描述
        QStringList conds;
        if (!typeFilter.isEmpty()) conds << typeFilter;
        if (!surnameFilter.isEmpty()) conds << QString("姓\"%1\"").arg(surnameFilter);
        if (year > 0) conds << QString("%1年出生").arg(year);
        if (month >= 1 && month <= 12) conds << QString("%1月生日").arg(month);
        if (!nameFilter.isEmpty()) conds << QString("姓名\"%1\"").arg(nameFilter);

        int count = 0;
        appendLog(conds.isEmpty()
                     ? "==> 列出全部联系人："
                     : QString("==> 筛选条件：%1").arg(conds.join("，")));
        for (const auto &p : manager.getContacts()) {
            if (match(p)) {
                QString line = captureStdout([&p] { p->display(); });
                appendLog(line.trimmed());
                count++;
            }
        }
        appendLog(QString("==> 共 %1 人。").arg(count));
    }

    else if (action == "save") {
        onSave();
    }

    else if (action == "reply") {
        appendLog(QString("<span style='color:#6aaf6a'><b>[AI]</b> %1</span>").arg(cmd["text"].toString()));
    }

    else {
        appendLog(QString("==> 无法识别的操作：%1").arg(action));
    }
}

// ===============================================================
//  ChartWidget / ChartDialog — 数据可视化
// ===============================================================
ChartWidget::ChartWidget(const std::vector<std::unique_ptr<Person>> &contacts,
                         QWidget *parent)
    : QWidget(parent), m_contacts(contacts) {
    setMinimumSize(660, 380);
}

void ChartWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width(), h = height();
    int pieW = w / 2;

    // ── 饼图（左半）──
    static const char *typeNames[] = {"同学", "同事", "朋友", "亲戚"};
    static const QColor colors[] = {
        QColor(0x00, 0x7a, 0xcc),  // 蓝
        QColor(0xd4, 0x6b, 0x08),  // 橙
        QColor(0x2c, 0xa0, 0x2c),  // 绿
        QColor(0x94, 0x60, 0xbd),  // 紫
    };

    int counts[4] = {0};
    for (const auto &c : m_contacts)
        if (c->getType() >= 1 && c->getType() <= 4)
            counts[c->getType() - 1]++;

    int total = counts[0] + counts[1] + counts[2] + counts[3];
    if (total == 0) { p.drawText(0, 0, w, h, Qt::AlignCenter, "暂无数据"); return; }

    // 自适应文字颜色
    bool isDark = palette().color(QPalette::Window).lightness() < 128;
    QColor textColor = isDark ? QColor(0xd4, 0xd4, 0xd4) : QColor(0x33, 0x33, 0x33);

    // 饼图 — 缩小并上移，给下方图例留空间
    int pieCX = pieW / 2, pieCY = h / 3;
    int pieR = std::min(pieCX, pieCY) - 50;
    if (pieR < 50) pieR = 50;
    QRect pieRect(pieCX - pieR, pieCY - pieR, pieR * 2, pieR * 2);

    int startAngle = 90 * 16; // 从12点方向开始
    for (int i = 0; i < 4; ++i) {
        if (counts[i] == 0) continue;
        int span = (int)(counts[i] * 360.0 * 16 / total);
        p.setBrush(colors[i]);
        p.setPen(Qt::NoPen);
        p.drawPie(pieRect, startAngle, span);
        startAngle += span;
    }

    // 饼图标题 — 放在饼图正上方
    p.setPen(QPen(textColor));
    QFont titleFont = p.font();
    titleFont.setPointSize(11); titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(QRect(0, pieCY - pieR - 30, pieW, 25), Qt::AlignCenter, "联系人分布");

    // 图例 — 饼图正下方，竖排四行
    QFont legendFont = p.font();
    legendFont.setPointSize(10); legendFont.setBold(false);
    p.setFont(legendFont);
    int legendStartY = pieCY + pieR + 25;
    for (int i = 0; i < 4; ++i) {
        double pct = total > 0 ? counts[i] * 100.0 / total : 0.0;
        int ly = legendStartY + i * 28;
        p.setBrush(colors[i]);
        p.setPen(Qt::NoPen);
        int blockX = pieCX - 80;
        p.drawRect(blockX, ly, 14, 14);
        p.setPen(QPen(textColor));
        p.drawText(blockX + 20, ly + 13,
                   QString("%1  %2人 (%3%)").arg(typeNames[i]).arg(counts[i]).arg(pct, 0, 'f', 1));
    }

    // ── 柱状图（右半）──
    int barX = pieW + 20, barW = w - barX - 20;
    int barBottom = h - 40, barTop = 50;

    // 坐标轴
    p.setPen(QPen(QColor(0x55, 0x55, 0x55)));
    p.drawLine(barX, barTop, barX, barBottom);          // Y轴
    p.drawLine(barX, barBottom, barX + barW, barBottom); // X轴

    // 每月生日人数
    int monthCounts[12] = {0};
    for (const auto &c : m_contacts)
        if (c->getBirthDate().month >= 1 && c->getBirthDate().month <= 12)
            monthCounts[c->getBirthDate().month - 1]++;

    int maxCount = 1;
    for (int m = 0; m < 12; ++m)
        if (monthCounts[m] > maxCount) maxCount = monthCounts[m];

    p.setPen(QPen(textColor));
    p.drawText(QRect(barX, 5, barW, 30), Qt::AlignCenter, "每月生日人数");

    double colW = (double)barW / 12;
    for (int m = 0; m < 12; ++m) {
        int barH = (int)(monthCounts[m] * (double)(barBottom - barTop) / maxCount);
        int x = barX + (int)(m * colW) + 3;
        int y = barBottom - barH;
        int bw = (int)colW - 6;
        if (bw < 4) bw = 4;

        p.setBrush(QColor(0x00, 0x7a, 0xcc));
        p.setPen(Qt::NoPen);
        p.drawRect(x, y, bw, barH);

        if (monthCounts[m] > 0) {
            p.setPen(QPen(textColor));
            QFont numFont = p.font();
            numFont.setPointSize(7);
            p.setFont(numFont);
            p.drawText(x, y - 12, bw, 14, Qt::AlignCenter, QString::number(monthCounts[m]));
        }

        // 月份标签
        p.setPen(QPen(QColor(0x88, 0x88, 0x88)));
        QFont mFont = p.font();
        mFont.setPointSize(7);
        p.setFont(mFont);
        p.drawText(x, barBottom + 5, bw, 20, Qt::AlignCenter,
                   QString("%1月").arg(m + 1));
    }
}

ChartDialog::ChartDialog(const std::vector<std::unique_ptr<Person>> &contacts,
                         QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("数据统计图表");
    setStyleSheet(qApp->styleSheet()); // 跟随主窗口主题
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new ChartWidget(contacts, this));
}
