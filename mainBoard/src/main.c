
#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#define NULL ((void *)0)
#define true 1
#define false 0

#define PREV_BUTTON 1
#define NEXT_BUTTON 2
#define ENTER_BUTTON 3

#define TEMPERATURE_MARGIN 3
#define OFF_THRESHOLD_ADDRESS ((uint8_t *)0)
#define ON_THRESHOLD_ADDRESS ((uint8_t *)1)

#define FAN_AMOUNT 6
#define RUN_STATE_OFF 0
#define RUN_STATE_ON 1

#define MAX_TACHOMETER_DELAY 10

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

#define button1PinInput() DDRD &= ~(1 << DDD5)
#define button1PinRead() (PIND & (1 << PIND5))

#define button2PinInput() DDRD &= ~(1 << DDD6)
#define button2PinRead() (PIND & (1 << PIND6))

#define button3PinInput() DDRD &= ~(1 << DDD7)
#define button3PinRead() (PIND & (1 << PIND7))

#define fan1ControlPinOutput() DDRC |= (1 << DDC5)
#define fan1ControlPinInput() DDRC &= ~(1 << DDC5)
#define fan1ControlPinHigh() PORTC |= (1 << PORTC5)
#define fan1ControlPinLow() PORTC &= ~(1 << PORTC5)

#define fan2ControlPinOutput() DDRC |= (1 << DDC4)
#define fan2ControlPinInput() DDRC &= ~(1 << DDC4)
#define fan2ControlPinHigh() PORTC |= (1 << PORTC4)
#define fan2ControlPinLow() PORTC &= ~(1 << PORTC4)

#define fan3ControlPinOutput() DDRC |= (1 << DDC3)
#define fan3ControlPinInput() DDRC &= ~(1 << DDC3)
#define fan3ControlPinHigh() PORTC |= (1 << PORTC3)
#define fan3ControlPinLow() PORTC &= ~(1 << PORTC3)

#define fan4ControlPinOutput() DDRC |= (1 << DDC0)
#define fan4ControlPinInput() DDRC &= ~(1 << DDC0)
#define fan4ControlPinHigh() PORTC |= (1 << PORTC0)
#define fan4ControlPinLow() PORTC &= ~(1 << PORTC0)

#define fan5ControlPinOutput() DDRC |= (1 << DDC1)
#define fan5ControlPinInput() DDRC &= ~(1 << DDC1)
#define fan5ControlPinHigh() PORTC |= (1 << PORTC1)
#define fan5ControlPinLow() PORTC &= ~(1 << PORTC1)

#define fan6ControlPinOutput() DDRC |= (1 << DDC2)
#define fan6ControlPinInput() DDRC &= ~(1 << DDC2)
#define fan6ControlPinHigh() PORTC |= (1 << PORTC2)
#define fan6ControlPinLow() PORTC &= ~(1 << PORTC2)

#define fan1TachoPinInput() DDRD &= ~(1 << DDD1)
#define fan1TachoPinRead() (PIND & (1 << PIND1))

#define fan2TachoPinInput() DDRD &= ~(1 << DDD0)
#define fan2TachoPinRead() (PIND & (1 << PIND0))

#define fan3TachoPinInput() DDRD &= ~(1 << DDD2)
#define fan3TachoPinRead() (PIND & (1 << PIND2))

#define fan4TachoPinInput() DDRB &= ~(1 << DDB2)
#define fan4TachoPinRead() (PINB & (1 << PINB2))

#define fan5TachoPinInput() DDRB &= ~(1 << DDB1)
#define fan5TachoPinRead() (PINB & (1 << PINB1))

#define fan6TachoPinInput() DDRB &= ~(1 << DDB0)
#define fan6TachoPinRead() (PINB & (1 << PINB0))

#define setLcdCursorPos(posX, posY) sendLcdCommand(0x80 | (posX + posY * 0x40))
#define clearLcd() sendLcdCommand(0x01)

const int8_t lcdInitCommands[] PROGMEM = {
    0x39, 0x1C, 0x52, 0x69, 0x74, 0x38, 0x0C, 0x01, 0x06
};

uint8_t lastSatelliteData = 0;
uint8_t lastPressedButton = 0;
uint8_t buttonIsPressed = false;
uint8_t secondDelay = 0;
uint8_t fanDelay = 0;
uint8_t tachometerDelay = 0;
uint8_t hasTemperatureFault = false;
uint8_t temperatureC = 0;
uint8_t offThreshold;
uint8_t onThreshold;
uint8_t runState = RUN_STATE_OFF;
uint8_t runningFanAmount = 0;
uint8_t stuckFan = 0;

void controlFans(uint8_t enableAmount) {
    if (enableAmount >= 1) {
        fan1ControlPinHigh();
        fan1ControlPinOutput();
    } else {
        fan1ControlPinLow();
        fan1ControlPinInput();
    }
    if (enableAmount >= 2) {
        fan4ControlPinHigh();
        fan4ControlPinOutput();
    } else {
        fan4ControlPinLow();
        fan4ControlPinInput();
    }
    if (enableAmount >= 3) {
        fan2ControlPinHigh();
        fan2ControlPinOutput();
    } else {
        fan2ControlPinLow();
        fan2ControlPinInput();
    }
    if (enableAmount >= 4) {
        fan5ControlPinHigh();
        fan5ControlPinOutput();
    } else {
        fan5ControlPinLow();
        fan5ControlPinInput();
    }
    if (enableAmount >= 5) {
        fan3ControlPinHigh();
        fan3ControlPinOutput();
    } else {
        fan3ControlPinLow();
        fan3ControlPinInput();
    }
    if (enableAmount >= 6) {
        fan6ControlPinHigh();
        fan6ControlPinOutput();
    } else {
        fan6ControlPinLow();
        fan6ControlPinInput();
    }
}

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
    
    button1PinInput();
    button2PinInput();
    button3PinInput();
    
    controlFans(0);
    
    fan1TachoPinInput();
    fan2TachoPinInput();
    fan3TachoPinInput();
    fan4TachoPinInput();
    fan5TachoPinInput();
    fan6TachoPinInput();
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

uint16_t readTemperatureV() {
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
    uint16_t temperatureV = 0;
    for (uint8_t offset = 0; offset < 10; offset++) {
        uint8_t runLength = readSatelliteRun();
        if (runLength == 2) {
            temperatureV |= ((uint16_t)1 << offset);
        }
        if (runLength > 2) {
            // Run lengths in payload must be 1 or 2.
            return 0;
        }
    }
    return temperatureV;
}

uint8_t readTachometers() {
    uint8_t output = 0;
    if (fan1TachoPinRead()) {
        output |= 0x01;
    }
    if (fan1TachoPinRead()) {
        output |= 0x02;
    }
    if (fan1TachoPinRead()) {
        output |= 0x04;
    }
    if (fan1TachoPinRead()) {
        output |= 0x08;
    }
    if (fan1TachoPinRead()) {
        output |= 0x10;
    }
    if (fan1TachoPinRead()) {
        output |= 0x20;
    }
    return output;
}

uint8_t getPressedButton() {
    if (!button1PinRead()) {
        return PREV_BUTTON;
    } else if (!button2PinRead()) {
        return NEXT_BUTTON;
    } else if (!button3PinRead()) {
        return ENTER_BUTTON;
    } else {
        return 0;
    }
}

void initializeTimer() {
    // Enable CTC timer mode, and use clock divided by 1024.
    TCCR1B |= (1 << WGM12) | (1 << CS02) | (1 << CS00);
    // Set maximum timer value to be 50 ms.
    OCR1A = 390;
    // Set initial timer value.
    TCNT1 = 0;
    // Configure interrupt to run when timer reaches maximum value.
    TIMSK1 |= (1 << OCIE1A);
    // Enable interrupts.
    sei();
}

// Interrupt triggered by timer.
ISR(TIMER1_COMPA_vect) {
    uint8_t lastButtonIsPressed = buttonIsPressed;
    uint8_t pressedButton = getPressedButton();
    buttonIsPressed = (pressedButton > 0);
    if (buttonIsPressed && !lastButtonIsPressed) {
        lastPressedButton = pressedButton;
    }
    secondDelay += 1;
    if (secondDelay >= 20) {
        fanDelay += 1;
        if (tachometerDelay < MAX_TACHOMETER_DELAY) {
            tachometerDelay += 1;
        }
        secondDelay = 0;
    }
}

void initializeThresholds() {
    offThreshold = eeprom_read_byte(OFF_THRESHOLD_ADDRESS);
    onThreshold = eeprom_read_byte(ON_THRESHOLD_ADDRESS);
    if (offThreshold == 0xFF || onThreshold == 0xFF) {
        offThreshold = 40;
        onThreshold = 43;
    }
}

void updateTemperature() {
    uint16_t temperatureV = readTemperatureV();
    hasTemperatureFault = (temperatureV == 0);
    if (hasTemperatureFault) {
        temperatureC = 0;
        return;
    }
    // At 25 degrees C, voltage = 750 mV = 153.6 ADC value
    // Increase of 1 degree C = 10 mV = 2.05 ADC value
    uint8_t instantTemperatureC = (uint8_t)(25 + ((int16_t)temperatureV - 154) / 2);
    if (temperatureC == 0) {
        temperatureC = instantTemperatureC;
    }
    if (instantTemperatureC < temperatureC - TEMPERATURE_MARGIN) {
        temperatureC -= 1;
    }
    if (instantTemperatureC > temperatureC + TEMPERATURE_MARGIN) {
        temperatureC += 1;
    }
}

void updateFans() {
    if (hasTemperatureFault) {
        runState = RUN_STATE_OFF;
    } else {
        if (temperatureC <= offThreshold) {
            runState = RUN_STATE_OFF;
        }
        if (temperatureC >= onThreshold) {
            runState = RUN_STATE_ON;
        }
    }
    if (fanDelay < 5) {
        return;
    }
    fanDelay = 0;
    if (runState == RUN_STATE_OFF && runningFanAmount > 0) {
        runningFanAmount -= 1;
    }
    if (runState == RUN_STATE_ON && runningFanAmount < FAN_AMOUNT) {
        runningFanAmount += 1;
    }
    controlFans(runningFanAmount);
}

void updateTachometers() {
    if (runningFanAmount < FAN_AMOUNT) {
        tachometerDelay = 0;
        return;
    }
    if (tachometerDelay < MAX_TACHOMETER_DELAY) {
        // Only measure tachometers after all fans have been running for a little while.
        return;
    }
    uint8_t stuckTachometers = (uint8_t)~(0xFF << FAN_AMOUNT);
    uint8_t lastTachometers = readTachometers();
    for (uint8_t count = 0; count < 100; count++) {
        sleepMilliseconds(5);
        uint8_t currentTachometers = readTachometers();
        uint8_t changedTachometers = currentTachometers ^ lastTachometers;
        stuckTachometers &= ~changedTachometers;
        if (stuckTachometers == 0) {
            stuckFan = 0;
            return;
        }
    }
    for (uint8_t index = 0; index < FAN_AMOUNT; index += 1) {
        if (stuckTachometers & (1 << index)) {
            stuckFan = index + 1;
            break;
        }
    }
}

int main(void) {
    
    initializePinModes();
    initializeLcd();
    initializeTimer();
    initializeThresholds();
    
    while (true) {
        updateTemperature();
        updateFans();
        updateTachometers();
        // TODO: Update display.
        
    }
    
    return 0;
}


