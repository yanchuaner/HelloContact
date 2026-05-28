// ===============================================================
// 个人通信录管理系统 - 终端版（独立单文件，纯标准 C++17）
// 编译：g++ -std=c++17 main_terminal.cpp -o terminal.exe
// 运行：./terminal.exe
// ===============================================================

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

// ==========================================
// Date 结构体
// ==========================================
struct Date {
    int year;
    int month;
    int day;

    std::string toString() const {
        return std::to_string(year) + "-" + std::to_string(month) + "-" + std::to_string(day);
    }

    bool isBirthdayWithin5Days(std::string &outWeekday) const {
        time_t nowTime = time(nullptr);
        tm *currentTm = localtime(&nowTime);

        tm bdayTm = *currentTm;
        bdayTm.tm_mon = month - 1;
        bdayTm.tm_mday = day;
        bdayTm.tm_hour = 0;
        bdayTm.tm_min = 0;
        bdayTm.tm_sec = 0;

        time_t bdayTime = mktime(&bdayTm);
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
// Person 继承体系
// ==========================================
class Person {
  protected:
    std::string name;
    Date birthDate;
    std::string phone;
    std::string email;
    int type; // 1=同学, 2=同事, 3=朋友, 4=亲戚

  public:
    Person(std::string n, Date d, std::string p, std::string e, int t)
        : name(std::move(n)), birthDate(d), phone(std::move(p)), email(std::move(e)), type(t) {}
    virtual ~Person() = default;

    virtual void display() const = 0;
    virtual std::string formatForFile() const = 0;

    const std::string &getName() const { return name; }
    Date getBirthDate() const { return birthDate; }
    int getType() const { return type; }
    const std::string &getPhone() const { return phone; }
    const std::string &getEmail() const { return email; }

    void setPhone(const std::string &p) { phone = p; }
    void setEmail(const std::string &e) { email = e; }
};

// ---- 同学 ----
class Classmate : public Person {
    std::string school;

  public:
    Classmate(std::string n, Date d, std::string p, std::string e, std::string s)
        : Person(std::move(n), d, std::move(p), std::move(e), 1), school(std::move(s)) {}
    void display() const override {
        std::cout << "[同学] 姓名: " << name << ", 生日: " << birthDate.toString()
                  << ", 电话: " << phone << ", Email: " << email << ", 学校: " << school << std::endl;
    }
    std::string formatForFile() const override {
        return name + "," + std::to_string(birthDate.year) + "," + std::to_string(birthDate.month) + "," + std::to_string(birthDate.day) + "," + phone + "," + email + "," + school;
    }
};

// ---- 同事 ----
class Colleague : public Person {
    std::string company;

  public:
    Colleague(std::string n, Date d, std::string p, std::string e, std::string c)
        : Person(std::move(n), d, std::move(p), std::move(e), 2), company(std::move(c)) {}
    void display() const override {
        std::cout << "[同事] 姓名: " << name << ", 生日: " << birthDate.toString()
                  << ", 电话: " << phone << ", Email: " << email << ", 单位: " << company << std::endl;
    }
    std::string formatForFile() const override {
        return name + "," + std::to_string(birthDate.year) + "," + std::to_string(birthDate.month) + "," + std::to_string(birthDate.day) + "," + phone + "," + email + "," + company;
    }
};

// ---- 朋友 ----
class Friend : public Person {
    std::string place;

  public:
    Friend(std::string n, Date d, std::string p, std::string e, std::string pl)
        : Person(std::move(n), d, std::move(p), std::move(e), 3), place(std::move(pl)) {}
    void display() const override {
        std::cout << "[朋友] 姓名: " << name << ", 生日: " << birthDate.toString()
                  << ", 电话: " << phone << ", Email: " << email << ", 地点: " << place << std::endl;
    }
    std::string formatForFile() const override {
        return name + "," + std::to_string(birthDate.year) + "," + std::to_string(birthDate.month) + "," + std::to_string(birthDate.day) + "," + phone + "," + email + "," + place;
    }
};

// ---- 亲戚 ----
class Relative : public Person {
    std::string relation;

  public:
    Relative(std::string n, Date d, std::string p, std::string e, std::string r)
        : Person(std::move(n), d, std::move(p), std::move(e), 4), relation(std::move(r)) {}
    void display() const override {
        std::cout << "[亲戚] 姓名: " << name << ", 生日: " << birthDate.toString()
                  << ", 电话: " << phone << ", Email: " << email << ", 称呼: " << relation << std::endl;
    }
    std::string formatForFile() const override {
        return name + "," + std::to_string(birthDate.year) + "," + std::to_string(birthDate.month) + "," + std::to_string(birthDate.day) + "," + phone + "," + email + "," + relation;
    }
};

// ==========================================
// PersonFactory (工厂模式)
// ==========================================
class PersonFactory {
  public:
    static std::unique_ptr<Person> create(int type, const std::string &n, Date d, const std::string &p, const std::string &e, const std::string &extra) {
        switch (type) {
        case 1:
            return std::make_unique<Classmate>(n, d, p, e, extra);
        case 2:
            return std::make_unique<Colleague>(n, d, p, e, extra);
        case 3:
            return std::make_unique<Friend>(n, d, p, e, extra);
        case 4:
            return std::make_unique<Relative>(n, d, p, e, extra);
        default:
            return nullptr;
        }
    }
};

// ==========================================
// AddressBookManager
// ==========================================
class AddressBookManager {
  private:
    std::vector<std::unique_ptr<Person>> contacts;

  public:
    void addPerson(std::unique_ptr<Person> p) {
        contacts.push_back(std::move(p));
    }

    const std::vector<std::unique_ptr<Person>> &getContacts() const {
        return contacts;
    }

    // ---- 查询 ----
    Person *searchByName(std::string_view targetName) const {
        for (const auto &p : contacts)
            if (p->getName() == targetName)
                return p.get();
        return nullptr;
    }

    Person *searchByPhone(std::string_view phone) const {
        for (const auto &p : contacts)
            if (p->getPhone() == phone)
                return p.get();
        return nullptr;
    }

    Person *searchByEmail(std::string_view email) const {
        for (const auto &p : contacts)
            if (p->getEmail() == email)
                return p.get();
        return nullptr;
    }

    std::vector<Person *> searchBySurname(std::string_view surname) const {
        std::vector<Person *> result;
        for (const auto &p : contacts)
            if (p->getName().size() >= surname.size() &&
                p->getName().compare(0, surname.size(), surname.data(), surname.size()) == 0)
                result.push_back(p.get());
        return result;
    }

    std::vector<Person *> searchByBirthYear(int year) const {
        std::vector<Person *> result;
        for (const auto &p : contacts)
            if (p->getBirthDate().year == year)
                result.push_back(p.get());
        return result;
    }

    // ---- 编辑 ----
    bool deletePerson(std::string_view targetName) {
        auto it = std::find_if(contacts.begin(), contacts.end(),
                               [&](const auto &p) { return p->getName() == targetName; });
        if (it != contacts.end()) {
            contacts.erase(it);
            std::cout << "==> 成功删除联系人：" << targetName << std::endl;
            return true;
        }
        std::cout << "==> 未找到名为 " << targetName << " 的联系人，删除失败。" << std::endl;
        return false;
    }

    void modifyPerson(std::string_view targetName, const std::string &newPhone, const std::string &newEmail) {
        Person *p = searchByName(targetName);
        if (p) {
            p->setPhone(newPhone);
            p->setEmail(newEmail);
            std::cout << "==> " << targetName << " 的联系方式已更新。" << std::endl;
        } else {
            std::cout << "==> 未找到联系人，修改失败。" << std::endl;
        }
    }

    // ---- 排序 ----
    void sortByName() {
        std::sort(contacts.begin(), contacts.end(),
                  [](const auto &a, const auto &b) { return a->getName() < b->getName(); });
        std::cout << "==> 已按姓名排序完成。" << std::endl;
    }

    void sortByBirthDate() {
        std::sort(contacts.begin(), contacts.end(),
                  [](const auto &a, const auto &b) {
                      Date d1 = a->getBirthDate(), d2 = b->getBirthDate();
                      if (d1.year != d2.year)
                          return d1.year < d2.year;
                      if (d1.month != d2.month)
                          return d1.month < d2.month;
                      return d1.day < d2.day;
                  });
        std::cout << "==> 已按出生日期排序完成。" << std::endl;
    }

    // ---- 统计与筛选 ----
    void countByBirthMonth(int targetMonth) const {
        int count = 0;
        std::cout << "==> " << targetMonth << " 月份出生的联系人如下：" << std::endl;
        for (const auto &p : contacts)
            if (p->getBirthDate().month == targetMonth) {
                p->display();
                count++;
            }
        std::cout << "==> 共 " << count << " 人。" << std::endl;
    }

    void findSameBirthday(int typeFilter = 0) const {
        std::vector<std::pair<std::string, size_t>> candidates;
        for (size_t i = 0; i < contacts.size(); ++i)
            if (typeFilter == 0 || contacts[i]->getType() == typeFilter)
                candidates.emplace_back(contacts[i]->getBirthDate().toString(), i);

        std::sort(candidates.begin(), candidates.end());
        bool foundAny = false;
        size_t j = 0;
        while (j < candidates.size()) {
            size_t k = j + 1;
            while (k < candidates.size() && candidates[k].first == candidates[j].first)
                ++k;
            if (k - j > 1) {
                if (!foundAny) {
                    std::cout << "==> 生日相同的联系人有：" << std::endl;
                    foundAny = true;
                }
                std::cout << "--- " << candidates[j].first << " ---" << std::endl;
                for (size_t m = j; m < k; ++m)
                    contacts[candidates[m].second]->display();
            }
            j = k;
        }
        if (!foundAny)
            std::cout << "==> 没有找到生日相同的联系人。" << std::endl;
    }

    void showByType(int targetType) const {
        int count = 0;
        for (const auto &p : contacts)
            if (p->getType() == targetType) {
                p->display();
                count++;
            }
        if (count == 0)
            std::cout << "==> 该分类下没有联系人。" << std::endl;
    }

    void countByType() const {
        const char *typeNames[] = {"同学", "同事", "朋友", "亲戚"};
        int counts[4] = {0};
        for (const auto &p : contacts)
            if (p->getType() >= 1 && p->getType() <= 4)
                counts[p->getType() - 1]++;
        std::cout << "==> 联系人数量统计：" << std::endl;
        for (int i = 0; i < 4; ++i)
            std::cout << "  " << typeNames[i] << "： " << counts[i] << " 人" << std::endl;
        std::cout << "==> 总计：" << (counts[0] + counts[1] + counts[2] + counts[3]) << " 人" << std::endl;
    }

    // ---- 生日贺信 ----
    std::string generateBirthdayEmail(std::string_view receiverName, std::string_view myName) const {
        std::string filename = "BirthdayEmail_";
        filename += receiverName;
        filename += ".txt";

        std::ofstream out(filename);
        if (out.is_open()) {
            out << receiverName << ":\n";
            out << "\t祝生日快乐，健康幸福。\n";
            out << "\t\t\t\t\t\t" << myName << "\n";
            out.close();

            std::cout << "已为 " << receiverName << " 生成祝福邮件: " << filename << std::endl;
            std::cout << "── 邮件内容 ──" << std::endl;
            std::ifstream infile(filename);
            std::string line;
            while (std::getline(infile, line))
                std::cout << line << std::endl;
            std::cout << "──────────────" << std::endl;
        } else {
            std::cout << "!! 生成祝福邮件失败：无法写入文件 " << filename << std::endl;
        }
        return filename;
    }

    void checkBirthdaysAndSendEmails(std::string_view myName) const {
        bool foundAny = false;
        std::cout << "==> 正在扫描未来 5 天内过生日的联系人..." << std::endl;
        for (const auto &p : contacts) {
            std::string weekdayStr;
            if (p->getBirthDate().isBirthdayWithin5Days(weekdayStr)) {
                foundAny = true;
                std::cout << p->getBirthDate().month << "月" << p->getBirthDate().day << "日"
                          << "（" << weekdayStr << "）";
                p->display();
                generateBirthdayEmail(p->getName(), myName);
            }
        }
        if (!foundAny)
            std::cout << "==> 未来 5 天内没有联系人过生日。" << std::endl;
    }

    // ---- 遍历 ----
    void showAll() const {
        for (const auto &p : contacts)
            p->display();
    }

    void showAllBrief() const {
        for (const auto &p : contacts)
            std::cout << p->getName() << " | " << p->getBirthDate().toString()
                      << " | " << p->getPhone() << " | " << p->getEmail() << std::endl;
    }

    // ---- 文件 I/O ----
    void saveToFile() {
        for (int type = 1; type <= 4; ++type) {
            std::string filename = "AddressBook" + std::to_string(type) + ".txt";
            std::ofstream out(filename);
            if (!out.is_open()) {
                std::cerr << "无法打开文件写入：" << filename << std::endl;
                continue;
            }
            for (const auto &p : contacts)
                if (p->getType() == type)
                    out << p->formatForFile() << "\n";
            out.close();
        }
        std::cout << "==> 数据已保存到文件。" << std::endl;
    }

    void loadFromFile() {
        contacts.clear();
        for (int type = 1; type <= 4; ++type) {
            std::string filename = "AddressBook" + std::to_string(type) + ".txt";
            std::ifstream in(filename);
            if (!in.is_open())
                continue;

            std::string line;
            while (std::getline(in, line)) {
                if (line.empty())
                    continue;
                if (line.back() == '\r')
                    line.pop_back();

                std::vector<std::string> tokens;
                std::stringstream ss(line);
                std::string token;
                while (std::getline(ss, token, ','))
                    tokens.push_back(token);

                if (tokens.size() >= 7) {
                    try {
                        Date d = {std::stoi(tokens[1]), std::stoi(tokens[2]), std::stoi(tokens[3])};
                        if (auto p = PersonFactory::create(type, tokens[0], d, tokens[4], tokens[5], tokens[6])) {
                            addPerson(std::move(p));
                        }
                    } catch (const std::exception &e) {
                        std::cerr << "!! 警告：联系人数据解析失败 (可能是数值损坏)，已跳过该行: " << tokens[0] << std::endl;
                    }
                }
            }
            in.close();
        }
        std::cout << "==> 系统提示：数据加载完毕，当前共有联系人 " << contacts.size() << " 个。" << std::endl;
    }
};

// ===============================================================
// 终端界面辅助函数
// ===============================================================
static void pauseAndContinue() {
    std::cout << "\n按 Enter 键继续...";
    std::string dummy;
    std::getline(std::cin, dummy);
}

static int inputInt(const std::string &prompt, int minVal, int maxVal) {
    int val;
    while (true) {
        std::cout << prompt;
        std::cin >> val;
        if (std::cin.fail() || val < minVal || val > maxVal) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "输入无效，请重新输入 (" << minVal << "-" << maxVal << ")。\n";
        } else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return val;
        }
    }
}

static std::string inputStr(const std::string &prompt) {
    std::cout << prompt;
    std::string s;
    std::getline(std::cin, s);
    return s;
}

static Date inputDate() {
    int y = inputInt("  年份: ", 1900, 2100);
    int m = inputInt("  月份: ", 1, 12);
    int d = inputInt("  日: ", 1, 31);
    return {y, m, d};
}

static const char *typeName(int t) {
    switch (t) {
    case 1:
        return "同学";
    case 2:
        return "同事";
    case 3:
        return "朋友";
    case 4:
        return "亲戚";
    default:
        return "未知";
    }
}

static void showMenu() {
    std::cout << "\n"
              << "╔══════════════════════════════╗\n"
              << "║    个人通信录管理系统         ║\n"
              << "╠══════════════════════════════╣\n"
              << "║ 1. 添加联系人                ║\n"
              << "║ 2. 删除联系人                ║\n"
              << "║ 3. 修改联系人                ║\n"
              << "║ 4. 按姓名搜索                ║\n"
              << "║ 5. 按电话搜索                ║\n"
              << "║ 6. 按姓名排序                ║\n"
              << "║ 7. 按生日排序                ║\n"
              << "║ 8. 统计某月生日              ║\n"
              << "║ 9. 查找生日相同的人          ║\n"
              << "║ 10. 列出所有联系人           ║\n"
              << "║ 11. 按类别列出               ║\n"
              << "║ 12. 扫描5天内生日并生成贺信  ║\n"
              << "║ 13. 保存数据到文件           ║\n"
              << "║ 0. 退出                      ║\n"
              << "╚══════════════════════════════╝\n"
              << "请选择: ";
}

static std::unique_ptr<Person> inputPerson() {
    std::cout << "\n--- 添加联系人 ---\n";
    int typeIdx = inputInt("类别 (1=同学, 2=同事, 3=朋友, 4=亲戚): ", 1, 4);
    std::string name = inputStr("姓名: ");
    std::cout << "生日:\n";
    Date d = inputDate();
    std::string phone = inputStr("电话: ");
    std::string email = inputStr("Email: ");

    const char *extraPrompts[] = {"学校: ", "单位: ", "地点: ", "称呼: "};
    std::string extra = inputStr(extraPrompts[typeIdx - 1]);

    return PersonFactory::create(typeIdx, name, d, phone, email, extra);
}

// ===============================================================
// main
// ===============================================================
int main() {
#ifdef _WIN32
    system("chcp 65001 >nul 2>nul");
#endif
    AddressBookManager manager;
    manager.loadFromFile();

    while (true) {
        showMenu();
        int choice = inputInt("", 0, 13);

        switch (choice) {
        case 1:
            manager.addPerson(inputPerson());
            std::cout << "联系人已添加。\n";
            break;
        case 2: {
            std::string name = inputStr("输入要删除的姓名: ");
            manager.deletePerson(name);
            break;
        }
        case 3: {
            std::string name = inputStr("输入要修改的联系人姓名: ");
            std::string newPhone = inputStr("新电话: ");
            std::string newEmail = inputStr("新Email: ");
            manager.modifyPerson(name, newPhone, newEmail);
            break;
        }
        case 4: {
            std::string name = inputStr("输入要搜索的姓名: ");
            Person *p = manager.searchByName(name);
            if (p)
                p->display();
            else
                std::cout << "未找到联系人: " << name << "\n";
            break;
        }
        case 5: {
            std::string phone = inputStr("输入要搜索的电话: ");
            Person *p = manager.searchByPhone(phone);
            if (p)
                p->display();
            else
                std::cout << "未找到电话为 " << phone << " 的联系人。\n";
            break;
        }
        case 6:
            manager.sortByName();
            manager.showAll();
            break;
        case 7:
            manager.sortByBirthDate();
            manager.showAll();
            break;
        case 8: {
            int m = inputInt("输入月份 (1-12): ", 1, 12);
            manager.countByBirthMonth(m);
            break;
        }
        case 9: {
            int t = inputInt("筛选类别 (0=不筛选, 1=同学, 2=同事, 3=朋友, 4=亲戚): ", 0, 4);
            manager.findSameBirthday(t);
            break;
        }
        case 10:
            std::cout << "--- 全部联系人 ---\n";
            manager.showAllBrief();
            break;
        case 11: {
            int t = inputInt("类别 (1=同学, 2=同事, 3=朋友, 4=亲戚): ", 1, 4);
            std::cout << "--- " << typeName(t) << " ---\n";
            manager.showByType(t);
            break;
        }
        case 12: {
            std::string sender = inputStr("请输入你的名字（用于贺信署名）: ");
            if (sender.empty())
                sender = "Admin";
            manager.checkBirthdaysAndSendEmails(sender);
            break;
        }
        case 13:
            manager.saveToFile();
            break;
        case 0:
            manager.saveToFile();
            std::cout << "数据已保存，再见。\n";
            return 0;
        }

        pauseAndContinue();
    }

    return 0;
}
