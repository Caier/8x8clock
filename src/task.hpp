#include <vector>
#include "function_objects.h"
#include <Arduino.h>

class Task {
    protected:
        FunctionObject<void()> callback;
        void virtual exec() {
            callback();
        }
        

    public: 
        unsigned long milis = millis(),
                      prevMilis = 0;
        Task(FunctionObject<void()> callback = [](){}, bool disabled = false);
        bool disabled = false;
        void render() {
            milis = millis();
            if(!disabled)
                exec();
            prevMilis = milis;
        }
};

std::vector<Task*> tasks;

Task::Task(FunctionObject<void()> callback, bool disabled) {
    tasks.push_back(this);
    this->callback = callback;
    this->disabled = disabled;
}

class Timer : public Task {
    unsigned long tim, interval;
    FunctionObject<void()> callback;

    void exec() override {
        tim += milis - prevMilis;
        if(tim >= interval) {
            callback();
            tim = 0;
        }
    }

    public:
    Timer(unsigned int interval, FunctionObject<void()> callback, bool atStart = false, bool disabled = false) : interval(interval), callback(callback), tim(atStart ? interval : 0) {
        this->disabled = disabled;
    };
};

void loop() {
    for(auto t : tasks)
        t->render();
}