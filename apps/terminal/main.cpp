// ===========================================================
// 个人通信录管理系统 — 终端版
//
// 本文件只负责菜单交互与用户输入。
// 全部业务逻辑封装在 core.hpp 中，两版程序共用。
// 菜单严格对应《高级语言程序设计II》任务书第8条要求。
// ===========================================================

#include "hello_contact/core.hpp"
#include <iostream>
#include <limits>
#include <string>

// ==== 辅助函数 ==============================================

// 安全读取整数，带范围检查
int inputInt(const std::string &prompt, int minVal, int maxVal) {
    int val;
    while (true) {
        std::cout << prompt;
        std::cin >> val;
        if (std::cin.fail() || val < minVal || val > maxVal) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "输入无效，请输入 "
                      << minVal << "~" << maxVal << " 之间的数字。\n";
        } else {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return val;
        }
    }
}

// 读取一行字符串
std::string inputStr(const std::string &prompt) {
    std::cout << prompt;
    std::string s;
    std::getline(std::cin, s);
    return s;
}

// 逐项输入日期
Date inputDate() {
    int y = inputInt("  年份: ", 1900, 2100);
    int m = inputInt("  月份: ", 1, 12);
    int d = inputInt("  日:   ", 1, 31);
    return {y, m, d};
}

// 逐项输入一个联系人（根据类别创建对应子类）
// 函数返回 unique_ptr<Person>，利用多态实现统一处理。
std::unique_ptr<Person> inputPerson() {
    std::cout << "\n--- 录入联系人 ---\n";
    int t = inputInt("类别 (1=同学 2=同事 3=朋友 4=亲戚): ", 1, 4);
    std::string name = inputStr("姓名: ");
    std::cout << "生日:\n";
    Date d = inputDate();
    std::string phone = inputStr("电话: ");
    std::string email = inputStr("Email: ");

    const char *prompts[] = {"学校: ", "单位: ", "地点: ", "称呼: "};
    std::string extra = inputStr(prompts[t - 1]);

    switch (t) {
    case 1:
        return std::make_unique<Classmate>(name, d, phone, email, extra);
    case 2:
        return std::make_unique<Colleague>(name, d, phone, email, extra);
    case 3:
        return std::make_unique<Friend>(name, d, phone, email, extra);
    case 4:
        return std::make_unique<Relative>(name, d, phone, email, extra);
    }
    return nullptr;
}

// 显示主菜单 — 每一条直接对应任务书一个要求
void showMenu() {
    std::cout << "\n"
              << " ╔══════════════════════════════════╗\n"
              << " ║   个人通信录管理系统 (终端版)    ║\n"
              << " ╠══════════════════════════════════╣\n"
              << " ║  [1] 添加联系人                  ║\n"
              << " ║  [2] 删除联系人                  ║\n"
              << " ║  [3] 修改联系人                  ║\n"
              << " ║  [4] 按姓名查询                  ║\n"
              << " ║  [5] 查找5天内生日 → 生成贺信    ║\n"
              << " ║  [6] 按姓名排序并显示            ║\n"
              << " ║  [7] 按生日排序并显示            ║\n"
              << " ║  [8] 统计某月出生的人数          ║\n"
              << " ║  [9] 列出全体人员简况            ║\n"
              << " ║ [10] 按类别列出详细信息          ║\n"
              << " ║  [0] 退出 (自动保存)             ║\n"
              << " ╚══════════════════════════════════╝\n"
              << " 请选择: ";
}

// 暂停等待回车
void pauseAndContinue() {
    std::cout << "\n按 Enter 返回菜单...";
    std::string dummy;
    std::getline(std::cin, dummy);
}

// ==== 主函数 ================================================
int main() {
#ifdef _WIN32
    system("chcp 65001 >nul 2>nul"); // 设置控制台为 UTF-8
#endif

    AddressBookManager manager;
    manager.loadFromFile(); // 启动时自动加载已有数据

    while (true) {
        showMenu();
        int ch = inputInt("", 0, 10);

        switch (ch) {
        // ─── (1) 录入、修改、删除 ───
        case 1: {
            manager.addPerson(inputPerson());
            std::cout << "==> 联系人已添加。\n";
            break;
        }
        case 2: {
            std::string name = inputStr("输入要删除的姓名: ");
            manager.deletePerson(name);
            break;
        }
        case 3: {
            std::string name = inputStr("输入要修改的联系人姓名: ");
            Person *p = manager.searchByName(name);
            if (p == nullptr) {
                std::cout << "==> 未找到该联系人。\n";
                break;
            }
            std::cout << "（姓名和出生日期不可修改）\n";
            std::string newPhone = inputStr("新电话: ");
            std::string newEmail = inputStr("新Email: ");
            manager.modifyPerson(name, newPhone, newEmail);
            break;
        }

        // ─── (2) 按姓名查询 ───
        case 4: {
            std::string name = inputStr("输入要查询的姓名: ");
            Person *p = manager.searchByName(name);
            if (p != nullptr)
                p->display();
            else
                std::cout << "==> 未找到联系人: " << name << "\n";
            break;
        }

        // ─── (3) 查找5天内生日 + 生成贺信 ───
        case 5: {
            std::string sender = inputStr("你的名字（贺信署名，直接回车默认 Admin）: ");
            if (sender.empty())
                sender = "Admin";
            manager.checkBirthdaysAndSendEmails(sender);
            break;
        }

        // ─── (4) 按姓名排序 ───
        case 6:
            manager.sortByName();
            manager.showAll();
            break;

        // ─── (4) 按生日排序 ───
        case 7:
            manager.sortByBirthDate();
            manager.showAll();
            break;

        // ─── (5) 统计某月出生人数 ───
        case 8: {
            int m = inputInt("请输入月份 (1-12): ", 1, 12);
            manager.countByBirthMonth(m);
            break;
        }

        // ─── (6) 列出全体人员简况 ───
        case 9:
            std::cout << "\n--- 全体人员（姓名 | 生日 | 电话 | Email）---\n";
            manager.showAllBrief();
            break;

        // ─── (7) 按类别列出详细信息 ───
        case 10: {
            int t = inputInt("类别 (1=同学 2=同事 3=朋友 4=亲戚): ", 1, 4);
            const char *names[] = {"", "同学", "同事", "朋友", "亲戚"};
            std::cout << "\n--- " << names[t] << " ---\n";
            manager.showByType(t);
            break;
        }

        // ─── 退出（自动存盘）───
        case 0:
            manager.saveToFile();
            std::cout << "\n数据已保存，再见！\n";
            return 0;
        }

        pauseAndContinue();
    }

    return 0;
}
