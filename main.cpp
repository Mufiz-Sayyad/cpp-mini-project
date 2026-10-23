/*
 * Banking / ATM Simulator (PYBANK ATM)
 * Author : Mufiz Mushtak Sayyad
 * Course : Object-Oriented Programming mini project
 *
 * Core features
 *   - Create an account (name, account number, PIN)
 *   - Login with account number + PIN (3 attempts, then the account locks)
 *   - Check balance, deposit, withdraw (rejected if balance is insufficient)
 *   - Mini statement (last 5 transactions)
 *
 * Bonus features
 *   - Change PIN
 *   - Transfer money between two accounts
 *   - Daily withdrawal limit
 *   - All accounts and transactions are saved to a file (accounts.txt)
 *
 * Build : g++ -std=c++11 -Wall -o atm main.cpp
 * Run   : ./atm
 *
 * Note: PINs are stored as plain text because this is a learning project.
 *       A real bank would store a salted hash instead.
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <ctime>
#include <algorithm>
using namespace std;

const string DATA_FILE      = "accounts.txt";
const double DAILY_LIMIT    = 20000.0;
const int    MAX_ATTEMPTS   = 3;
const size_t STATEMENT_SIZE = 5;

// ---------- Helpers ----------

string currentDate() {
    time_t t = time(nullptr);
    char buf[16];
    strftime(buf, sizeof(buf), "%Y-%m-%d", localtime(&t));
    return buf;
}

string currentDateTime() {
    time_t t = time(nullptr);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&t));
    return buf;
}

vector<string> split(const string& line, char delim) {
    vector<string> parts;
    string item;
    stringstream ss(line);
    while (getline(ss, item, delim)) parts.push_back(item);
    return parts;
}

string readLine(const string& prompt) {
    string s;
    cout << prompt;
    if (!getline(cin, s)) {          // end of input (Ctrl+D / Ctrl+Z)
        cout << "\nInput closed. Goodbye.\n";
        exit(0);
    }
    return s;
}

// Reads a whole number. Returns -1 if the input is not a valid non-negative integer.
long long readInteger(const string& prompt) {
    string s = readLine(prompt);
    if (s.empty() || s.size() > 12) return -1;
    for (char c : s) if (!isdigit(static_cast<unsigned char>(c))) return -1;
    return stoll(s);
}

// Reads a positive money amount. Returns -1 if invalid.
double readAmount(const string& prompt) {
    string s = readLine(prompt);
    istringstream in(s);
    double v;
    char extra;
    if (!(in >> v) || (in >> extra) || v <= 0 || v > 100000000) return -1;
    return v;
}

bool isValidPin(const string& pin) {
    if (pin.size() != 4) return false;
    for (char c : pin) if (!isdigit(static_cast<unsigned char>(c))) return false;
    return true;
}

// ---------- Transaction ----------

struct Transaction {
    string type;          // e.g. "Deposit", "Withdraw", "Transfer out"
    double amount;
    double balanceAfter;
    string time;
};

// ---------- Account ----------

class Account {
private:
    long long accountNumber;
    string holderName;
    string pin;
    double balance;
    bool locked;
    double withdrawnToday;
    string lastWithdrawDate;
    vector<Transaction> history;

    void resetDailyIfNewDay() {
        string today = currentDate();
        if (lastWithdrawDate != today) {
            lastWithdrawDate = today;
            withdrawnToday = 0;
        }
    }

    void record(const string& type, double amount) {
        history.push_back({type, amount, balance, currentDateTime()});
    }

public:
    Account(long long no, const string& name, const string& p, double opening = 0)
        : accountNumber(no), holderName(name), pin(p), balance(opening),
          locked(false), withdrawnToday(0), lastWithdrawDate(currentDate()) {
        if (opening > 0) record("Opening deposit", opening);
    }

    // Getters
    long long getNumber() const { return accountNumber; }
    const string& getName() const { return holderName; }
    double getBalance() const { return balance; }
    bool isLocked() const { return locked; }

    // Security
    bool checkPin(const string& p) const { return pin == p; }
    void lock() { locked = true; }
    void changePin(const string& newPin) { pin = newPin; }

    // Money operations. Each returns an empty string on success, or the reason for failure.
    string deposit(double amount) {
        if (amount <= 0) return "Amount must be greater than zero.";
        balance += amount;
        record("Deposit", amount);
        return "";
    }

    string withdraw(double amount) {
        if (amount <= 0) return "Amount must be greater than zero.";
        resetDailyIfNewDay();
        if (amount > balance) return "Insufficient balance.";
        if (withdrawnToday + amount > DAILY_LIMIT) {
            ostringstream msg;
            msg << "Daily withdrawal limit exceeded. You can still withdraw "
                << fixed << setprecision(2) << (DAILY_LIMIT - withdrawnToday) << " today.";
            return msg.str();
        }
        balance -= amount;
        withdrawnToday += amount;
        record("Withdraw", amount);
        return "";
    }

    string transferOut(double amount, long long toAccount) {
        if (amount <= 0) return "Amount must be greater than zero.";
        if (amount > balance) return "Insufficient balance.";
        balance -= amount;
        record("Transfer to " + to_string(toAccount), amount);
        return "";
    }

    void transferIn(double amount, long long fromAccount) {
        balance += amount;
        record("Transfer from " + to_string(fromAccount), amount);
    }

    void showMiniStatement() const {
        cout << "\n----- MINI STATEMENT (last " << STATEMENT_SIZE << ") -----\n";
        if (history.empty()) {
            cout << "No transactions yet.\n";
            return;
        }
        size_t start = history.size() > STATEMENT_SIZE ? history.size() - STATEMENT_SIZE : 0;
        cout << fixed << setprecision(2);
        for (size_t i = history.size(); i > start; --i) {   // newest first
            const Transaction& t = history[i - 1];
            cout << t.time << "  " << left << setw(22) << t.type
                 << right << setw(10) << t.amount
                 << "  Bal: " << setw(10) << t.balanceAfter << "\n";
        }
    }

    // ----- File handling -----
    // Line 1 : number|name|pin|balance|locked|withdrawnToday|lastWithdrawDate|transactionCount
    // Then   : one line per transaction: type|amount|balanceAfter|time
    void save(ostream& out) const {
        out << fixed << setprecision(2)
            << accountNumber << '|' << holderName << '|' << pin << '|' << balance << '|'
            << (locked ? 1 : 0) << '|' << withdrawnToday << '|' << lastWithdrawDate << '|'
            << history.size() << '\n';
        for (const Transaction& t : history)
            out << t.type << '|' << t.amount << '|' << t.balanceAfter << '|' << t.time << '\n';
    }

    static bool load(istream& in, Account& result) {
        string line;
        if (!getline(in, line) || line.empty()) return false;
        vector<string> f = split(line, '|');
        if (f.size() != 8) return false;
        try {
            Account a(stoll(f[0]), f[1], f[2], 0);
            a.balance = stod(f[3]);
            a.locked = (f[4] == "1");
            a.withdrawnToday = stod(f[5]);
            a.lastWithdrawDate = f[6];
            size_t count = stoul(f[7]);
            for (size_t i = 0; i < count; ++i) {
                if (!getline(in, line)) return false;
                vector<string> t = split(line, '|');
                if (t.size() != 4) return false;
                a.history.push_back({t[0], stod(t[1]), stod(t[2]), t[3]});
            }
            result = a;
            return true;
        } catch (...) {
            return false;
        }
    }
};

// ---------- Bank ----------

class Bank {
private:
    vector<Account> accounts;

    Account* find(long long number) {
        for (Account& a : accounts) if (a.getNumber() == number) return &a;
        return nullptr;
    }

    void saveAll() const {
        ofstream out(DATA_FILE.c_str());
        if (!out) {
            cout << "Warning: could not save data to " << DATA_FILE << ".\n";
            return;
        }
        for (const Account& a : accounts) a.save(out);
    }

    void loadAll() {
        ifstream in(DATA_FILE.c_str());
        if (!in) return;                       // first run: no file yet
        Account temp(0, "", "0000");
        while (Account::load(in, temp)) accounts.push_back(temp);
    }

public:
    Bank() { loadAll(); }

    void createAccount() {
        cout << "\n----- CREATE ACCOUNT -----\n";
        string name = readLine("Full name: ");
        name.erase(remove(name.begin(), name.end(), '|'), name.end());
        if (name.empty()) { cout << "Name cannot be empty.\n"; return; }

        long long number = readInteger("Choose an account number (digits only): ");
        if (number <= 0) { cout << "Invalid account number.\n"; return; }
        if (find(number)) { cout << "That account number is already taken.\n"; return; }

        string pin = readLine("Set a 4-digit PIN: ");
        if (!isValidPin(pin)) { cout << "PIN must be exactly 4 digits.\n"; return; }

        accounts.push_back(Account(number, name, pin));
        saveAll();
        cout << "Account created successfully. You can log in now.\n";
    }

    void login() {
        cout << "\n----- LOGIN -----\n";
        long long number = readInteger("Account number: ");
        Account* acc = (number > 0) ? find(number) : nullptr;
        if (!acc) { cout << "Account not found.\n"; return; }
        if (acc->isLocked()) {
            cout << "This account is locked after too many wrong PIN attempts.\n";
            return;
        }

        for (int attempt = 1; attempt <= MAX_ATTEMPTS; ++attempt) {
            string pin = readLine("Enter PIN: ");
            if (acc->checkPin(pin)) {
                cout << "\nLogin successful. Welcome, " << acc->getName() << "!\n";
                session(*acc);
                return;
            }
            int left = MAX_ATTEMPTS - attempt;
            if (left > 0) cout << "Wrong PIN. " << left << " attempt(s) left.\n";
        }
        acc->lock();
        saveAll();
        cout << "Too many wrong attempts. The account is now locked.\n";
    }

    void session(Account& acc) {
        while (true) {
            cout << "\n===== WELCOME TO PYBANK ATM =====\n"
                 << "1. Check Balance\n"
                 << "2. Deposit\n"
                 << "3. Withdraw\n"
                 << "4. Mini Statement\n"
                 << "5. Change PIN\n"
                 << "6. Transfer Money\n"
                 << "7. Exit\n";
            long long choice = readInteger("Choose an option: ");
            cout << fixed << setprecision(2);

            if (choice == 1) {
                cout << "Current balance: " << acc.getBalance() << "\n";
            } else if (choice == 2) {
                double amt = readAmount("Amount to deposit: ");
                if (amt < 0) { cout << "Enter a valid positive amount.\n"; continue; }
                string err = acc.deposit(amt);
                if (err.empty()) { saveAll(); cout << "Deposited " << amt << ". New balance: " << acc.getBalance() << "\n"; }
                else cout << err << "\n";
            } else if (choice == 3) {
                double amt = readAmount("Amount to withdraw: ");
                if (amt < 0) { cout << "Enter a valid positive amount.\n"; continue; }
                string err = acc.withdraw(amt);
                if (err.empty()) { saveAll(); cout << "Withdrew " << amt << ". New balance: " << acc.getBalance() << "\n"; }
                else cout << err << "\n";
            } else if (choice == 4) {
                acc.showMiniStatement();
            } else if (choice == 5) {
                string oldPin = readLine("Current PIN: ");
                if (!acc.checkPin(oldPin)) { cout << "Wrong PIN.\n"; continue; }
                string newPin = readLine("New 4-digit PIN: ");
                if (!isValidPin(newPin)) { cout << "PIN must be exactly 4 digits.\n"; continue; }
                acc.changePin(newPin);
                saveAll();
                cout << "PIN changed successfully.\n";
            } else if (choice == 6) {
                long long toNo = readInteger("Receiver account number: ");
                Account* to = (toNo > 0) ? find(toNo) : nullptr;
                if (!to) { cout << "Receiver account not found.\n"; continue; }
                if (to->getNumber() == acc.getNumber()) { cout << "You cannot transfer to your own account.\n"; continue; }
                double amt = readAmount("Amount to transfer: ");
                if (amt < 0) { cout << "Enter a valid positive amount.\n"; continue; }
                string err = acc.transferOut(amt, to->getNumber());
                if (err.empty()) {
                    to->transferIn(amt, acc.getNumber());
                    saveAll();
                    cout << "Transferred " << amt << " to " << to->getName()
                         << ". New balance: " << acc.getBalance() << "\n";
                } else cout << err << "\n";
            } else if (choice == 7) {
                cout << "Thank you for using PYBANK ATM. Goodbye!\n";
                return;
            } else {
                cout << "Invalid option. Choose a number from 1 to 7.\n";
            }
        }
    }

    void run() {
        while (true) {
            cout << "\n===== PYBANK ATM =====\n"
                 << "1. Create Account\n"
                 << "2. Login\n"
                 << "3. Exit\n";
            long long choice = readInteger("Choose an option: ");
            if (choice == 1) createAccount();
            else if (choice == 2) login();
            else if (choice == 3) { cout << "Goodbye!\n"; return; }
            else cout << "Invalid option. Choose 1, 2 or 3.\n";
        }
    }
};

int main() {
    Bank bank;
    bank.run();
    return 0;
}
