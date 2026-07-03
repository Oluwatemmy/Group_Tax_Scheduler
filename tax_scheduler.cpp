#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iomanip>
#include <limits>
#include <ctime>
#include <stdexcept>

void clearInputLine() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

std::string todayDate() {
    std::time_t now = std::time(nullptr);
    std::tm*    lt  = std::localtime(&now);
    char buf[11];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", lt);
    return std::string(buf);
}

bool isValidDate(const std::string& d) {
    if (d.size() != 10) return false;
    if (d[4] != '-' || d[7] != '-') return false;
    for (int i = 0; i < 10; ++i) {
        if (i == 4 || i == 7) continue;
        if (d[i] < '0' || d[i] > '9') return false;
    }
    if      (d[5] == '0') { if (d[6] < '1' || d[6] > '9') return false; }
    else if (d[5] == '1') { if (d[6] < '0' || d[6] > '2') return false; }
    else return false;
    if      (d[8] == '0') { if (d[9] < '1' || d[9] > '9') return false; }
    else if (d[8] == '1' || d[8] == '2') { if (d[9] < '0' || d[9] > '9') return false; }
    else if (d[8] == '3') { if (d[9] < '0' || d[9] > '1') return false; }
    else return false;
    return true;
}

class InvalidInputException : public std::runtime_error {
public:
    explicit InvalidInputException(const std::string& msg)
        : std::runtime_error(msg) {}
};

class Taxpayer {
protected:
    std::string name;
    double      income;
    bool        paid;
public:
    Taxpayer(std::string n, double inc) : name(n), income(inc), paid(false) {}
    virtual ~Taxpayer() {}

    virtual double      calculateTax() const = 0;
    virtual std::string category()     const = 0;

    std::string getName()   const { return name; }
    double      getIncome() const { return income; }
    bool        isPaid()    const { return paid; }
    void        markPaid()        { paid = true; }
};

class Individual : public Taxpayer {
public:
    Individual(std::string n, double inc) : Taxpayer(n, inc) {}
    double      calculateTax() const override { return getIncome() * 0.10; }
    std::string category()     const override { return "Individual"; }
};

class Business : public Taxpayer {
public:
    Business(std::string n, double inc) : Taxpayer(n, inc) {}
    double      calculateTax() const override { return getIncome() * 0.20; }
    std::string category()     const override { return "Business"; }
};

class PremiumIndividual : public Individual {
public:
    PremiumIndividual(std::string n, double inc) : Individual(n, inc) {}
    double calculateTax() const override {
        return Individual::calculateTax() + getIncome() * 0.05;
    }
    std::string category() const override { return "Premium Individual"; }
};

Taxpayer* makeTaxpayer(const std::string& category,
                       const std::string& name, double income) {
    if (category == "Individual")         return new Individual(name, income);
    if (category == "Business")           return new Business(name, income);
    if (category == "Premium Individual") return new PremiumIndividual(name, income);
    throw InvalidInputException("Unknown taxpayer category: " + category);
}

void addTaxpayer(std::vector<Taxpayer*>& group) {
    std::cout << "Type - 1=Individual, 2=Business, 3=Premium Individual\nChoose: ";
    int t;
    if (!(std::cin >> t)) { std::cin.clear(); clearInputLine();
        throw InvalidInputException("Type must be a number."); }
    clearInputLine();

    std::string name;
    std::cout << "Name: ";
    std::getline(std::cin, name);
    if (name.empty()) throw InvalidInputException("Name cannot be empty.");

    std::cout << "Income: ";
    double income;
    if (!(std::cin >> income)) { std::cin.clear(); clearInputLine();
        throw InvalidInputException("Income must be a number."); }
    clearInputLine();
    if (income < 0) throw InvalidInputException("Income cannot be negative.");

    std::string category;
    if      (t == 1) category = "Individual";
    else if (t == 2) category = "Business";
    else if (t == 3) category = "Premium Individual";
    else throw InvalidInputException("Type must be 1, 2 or 3.");

    group.push_back(makeTaxpayer(category, name, income));
    std::cout << "Added " << name << " (" << category << ").\n";
}

void viewSchedule(const std::vector<Taxpayer*>& group, const std::string& dueDate) {
    if (group.empty()) { std::cout << "No taxpayers added yet.\n"; return; }

    std::string today = todayDate();
    std::cout << "(Today is " << today << ")\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::left
              << std::setw(4)  << "#"
              << std::setw(18) << "Name"
              << std::setw(20) << "Category"
              << std::setw(12) << "Income"
              << std::setw(12) << "Tax"
              << std::setw(14) << "Due"
              << "Status\n";

    for (std::size_t i = 0; i < group.size(); ++i) {
        Taxpayer* t = group[i];
        std::string status = t->isPaid() ? "PAID"
                           : (dueDate < today ? "OVERDUE" : "unpaid");
        std::cout << std::left
                  << std::setw(4)  << (i + 1)
                  << std::setw(18) << t->getName()
                  << std::setw(20) << t->category()
                  << std::setw(12) << t->getIncome()
                  << std::setw(12) << t->calculateTax()
                  << std::setw(14) << dueDate
                  << status << "\n";
    }
}

void viewTotals(const std::vector<Taxpayer*>& group, const std::string& dueDate) {
    if (group.empty()) { std::cout << "No taxpayers added yet.\n"; return; }

    std::map<std::string, double> taxByCategory;
    double totalTax = 0.0, totalPaid = 0.0, overdueTax = 0.0;
    int overdueCount = 0;
    std::string today = todayDate();
    for (Taxpayer* t : group) {
        double tax = t->calculateTax();
        taxByCategory[t->category()] += tax;
        totalTax += tax;
        if (t->isPaid()) totalPaid += tax;
        else if (dueDate < today) {
            overdueTax += tax;
            overdueCount++;
        }
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Tax by category:\n";
    for (const auto& entry : taxByCategory)
        std::cout << "  " << entry.first << ": " << entry.second << "\n";
    std::cout << "People:      " << group.size()          << "\n";
    std::cout << "Total tax:   " << totalTax              << "\n";
    std::cout << "Paid:        " << totalPaid             << "\n";
    std::cout << "Outstanding: " << (totalTax - totalPaid) << "\n";
    if (overdueCount > 0)
        std::cout << "OVERDUE:     " << overdueCount << " taxpayer(s) owing "
                  << overdueTax << " past the due date!\n";
}

void markAsPaid(std::vector<Taxpayer*>& group) {
    if (group.empty()) { std::cout << "No taxpayers added yet.\n"; return; }

    for (std::size_t i = 0; i < group.size(); ++i)
        std::cout << "  " << (i + 1) << ". " << group[i]->getName()
                  << (group[i]->isPaid() ? " (already PAID)" : "") << "\n";

    std::cout << "Enter number: ";
    int c;
    if (!(std::cin >> c)) { std::cin.clear(); clearInputLine();
        std::cout << "  That was not a number.\n"; return; }
    clearInputLine();

    if (c < 1 || c > static_cast<int>(group.size())) {
        std::cout << "  That number is not on the list.\n"; return;
    }
    group[c - 1]->markPaid();
    std::cout << group[c - 1]->getName() << " marked as PAID.\n";
}

void deleteTaxpayer(std::vector<Taxpayer*>& group) {
    if (group.empty()) { std::cout << "No taxpayers added yet.\n"; return; }

    for (std::size_t i = 0; i < group.size(); ++i)
        std::cout << "  " << (i + 1) << ". " << group[i]->getName()
                  << " (" << group[i]->category() << ")\n";

    std::cout << "Enter number to DELETE: ";
    int c;
    if (!(std::cin >> c)) { std::cin.clear(); clearInputLine();
        std::cout << "  That was not a number.\n"; return; }
    clearInputLine();

    if (c < 1 || c > static_cast<int>(group.size())) {
        std::cout << "  That number is not on the list.\n"; return;
    }

    std::string name = group[c - 1]->getName();
    delete group[c - 1];
    group.erase(group.begin() + (c - 1));
    std::cout << name << " deleted.\n";
}

void changeDueDate(std::string& dueDate) {
    std::cout << "Current due date: " << dueDate << "\n";
    while (true) {
        std::cout << "Enter the new due date (YYYY-MM-DD) [e.g. 2026-08-01]: ";
        std::getline(std::cin, dueDate);
        if (isValidDate(dueDate)) break;
        std::cout << "  That date isn't valid. Please use YYYY-MM-DD.\n";
    }
    std::cout << "Due date changed to " << dueDate << ".\n";
}

void saveToFile(const std::vector<Taxpayer*>& group,
                const std::string& dueDate, const std::string& filename) {
    std::ofstream out(filename);
    if (!out) throw InvalidInputException("Could not open file for writing.");

    out << dueDate << "\n";
    for (Taxpayer* t : group) {
        out << t->category() << "\n"
            << t->getName()  << "\n"
            << t->getIncome() << "\n"
            << t->isPaid()   << "\n";
    }
    std::cout << "Saved " << group.size() << " taxpayer(s) to " << filename << "\n";
}

void loadFromFile(std::vector<Taxpayer*>& group,
                  std::string& dueDate, const std::string& filename) {
    std::ifstream in(filename);
    if (!in) throw InvalidInputException("Could not open file: " + filename);

    for (Taxpayer* t : group) delete t;
    group.clear();

    std::getline(in, dueDate);
    std::string category, name, incomeText, paidText;
    while (std::getline(in, category) && std::getline(in, name) &&
           std::getline(in, incomeText) && std::getline(in, paidText)) {
        double income = std::stod(incomeText);
        Taxpayer* t = makeTaxpayer(category, name, income);
        if (paidText == "1") t->markPaid();
        group.push_back(t);
    }
    std::cout << "Loaded " << group.size() << " taxpayer(s) from " << filename << "\n";
}

int main() {
    std::vector<Taxpayer*> group;
    std::string dueDate;
    const std::string dataFile = "taxpayers.txt";

    std::cout << "=== Group Tax Scheduler (OOP) ===\n";

    std::ifstream existing(dataFile);
    if (existing.good()) {
        existing.close();
        std::cout << "Saved data found (" << dataFile << ").\n"
                  << "  1. Continue with saved data\n"
                  << "  2. Start fresh\n"
                  << "Choose: ";
        int startChoice;
        if (!(std::cin >> startChoice)) { std::cin.clear(); startChoice = 1; }
        clearInputLine();

        if (startChoice == 2) {
            std::cout << "Starting fresh. (The old file stays until you Save over it.)\n";
        } else {
            try {
                loadFromFile(group, dueDate, dataFile);
            } catch (const InvalidInputException& e) {
                std::cout << "  Could not load saved data: " << e.what() << "\n";
            }
        }
    }

    if (!isValidDate(dueDate)) {
        while (true) {
            std::cout << "Enter the payment due date (YYYY-MM-DD) [e.g. 2026-08-01]: ";
            std::getline(std::cin, dueDate);
            if (isValidDate(dueDate)) break;
            std::cout << "  That date isn't valid. Please use YYYY-MM-DD.\n";
        }
    }

    while (true) {
        std::cout << "\n1. Add taxpayer\n"
                  << "2. View tax schedule\n"
                  << "3. View totals\n"
                  << "4. Mark as paid\n"
                  << "5. Delete taxpayer\n"
                  << "6. Change due date\n"
                  << "7. Save to file\n"
                  << "8. Load from file\n"
                  << "9. Exit\n"
                  << "Choose an option: ";

        int choice;
        if (!(std::cin >> choice)) {
            if (std::cin.eof()) break;
            std::cin.clear();
            clearInputLine();
            std::cout << "  Please enter a number (1-9).\n";
            continue;
        }
        clearInputLine();

        try {
            if      (choice == 1) addTaxpayer(group);
            else if (choice == 2) viewSchedule(group, dueDate);
            else if (choice == 3) viewTotals(group, dueDate);
            else if (choice == 4) markAsPaid(group);
            else if (choice == 5) deleteTaxpayer(group);
            else if (choice == 6) changeDueDate(dueDate);
            else if (choice == 7) saveToFile(group, dueDate, dataFile);
            else if (choice == 8) loadFromFile(group, dueDate, dataFile);
            else if (choice == 9) break;
            else std::cout << "  Invalid option, please pick 1-9.\n";
        } catch (const InvalidInputException& e) {
            std::cout << "  Error: " << e.what() << "\n";
        }
    }

    for (Taxpayer* t : group) delete t;
    std::cout << "Goodbye!\n";
    return 0;
}
