
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>

#define NULL ((void *)0)
#define true 1
#define false 0

#define sleepMicroseconds(microseconds) _delay_us(microseconds)

#define tempPinInput() DDRB &= ~(1 << DDB3)

#define sckPinInput() DDRB &= ~(1 << DDB4)
#define sckPinRead() (PINB & (1 << PINB4))

#define dataPinOutput() DDRB |= (1 << DDB1)
#define dataPinHigh() PORTB |= (1 << PORTB1)
#define dataPinLow() PORTB &= ~(1 << PORTB1)

uint8_t currentData = 0;
uint8_t runDelay = 0;
uint8_t messageIndex = 0;
uint16_t messageTemperature = 0;

void initializePinModes() {
    tempPinInput();
    sckPinInput();
    dataPinOutput();
}

uint16_t readTemperature() {
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC)) {
        // Wait in a loop.
    }
    return ADC;
}

void invertData() {
    if (currentData) {
        dataPinLow();
        currentData = 0;
    } else {
        dataPinHigh();
        currentData = 1;
    }
}

// Our little rinky-dink serial protocol:
// > The main board will read satellite data on rising edge of SCK
// > Satellite sends temperature "messages" repeatedly
// > Each message consists of 11 "runs" of different lengths
// > Satellite data is inverted between each run
// > First run of each message is 3 cycles long
// > The remaining 10 runs encode the temperature as a 10-bit integer
// > A 2-cycle run represents bit 1, and a 1-cycle run represents bit 0

void handleSckEdge() {
    if (runDelay > 0) {
        runDelay -= 1;
        return;
    }
    invertData();
    if (messageIndex == 0) {
        runDelay = 2;
    } else {
        if (messageIndex == 1) {
            messageTemperature = readTemperature();
        }
        uint16_t mask = ((uint16_t)1 << (messageIndex - 1));
        runDelay = (messageTemperature & mask) ? 1 : 0;
    }
    messageIndex += 1;
    if (messageIndex > 10) {
        messageIndex = 0;
    }
}

int main(void) {
    
    initializePinModes();
    
    // Configure PB3 as analog input.
    ADMUX = (1 << MUX1) | (1 << MUX0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    
    uint8_t lastSck = sckPinRead();
    while (true) {
        uint8_t currentSck = sckPinRead();
        if (lastSck && !currentSck) {
            handleSckEdge();
        }
        lastSck = currentSck;
    }
    
    return 0;
}


