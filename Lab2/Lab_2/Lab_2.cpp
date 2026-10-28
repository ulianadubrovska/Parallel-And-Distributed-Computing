#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <chrono>
#include <queue>
#include <deque>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <cstdint>
#include <limits>
#include <utility>
#include <exception>
#include <windows.h>
using namespace std;

bool isPrime(uint64_t n) {
    if (n < 2) return false;
    for (uint64_t d = 2; d <= n / d; ++d)
        if (n % d == 0) return false;
    return true;
}

uint64_t nthPrime(unsigned n) {
    uint64_t number = 1;
    unsigned found = 0;
    while (found < n) {
        ++number;
        if (isPrime(number)) ++found;
    }
    return number;
}

bool parseInt(const string& line, int& value) {
    istringstream in(line);
    char extra;
    return bool(in >> value) && !(in >> extra);
}

bool parseN(const string& line, unsigned& value) {
    if (line.find('-') != string::npos) return false;
    istringstream in(line);
    char extra;
    return bool(in >> value) && value > 0 && !(in >> extra);
}

unsigned readN() {
    string line;
    unsigned n;
    while (true) {
        cout << "Введіть додатний номер простого числа n: " << flush;
        if (!getline(cin, line)) return 0;
        if (parseN(line, n)) return n;
        cout << "Помилка: введіть ціле число більше нуля.\n";
    }
}

namespace Task1 {
    queue<int> numbers;
    mutex m;
    bool finished = false;

    void DataPreparation() {
        cout << "Вводьте цілі числа по одному в рядку. Для завершення введіть stop:\n";
        string line;
        while (getline(cin, line) && line != "stop") {
            int x;
            if (!parseInt(line, x)) {
                cout << "Помилка: потрібно ввести ціле число.\n";
                continue;
            }
            lock_guard<mutex> lock(m);
            numbers.push(x);
        }
        lock_guard<mutex> lock(m);
        finished = true;
    }

    void DataProcessing() {
        unique_lock<mutex> lock(m);
        while (!finished) {
            lock.unlock();
            this_thread::sleep_for(chrono::milliseconds(100));
            lock.lock();
        }
        cout << "Прості числа з введених: ";
        bool any = false;
        while (!numbers.empty()) {
            int x = numbers.front();
            numbers.pop();
            if (x >= 2 && isPrime(static_cast<uint64_t>(x))) {
                cout << x << " ";
                any = true;
            }
        }
        if (!any) cout << "немає";
        cout << '\n';
    }

    void run() {
        thread producer(DataPreparation);
        producer.detach();
        thread consumer(DataProcessing);
        consumer.join();
    }
}

namespace Task2 {
    mutex m;
    condition_variable cv;
    int i = 0;

    void Waits(int id) {
        unique_lock<mutex> lock(m);
        cout << "Потік " << id << " очікує на повідомлення.\n";
        cv.wait(lock, [] { return i == 1; });
        cout << "Потік " << id << " завершив очікування.\n";
    }

    void Awake() {
        this_thread::sleep_for(chrono::milliseconds(100));
        {
            lock_guard<mutex> lock(m);
            cout << "Перше notify_all(): умова ще не виконана (i = 0).\n";
        }
        cv.notify_all();
        this_thread::sleep_for(chrono::milliseconds(100));
        {
            lock_guard<mutex> lock(m);
            i = 1;
            cout << "Друге notify_all(): умова виконана (i = 1).\n";
        }
        cv.notify_all();
    }

    void run() {
        i = 0;
        thread a(Waits, 1), b(Waits, 2), c(Waits, 3);
        thread awake(Awake);
        awake.join();
        a.join(); b.join(); c.join();
    }
}

// 1.2.3: notify_one() пробуджує один потік
namespace Task3 {
    mutex m;
    condition_variable cv;
    int i = 0;

    void Waits(int id) {
        unique_lock<mutex> lock(m);
        cout << "Потік " << id << " очікує.\n";
        cv.wait(lock, [] { return i == 1; });
        cout << "Повідомлення з потоку " << id << ".\n";
    }

    void Notify() {
        this_thread::sleep_for(chrono::milliseconds(100));
        {
            lock_guard<mutex> lock(m);
            i = 1;
            cout << "Виклик notify_one(): повідомляємо один потік.\n";
        }
        cv.notify_one();
    }

    void run() {
        i = 0;
        thread a(Waits, 1), b(Waits, 2), c(Waits, 3);
        thread notifier(Notify);
        notifier.join();
        this_thread::sleep_for(chrono::milliseconds(100));
        cout << "Завершуємо очікування решти потоків через notify_all().\n";
        cv.notify_all();
        a.join(); b.join(); c.join();
    }
}

namespace Task4 {
    queue<int> numbers;
    mutex m;
    condition_variable cv;
    bool finished = false;

    void DataPreparation() {
        cout << "Вводьте цілі числа по одному в рядку. Для завершення введіть stop:\n";
        string line;
        while (getline(cin, line) && line != "stop") {
            int x;
            if (!parseInt(line, x)) {
                cout << "Помилка: потрібно ввести ціле число.\n";
                continue;
            }
            {
                lock_guard<mutex> lock(m);
                numbers.push(x);
            }
            cv.notify_one();
        }
        {
            lock_guard<mutex> lock(m);
            finished = true;
        }
        cv.notify_one();
    }

    void DataProcessing() {
        while (true) {
            unique_lock<mutex> lock(m);
            cv.wait(lock, [] { return !numbers.empty() || finished; });
            if (numbers.empty() && finished) break;
            int x = numbers.front();
            numbers.pop();
            lock.unlock();
            if (x >= 2 && isPrime(static_cast<uint64_t>(x)))
                cout << "Знайдено просте число: " << x << '\n';
        }
        cout << "Усі введені числа перевірено.\n";
    }

    void run() {
        thread producer(DataPreparation);
        producer.detach();
        thread consumer(DataProcessing);
        consumer.join();
    }
}

void task5() {
    unsigned n = readN();
    if (n == 0) return;

    cout << "Оберіть режим (1 — deferred, 2 — async): " << flush;
    string line;
    if (!getline(cin, line)) return;
    int mode;
    if (!parseInt(line, mode) || (mode != 1 && mode != 2)) {
        cout << "Невірний режим.\n";
        return;
    }

    auto start = chrono::steady_clock::now();
    future<uint64_t> result = async(
        mode == 1 ? launch::deferred : launch::async, nthPrime, n
    );

    cout << "Оберіть функцію: 1 — корінь, 2 — синус, 3 — натуральний логарифм: " << flush;
    if (!getline(cin, line)) return;
    int choice;
    if (parseInt(line, choice)) {
        double x = static_cast<double>(n);
        if (choice == 1) cout << "Квадратний корінь з n = " << sqrt(x) << '\n';
        else if (choice == 2) cout << "Синус n = " << sin(x) << '\n';
        else if (choice == 3) cout << "Натуральний логарифм n = " << log(x) << '\n';
        else cout << "Невідома функція.\n";
    }
    else {
        cout << "Невірний вибір функції.\n";
    }

    auto before = chrono::steady_clock::now();
    uint64_t prime = result.get();
    auto end = chrono::steady_clock::now();

    cout << "Просте число за номером " << n << " = " << prime << '\n';
    cout << "Час очікування результату: "
        << chrono::duration<double>(end - before).count() << " с\n";
    cout << "Загальний час після запуску обчислення: "
        << chrono::duration<double>(end - start).count() << " с\n";
    cout << "Для порівняння запустіть це завдання ще раз з тим самим n та іншим режимом.\n";
}

void task6() {
    deque<packaged_task<uint64_t()>> tasks;
    deque<unsigned> numbers;
    vector<pair<unsigned, future<uint64_t>>> results;
    mutex m;
    condition_variable cv;
    bool finished = false;

    thread worker([&] {
        while (true) {
            packaged_task<uint64_t()> task;
            unsigned n;
            {
                unique_lock<mutex> lock(m);
                cv.wait(lock, [&] { return finished || !tasks.empty(); });
                if (finished && tasks.empty()) break;
                task = move(tasks.front());
                tasks.pop_front();
                n = numbers.front();
                numbers.pop_front();
            }
            (void)n;
            task();
        }
        });

    cout << "Вводьте номери простих чисел n (> 0), по одному в рядку. Для завершення — stop:\n";
    string line;
    while (getline(cin, line) && line != "stop") {
        unsigned n;
        if (!parseN(line, n)) {
            cout << "Помилка: введіть додатне ціле число.\n";
            continue;
        }
        packaged_task<uint64_t()> task([n] { return nthPrime(n); });
        future<uint64_t> f = task.get_future();
        {
            lock_guard<mutex> lock(m);
            tasks.push_back(move(task));
            numbers.push_back(n);
        }
        results.emplace_back(n, move(f));
        cv.notify_one();
    }
    {
        lock_guard<mutex> lock(m);
        finished = true;
    }
    cv.notify_one();
    worker.join();
    cout << "Результати обчислень:\n";
    for (auto& result : results)
        cout << "Просте число №" << result.first << " = " << result.second.get() << '\n';
}

void task7() {
    unsigned n = readN();
    if (n == 0) return;

    if (n > 429496729U) {
        cout << "Помилка: надто велике n для множення на 10.\n";
        return;
    }

    promise<uint64_t> p1, p2;
    promise<bool> signal;
    future<uint64_t> f1 = p1.get_future();
    future<uint64_t> f2 = p2.get_future();
    future<bool> ready = signal.get_future();
    mutex outputMutex;

    thread first([n](promise<uint64_t> a, promise<bool> s, promise<uint64_t> b) {
        try {
            a.set_value(nthPrime(n));
            s.set_value(true);
            b.set_value(nthPrime(n * 10));
        }
        catch (...) {
            auto error = current_exception();
            try { a.set_exception(error); }
            catch (...) {}
            try { s.set_exception(error); }
            catch (...) {}
            try { b.set_exception(error); }
            catch (...) {}
        }
        }, move(p1), move(signal), move(p2));

    thread second([n, &outputMutex](future<bool> start) {
        try {
            if (start.get()) {
                this_thread::sleep_for(chrono::seconds(2));
                lock_guard<mutex> lock(outputMutex);
                cout << "Другий потік: квадратний корінь з " << n
                    << " = " << sqrt(static_cast<double>(n)) << '\n';
            }
        }
        catch (const exception& e) {
            lock_guard<mutex> lock(outputMutex);
            cout << "Помилка другого потоку: " << e.what() << '\n';
        }
        }, move(ready));

    try {
        uint64_t firstPrime = f1.get();
        uint64_t tenthPrime = f2.get();
        lock_guard<mutex> lock(outputMutex);
        cout << "Просте число №" << n << " = " << firstPrime << '\n';
        cout << "Просте число №" << n * 10 << " = " << tenthPrime << '\n';
    }
    catch (const exception& e) {
        lock_guard<mutex> lock(outputMutex);
        cout << "Помилка обчислення: " << e.what() << '\n';
    }
    first.join();
    second.join();
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    string line;
    while (true) {
        cout << "\n========== ЛАБОРАТОРНА РОБОТА №2 ==========\n"
            << "1. Завдання 1.2.1 — Синхронізація прапорцем\n"
            << "2. Завдання 1.2.2 — Повідомлення всіх потоків\n"
            << "3. Завдання 1.2.3 — Повідомлення одного потоку\n"
            << "4. Завдання 1.2.4 — Умовна змінна\n"
            << "5. Завдання 1.2.5 — Асинхронні обчислення\n"
            << "6. Завдання 1.2.6 — Черга обчислювальних завдань\n"
            << "7. Завдання 1.2.7 — Передача результатів між потоками\n"
            << "0. Вихід\n"
            << "Оберіть завдання: " << flush;
        if (!getline(cin, line)) break;
        int choice;
        if (!parseInt(line, choice)) {
            cout << "Помилка: введіть номер пункту меню.\n";
            continue;
        }
        try {
            switch (choice) {
            case 1: Task1::run(); break;
            case 2: Task2::run(); break;
            case 3: Task3::run(); break;
            case 4: Task4::run(); break;
            case 5: task5(); break;
            case 6: task6(); break;
            case 7: task7(); break;
            case 0: cout << "Завершення програми.\n"; return 0;
            default: cout << "Немає такого пункту меню.\n";
            }
        }
        catch (const exception& e) {
            cout << "Помилка: " << e.what() << '\n';
        }
    }
}
