#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <chrono>
#include <list>

using namespace std;

void Thread1() {
    cout << "1" << endl;
}

void Thread2() {
    cout << "2" << endl;
}

void runTask_1_2_1() {
    cout << "--- Task 1.2.1 ---" << endl;

    thread thr1(Thread1);
    thread thr2(Thread2);
}

void runTask_1_2_2() {
    cout << "--- Task 1.2.2 ---" << endl;

    thread thr1(Thread1);
    thread thr2(Thread2);

    thr1.detach();
    thr2.detach();

    this_thread::sleep_for(chrono::milliseconds(100));
}

list<int> l;

void AddToList(int startValue) {
    for (int i = 0; i < 10; ++i) {
        int value = startValue + i;
        l.push_back(value);

        cout << "Added: " << value << endl;
    }
}

void ListContains(int searchValue) {
    for (int i = 1; i <= 10; ++i) {
        bool found = false;

        for (int value : l) {
            if (value == searchValue) {
                found = true;
                break;
            }
        }

        cout << "Attempt " << i << ": " << searchValue
            << (found ? " (in)" : " (not in)") << endl;
    }
}

void runTask_1_2_3() {
    cout << "\n--- Task 1.2.3 (Without mutex) ---" << endl;

    l.clear();

    int startValue = 10;
    int searchValue = 10;

    thread workerAdd(AddToList, startValue);
    thread workerSearch(ListContains, searchValue);

    workerAdd.join();
    workerSearch.join();
}

list<int> l_mutex;
mutex listMutex;

void AddToList_Mutex(int startValue) {
    listMutex.lock();

    for (int i = 0; i < 10; ++i) {
        int value = startValue + i;
        l_mutex.push_back(value);

        cout << "Added: " << value << endl;
    }

    listMutex.unlock();
}

void ListContains_Mutex(int searchValue) {
    for (int i = 1; i <= 10; ++i) {
        listMutex.lock();

        bool found = false;

        for (int value : l_mutex) {
            if (value == searchValue) {
                found = true;
                break;
            }
        }

        cout << "Attempt " << i << ": " << searchValue
            << (found ? " (in)" : " (not in)") << endl;

        listMutex.unlock();
    }
}

void runTask_1_2_4() {
    cout << "\n--- Task 1.2.4 (mutex) ---" << endl;

    l_mutex.clear();

    int startValue = 10;
    int searchValue = 10;

    thread workerAdd(AddToList_Mutex, startValue);
    thread workerSearch(ListContains_Mutex, searchValue);

    workerAdd.join();
    workerSearch.join();
}

list<int> l_guard;
mutex guardMutex;

void AddToList_Guard(int value) {
    lock_guard<mutex> guard(guardMutex);

    l_guard.push_back(value);

    cout << "Added: " << value << endl;
}

void ListContains_Guard(int searchValue) {
    lock_guard<mutex> guard(guardMutex);

    bool found = false;

    for (int value : l_guard) {
        if (value == searchValue) {
            found = true;
            break;
        }
    }

    cout << "Check: " << searchValue
        << (found ? " (in)" : " (not in)") << endl;
}

void runTask_1_2_5() {
    cout << "\n--- Task 1.2.5 (lock_guard) ---" << endl;

    l_guard.clear();

    int startValue = 100;
    int searchValue = 100;

    for (int i = 0; i < 10; ++i) {
        thread addThread(AddToList_Guard, startValue + i);
        thread searchThread(ListContains_Guard, searchValue);

        addThread.join();
        searchThread.join();
    }

    cout << "All threads completed." << endl;
}

class someData {
public:
    string name;
    string surname;
    string address;
    int age;

    someData() : name(""), surname(""), address(""), age(0) {}

    someData(string n, string s, string a, int ag)
        : name(n), surname(s), address(a), age(ag) {
    }
};

class exchangePerson {
public:
    someData data;
    mutex dataMutex;

    exchangePerson(string n, string s, string a, int ag)
        : data(n, s, a, ag) {
    }

    static void JohnDoe(exchangePerson& person) {
        lock_guard<mutex> guard(person.dataMutex);

        person.data = someData("John", "Doe", "Unknown", 120);
    }

    static void JacobSmith(exchangePerson& person) {
        lock_guard<mutex> guard(person.dataMutex);

        person.data = someData("Jacob", "Smith", "Known", 1);
    }

    static void Swap(exchangePerson& p1, exchangePerson& p2) {
        if (&p1 == &p2) {
            return;
        }

        lock(p1.dataMutex, p2.dataMutex);

        lock_guard<mutex> lock1(p1.dataMutex, adopt_lock);
        lock_guard<mutex> lock2(p2.dataMutex, adopt_lock);

        someData temp = p1.data;
        p1.data = p2.data;
        p2.data = temp;

        cout << "Swap completed using adopt_lock." << endl;
    }

    static void SwapUnique(exchangePerson& p1, exchangePerson& p2) {
        if (&p1 == &p2) {
            return;
        }

        unique_lock<mutex> lock1(p1.dataMutex, defer_lock);
        unique_lock<mutex> lock2(p2.dataMutex, defer_lock);

        lock(lock1, lock2);

        someData temp = p1.data;
        p1.data = p2.data;
        p2.data = temp;

        cout << "Swap completed using unique_lock." << endl;
    }

    void display() {
        lock_guard<mutex> guard(dataMutex);

        cout << "Name: " << data.name
            << ", Surname: " << data.surname
            << ", Address: " << data.address
            << ", Age: " << data.age << endl;
    }
};

void runTask_1_2_6() {
    cout << "\n--- Task 1.2.6 (lock_guard + adopt_lock) ---" << endl;

    exchangePerson person1("", "", "", 0);
    exchangePerson person2("", "", "", 0);

    thread t1([&person1]() {
        exchangePerson::JohnDoe(person1);
        });

    thread t2([&person2]() {
        exchangePerson::JacobSmith(person2);
        });

    t1.join();
    t2.join();

    cout << "Before Swap:" << endl;
    person1.display();
    person2.display();

    thread tSwap([&person1, &person2]() {
        exchangePerson::Swap(person1, person2);
        });

    tSwap.join();

    cout << "After Swap:" << endl;
    person1.display();
    person2.display();
}

void runTask_1_2_7() {
    cout << "\n--- Task 1.2.7 (unique_lock + defer_lock) ---" << endl;

    exchangePerson person1("John", "Doe", "Unknown", 120);
    exchangePerson person2("Jacob", "Smith", "Known", 1);

    cout << "Before Swap:" << endl;
    person1.display();
    person2.display();

    thread tSwap([&person1, &person2]() {
        exchangePerson::SwapUnique(person1, person2);
        });

    tSwap.join();

    cout << "After Swap:" << endl;
    person1.display();
    person2.display();
}

int main() {
    //runTask_1_2_1();
    runTask_1_2_2();
    runTask_1_2_3();
    runTask_1_2_4();
    runTask_1_2_5();
    runTask_1_2_6();
    runTask_1_2_7();

    cout << "\nAll tasks completed." << endl;

    return 0;
}