#include "Core.hpp"
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>

// ===============================================================
// 工具函数
// ===============================================================
static void pauseAndContinue() {
    std::cout << "\n按 Enter 键继续...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
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

// ===============================================================
// 主菜单
// ===============================================================
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

// ===============================================================
// 联系人录入（各类别通用）
// ===============================================================
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

    switch (typeIdx) {
    case 1:
        return std::make_unique<Classmate>(name, d, phone, email, extra);
    case 2:
        return std::make_unique<Colleague>(name, d, phone, email, extra);
    case 3:
        return std::make_unique<Friend>(name, d, phone, email, extra);
    case 4:
        return std::make_unique<Relative>(name, d, phone, email, extra);
    default:
        return nullptr;
    }
}

// ===============================================================
// main
// ===============================================================
int main() {
#ifdef _WIN32
    system("chcp 65001 >nul 2>nul");
#endif
    AddressBookManager manager;

    // 自动加载已有数据
    manager.loadFromFile();

    while (true) {
        showMenu();
        int choice = inputInt("", 0, 13);

        switch (choice) {
        case 1: {
            manager.addPerson(inputPerson());
            std::cout << "联系人已添加。\n";
            break;
        }
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
            if (p) {
                p->display();
            } else {
                std::cout << "未找到联系人: " << name << "\n";
            }
            break;
        }
        case 5: {
            std::string phone = inputStr("输入要搜索的电话: ");
            Person *p = manager.searchByPhone(phone);
            if (p) {
                p->display();
            } else {
                std::cout << "未找到电话为 " << phone << " 的联系人。\n";
            }
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
