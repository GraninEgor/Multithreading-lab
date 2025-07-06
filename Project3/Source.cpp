#include <iostream>
#include <queue>
#include <thread>
#include <random>
#include <chrono>
#include <mutex>
#include <stdlib.h>
#include <csignal>
#include <atomic>

using namespace std;

atomic<bool> stopFlag(false);

mutex mtx;

void signalHandler(int signum) {
    stopFlag.store(true);
}

int getRandomNumber(int n) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, n);
    return distrib(gen);
}

void clearLines(int count) {
    for (int i = 0; i < count; i++) {
        cout << "\033[A" << "\033[K";
    }
}

struct Request {
    int priority;
    int status;
    int group;
};

struct CompareTask {
    bool operator()(const Request& a, const Request& b) {
        return a.priority < b.priority;
    }
};

struct Generator {
    int storageCapacity;
    int amountOfGroups;
    int amountOfDevices;
    priority_queue<Request, std::vector<Request>, CompareTask>* queue;
    bool isActive = true;

    Request generateRequest() {
        Request request;
        request.group = getRandomNumber(amountOfGroups);
        request.priority = getRandomNumber(3);
        request.status = 0;
        return request;
    }

    void generate() {
        while (!stopFlag.load()) {
            mtx.lock();
            if (queue->size() < storageCapacity) {
                isActive = true;
            }
            else {
                isActive = false;
            }
            if (isActive) {
                queue->push(generateRequest());
            }
            mtx.unlock();
            this_thread::sleep_for(chrono::milliseconds(600));
        }
    }

    Generator(int p_storageCapacity, int p_amountOfGroups, int p_amountOfDevices, priority_queue<Request, vector<Request>, CompareTask>* p_queue)
    {
        storageCapacity = p_storageCapacity;
        amountOfGroups = p_amountOfGroups;
        amountOfDevices = p_amountOfDevices;
        queue = p_queue;
    }
};

struct Device {
    int amountOfGroups;
    int amountOfDevices;
    bool isBusy = false;
    int group;
    int currentTime = 0;

    void work(int i) {
        isBusy = true;
        int time = getRandomNumber(10);
        currentTime = time;
        for (int i = 0; i < time; i++) {
            currentTime--;
            this_thread::sleep_for(chrono::seconds(1));
        }
        isBusy = false;
    }
};

struct System {
    int amountOfGroups;
    int amountOfDevices;
    vector<Device*> devices;
    priority_queue<Request, vector<Request>, CompareTask>* queue;

    void createDevices() {
        int groupId = 1;
        for (int i = 0; i < amountOfDevices; i++) {
            if (groupId == amountOfGroups + 1) {
                groupId = 1;
            }
            Device* device = new Device;
            device->group = groupId;
            groupId++;
            devices.push_back(device);
        }
    }
    Request request;

    void printStatus() {
        while (!stopFlag.load()) {
            for (int i = 0; i < devices.size(); i++) {
                cout << "Device " << i + 1 << " group: " << devices[i]->group << flush;
                if (devices[i]->isBusy) {
                    cout << " Busy " << devices[i]->currentTime << flush;
                }
                else {
                    cout << " Not busy " << devices[i]->currentTime << flush;
                }
                cout << endl;
            }
            cout << "amountOfRequests: " << queue->size() << endl;
            this_thread::sleep_for(chrono::milliseconds(1));
            clearLines(devices.size() + 1);
        }
    }

    void start() {
        while (!stopFlag.load()) {
            if (!queue->empty()) {
                mtx.lock();
                request = queue->top();
                for (int i = 0; i < devices.size(); i++) {
                    if (request.group == devices[i]->group && devices[i]->isBusy == false) {
                        new thread([&]() {devices[i]->work(i); });
                        queue->pop();
                        break;
                    }
                }
                mtx.unlock();
                this_thread::sleep_for(chrono::milliseconds(300));
            }
        }
    }

    System(int p_amountOfGroups, int p_amountOfDevices, priority_queue<Request, std::vector<Request>, CompareTask>* p_queue) {
        amountOfGroups = p_amountOfGroups;
        amountOfDevices = p_amountOfDevices;
        queue = p_queue;
        createDevices();
    }
};

int main() {
    signal(SIGINT, signalHandler);

    int storageCapacity, amountOfGroups, amountOfDevices;
    cin >> storageCapacity >> amountOfGroups >> amountOfDevices;
    priority_queue<Request, std::vector<Request>, CompareTask>* queue = new priority_queue<Request, vector<Request>, CompareTask>;
    Generator generator(storageCapacity, amountOfGroups, amountOfDevices, queue);
    System system(amountOfGroups, amountOfDevices, queue);
    thread gent([&]() {generator.generate(); });
    thread startt([&]() {system.start(); });
    thread printt([&]() {system.printStatus(); });

    gent.join();
    startt.join();
    printt.join();

    cout << "EXIT";

    return 0;
}