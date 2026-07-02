# Group Tax Scheduler — Complete Code Guide

A plain-English walkthrough of every part of `tax_scheduler.cpp`, in the order
it appears in the file. Line numbers match the current code.

---

## 0. The Big Picture (read this first)

The program is built in **layers**, from bottom to top:

```
┌─────────────────────────────────────────────────────┐
│ main()                    - the menu loop (line 341) │
├─────────────────────────────────────────────────────┤
│ Menu actions              - one function per option  │
│ addTaxpayer, viewSchedule, viewTotals, markAsPaid,   │
│ deleteTaxpayer, changeDueDate, saveToFile, loadFrom… │
├─────────────────────────────────────────────────────┤
│ The class hierarchy       - the OOP heart            │
│ Taxpayer (abstract) → Individual → PremiumIndividual │
│                     → Business                       │
├─────────────────────────────────────────────────────┤
│ Helpers                   - small reusable tools     │
│ clearInputLine, todayDate, isValidDate,              │
│ InvalidInputException, makeTaxpayer                  │
└─────────────────────────────────────────────────────┘
```

**The one sentence that explains the whole design:**
Every taxpayer in the program is stored as a `Taxpayer*` (a pointer to the
base class), but each one *actually is* an `Individual`, `Business`, or
`PremiumIndividual` — and when we call `calculateTax()`, C++ automatically
runs the right version for what the object really is. That is polymorphism,
and everything else in the file exists to support or use it.

---

## 1. The Includes (lines 13–21)

Each `#include` pulls in one toolbox from the C++ standard library:

| Include | What we use from it | Where |
|---|---|---|
| `<iostream>` | `std::cin` (keyboard input), `std::cout` (screen output) | everywhere |
| `<string>` | `std::string`, `std::getline`, `std::stod` (text → number) | everywhere |
| `<vector>` | `std::vector` — a growable list (STL container #1) | holds the group |
| `<map>` | `std::map` — key→value store (STL container #2) | totals per category |
| `<fstream>` | `std::ofstream` (write file), `std::ifstream` (read file) | save / load |
| `<iomanip>` | `std::setw`, `std::fixed`, `std::setprecision` — column formatting | the schedule table |
| `<limits>` | `std::numeric_limits` — "the biggest possible number" | clearing input |
| `<ctime>` | `std::time`, `std::localtime`, `std::strftime` — the system clock | today's date |
| `<stdexcept>` | `std::runtime_error` — the standard error class we inherit from | our exception |

---

## 2. Helper Functions

### 2.1 `clearInputLine()` (lines 29–31)

```cpp
void clearInputLine() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
```

**The problem it solves:** when you type `50000` and press Enter, `std::cin >>`
reads the *number* but leaves the *Enter key* (a `\n` newline) sitting in the
input buffer. The next `getline()` would immediately grab that leftover
newline and return an empty string — a classic beginner bug.

**How it works:** `ignore(max, '\n')` means *"throw away characters until you
hit a newline (or you've thrown away the maximum possible amount)."* After
this call, the buffer is clean and the next `getline()` behaves.

**Rule of thumb in this program:** after every `std::cin >> something`, we
call `clearInputLine()`.

### 2.2 `todayDate()` (lines 36–42)

```cpp
std::time_t now = std::time(nullptr);   // seconds since Jan 1, 1970
std::tm*    lt  = std::localtime(&now);  // split into year/month/day parts
char buf[11];                            // "YYYY-MM-DD" is 10 chars + 1 end mark
std::strftime(buf, sizeof(buf), "%Y-%m-%d", lt);
return std::string(buf);
```

Line by line:
1. `std::time(nullptr)` asks the OS: *how many seconds have passed since
   1 January 1970?* (that's how computers store time — one big number).
2. `std::localtime(&now)` converts that big number into a struct with
   separate fields: year, month, day, hour…
3. `std::strftime` prints those fields into text using a format:
   `%Y` = 4-digit year, `%m` = 2-digit month, `%d` = 2-digit day.
4. `buf[11]` — the text is 10 characters, plus 1 for the invisible `\0`
   end-of-string marker every C-style string needs.

**The clever trick that powers OVERDUE:** dates written as `YYYY-MM-DD`
sort correctly *as plain text*. `"2025-01-01" < "2026-07-02"` is true as a
string comparison, so we never need real calendar math — `dueDate < today`
just works.

### 2.3 `isValidDate()` (lines 45–62)

Checks a date string in five steps, using only character comparisons:

1. **Length** must be exactly 10 (`2026-08-01` → 10 chars).
2. **Dashes** must sit at positions 4 and 7 (`d[4]`, `d[7]`).
3. **Digits**: every other position must be `'0'`–`'9'`.
   (Characters compare by their codes, so `d[i] < '0' || d[i] > '9'` means
   "not a digit".)
4. **Month** (chars 5–6): first digit `'0'` → second must be 1–9 (01–09);
   first digit `'1'` → second must be 0–2 (10–12); anything else → invalid.
5. **Day** (chars 8–9): `'0'`→01–09, `'1'/'2'`→10–29, `'3'`→30–31.

**Known limitation (own it in the defense):** it accepts day 01–31 for *any*
month, so `2026-02-30` or `2026-04-31` slip through. Real calendar rules
(month lengths, leap years) were deliberately left out to keep the function
simple. Say: *"we validate format and ranges; full calendar awareness is a
listed future improvement."*

---

## 3. The Custom Exception (lines 69–73)

```cpp
class InvalidInputException : public std::runtime_error {
public:
    explicit InvalidInputException(const std::string& msg)
        : std::runtime_error(msg) {}
};
```

- **What it is:** our own error type. When input is bad we `throw` one of
  these; the `catch` block in `main()` grabs it and prints the message.
- **Why inherit from `std::runtime_error`?** It already stores a message and
  provides `.what()` to read it — we get all that for free (code reuse via
  inheritance, notes Ch 10).
- **`explicit`** stops C++ from silently converting a random string into an
  exception by accident. You must write it out deliberately:
  `throw InvalidInputException("Income must be a number.");`
- **`: std::runtime_error(msg)`** is a *constructor initialization list* —
  it passes the message up to the parent class's constructor (notes Ch 4.5).

**Why exceptions instead of if/else returns?** The error can be thrown deep
inside `addTaxpayer()` and caught once, centrally, in `main()`. Error
handling is separated from normal logic (notes Ch 10 intro).

---

## 4. The Class Hierarchy — the OOP Heart (lines 84–129)

```
        Taxpayer            (abstract - cannot be created directly)
        /       \
  Individual   Business     (10%)         (20%)
      |
PremiumIndividual           (10% + 5% surcharge)
```

### 4.1 `Taxpayer` — the abstract base (lines 84–101)

```cpp
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
    ...getters...
};
```

Piece by piece:

- **`protected:`** — the data is hidden from outside code, but child classes
  may still use it. That's **encapsulation** (notes Ch 5). Outside code can
  only *read* the data through getters, and can only change `paid` through
  `markPaid()` — there is no way to corrupt a taxpayer from outside.

- **The constructor** uses an initialization list
  (`: name(n), income(inc), paid(false)`) — members are set before the body
  runs (notes Ch 4.5). Every new taxpayer starts unpaid.

- **`virtual ~Taxpayer() {}`** — the virtual destructor. We delete objects
  through base-class pointers (`delete group[i]` where `group[i]` is a
  `Taxpayer*`). Without `virtual`, C++ would only run the base class's
  destructor and skip the child's — undefined behaviour / leaks. With it,
  the correct child destructor runs first, then the base one.
  (Notes p.22: "virtual destructor — always needed!")

- **`= 0` (pure virtual)** on `calculateTax()` and `category()` does two jobs:
  1. Makes `Taxpayer` **abstract** — `Taxpayer t("x", 5);` will not compile.
     You can only create concrete children. (**Abstraction**, notes Ch 5.3.)
  2. **Forces** every child class to provide its own version — the compiler
     will refuse a child that forgets.

- **`const` on the getters** (`double getIncome() const`) promises "this
  function does not modify the object". Good practice for read-only methods
  (notes Ch 3.4), and required so they can be called on `const` objects.

### 4.2 `Individual` (lines 104–109) and `Business` (lines 112–117)

```cpp
class Individual : public Taxpayer {
public:
    Individual(std::string n, double inc) : Taxpayer(n, inc) {}
    double      calculateTax() const override { return getIncome() * 0.10; }
    std::string category()     const override { return "Individual"; }
};
```

- **`: public Taxpayer`** — public inheritance: an Individual *is a*
  Taxpayer (notes Ch 6).
- **`: Taxpayer(n, inc)`** — constructor chaining: the child passes the
  name and income up to the parent's constructor, which stores them.
- **`override`** — tells the compiler "this is meant to replace a virtual
  function from the parent". If you typo the name or signature, the compiler
  errors instead of silently creating a new unrelated function
  (notes p.22, "The override Keyword").
- `Business` is identical except the rate is `0.20` (20%).

### 4.3 `PremiumIndividual` — the third level (lines 121–129)

```cpp
class PremiumIndividual : public Individual {
    double calculateTax() const override {
        return Individual::calculateTax() + getIncome() * 0.05;
    }
};
```

- Inherits from **`Individual`**, not from `Taxpayer` — that's what makes the
  chain three levels deep: `Taxpayer → Individual → PremiumIndividual`
  (**multilevel inheritance**, notes Table 6.1).
- **`Individual::calculateTax()`** explicitly calls the *parent's* version
  (the 10%), then adds a 5% surcharge. This is **code reuse**: if the base
  Individual rate ever changes, Premium automatically follows.
  Total: 10% + 5% = 15% of income.

### 4.4 `makeTaxpayer()` — the factory function (lines 133–139)

```cpp
Taxpayer* makeTaxpayer(const std::string& category,
                       const std::string& name, double income) {
    if (category == "Individual")         return new Individual(name, income);
    if (category == "Business")           return new Business(name, income);
    if (category == "Premium Individual") return new PremiumIndividual(name, income);
    throw InvalidInputException("Unknown taxpayer category: " + category);
}
```

- Give it a category *name* (text), get back the right *object* — created on
  the heap with `new`. The return type is `Taxpayer*`, so the caller doesn't
  need to know or care which concrete class it got.
- It's used in **two places** — `addTaxpayer()` (user input) and
  `loadFromFile()` (reading the save file) — so the "which class matches
  which name" logic lives in exactly one spot. If we add a fourth taxpayer
  type someday, this is the only mapping to update.
- This is a simplified version of the **Factory Method pattern**
  (notes Appendix A.2).
- **Ownership rule:** whoever receives the pointer is responsible for
  `delete`-ing it eventually. In this program that's the `group` vector's
  cleanup code (delete option, load, and the end of `main`).

---

## 5. The Menu Actions (one function = one job)

### 5.1 `addTaxpayer()` — option 1 (lines 146–173)

Flow: ask type (1/2/3) → ask name → ask income → build object → add to list.

Key details:
- Every read is checked. `if (!(std::cin >> t))` is true when the user typed
  letters instead of a number. Then: `std::cin.clear()` un-jams the stream
  (it goes into a "failed" state after bad input and ignores everything
  until cleared), `clearInputLine()` throws the junk away, and we **throw**
  an exception with a human message.
- Name uses `getline` so it can contain spaces ("Acme Ltd").
- Empty names and negative incomes are rejected.
- `group.push_back(makeTaxpayer(...))` — the vector stores the new
  object's *pointer*.

**Why throw instead of printing here?** All errors funnel to one `catch` in
`main()` — one consistent error style everywhere.

### 5.2 `viewSchedule()` — option 2 (lines 177–205)

Prints the table. Two things to understand:

**Formatting** (`<iomanip>`):
- `std::fixed << std::setprecision(2)` → money style: `5000.00`, not `5e+03`.
- `std::left << std::setw(18)` → left-align in a column 18 characters wide.
  `setw` applies to the *next* item only, so it's repeated per column.

**The status logic:**
```cpp
std::string status = t->isPaid() ? "PAID"
                   : (dueDate < today ? "OVERDUE" : "unpaid");
```
Read it as: paid wins; otherwise, if the due date is already behind today,
OVERDUE; otherwise plain unpaid. (`?:` is the compact if/else operator.)

**The polymorphism moment (the line to show the lecturer):**
```cpp
t->calculateTax()   // t is a Taxpayer*, but the CHILD's version runs
```
The vector holds `Taxpayer*` pointers, and this one line yields 10%, 20%, or
15% depending on what each object really is. C++ resolves it at runtime
through the **vtable** (notes Ch 7.2). Same for `t->category()`.

### 5.3 `viewTotals()` — option 3 (lines 208–237)

- **`std::map<std::string, double> taxByCategory;`** — our second STL
  container. A map stores key→value pairs, sorted by key.
  `taxByCategory[t->category()] += tax;` is the elegant line: if the key
  ("Business") doesn't exist yet, the map creates it starting at 0.0, then
  adds. Three categories → at most three entries, built automatically.
- The same loop also accumulates: total tax, total paid, and — if a person
  is unpaid *and* the due date has passed — the overdue count and amount.
- `for (const auto& entry : taxByCategory)` — a range-based for over the
  map; `entry.first` is the key (category name), `entry.second` the value
  (summed tax).
- Prints: per-category tax, people count, total, paid, outstanding, and an
  OVERDUE alert line only when someone actually is overdue.

### 5.4 `markAsPaid()` — option 4 (lines 240–258)

- Shows a numbered list (1, 2, 3…), user picks a number.
- **`group[c - 1]`** — the `-1` matters: humans count from 1, vectors from 0.
- Range check first: `if (c < 1 || c > (int)group.size())` — without it,
  entering `99` would access memory that isn't ours (undefined behaviour).
- `static_cast<int>(...)` converts `size()` (an unsigned type) to `int` so
  the comparison doesn't mix signed/unsigned.
- Then simply: `group[c - 1]->markPaid();` — the only way `paid` can ever
  change, because the field itself is protected. Encapsulation in action.

### 5.5 `deleteTaxpayer()` — option 5 (lines 264–285)

The important part is that removal takes **two steps, in this order**:

```cpp
delete group[c - 1];                      // 1. free the object's memory
group.erase(group.begin() + (c - 1));     // 2. remove the pointer from the vector
```

- Only `delete` → the vector still holds a **dangling pointer** to freed
  memory; using it later crashes.
- Only `erase` → the pointer is gone but the object still occupies memory
  forever: a **memory leak**.
- Both, in this order → clean. (Notes E.2 lists both mistakes.)
- `group.begin() + (c - 1)` converts the index into the *iterator* that
  `erase` requires.

### 5.6 `changeDueDate()` — option 6 (lines 290–299)

- Shows the current date, then loops until a valid new one is typed —
  the exact same `isValidDate` retry loop as at startup (consistency).
- Takes the date as **`std::string&`** — a *reference*. It modifies the
  caller's (`main`'s) actual variable, not a copy. Without the `&`, the
  change would silently vanish when the function returned.
- Demo use: set a past date → unpaid people flip to OVERDUE instantly.

### 5.7 `saveToFile()` — option 7 (lines 302–315)

File format — deliberately simple, one item per line:

```
2026-08-01            ← line 1: the due date
Individual            ← then 4 lines per taxpayer:
Amina                 ←   category, name, income, paid
50000                 ←
0                     ←   (1 = paid, 0 = unpaid)
Business
Acme Ltd
...
```

- `std::ofstream out(filename);` opens the file for writing (creates it, or
  **overwrites** what was there). `if (!out)` catches "couldn't open".
- Writing uses `<<` exactly like `cout` — file streams and console streams
  share the same interface (notes Ch 11).
- Note we save `t->category()` — that's how load knows which *class* to
  rebuild.

### 5.8 `loadFromFile()` — option 8 (lines 318–336)

- Opens with `std::ifstream`; throws if the file doesn't exist.
- **First frees the current list** (`delete` each, then `clear()`), because
  load *replaces* what's in memory — otherwise we'd leak the old objects.
- Reads the due date, then loops reading 4 lines at a time:
  ```cpp
  while (std::getline(in, category) && std::getline(in, name) && ...)
  ```
  The `&&` chain stops cleanly at end-of-file: if any of the four reads
  fails, the loop ends.
- `std::stod(incomeText)` converts the income text back into a `double`.
- `makeTaxpayer(category, ...)` rebuilds the object *of the right class* —
  the factory again.
- If the paid flag was `"1"`, we call `markPaid()`.

**Save + load together = "object persistence":** the objects live in memory,
but their *state* survives program restarts via the file (notes Ch 11).

---

## 6. `main()` — Putting It All Together (lines 341–429)

### 6.1 Setup (lines 342–344)
```cpp
std::vector<Taxpayer*> group;            // the whole group lives here
std::string dueDate;
const std::string dataFile = "taxpayers.txt";
```
One vector of base-class pointers holds every taxpayer, whatever its type.
`dataFile` is `const` — it never changes, and the compiler enforces that.

### 6.2 Smart startup (lines 348–383)

1. `std::ifstream existing(dataFile); if (existing.good())` — a standard
   trick to test "does this file exist?" (if it opened, it exists).
2. If it exists, ask: **1. Continue with saved data / 2. Start fresh**.
   - Anything that isn't `2` (including junk input) counts as *continue* —
     the **safe default**: you can never lose data by mistyping.
   - "Start fresh" just *ignores* the file. It is not deleted; it only gets
     replaced if the user later chooses Save.
3. After that, `if (!isValidDate(dueDate))` — if there was no file (first
   run) or its first line was unreadable, ask for a due date with the
   validation retry loop. If load succeeded, `dueDate` is already valid and
   this block is skipped entirely.

### 6.3 The menu loop (lines 385–423)

- `while (true)` — runs forever until `break` (option 9 or end of input).
- The menu-choice read has a three-way guard:
  - read OK → proceed;
  - `std::cin.eof()` → the input stream itself ended (e.g. the terminal
    closed) → break out cleanly instead of looping forever;
  - otherwise → the user typed letters → `clear()` + `clearInputLine()` +
    a polite message, then `continue` back to the menu.
- The dispatch is a plain if/else-if chain mapping 1–9 to the functions.
- **The whole dispatch sits inside `try { ... } catch`** — any
  `InvalidInputException` thrown anywhere below lands here, prints
  `Error: <message>`, and the loop continues. One catch handles every
  error in the program. The program cannot be crashed by bad typing.

### 6.4 Shutdown (lines 425–428)
```cpp
for (Taxpayer* t : group) delete t;
```
Every object created with `new` gets its `delete` before exit — thanks to
the virtual destructor, each is destroyed correctly. No leaks.

---

## 7. Where Each OOP Concept Lives (quick map)

| Concept | Exact location | One-line defense |
|---|---|---|
| Encapsulation | `Taxpayer`'s `protected:` data + getters (lines 85–100) | "Data is hidden; the only way to change `paid` is `markPaid()`." |
| Abstraction | pure virtuals `= 0` (lines 93–94) | "Taxpayer is abstract — you can't instantiate it, only concrete children." |
| Inheritance (3 levels) | `Individual : public Taxpayer` (104), `PremiumIndividual : public Individual` (121) | "Taxpayer → Individual → PremiumIndividual, plus Business as a branch." |
| Polymorphism | `t->calculateTax()` in viewSchedule (201) & viewTotals (216) | "One call, three behaviours — resolved at runtime via the vtable." |
| Virtual destructor | line 91 | "We delete through base pointers, so the child destructor must run too." |
| `override` keyword | every child method | "Compiler-checked overriding — typos become errors." |
| Constructor init lists | line 90, 106, etc. | "Members initialised before the constructor body runs." |
| Exception handling | class at 69, throws in addTaxpayer/factory/file fns, catch at 420 | "Thrown at the point of failure, caught once in main." |
| File persistence | saveToFile (302), loadFromFile (318) | "Objects rebuilt from text via the factory." |
| STL `vector` | `group` (line 342) | "Growable list of base-class pointers." |
| STL `map` | `taxByCategory` (line 211) | "Auto-creating key→value sums per category." |
| Factory pattern | makeTaxpayer (133) | "Category name in, correct object out — one place to extend." |
| Memory management | delete in deleteTaxpayer (282), loadFromFile (323), main (426) | "Every new has a matching delete." |

---

## 8. Likely Lecturer Questions — with answers

**Q. Why is `calculateTax()` pure virtual instead of just virtual?**
Because there is no sensible "default" tax — every taxpayer type must define
its own. Pure virtual makes forgetting it a *compile error*, and it makes
`Taxpayer` abstract so nobody can create a typeless taxpayer.

**Q. What happens if you remove `virtual` from the destructor?**
Deleting through a `Taxpayer*` would only run `~Taxpayer()`, never the
child's destructor — undefined behaviour. With `virtual`, the child's runs
first, then the base's.

**Q. Where exactly does polymorphism happen at runtime?**
At `t->calculateTax()`. The compiler can't know what `t` points to, so it
emits a lookup through the object's **vtable** — a hidden table of function
pointers each polymorphic class carries. The object's real class decides
which entry is there.

**Q. Why store `Taxpayer*` pointers instead of `Taxpayer` objects in the vector?**
Two reasons. (1) You can't create plain `Taxpayer` objects — it's abstract.
(2) Storing child objects *by value* in a base-typed container would cause
**object slicing** — the child's extra behaviour would be cut off. Pointers
preserve the real type. (Notes E.2, "Object slicing".)

**Q. How does the program avoid memory leaks?**
Every `new` (all inside `makeTaxpayer`) has a matching `delete`: when a
taxpayer is deleted (option 5), before a load replaces the list, and in the
final cleanup loop in `main`. The virtual destructor makes each delete
destroy the full object.

**Q. Why do you call `std::cin.clear()` after bad input?**
When `>>` fails, the stream sets a fail flag and *stops reading anything*
until the flag is cleared. `clear()` resets the flag; `clearInputLine()`
then discards the junk text that caused the failure.

**Q. Why is the date compared as a string?**
`YYYY-MM-DD` has its parts ordered biggest-to-smallest (year, month, day),
so alphabetical order equals chronological order. `dueDate < today` on
strings is therefore a correct date comparison — no date library needed.

**Q. Your date validation accepts 2026-02-30. Why?**
By design, it checks format and digit ranges only (month 01–12, day 01–31).
Full calendar rules (month lengths, leap years) would triple the function's
size for little teaching value — it's documented as a future improvement.

**Q. Why the factory function instead of `new Individual(...)` everywhere?**
The category-name→class mapping is needed twice (user input and file load).
Centralising it means one place to change, and neither caller needs to know
the concrete classes — they just get a `Taxpayer*`.

**Q. What's the difference between `protected` and `private` here?**
`private` would hide the data even from child classes; `protected` hides it
from the outside world but lets `Individual` etc. inherit access. We chose
protected because children are trusted parts of the same design.

---

## 9. Mini Glossary

| Term | Meaning |
|---|---|
| **abstract class** | A class with ≥1 pure virtual function; cannot be instantiated. |
| **pure virtual** | `virtual ... = 0;` — a method a child *must* implement. |
| **override** | Child replaces a parent's virtual method; keyword makes the compiler verify it. |
| **vtable** | Hidden per-class table of function pointers that makes virtual calls pick the right version at runtime. |
| **dangling pointer** | A pointer to memory that has been freed. Using it = crash/corruption. |
| **memory leak** | Heap memory that is never freed; lost until the program exits. |
| **object slicing** | Copying a child object into a base variable, losing the child parts. |
| **initialization list** | `Ctor() : member(value) {}` — sets members before the body runs. |
| **factory** | A function/class that builds the right object type for you. |
| **STL** | Standard Template Library — ready-made containers (`vector`, `map`) and algorithms. |
| **stream fail state** | After bad input, `cin` refuses further reads until `clear()` is called. |
| **`\0` terminator** | The invisible character that marks the end of a C-style string buffer. |

---

## 10. Suggested Study Path

1. Read section 0 until the "one sentence" makes sense.
2. Read the class hierarchy (section 4) alongside the real code, lines 84–139.
3. Trace one full flow by hand: choose option 1, follow `addTaxpayer` →
   `makeTaxpayer` → `push_back`; then option 2, follow the loop and the
   polymorphic call.
4. Do the same for delete (5) and save/load (7/8).
5. Close the file and try to redraw the hierarchy diagram and the save-file
   format from memory.
6. Have someone quiz you from section 8.

If you can explain sections 4, 5.5, and 6.3 without looking, you can defend
this project.
