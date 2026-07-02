// ============================================================================
//  GROUP TAX SCHEDULER  (Object-Oriented version) - School Project
// ----------------------------------------------------------------------------
//  Demonstrates the core OOP concepts from the course:
//    * Encapsulation      - data hidden inside a class, reached through methods
//    * Inheritance        - Taxpayer -> Individual -> PremiumIndividual (3 levels)
//    * Polymorphism       - one calculateTax() call, different behaviour per type
//    * Exception handling  - custom InvalidInputException with try/catch
//    * File persistence    - save/load taxpayers to a text file
//    * STL containers      - std::vector (the group) and std::map (totals)
// ============================================================================

#include <iostream>    // std::cin, std::cout
#include <string>      // std::string, std::getline, std::stod
#include <vector>      // std::vector  (STL container #1)
#include <map>         // std::map     (STL container #2)
#include <fstream>     // std::ofstream, std::ifstream (file handling)
#include <iomanip>     // std::setw, std::fixed, std::setprecision (tidy columns)
#include <limits>      // std::numeric_limits (clear leftover input)
#include <ctime>       // std::time, std::localtime, std::strftime (today's date)
#include <stdexcept>   // std::runtime_error (base for our custom exception)

// ---------------------------------------------------------------------------
//  Small helpers
// ---------------------------------------------------------------------------

// After reading a number with >>, a newline stays in the input buffer.
// This throws that leftover line away so the next getline() works correctly.
void clearInputLine() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Ask the computer's clock for today's date, formatted as "YYYY-MM-DD".
// Dates written this way compare correctly as plain text, so we can tell
// which date is earlier just with the < operator.
std::string todayDate() {
    std::time_t now = std::time(nullptr);      // seconds since 1970 (right now)
    std::tm*    lt  = std::localtime(&now);     // split into year / month / day
    char buf[11];                               // room for "YYYY-MM-DD" + end mark
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", lt);
    return std::string(buf);
}

// Check a date is YYYY-MM-DD with a real month (01-12) and day (01-31).
bool isValidDate(const std::string& d) {
    if (d.size() != 10) return false;
    if (d[4] != '-' || d[7] != '-') return false;
    for (int i = 0; i < 10; ++i) {
        if (i == 4 || i == 7) continue;         // skip the dash positions
        if (d[i] < '0' || d[i] > '9') return false;
    }
    // month 01-12
    if      (d[5] == '0') { if (d[6] < '1' || d[6] > '9') return false; }
    else if (d[5] == '1') { if (d[6] < '0' || d[6] > '2') return false; }
    else return false;
    // day 01-31
    if      (d[8] == '0') { if (d[9] < '1' || d[9] > '9') return false; }
    else if (d[8] == '1' || d[8] == '2') { if (d[9] < '0' || d[9] > '9') return false; }
    else if (d[8] == '3') { if (d[9] < '0' || d[9] > '1') return false; }
    else return false;
    return true;
}

// ---------------------------------------------------------------------------
//  CUSTOM EXCEPTION  (Chapter 10)
//  We throw this when the user types something invalid. It inherits from
//  std::runtime_error, so it carries a message we can read with .what().
// ---------------------------------------------------------------------------
class InvalidInputException : public std::runtime_error {
public:
    explicit InvalidInputException(const std::string& msg)
        : std::runtime_error(msg) {}
};

// ===========================================================================
//  CLASS HIERARCHY  (Encapsulation + Inheritance + Polymorphism)
// ===========================================================================

// ---- Abstract base class ----
// Taxpayer keeps its data protected (hidden) and exposes a clean interface.
// calculateTax() and category() are PURE VIRTUAL (= 0), which:
//   (a) makes Taxpayer an ABSTRACT class - you cannot create a plain Taxpayer,
//   (b) forces every child type to supply its own version -> polymorphism.
class Taxpayer {
protected:
    std::string name;
    double      income;
    bool        paid;
public:
    Taxpayer(std::string n, double inc) : name(n), income(inc), paid(false) {}
    virtual ~Taxpayer() {}                       // virtual destructor (good practice)

    virtual double      calculateTax() const = 0;  // each type taxes differently
    virtual std::string category()     const = 0;  // "Individual", "Business"...

    // Getters give controlled, read-only access to the hidden data.
    std::string getName()   const { return name; }
    double      getIncome() const { return income; }
    bool        isPaid()    const { return paid; }
    void        markPaid()        { paid = true; }
};

// ---- Level 2: an Individual pays a flat 10% ----
class Individual : public Taxpayer {
public:
    Individual(std::string n, double inc) : Taxpayer(n, inc) {}
    double      calculateTax() const override { return getIncome() * 0.10; }
    std::string category()     const override { return "Individual"; }
};

// ---- Level 2: a Business pays a flat 20% ----
class Business : public Taxpayer {
public:
    Business(std::string n, double inc) : Taxpayer(n, inc) {}
    double      calculateTax() const override { return getIncome() * 0.20; }
    std::string category()     const override { return "Business"; }
};

// ---- Level 3: a PremiumIndividual IS an Individual, plus a 5% surcharge ----
// This gives a three-level chain: Taxpayer -> Individual -> PremiumIndividual.
class PremiumIndividual : public Individual {
public:
    PremiumIndividual(std::string n, double inc) : Individual(n, inc) {}
    // Reuse Individual's 10% and add a 5% surcharge on top of it.
    double calculateTax() const override {
        return Individual::calculateTax() + getIncome() * 0.05;
    }
    std::string category() const override { return "Premium Individual"; }
};

// Build the right taxpayer object from a category name.
// The caller owns the returned pointer and must delete it.
Taxpayer* makeTaxpayer(const std::string& category,
                       const std::string& name, double income) {
    if (category == "Individual")         return new Individual(name, income);
    if (category == "Business")           return new Business(name, income);
    if (category == "Premium Individual") return new PremiumIndividual(name, income);
    throw InvalidInputException("Unknown taxpayer category: " + category);
}

// ===========================================================================
//  MENU ACTIONS  (each function does one job)
// ===========================================================================

// Option 1: add a taxpayer. Throws InvalidInputException on bad input.
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

// Option 2: print a schedule. calculateTax()/category() are resolved
// polymorphically - the correct child version runs for each object.
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
                  << std::setw(20) << t->category()      // polymorphic call
                  << std::setw(12) << t->getIncome()
                  << std::setw(12) << t->calculateTax()  // polymorphic call
                  << std::setw(14) << dueDate
                  << status << "\n";
    }
}

// Option 3: totals. Uses a std::map to add up tax per category (STL #2).
void viewTotals(const std::vector<Taxpayer*>& group, const std::string& dueDate) {
    if (group.empty()) { std::cout << "No taxpayers added yet.\n"; return; }

    std::map<std::string, double> taxByCategory;   // second STL container
    double totalTax = 0.0, totalPaid = 0.0, overdueTax = 0.0;
    int overdueCount = 0;
    std::string today = todayDate();
    for (Taxpayer* t : group) {
        double tax = t->calculateTax();
        taxByCategory[t->category()] += tax;
        totalTax += tax;
        if (t->isPaid()) totalPaid += tax;
        else if (dueDate < today) {               // unpaid AND past the due date
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

// Option 4: mark a taxpayer as paid.
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

// Option 5: delete a taxpayer (e.g. added by mistake, or left the group).
// We must do TWO things: free the object's memory (delete), then remove
// its pointer from the vector (erase) - otherwise we'd leak memory or
// keep a dangling pointer.
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
    delete group[c - 1];                          // free the object's memory
    group.erase(group.begin() + (c - 1));         // remove the pointer from the list
    std::cout << name << " deleted.\n";
}

// Option 6: change the shared payment due date (with the same validation
// as at startup). Handy for testing: set a past date and unpaid taxpayers
// show OVERDUE; set a future date and they go back to unpaid.
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

// Option 7: save everything to a text file (4 lines per taxpayer).
void saveToFile(const std::vector<Taxpayer*>& group,
                const std::string& dueDate, const std::string& filename) {
    std::ofstream out(filename);
    if (!out) throw InvalidInputException("Could not open file for writing.");

    out << dueDate << "\n";                       // first line = the due date
    for (Taxpayer* t : group) {
        out << t->category() << "\n"
            << t->getName()  << "\n"
            << t->getIncome() << "\n"
            << t->isPaid()   << "\n";             // 1 = paid, 0 = unpaid
    }
    std::cout << "Saved " << group.size() << " taxpayer(s) to " << filename << "\n";
}

// Option 8: load taxpayers back from the file (replaces the current list).
void loadFromFile(std::vector<Taxpayer*>& group,
                  std::string& dueDate, const std::string& filename) {
    std::ifstream in(filename);
    if (!in) throw InvalidInputException("Could not open file: " + filename);

    for (Taxpayer* t : group) delete t;           // free the old list first
    group.clear();

    std::getline(in, dueDate);                    // first line = the due date
    std::string category, name, incomeText, paidText;
    while (std::getline(in, category) && std::getline(in, name) &&
           std::getline(in, incomeText) && std::getline(in, paidText)) {
        double income = std::stod(incomeText);    // text -> number
        Taxpayer* t = makeTaxpayer(category, name, income);
        if (paidText == "1") t->markPaid();
        group.push_back(t);
    }
    std::cout << "Loaded " << group.size() << " taxpayer(s) from " << filename << "\n";
}

// ===========================================================================
//  MAIN
// ===========================================================================
int main() {
    std::vector<Taxpayer*> group;                 // STL container #1 (the group)
    std::string dueDate;
    const std::string dataFile = "taxpayers.txt";

    std::cout << "=== Group Tax Scheduler (OOP) ===\n";

    // If a saved file exists, let the user choose: continue where they left
    // off (load it) or start fresh (ignore it). Starting fresh does NOT
    // delete the old file - it is only replaced if they Save later.
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
            // Anything other than 2 = continue: the safe default, no data lost.
            try {
                loadFromFile(group, dueDate, dataFile);
            } catch (const InvalidInputException& e) {
                std::cout << "  Could not load saved data: " << e.what() << "\n";
            }
        }
    }

    // If there was no saved file (first run) or its date was unreadable,
    // ask for a valid due date.
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
            if (std::cin.eof()) break;            // input ended - stop cleanly
            std::cin.clear();                     // recover from a non-number
            clearInputLine();
            std::cout << "  Please enter a number (1-9).\n";
            continue;
        }
        clearInputLine();

        // Any InvalidInputException thrown inside an action is caught here,
        // so a bad entry shows a message instead of crashing the program.
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

    // Free every heap object we created (no memory leaks).
    for (Taxpayer* t : group) delete t;
    std::cout << "Goodbye!\n";
    return 0;
}
