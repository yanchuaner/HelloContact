#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

/**
 * @brief 日期处理结构体，封装了日期的基本属性及相关操作
 */
struct Date {
    int year;
    int month;
    int day;

    /**
     * @brief 格式化输出日期
     * @return YYYY-MM-DD 格式的字符串
     */
    string toString() const {
        return to_string(year) + "-" + to_string(month) + "-" + to_string(day);
    }

    /**
     * @brief 检查该日期的生日是否在当前时间的 5 天内
     * @param outWeekday 若在 5 天内，此引用参数返回该生日当天的中文星期几（如"星期一"）
     * @return true 若在5天内，否则 false
     */
    bool isBirthdayWithin5Days(string &outWeekday) const {
        time_t nowTime = time(nullptr);
        tm *currentTm = localtime(&nowTime);

        tm bdayTm = *currentTm;
        bdayTm.tm_mon = month - 1;
        bdayTm.tm_mday = day;
        bdayTm.tm_hour = 0;
        bdayTm.tm_min = 0;
        bdayTm.tm_sec = 0;

        time_t bdayTime = mktime(&bdayTm);

        // 如果今年的生日已经过去了，就算明年的
        if (bdayTime < nowTime) {
            bdayTm.tm_year += 1;
            bdayTime = mktime(&bdayTm);
        }

        double diffDays = difftime(bdayTime, nowTime) / (60 * 60 * 24);

        if (diffDays >= 0 && diffDays <= 5) {
            const char *weekdays[] = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"};
            outWeekday = weekdays[bdayTm.tm_wday];
            return true;
        }
        return false;
    }
};

// ==========================================
// 2. 类继承体系 (C++ 多态与面向对象设计)
// ==========================================

/**
 * @brief 联系人基类，包含基本信息并提供多态接口
 */
class Person {
  protected:
    string name;    // 姓名
    Date birthDate; // 出生日期
    string phone;   // 电话号码
    string email;   // 电子邮件
    int type;       // 联系人分类: 1-同学, 2-同事, 3-朋友, 4-亲戚

  public:
    Person(const string &n, const Date &d, const string &p, const string &e, int t)
        : name(n), birthDate(d), phone(p), email(e), type(t) {}
    virtual ~Person() {}

    /**
     * @brief 纯虚函数：多态输出联系人信息到控制台
     */
    virtual void display() const = 0;

    /**
     * @brief 纯虚函数：将联系人信息序列化为用于保存到文件的格式（如 CSV）
     */
    virtual string formatForFile() const = 0;

    // --- Getter 方法 ---
    string getName() const { return name; }
    Date getBirthDate() const { return birthDate; }
    int getType() const { return type; }
    string getEmail() const { return email; }

    // --- Setter 方法 ---
    void setPhone(const string &newPhone) { phone = newPhone; }
    void setEmail(const string &newEmail) { email = newEmail; }
};

/**
 * @brief 派生类 1：同学 (新增 school 字段)
 */
class Classmate : public Person {
  private:
    string school; // 学校名称
  public:
    Classmate(const string &n, const Date &d, const string &p, const string &e, const string &s)
        : Person(n, d, p, e, 1), school(s) {}

    void display() const override {
        cout << "[同学] 姓名: " << name << ", 生日: " << birthDate.toString()
             << ", 电话: " << phone << ", Email: " << email << ", 学校: " << school << endl;
    }
    string formatForFile() const override {
        return name + "," + to_string(birthDate.year) + "," + to_string(birthDate.month) + "," + to_string(birthDate.day) + "," + phone + "," + email + "," + school;
    }
};

/**
 * @brief 派生类 2：同事 (新增 company 字段)
 */
class Colleague : public Person {
  private:
    string company; // 单位名称
  public:
    Colleague(const string &n, const Date &d, const string &p, const string &e, const string &c)
        : Person(n, d, p, e, 2), company(c) {}

    void display() const override {
        cout << "[同事] 姓名: " << name << ", 生日: " << birthDate.toString()
             << ", 电话: " << phone << ", Email: " << email << ", 单位: " << company << endl;
    }
    string formatForFile() const override {
        return name + "," + to_string(birthDate.year) + "," + to_string(birthDate.month) + "," + to_string(birthDate.day) + "," + phone + "," + email + "," + company;
    }
};

/**
 * @brief 派生类 3：朋友 (新增 place 字段)
 */
class Friend : public Person {
  private:
    string place; // 认识地点
  public:
    Friend(const string &n, const Date &d, const string &p, const string &e, const string &pl)
        : Person(n, d, p, e, 3), place(pl) {}

    void display() const override {
        cout << "[朋友] 姓名: " << name << ", 生日: " << birthDate.toString()
             << ", 电话: " << phone << ", Email: " << email << ", 地点: " << place << endl;
    }
    string formatForFile() const override {
        return name + "," + to_string(birthDate.year) + "," + to_string(birthDate.month) + "," + to_string(birthDate.day) + "," + phone + "," + email + "," + place;
    }
};

/**
 * @brief 派生类 4：亲戚 (新增 relation 字段)
 */
class Relative : public Person {
  private:
    string relation; // 称呼
  public:
    Relative(const string &n, const Date &d, const string &p, const string &e, const string &r)
        : Person(n, d, p, e, 4), relation(r) {}

    void display() const override {
        cout << "[亲戚] 姓名: " << name << ", 生日: " << birthDate.toString()
             << ", 电话: " << phone << ", Email: " << email << ", 称呼: " << relation << endl;
    }
    string formatForFile() const override {
        return name + "," + to_string(birthDate.year) + "," + to_string(birthDate.month) + "," + to_string(birthDate.day) + "," + phone + "," + email + "," + relation;
    }
};

// ==========================================
// 3. 通信录管理器 (负责业务逻辑与文件调度)
// ==========================================
/**
 * @brief 管理所有联系人的添加、删除、排序、存取文件等操作
 */
class AddressBookManager {
  private:
    vector<Person *> contacts; // 存放所有多态的联系人指针

  public:
    ~AddressBookManager() {
        for (Person *p : contacts) {
            delete p;
        }
    }

    /**
     * @brief 向管理器的内存中添加一名联系人
     * @param p 动态分配的派生类指针
     */
    void addPerson(Person *p) {
        contacts.push_back(p);
    }

    /**
     * @brief 保存当前内存中的联系人至本地 TXT 文件
     * 根据类别分成4个文件 (AddressBook1~4.txt)
     */
    void saveToFile() {
        ofstream out1("AddressBook1.txt"); // 同学
        ofstream out2("AddressBook2.txt"); // 同事
        ofstream out3("AddressBook3.txt"); // 朋友
        ofstream out4("AddressBook4.txt"); // 亲戚

        for (Person *p : contacts) {
            string dataLine = p->formatForFile() + "\n";
            switch (p->getType()) {
            case 1:
                out1 << dataLine;
                break;
            case 2:
                out2 << dataLine;
                break;
            case 3:
                out3 << dataLine;
                break;
            case 4:
                out4 << dataLine;
                break;
            }
        }
        cout << "==> 系统提示：所有通信录数据已成功保存至本地文件！" << endl;
    }

    // 2. 从 4 个 TXT 文件加载数据
    void loadFromFile() {
        // 先清空当前内存数据，避免重复加载
        for (Person *p : contacts) {
            delete p;
        }
        contacts.clear();

        // 辅助 Lambda 表达式：用于解析单行 CSV 并返回分割后的字符串数组
        auto parseCSVLine = [](const string &line) -> vector<string> {
            vector<string> result;
            stringstream ss(line);
            string token;
            while (getline(ss, token, ',')) {
                result.push_back(token);
            }
            return result;
        };

        // 辅助 Lambda 表达式：加载特定类别的文件
        auto loadCategory = [&](const string &filename, int type) {
            ifstream in(filename);
            string line;
            while (getline(in, line)) {
                if (line.empty())
                    continue;
                vector<string> tokens = parseCSVLine(line);
                if (tokens.size() == 7) {
                    Date d = {stoi(tokens[1]), stoi(tokens[2]), stoi(tokens[3])};
                    switch (type) {
                    case 1:
                        addPerson(new Classmate(tokens[0], d, tokens[4], tokens[5], tokens[6]));
                        break;
                    case 2:
                        addPerson(new Colleague(tokens[0], d, tokens[4], tokens[5], tokens[6]));
                        break;
                    case 3:
                        addPerson(new Friend(tokens[0], d, tokens[4], tokens[5], tokens[6]));
                        break;
                    case 4:
                        addPerson(new Relative(tokens[0], d, tokens[4], tokens[5], tokens[6]));
                        break;
                    }
                }
            }
        };

        loadCategory("AddressBook1.txt", 1); // 加载同学
        loadCategory("AddressBook2.txt", 2); // 加载同事
        loadCategory("AddressBook3.txt", 3); // 加载朋友
        loadCategory("AddressBook4.txt", 4); // 加载亲戚

        cout << "==> 系统提示：本地数据加载完毕！当前共有联系人 " << contacts.size() << " 名。" << endl;
    }

    // 3. 自动生成祝贺生日邮件文本文件
    void generateBirthdayEmail(const string &receiverName, const string &myName) {
        // 使用被祝贺人的姓名作为文件名的一部分
        string filename = "BirthdayEmail_" + receiverName + ".txt";
        ofstream out(filename);
        if (out.is_open()) {
            out << "被祝贺人姓名: " << receiverName << endl;
            out << "祝生日快乐，健康幸福。" << endl;
            out << "祝贺人姓名: " << myName << endl;
            out.close();
            cout << "已自动为 " << receiverName << " 生成祝贺邮件: " << filename << endl;
        }
    }
    // ==========================================
    // 1. 查询功能：按姓名查找
    // ==========================================
    Person *searchByName(const string &targetName) const {
        for (Person *p : contacts) {
            if (p->getName() == targetName) {
                return p; // 找到直接返回指针
            }
        }
        return nullptr; // 未找到
    }

    // ==========================================
    // 2. 编辑功能：修改与删除
    // ==========================================
    bool deletePerson(const string &targetName) {
        for (auto it = contacts.begin(); it != contacts.end(); ++it) {
            if ((*it)->getName() == targetName) {
                delete *it;         // 释放堆内存，防止内存泄漏
                contacts.erase(it); // 从 vector 中移除
                cout << "==> 成功删除联系人：" << targetName << endl;
                return true;
            }
        }
        cout << "==> 未找到名为 " << targetName << " 的联系人，删除失败。" << endl;
        return false;
    }

    // 修改基本信息 (供外层菜单调用)
    void modifyPerson(const string &targetName, const string &newPhone, const string &newEmail) {
        Person *p = searchByName(targetName);
        if (p != nullptr) {
            p->setPhone(newPhone);
            p->setEmail(newEmail);
            cout << "==> " << targetName << " 的联系方式已更新！" << endl;
        } else {
            cout << "==> 未找到联系人，修改失败。" << endl;
        }
    }

    // ==========================================
    // 3. 排序功能：按姓名或出生日期排序
    // ==========================================
    void sortByName() {
        // 使用 Lambda 表达式定义排序规则
        sort(contacts.begin(), contacts.end(), [](Person *a, Person *b) {
            return a->getName() < b->getName();
        });
        cout << "==> 已按姓名(拼音/英文)排序完成。" << endl;
    }

    void sortByBirthDate() {
        sort(contacts.begin(), contacts.end(), [](Person *a, Person *b) {
            Date d1 = a->getBirthDate();
            Date d2 = b->getBirthDate();
            if (d1.year != d2.year)
                return d1.year < d2.year;
            if (d1.month != d2.month)
                return d1.month < d2.month;
            return d1.day < d2.day;
        });
        cout << "==> 已按出生日期排序完成。" << endl;
    }

    // ==========================================
    // 4. 统计与筛选功能
    // ==========================================
    // 统计给定月份出生的人数
    void countByBirthMonth(int targetMonth) const {
        int count = 0;
        cout << "==> " << targetMonth << " 月份出生的联系人如下：" << endl;
        for (const Person *p : contacts) {
            if (p->getBirthDate().month == targetMonth) {
                p->display();
                count++;
            }
        }
        cout << "==> 共计 " << count << " 人。" << endl;
    }

    // 分别列出特定类型的所有信息 (1-同学, 2-同事, 3-朋友, 4-亲戚)
    void showByType(int targetType) const {
        int count = 0;
        for (const Person *p : contacts) {
            if (p->getType() == targetType) {
                p->display();
                count++;
            }
        }
        if (count == 0)
            cout << "==> 该类别下暂无联系人。" << endl;
    }

    // ==========================================
    // 5. 综合业务：检查5天内生日并触发邮件
    // ==========================================
    void checkBirthdaysAndSendEmails(const string &myName) {
        bool foundAny = false;
        cout << "==> 正在扫描最近 5 天内过生日的联系人..." << endl;

        for (Person *p : contacts) {
            string weekdayStr;
            if (p->getBirthDate().isBirthdayWithin5Days(weekdayStr)) {
                foundAny = true;
                // 按照任务书要求格式化输出
                cout << p->getBirthDate().month << "月" << p->getBirthDate().day << "日";
                cout << "（" << weekdayStr << "） ";
                p->display();

                // 触发之前写好的生成邮件逻辑
                generateBirthdayEmail(p->getName(), myName);
            }
        }
        if (!foundAny) {
            cout << "==> 最近 5 天内没有联系人过生日。" << endl;
        }
    }
    // 测试：打印所有人
    void showAll() const {
        for (const Person *p : contacts) {
            p->display();
        }
    }
};

// ==========================================
// 4. 主程序入口与人机交互菜单
// ==========================================
void printMenu() {
    cout << "\n=========================================\n";
    cout << "          个人通信录管理系统\n";
    cout << "=========================================\n";
    cout << "  1. 录入个人信息\n";
    cout << "  2. 修改个人联系方式\n";
    cout << "  3. 删除个人信息\n";
    cout << "  4. 按姓名查询信息\n";
    cout << "  5. 显示所有联系人 (按原序)\n";
    cout << "  6. 按类别列出信息\n";
    cout << "  7. 按姓名排序输出\n";
    cout << "  8. 按出生日期排序输出\n";
    cout << "  9. 统计指定月份出生人数\n";
    cout << " 10. 扫描5天内生日并生成祝贺邮件\n";
    cout << " 11. 保存数据并退出系统\n";
    cout << "=========================================\n";
    cout << "请选择操作指令 (1-11): ";
}

int main() {
    AddressBookManager manager;

    // 启动时自动加载本地数据
    cout << "系统初始化中...\n";
    // 如果是第一次运行，没有TXT文件会静默跳过；如果有则加载。
    manager.loadFromFile();

    int choice;
    string myName = "Eon"; // 你的名字，用于邮件落款

    while (true) {
        printMenu();
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "==> 输入无效，请输入数字！\n";
            continue;
        }

        switch (choice) {
        case 1: {
            // 录入功能的交互逻辑
            cout << "请选择录入类别 (1-同学, 2-同事, 3-朋友, 4-亲戚): ";
            int type;
            cin >> type;

            string name, phone, email, extra;
            int y, m, d;
            cout << "请输入姓名: ";
            cin >> name;
            cout << "请输入出生日期 (格式: 年 月 日，用空格分隔): ";
            cin >> y >> m >> d;
            cout << "请输入电话: ";
            cin >> phone;
            cout << "请输入Email: ";
            cin >> email;

            Date birth = {y, m, d};
            if (type == 1) {
                cout << "请输入学校名称: ";
                cin >> extra;
                manager.addPerson(new Classmate(name, birth, phone, email, extra));
            } else if (type == 2) {
                cout << "请输入单位名称: ";
                cin >> extra;
                manager.addPerson(new Colleague(name, birth, phone, email, extra));
            } else if (type == 3) {
                cout << "请输入认识地点: ";
                cin >> extra;
                manager.addPerson(new Friend(name, birth, phone, email, extra));
            } else if (type == 4) {
                cout << "请输入称呼: ";
                cin >> extra;
                manager.addPerson(new Relative(name, birth, phone, email, extra));
            } else {
                cout << "==> 无效的类别！\n";
            }
            cout << "==> 录入成功！\n";
            break;
        }
        case 2: {
            string name, phone, email;
            cout << "请输入要修改的联系人姓名: ";
            cin >> name;
            cout << "请输入新电话: ";
            cin >> phone;
            cout << "请输入新Email: ";
            cin >> email;
            manager.modifyPerson(name, phone, email);
            break;
        }
        case 3: {
            string name;
            cout << "请输入要删除的联系人姓名: ";
            cin >> name;
            manager.deletePerson(name);
            break;
        }
        case 4: {
            string name;
            cout << "请输入要查询的联系人姓名: ";
            cin >> name;
            Person *p = manager.searchByName(name);
            if (p)
                p->display();
            else
                cout << "==> 未找到该联系人。\n";
            break;
        }
        case 5:
            manager.showAll();
            break;
        case 6: {
            int type;
            cout << "请选择类别 (1-同学, 2-同事, 3-朋友, 4-亲戚): ";
            cin >> type;
            manager.showByType(type);
            break;
        }
        case 7:
            manager.sortByName();
            manager.showAll();
            break;
        case 8:
            manager.sortByBirthDate();
            manager.showAll();
            break;
        case 9: {
            int month;
            cout << "请输入月份 (1-12): ";
            cin >> month;
            manager.countByBirthMonth(month);
            break;
        }
        case 10:
            manager.checkBirthdaysAndSendEmails(myName);
            break;
        case 11:
            manager.saveToFile();
            cout << "==> 感谢使用，再见！\n";
            return 0; // 退出程序
        default:
            cout << "==> 无效指令，请重新输入！\n";
            break;
        }
    }
    return 0;
}