#ifndef MANAGER_CORE_HPP
#define MANAGER_CORE_HPP

#include <algorithm>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <QFile>
#include <QTextStream>

// ==========================================
// 1. 日期结构体
// ==========================================
struct Date {
    int year;
    int month;
    int day;

    [[nodiscard]] std::string toString() const {
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
// 2. 类继承体系
// ==========================================
class Person {
  protected:
    std::string name;
    Date birthDate;
    std::string phone;
    std::string email;
    int type; // 1-同学, 2-同事, 3-朋友, 4-亲戚

  public:
    Person(std::string n, Date d, std::string p, std::string e, int t)
        : name(std::move(n)), birthDate(d),
          phone(std::move(p)), email(std::move(e)), type(t) {}

    virtual ~Person() = default;

    virtual void display() const = 0;
    virtual std::string formatForFile() const = 0;

    [[nodiscard]] const std::string &getName() const noexcept { return name; }
    [[nodiscard]] Date getBirthDate() const noexcept { return birthDate; }
    [[nodiscard]] int getType() const noexcept { return type; }
    [[nodiscard]] const std::string &getPhone() const noexcept { return phone; }
    [[nodiscard]] const std::string &getEmail() const noexcept { return email; }

    void setPhone(const std::string &newPhone) { phone = newPhone; }
    void setEmail(const std::string &newEmail) { email = newEmail; }
};

/** 类型 1：同学 */
class Classmate : public Person {
    std::string school;

  public:
    Classmate(std::string n, Date d, std::string p, std::string e, std::string s)
        : Person(std::move(n), d, std::move(p), std::move(e), 1),
          school(std::move(s)) {}

    void display() const override {
        std::cout << "[同学] 姓名: " << name << ", 生日: " << birthDate.toString()
                  << ", 电话: " << phone << ", Email: " << email << ", 学校: " << school << std::endl;
    }

    std::string formatForFile() const override {
        return name + "," + std::to_string(birthDate.year) + "," + std::to_string(birthDate.month) + "," + std::to_string(birthDate.day) + "," + phone + "," + email + "," + school;
    }
};

/** 类型 2：同事 */
class Colleague : public Person {
    std::string company;

  public:
    Colleague(std::string n, Date d, std::string p, std::string e, std::string c)
        : Person(std::move(n), d, std::move(p), std::move(e), 2),
          company(std::move(c)) {}

    void display() const override {
        std::cout << "[同事] 姓名: " << name << ", 生日: " << birthDate.toString()
                  << ", 电话: " << phone << ", Email: " << email << ", 单位: " << company << std::endl;
    }

    std::string formatForFile() const override {
        return name + "," + std::to_string(birthDate.year) + "," + std::to_string(birthDate.month) + "," + std::to_string(birthDate.day) + "," + phone + "," + email + "," + company;
    }
};

/** 类型 3：朋友 */
class Friend : public Person {
    std::string place;

  public:
    Friend(std::string n, Date d, std::string p, std::string e, std::string pl)
        : Person(std::move(n), d, std::move(p), std::move(e), 3),
          place(std::move(pl)) {}

    void display() const override {
        std::cout << "[朋友] 姓名: " << name << ", 生日: " << birthDate.toString()
                  << ", 电话: " << phone << ", Email: " << email << ", 地点: " << place << std::endl;
    }

    std::string formatForFile() const override {
        return name + "," + std::to_string(birthDate.year) + "," + std::to_string(birthDate.month) + "," + std::to_string(birthDate.day) + "," + phone + "," + email + "," + place;
    }
};

/** 类型 4：亲戚 */
class Relative : public Person {
    std::string relation;

  public:
    Relative(std::string n, Date d, std::string p, std::string e, std::string r)
        : Person(std::move(n), d, std::move(p), std::move(e), 4),
          relation(std::move(r)) {}

    void display() const override {
        std::cout << "[亲戚] 姓名: " << name << ", 生日: " << birthDate.toString()
                  << ", 电话: " << phone << ", Email: " << email << ", 称呼: " << relation << std::endl;
    }

    std::string formatForFile() const override {
        return name + "," + std::to_string(birthDate.year) + "," + std::to_string(birthDate.month) + "," + std::to_string(birthDate.day) + "," + phone + "," + email + "," + relation;
    }
};

// ==========================================
// 3. 通讯录管理器 (业务逻辑层)
//    vector<unique_ptr<Person>>，RAII 自动管理
//    数据文件：AddressBook1~4.txt (纯文本 CSV，按类型分文件)
// ==========================================
class AddressBookManager {
  private:
    std::vector<std::unique_ptr<Person>> contacts;

  public:
    void addPerson(std::unique_ptr<Person> p) {
        contacts.push_back(std::move(p));
    }

    [[nodiscard]] const std::vector<std::unique_ptr<Person>> &getContacts() const noexcept {
        return contacts;
    }

    // ========== 查询与编辑 ==========
    [[nodiscard]] Person *searchByName(std::string_view targetName) const {
        for (const auto &p : contacts) {
            if (p->getName() == targetName)
                return p.get();
        }
        return nullptr;
    }

    /** 按电话号码查找（精确匹配） */
    [[nodiscard]] Person *searchByPhone(std::string_view phone) const {
        for (const auto &p : contacts) {
            if (p->getPhone() == phone)
                return p.get();
        }
        return nullptr;
    }

    /** 按邮箱查找（精确匹配） */
    [[nodiscard]] Person *searchByEmail(std::string_view email) const {
        for (const auto &p : contacts) {
            if (p->getEmail() == email)
                return p.get();
        }
        return nullptr;
    }

    /** 按姓氏搜索（返回所有匹配的人） */
    [[nodiscard]] std::vector<Person *> searchBySurname(std::string_view surname) const {
        std::vector<Person *> result;
        for (const auto &p : contacts) {
            if (p->getName().size() >= surname.size() &&
                p->getName().compare(0, surname.size(), surname.data(), surname.size()) == 0)
                result.push_back(p.get());
        }
        return result;
    }

    /** 按出生年份搜索 */
    [[nodiscard]] std::vector<Person *> searchByBirthYear(int year) const {
        std::vector<Person *> result;
        for (const auto &p : contacts) {
            if (p->getBirthDate().year == year)
                result.push_back(p.get());
        }
        return result;
    }

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

    void modifyPerson(std::string_view targetName,
                      const std::string &newPhone,
                      const std::string &newEmail) {
        Person *p = searchByName(targetName);
        if (p != nullptr) {
            p->setPhone(newPhone);
            p->setEmail(newEmail);
            std::cout << "==> " << targetName << " 的联系方式已更新。" << std::endl;
        } else {
            std::cout << "==> 未找到联系人，修改失败。" << std::endl;
        }
    }

    // ========== 排序 ==========
    void sortByName() {
        std::sort(contacts.begin(), contacts.end(),
                  [](const auto &a, const auto &b) { return a->getName() < b->getName(); });
        std::cout << "==> 已按姓名（拼音/英文）排序完成。" << std::endl;
    }

    void sortByBirthDate() {
        std::sort(contacts.begin(), contacts.end(),
                  [](const auto &a, const auto &b) {
                      Date d1 = a->getBirthDate();
                      Date d2 = b->getBirthDate();
                      if (d1.year != d2.year)
                          return d1.year < d2.year;
                      if (d1.month != d2.month)
                          return d1.month < d2.month;
                      return d1.day < d2.day;
                  });
        std::cout << "==> 已按出生日期排序完成。" << std::endl;
    }

    // ========== 统计与筛选 ==========
    void countByBirthMonth(int targetMonth) const {
        int count = 0;
        std::cout << "==> " << targetMonth << " 月份出生的联系人如下：" << std::endl;
        for (const auto &p : contacts) {
            if (p->getBirthDate().month == targetMonth) {
                p->display();
                count++;
            }
        }
        std::cout << "==> 共 " << count << " 人。" << std::endl;
    }

    /** 查找生日相同的人（可选按类别过滤） */
    void findSameBirthday(int typeFilter = 0) const {
        std::vector<std::pair<std::string, size_t>> candidates;
        for (size_t i = 0; i < contacts.size(); ++i) {
            if (typeFilter == 0 || contacts[i]->getType() == typeFilter)
                candidates.emplace_back(contacts[i]->getBirthDate().toString(), i);
        }

        // 按日期字符串分组（已排序则相邻相同）
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
        for (const auto &p : contacts) {
            if (p->getType() == targetType) {
                p->display();
                count++;
            }
        }
        if (count == 0)
            std::cout << "==> 该分类下没有联系人。" << std::endl;
    }

    /** 统计各类别联系人数量 */
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

    // ========== 生日检查与祝福邮件 ==========
    std::string generateBirthdayEmail(std::string_view receiverName,
                                      std::string_view myName) const {
        QString fn = QStringLiteral("BirthdayEmail_%1.txt")
                         .arg(QString::fromUtf8(receiverName.data(),
                                                static_cast<int>(receiverName.size())));
        std::string filename = fn.toUtf8().toStdString();

        QFile file(fn);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8);
            out << QString::fromUtf8(receiverName.data(),
                                     static_cast<int>(receiverName.size()))
                << ":\n";
            out << "\t祝生日快乐，健康幸福。\n";
            out << "\t\t\t\t\t\t"
                << QString::fromUtf8(myName.data(),
                                     static_cast<int>(myName.size()))
                << "\n";
            file.close();

            std::cout << "已为 " << receiverName << " 生成祝福邮件: " << filename << std::endl;
            std::cout << "── 邮件内容 ──" << std::endl;

            QFile infile(fn);
            if (infile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&infile);
                in.setEncoding(QStringConverter::Utf8);
                while (!in.atEnd())
                    std::cout << in.readLine().toStdString() << std::endl;
                infile.close();
            }
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
                std::cout << p->getBirthDate().month << "月" << p->getBirthDate().day << "日";
                std::cout << "（" << weekdayStr << "）";
                p->display();
                generateBirthdayEmail(p->getName(), myName);
            }
        }
        if (!foundAny)
            std::cout << "==> 未来 5 天内没有联系人过生日。" << std::endl;
    }

    // ========== 遍历打印 ==========
    void showAll() const {
        for (const auto &p : contacts)
            p->display();
    }

    /** 仅输出姓名、生日、电话、Email（任务书要求6） */
    void showAllBrief() const {
        for (const auto &p : contacts) {
            std::cout << p->getName() << " | "
                      << p->getBirthDate().toString() << " | "
                      << p->getPhone() << " | "
                      << p->getEmail() << std::endl;
        }
    }

    // ==========================================
    // 文件 I/O（纯文本 CSV，按类型分文件）
    //   分别存入 AddressBook1~4.txt
    //   CSV 格式：name,Y,M,D,phone,email,extra
    // ==========================================
    void saveToFile() {
        for (int type = 1; type <= 4; ++type) {
            std::string filename = "AddressBook" + std::to_string(type) + ".txt";
            std::ofstream out(filename);
            if (!out.is_open()) {
                std::cerr << "无法打开文件写入：" << filename << std::endl;
                continue;
            }
            for (const auto &p : contacts) {
                if (p->getType() == type)
                    out << p->formatForFile() << "\n";
            }
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
                    Date d = {std::stoi(tokens[1]), std::stoi(tokens[2]), std::stoi(tokens[3])};
                    switch (type) {
                    case 1:
                        addPerson(std::make_unique<Classmate>(
                            tokens[0], d, tokens[4], tokens[5], tokens[6]));
                        break;
                    case 2:
                        addPerson(std::make_unique<Colleague>(
                            tokens[0], d, tokens[4], tokens[5], tokens[6]));
                        break;
                    case 3:
                        addPerson(std::make_unique<Friend>(
                            tokens[0], d, tokens[4], tokens[5], tokens[6]));
                        break;
                    case 4:
                        addPerson(std::make_unique<Relative>(
                            tokens[0], d, tokens[4], tokens[5], tokens[6]));
                        break;
                    }
                }
            }
            in.close();
        }
        std::cout << "==> 系统提示：数据加载完毕，当前共有联系人 " << contacts.size() << " 个。" << std::endl;
    }
};

#endif // MANAGER_CORE_HPP
