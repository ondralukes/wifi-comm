#include <cstdint>
#include <Arduino.h>
#include "Keyboard.h"

ICACHE_RAM_ATTR const char** Keyboard::map = new const char*[16]{
    "~", "abc", "def", "~",
    "ghi", "jkl", "mno", "~",
    "pqrs", "tuv", "wxyz", "~",
    ".?!", " ", "~", "#"
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
    Serial.println(sizeof(Hit));
    isrData = new ISRAccessedData();
    isrData->rPins = new uint8_t[rows];
    memcpy(isrData->rPins, rowPins, rows * sizeof(uint8_t));
    isrData->cPins = new uint8_t [cols];
    memcpy(isrData->cPins, colPins, cols * sizeof(uint8_t));
    isrData->cols = cols;
    isrData->rows = rows;

    interruptArgs = new InterruptArg[rows];
    for(int i = 0;i<rows;i++){
        pinMode(rowPins[i], INPUT_PULLUP);
        interruptArgs[i].data = isrData;
        interruptArgs[i].row = i;
        attachInterruptArg(rowPins[i], reinterpret_cast<void (*)(void *)>(ISR), &interruptArgs[i], FALLING);
    }
    for(int i = 0;i<cols;i++){
        pinMode(colPins[i], OUTPUT);
        digitalWrite(colPins[i], LOW);
    }
    previousHit.pos = 0xff;
}

void ICACHE_RAM_ATTR  Keyboard::ISR(InterruptArg *arg) {
    auto* data = arg->data;
    unsigned long delta = millis() - data->lastPress;
    if(delta < 100) return;

    int r = arg->row;
    int c = -1;
    for(int i = 0; i< data->cols;i++){
        digitalWrite(data->cPins[i], HIGH);
        if(digitalRead(data->rPins[r]) == HIGH){
            c = i;
            digitalWrite(data->cPins[i], LOW);
            break;
        }
        digitalWrite(data->cPins[i], LOW);
    }
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
