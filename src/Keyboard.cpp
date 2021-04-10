#include <cstdint>
#include <Arduino.h>
#include "Keyboard.h"

ICACHE_RAM_ATTR const char** Keyboard::map = new const char*[16]{
    "1", "abc2", "def3", "^",
    "ghi4", "jkl5", "mno6", "<",
    "pqrs7", "tuv8", "wxyz9", ";",
    ".?!", " 0", "~", "#"
};
template<class T, int S>
T *Keyboard::Queue<T, S>::Dequeue() {
    if(dequeue == enqueue) return nullptr;
    auto c = &items[dequeue++];
    dequeue = dequeue % S;
    return c;
}

template<class T, int S>
T *Keyboard::Queue<T, S>::ItemToEnqueue() {
    return &items[enqueue];
}

template<class T, int S>
void Keyboard::Queue<T, S>::Enqueue() {
    enqueue++;
    enqueue = enqueue % S;
}

template<class T, int S>
void Keyboard::Queue<T, S>::Enqueue(T item) {
    items[enqueue++] = item;
    enqueue = enqueue % S;
}

Keyboard::Keyboard(const uint8_t *rowPins, int rows, const uint8_t* colPins, int cols){
    isrData = new ISRAccessedData();
    isrData->rPins = new uint8_t[rows];
    memcpy(isrData->rPins, rowPins, rows * sizeof(uint8_t));
    isrData->cPins = new uint8_t [cols];
    memcpy(isrData->cPins, colPins, cols * sizeof(uint8_t));
    isrData->cols = cols;
    isrData->rows = rows;

    interruptArgs = new InterruptArg[rows];
    for(int i = 0;i<rows;i++){
        pinMode(rowPins[i], INPUT);
        interruptArgs[i].data = isrData;
        interruptArgs[i].row = i;
        attachInterruptArg(rowPins[i], reinterpret_cast<void (*)(void *)>(ISR), &interruptArgs[i], RISING);
    }
    for(int i = 0;i<cols;i++){
        if(colPins[i] == 255) continue;
        pinMode(colPins[i], OUTPUT);
        digitalWrite(colPins[i], HIGH);
    }
    previousHit.pos = 0xff;
}

void ICACHE_RAM_ATTR  Keyboard::ISR(InterruptArg *arg) {
    auto* data = arg->data;
    unsigned long delta = millis() - data->lastPress;
    if(delta < 100) return;

    int r = arg->row;
    int c = -1;
    int fallback = -1;
    for(int i = 0; i< data->cols;i++){
        if(data->cPins[i] == 255){
            fallback = i;
            continue;
        }
        digitalWrite(data->cPins[i], LOW);
        if(digitalRead(data->rPins[r]) == LOW){
            c = i;
            digitalWrite(data->cPins[i], HIGH);
            break;
        }
        digitalWrite(data->cPins[i], HIGH);
    }
    if(c == -1) c = fallback;
    if(c == -1) return;
    auto * ev = data->hits.ItemToEnqueue();
    ev->time = millis();
    ev->pos = r*data->cols+c;
    data->hits.Enqueue();
    data->lastPress = millis();
}

void Keyboard::Update() {
    Hit * hit;
    while((hit = isrData->hits.Dequeue()) != nullptr){
        if (hit->pos == previousHit.pos && hit->time - previousHit.time <= 2000) {
            currentOption++;
            if(map[hit->pos][currentOption] == '\0') currentOption = 0;
            previousHit.time = hit->time;
        } else {
            if(previousHit.pos != 0xff)
                chars.Enqueue(map[previousHit.pos][currentOption]);
            if(map[hit->pos][1] == '\0'){
                chars.Enqueue(map[hit->pos][0]);
                previousHit.pos = 0xff;
            } else {
                previousHit = *hit;
            }
            currentOption = 0;
        }
    }

}

int Keyboard::Peek() {
    Update();
    if(previousHit.pos == 0xff) return -1;
    return map[previousHit.pos][currentOption];
}

int Keyboard::Read() {
    Update();
    char* ptr = chars.Dequeue();
    if(ptr == nullptr) return -1;
    return *ptr;
}
