#ifndef WIFI_COMM_KEYBOARD_H
#define WIFI_COMM_KEYBOARD_H

class Keyboard {
public:
    Keyboard(const uint8_t *rowPins, int rows, const uint8_t *colPins, int cols);

    int Peek();

    int Read();

    void DiscardAll();

private:
    template<class T, int S>
    class Queue {
    public:
        T *Dequeue();

        T *ItemToEnqueue();

        void Enqueue();

        void Enqueue(T item);

        void Clear();

    private:
        T items[S];
        int enqueue = 0;
        int dequeue = 0;
    };

    struct Hit {
        uint8_t pos;
        unsigned long time;
    };
    struct ISRAccessedData {
        Queue<Hit, 256> hits;
        volatile unsigned long lastPress = 0;
        uint8_t *rPins;
        int rows;
        uint8_t *cPins;
        int cols;
    };
    struct InterruptArg {
        uint8_t row;
        ISRAccessedData *data;
    };

    static void ISR(InterruptArg *arg);

    ISRAccessedData *isrData;
    InterruptArg *interruptArgs;
    Hit previousHit;
    int currentOption;

    static const char **map;
    Queue<char, 32> chars;

    void Update();
};


#endif //WIFI_COMM_KEYBOARD_H
