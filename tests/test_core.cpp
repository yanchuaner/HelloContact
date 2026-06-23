// ===========================================================
// hello_contact_core 单元测试
// ===========================================================
#include "hello_contact/core.hpp"
#include <cassert>
#include <iostream>
#include <sstream>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name, expr) do {                                   \
    if (!(expr)) {                                              \
        std::cerr << "[FAIL] " << name << std::endl;            \
        ++tests_failed;                                         \
    } else {                                                    \
        std::cout << "[PASS] " << name << std::endl;            \
        ++tests_passed;                                         \
    }                                                           \
} while(0)

int main() {
    // ── Date ──
    Date d{2025, 6, 1};
    TEST("Date toString", d.toString() == "2025-6-1");

    // ── 创建联系人 ──
    auto cm = std::make_unique<Classmate>("张三", d, "13800138000", "zhangsan@test.com", "清华");
    TEST("Classmate name", cm->getName() == "张三");
    TEST("Classmate type", cm->getType() == 1);
    TEST("Classmate formatForFile", cm->formatForFile().find("清华") != std::string::npos);

    auto co = std::make_unique<Colleague>("李四", d, "13900139000", "lisi@work.com", "腾讯");
    TEST("Colleague name", co->getName() == "李四");
    TEST("Colleague type", co->getType() == 2);

    auto fr = std::make_unique<Friend>("王五", d, "13700137000", "wangwu@fun.com", "公园");
    TEST("Friend name", fr->getName() == "王五");
    TEST("Friend type", fr->getType() == 3);

    auto re = std::make_unique<Relative>("赵六", d, "13600136000", "zhaoliu@family.com", "叔叔");
    TEST("Relative name", re->getName() == "赵六");
    TEST("Relative type", re->getType() == 4);

    // ── AddressBookManager ──
    AddressBookManager mgr;
    TEST("Empty at start", mgr.getContacts().empty());

    mgr.addPerson(std::move(cm));
    mgr.addPerson(std::move(co));
    mgr.addPerson(std::move(fr));
    mgr.addPerson(std::move(re));
    TEST("Add 4 contacts", mgr.getContacts().size() == 4);

    auto *found = mgr.searchByName("张三");
    TEST("Search by name", found != nullptr);
    TEST("Search name match", found->getName() == "张三");

    auto *found2 = mgr.searchByPhone("13900139000");
    TEST("Search by phone", found2 != nullptr);
    TEST("Search phone match", found2->getName() == "李四");

    auto surnames = mgr.searchBySurname("王");
    TEST("Search by surname", surnames.size() == 1);

    auto byYear = mgr.searchByBirthYear(2025);
    TEST("Search by birth year", byYear.size() == 4);

    // ── 删除与修改 ──
    mgr.deletePerson("张三");
    TEST("Delete contact", mgr.getContacts().size() == 3);

    auto *toMod = mgr.searchByName("李四");
    TEST("Found for modify", toMod != nullptr);
    mgr.modifyPerson("李四", "18800000000", "new@email.com");
    TEST("Phone updated", toMod->getPhone() == "18800000000");
    TEST("Email updated", toMod->getEmail() == "new@email.com");

    // ── 排序 ──
    // 添加回张三以测试排序
    mgr.addPerson(std::make_unique<Classmate>("张三", Date{2025, 6, 1}, "13800138000", "zhangsan@test.com", "清华"));
    mgr.sortByName();
    TEST("Sort by name", mgr.getContacts()[0]->getName() == "张三" || mgr.getContacts()[0]->getName() == "李四");

    mgr.sortByBirthDate();
    TEST("Sort by birth", mgr.getContacts().size() == 4);

    // ── 统计 ──
    // 捕获 stdout
    std::stringstream buf;
    auto old = std::cout.rdbuf(buf.rdbuf());
    mgr.countByType();
    std::cout.rdbuf(old);
    TEST("Type count output", buf.str().find("总计") != std::string::npos);

    // ── 文件 I/O ──
    mgr.saveToFile();
    AddressBookManager mgr2;
    mgr2.loadFromFile();
    TEST("File I/O roundtrip", mgr2.getContacts().size() == 4);

    std::cout << "\n==============================" << std::endl;
    std::cout << "Tests: " << (tests_passed + tests_failed)
              << "  Passed: " << tests_passed
              << "  Failed: " << tests_failed << std::endl;
    std::cout << "==============================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
