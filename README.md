# Group Tax Scheduler

A C++ console application that manages tax for a group of taxpayers,
built as an Object-Oriented Programming school project.

You set a payment due date, add taxpayers of different types, and the app
automatically calculates each one's tax, tracks who has paid, and flags
anyone overdue. Data can be saved to a file and is reloaded on startup.

**Taxpayer types** (inheritance + polymorphism):

| Type | Tax rate |
|---|---|
| Individual | 10% |
| Business | 20% |
| Premium Individual | 15% (10% + 5% surcharge) |

**OOP concepts demonstrated:** encapsulation, abstraction (abstract base
class), 3-level inheritance, runtime polymorphism (virtual functions),
custom exception handling, file persistence, and STL containers
(`vector` and `map`).

## How to run

Just run the included executable — no installation needed:

```powershell
.\tax_scheduler.exe
```

(or double-click `tax_scheduler.exe` — a console window will open)

Then follow the on-screen menu.

## If Windows blocks the .exe

On some PCs, Windows Smart App Control may block the copied executable
("An Application Control policy has blocked this file"). If that happens,
simply recompile it from the included source — you need any C++ compiler
(e.g. g++ from MSYS2):

```powershell
g++ tax_scheduler.cpp -o tax_scheduler.exe -static
.\tax_scheduler.exe
```

## Files

| File | Purpose |
|---|---|
| `tax_scheduler.cpp` | Full source code (single file, commented) |
| `tax_scheduler.exe` | Compiled program, ready to run |
| `taxpayers.txt` | Created by the app when you choose "Save to file" |
