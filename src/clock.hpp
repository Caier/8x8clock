#include "vector"
#include "algorithm"
#include "ClampedInt.hpp"

extern const int col[8], row[8];
extern volatile uint64_t selfSeconds;

class Clock {
    std::vector<bool> scrbuf[8];
    String scrollWriteBuffer = "";
    const char *months[12] = { "Stycznia", "Lutego", "Marca", "Kwietnia", "Maja", "Czerwca", "Lipca", "Sierpnia", "Wrze¶nia", "Pa¼dziernika", "Listopada", "Grudnia" };
    const char *days[7] = { "Niedziela", "Poniedzia³ek", "Wtorek", "¦roda", "Czwartek", "Pi±tek", "Sobota" };

    //for time initialization
    enum steps { S_year, S_month, S_day1, S_day2, S_hour1, S_hour2, S_minute1, S_minute2 } currentStep = S_year;
    enum status { start, waitingForInput, scrollingText, done } currentStatus = start;
    ClampedInt<uint8_t> timeSetNumberSwap { 20, 20, 30 };
    uint16_t incrementTime = 0;
    bool buttonPressed = false;
    uint8_t constructedTime[8];
    
    Task drawScreen{[this](){
        for(uint8_t y = 0; y < 8; y++) {
            digitalWrite(row[y], 1);
            for(uint8_t x = 0; x < 8; x++)
              digitalWrite(col[x], !scrbuf[y][x]);
            delayMicroseconds(100);
            for(uint8_t x = 0; x < 8; x++)
              digitalWrite(col[x], 1);
            digitalWrite(row[y], 0);
        }
    }, true};

    Task initTime{[this](){
        if(currentStatus == start) {
            scrollWriteBuffer += ((String)"Ustaw " + (currentStep == S_year ? "rok" : (currentStep == S_month ? "miesi±c" : (currentStep == S_day1 ? "dzieñ" : (currentStep == S_hour1 ? "godzinê" : "minutê")))) + "      ");
            scrollTask.disabled = false;
            currentStatus = scrollingText;
        }

        if(currentStatus == scrollingText) {
            if(scrollWriteBuffer.length() == 0 && isScreenEmpty()) {
                currentStatus = waitingForInput;
                scrollTask.disabled = true;
            }
        }

        if(currentStatus == waitingForInput) {
            for(uint8_t y = 0; y < 6; y++) {
                uint8_t patt = getMinimap(timeSetNumberSwap.cur % 10, y) << 1;
                for(uint8_t x = 7; x > 4; x--)
                    scrbuf[y+1][x] = (patt >>= 1) & 1;
                patt = getMinimap(timeSetNumberSwap.cur / 10, y) << 1;
                for(uint8_t x = 3; x > 0; x--)
                    scrbuf[y+1][x] = (patt >>= 1) & 1;
            }

            if(buttonPressed) {
                constructedTime[currentStep] = timeSetNumberSwap.cur;
                
                if(currentStep != S_minute2) {
                    if(currentStep != S_day1 && currentStep != S_hour1 && currentStep != S_minute1)
                        currentStatus = start;
                    currentStep = static_cast<steps>(static_cast<int>(currentStep) + 1);
                    buttonPressed = false;
                    #define tis(c, mn, mx, s) timeSetNumberSwap = ClampedInt<uint8_t>(c, mn, mx, s); break;
                    switch(currentStep) {
                        case S_month: tis(1, 1, 12, 1);
                        case S_day1: tis(0, 0, 30, 10);
                        case S_day2: tis(timeSetNumberSwap.cur, timeSetNumberSwap.cur, std::min(timeSetNumberSwap.cur + 9, 31), 1);
                        case S_hour1: tis(0, 0, 20, 10);
                        case S_hour2: tis(timeSetNumberSwap.cur, timeSetNumberSwap.cur, std::min(timeSetNumberSwap.cur + 9, 23), 1);
                        case S_minute1: tis(0, 0, 50, 10);
                        case S_minute2: tis(timeSetNumberSwap.cur, timeSetNumberSwap.cur, timeSetNumberSwap.cur + 9, 1);
                    }
                } else {
                    initTime.disabled = true;
                    showTime.disabled = false;
                    scrollTask.disabled = false;
                    currentStatus = done;
                    setTime(constructedTime[S_hour2], constructedTime[S_minute2], 0, constructedTime[S_day2], constructedTime[S_month], constructedTime[S_year] + 2000);
                    selfSeconds = now();
                    setSyncProvider([]() -> time_t { return selfSeconds; });
                    setSyncInterval(5);
                }
            }

            incrementTime += initTime.milis - initTime.prevMilis;
            if(incrementTime >= 700) {
                timeSetNumberSwap++;
                incrementTime = 0;
            }
        }
    }};

    bool isScreenEmpty() {
        for(uint8_t i = 0; i < 8; i++)
            for(auto v : scrbuf[i])
                if(v == 1)
                    return false;
        return true;
    }

    Timer scrollTask{70, [this](){
        for(int i = 0; i < 8; i++) {
            scrbuf[i].erase(scrbuf[i].begin());
            if(scrbuf[i].size() < 8)
                scrbuf[i].push_back(0);
        }
    }, true, true};

    Task appendTextToBuf{[this](){
        if(scrbuf[0].size() > 16 || scrollWriteBuffer.length() == 0)
            return;

        String toAp = readFromPmem(scrollWriteBuffer[0] - 32);
        drawScreen.render(); //periods without screen being drawn cause noticeable blinking sooo... well you can't overdraw the screen
        scrollWriteBuffer.remove(0, 1);
        drawScreen.render();
        int len = toAp.indexOf(',');
        drawScreen.render();
        for(int i = 0; i < 8; i++) {
            for(char c : toAp.substring(0, len))
                scrbuf[i].push_back(c == '1' ? 1 : 0);
            scrbuf[i].push_back(0);
            drawScreen.render();
            toAp = toAp.substring(len + 1);
            drawScreen.render();
        }
    }};

    Task showTime{[this](){
        if(scrollWriteBuffer.length() == 0 && isScreenEmpty())
            scrollWriteBuffer += (String)hour() + ":" + (minute() < 10 ? "0" + (String)minute() : (String)minute()) + "  " + days[weekday()-1] + "  " + day() + ". " + months[month()-1];
    }, true};

    public:
    Clock() {
        for(uint8_t i = 0; i < 8; i++)
            scrbuf[i] = {0,0,0,0,0,0,0,0};
        drawScreen.disabled = false;
    }

    void press() {
        if(currentStatus == waitingForInput)
            buttonPressed = true;
    }
};