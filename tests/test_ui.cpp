#include <cstdio>
#include <cstring>

#include "ui/budget_panel.h"
#include "ui/population_panel.h"
#include "ui/rci_panel.h"
#include "sim/city.h"

static int failures = 0;
#define CHECK(cond)                                                         \
    do {                                                                    \
        if (!(cond)) {                                                      \
            printf("FAIL: %s (line %d)\n", #cond, __LINE__);                \
            ++failures;                                                     \
        }                                                                   \
    } while (0)

static void test_budget_format_funds() {
    printf("-- test_budget_format_funds\n");
    char buf[64];

    ui::BudgetPanel::format_funds(0, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "$0") == 0);

    ui::BudgetPanel::format_funds(500, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "$500") == 0);

    ui::BudgetPanel::format_funds(1000, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "$1,000") == 0);

    ui::BudgetPanel::format_funds(20000, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "$20,000") == 0);

    ui::BudgetPanel::format_funds(1234567, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "$1,234,567") == 0);

    ui::BudgetPanel::format_funds(-500, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "-$500") == 0);

    ui::BudgetPanel::format_funds(-20000, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "-$20,000") == 0);
}

static void test_budget_format_income() {
    printf("-- test_budget_format_income\n");
    char buf[64];

    ui::BudgetPanel::format_income(0, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "+$0") == 0);

    ui::BudgetPanel::format_income(50, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "+$50") == 0);

    ui::BudgetPanel::format_income(1250, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "+$1,250") == 0);

    ui::BudgetPanel::format_income(-500, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "-$500") == 0);

    ui::BudgetPanel::format_income(-1250, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "-$1,250") == 0);
}

static void test_budget_format_tax_rate() {
    printf("-- test_budget_format_tax_rate\n");
    char buf[64];

    ui::BudgetPanel::format_tax_rate(0, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "0%") == 0);

    ui::BudgetPanel::format_tax_rate(5, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "5%") == 0);

    ui::BudgetPanel::format_tax_rate(20, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "20%") == 0);
}

static void test_population_format() {
    printf("-- test_population_format\n");
    char buf[64];

    ui::PopulationPanel::format_population(0, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "0") == 0);

    ui::PopulationPanel::format_population(5, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "5") == 0);

    ui::PopulationPanel::format_population(999, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "999") == 0);

    ui::PopulationPanel::format_population(1000, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "1,000") == 0);

    ui::PopulationPanel::format_population(12345, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "12,345") == 0);

    ui::PopulationPanel::format_population(100000, buf, sizeof(buf));
    CHECK(std::strcmp(buf, "100,000") == 0);
}

static void test_rci_demand_width() {
    printf("-- test_rci_demand_width\n");
    // max_width = 200
    CHECK(ui::RciPanel::demand_width(sim::Demand::Low, 200) == 33);    // 200/6 = 33
    CHECK(ui::RciPanel::demand_width(sim::Demand::Medium, 200) == 100); // 200/2 = 100
    CHECK(ui::RciPanel::demand_width(sim::Demand::High, 200) == 200);   // 200

    // max_width = 180
    CHECK(ui::RciPanel::demand_width(sim::Demand::Low, 180) == 30);    // 180/6 = 30
    CHECK(ui::RciPanel::demand_width(sim::Demand::Medium, 180) == 90);  // 180/2 = 90
    CHECK(ui::RciPanel::demand_width(sim::Demand::High, 180) == 180);   // 180
}

static void test_panels_integration() {
    printf("-- test_panels_integration\n");
    // Verify we can instantiate all panels and call format methods
    // without SDL (headless test)
    sim::City city;
    city.place_power_plant(10, 10, sim::PowerPlantType::Coal);
    for (int i = 1; i <= 6; ++i) city.place_power_line(12 + i, 11);
    city.set_zone(19, 11, sim::Zone::Industrial);
    city.set_terrain(15, 10, sim::Terrain::Road);
    city.set_zone(16, 10, sim::Zone::Residential);

    for (int m = 0; m < 12; ++m) city.step_month();

    sim::Stats s = city.stats();

    // Just verify the format functions don't crash and produce reasonable output
    char buf[64];
    ui::BudgetPanel::format_funds(s.funds, buf, sizeof(buf));
    CHECK(buf[0] != '\0');

    ui::BudgetPanel::format_income(s.income, buf, sizeof(buf));
    CHECK(buf[0] != '\0');

    ui::BudgetPanel::format_tax_rate(s.tax_rate, buf, sizeof(buf));
    CHECK(buf[0] != '\0');

    ui::PopulationPanel::format_population(s.population, buf, sizeof(buf));
    CHECK(buf[0] != '\0');

    CHECK(ui::RciPanel::demand_width(s.res_demand, 200) >= 0);
    CHECK(ui::RciPanel::demand_width(s.com_demand, 200) >= 0);
    CHECK(ui::RciPanel::demand_width(s.ind_demand, 200) >= 0);
}

int main() {
    printf("UI test suite\n");
    test_budget_format_funds();
    test_budget_format_income();
    test_budget_format_tax_rate();
    test_population_format();
    test_rci_demand_width();
    test_panels_integration();
    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d test(s) FAILED\n", failures);
    return 1;
}