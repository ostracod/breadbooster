
#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#define NULL ((void *)0)
#define true 1
#define false 0

#define sleepMilliseconds(milliseconds) _delay_ms(milliseconds)
#define sleepMicroseconds(microseconds) _delay_us(microseconds)

#define lcdResetPinOutput() DDRB |= (1 << DDB6)
#define lcdResetPinHigh() PORTB |= (1 << PORTB6)
#define lcdResetPinLow() PORTB &= ~(1 << PORTB6)

#define lcdCsPinOutput() DDRB |= 1 << DDB7
#define lcdCsPinHigh() PORTB |= 1 << PORTB7
#define lcdCsPinLow() PORTB &= ~(1 << PORTB7)

#define lcdModePinOutput() DDRB |= (1 << DDB4)
#define lcdModePinHigh() PORTB |= (1 << PORTB4)
#define lcdModePinLow() PORTB &= ~(1 << PORTB4)

#define lcdDataPinOutput() DDRB |= (1 << DDB3)
#define lcdDataPinHigh() PORTB |= (1 << PORTB3)
#define lcdDataPinLow() PORTB &= ~(1 << PORTB3)

#define lcdSckPinOutput() DDRB |= (1 << DDB5)
#define lcdSckPinHigh() PORTB |= (1 << PORTB5)
#define lcdSckPinLow() PORTB &= ~(1 << PORTB5)

#define satelliteSckPinOutput() DDRD |= (1 << DDD3)
#define satelliteSckPinHigh() PORTD |= (1 << PORTD3)
#define satelliteSckPinLow() PORTD &= ~(1 << PORTD3)

#define satelliteDataPinInput() DDRD &= ~(1 << DDD4)
#define satelliteDataPinRead() (PIND & (1 << PIND4))

#define setLcdCursorPos(posX, posY) sendLcdCommand(0x80 | (posX + posY * 0x40))
#define clearLcd() sendLcdCommand(0x01)

const int8_t lcdInitCommands[] PROGMEM = {
    0x39, 0x1C, 0x52, 0x69, 0x74, 0x38, 0x0C, 0x01, 0x06
};

uint8_t lastSatelliteData = 0;

void initializePinModes() {
    
    lcdResetPinHigh();
    lcdCsPinHigh();
    lcdSckPinHigh();
    lcdResetPinOutput();
    lcdCsPinOutput();
    lcdModePinOutput();
    lcdDataPinOutput();
    lcdSckPinOutput();
    
    satelliteSckPinHigh();
    satelliteSckPinOutput();
    satelliteDataPinInput();
}

void sendLcdInt8(int8_t data) {
    for (uint8_t count = 0; count < 8; count++) {
        lcdSckPinLow();
        uint8_t mask = (uint8_t)1 << ((uint8_t)7 - count);
        if (data & mask) {
            lcdDataPinHigh();
        } else {
            lcdDataPinLow();
        }
        sleepMicroseconds(10);
        lcdSckPinHigh();
        sleepMicroseconds(10);
    }
}

void sendLcdCommand(int8_t command) {
    lcdModePinLow();
    sendLcdInt8(command);
    sleepMilliseconds(5);
}

void sendLcdCharacter(int8_t character) {
    lcdModePinHigh();
    sendLcdInt8(character);
    sleepMilliseconds(2);
}

void initializeLcd() {
    
    sleepMilliseconds(20);
    lcdResetPinLow();
    sleepMilliseconds(20);
    lcdResetPinHigh();
    sleepMilliseconds(20);
    lcdCsPinLow();
    
    for (int8_t index = 0; index < sizeof(lcdInitCommands); index++) {
        int8_t command = pgm_read_byte(lcdInitCommands + index);
        sendLcdCommand(command);
    }
}

uint8_t readSatelliteRun() {
    uint8_t runLength = 1;
    while (runLength < 4) {
        satelliteSckPinLow();
        sleepMicroseconds(500);
        satelliteSckPinHigh();
        uint8_t currentData = satelliteDataPinRead();
        sleepMicroseconds(500);
        if (currentData != lastSatelliteData) {
            lastSatelliteData = currentData;
            break;
        }
        runLength += 1;
    }
    return runLength;
}

uint16_t readTemperature() {
    // Wait for run length 3, which occurs at the start of a message.
    uint8_t count = 0;
    while (true) {
        uint8_t runLength = readSatelliteRun();
        if (runLength == 3) {
            break;
        }
        if (runLength > 3) {
            // Run length above 3 is not possible under normal circumstances.
            return 0;
        }
        count += 1;
        if (count > 20) {
            // We failed to find the start of a message.
            return 0;
        }
    }
    // Allow ADC to run on satellite.
    sleepMilliseconds(4);
    uint16_t temperature = 0;
    for (uint8_t offset = 0; offset < 10; offset++) {
        uint8_t runLength = readSatelliteRun();
        if (runLength == 2) {
            temperature |= ((uint16_t)1 << offset);
        }
        if (runLength > 2) {
            // Run lengths in payload must be 1 or 2.
            return 0;
        }
    }
    return temperature;
}

int main(void) {
    
    initializePinModes();
    initializeLcd();
    
    clearLcd();
    while (true) {
        uint16_t temperature = readTemperature();
        uint8_t text[10];
        itoa(temperature, text, 10);
        setLcdCursorPos(0, 0);
        uint8_t hasFoundEnd = false;
        for (uint8_t index = 0; index < 10; index++) {
            int8_t character = text[index];
            if (character == 0) {
                hasFoundEnd = true;
            }
            if (hasFoundEnd) {
                character = ' ';
            }
            sendLcdCharacter(character);
        }
    }
    
    return 0;
}


