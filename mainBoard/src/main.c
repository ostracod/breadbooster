
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

#define BUTTON_NONE 0
#define BUTTON_PREV 1
#define BUTTON_NEXT 2
#define BUTTON_ENTER 3

#define TEMPERATURE_MARGIN 3
#define ADDRESS_OFF_THRESHOLD 0
#define ADDRESS_ON_THRESHOLD 1
#define ADDRESS_SPIKE_WIDTH 2
#define ADDRESS_SPIKE_HEIGHT 3
#define ADDRESS_SPIKE_RESET 4

#define FAN_AMOUNT 6
#define RUN_STATE_OFF 0
#define RUN_STATE_ON 1
#define RUN_STATE_SPIKE 2

#define MAX_TACHOMETER_DELAY 10
#define MAX_TIMEOUT_DELAY 30
#define MAX_STUCK_COUNT 5
#define MAX_SPIKE_WIDTH 10

#define FAULT_NONE 0
#define FAULT_TEMPERATURE 1
#define FAULT_FAN 2

#define TUNABLE_AMOUNT 5
#define SCREEN_AMOUNT (1 + TUNABLE_AMOUNT)
#define SCREEN_MAIN 0

#define TUNABLE_TEMP 0
#define TUNABLE_TIME 1

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

typedef struct {
    const int8_t *title; // Must be a pointer in PROGMEM.
    uint8_t tunableType;
    uint8_t *valuePointer;
    uint8_t minValue;
    uint8_t maxValue;
    void (*save)(void);
} tunableScreen_t;

const int8_t lcdInitCommands[] PROGMEM = {
    0x39, 0x1C, 0x52, 0x69, 0x74, 0x38, 0x0C, 0x01, 0x06
};

const int8_t idleText[] PROGMEM = "Idle   ";
const int8_t runningText[] PROGMEM = "Running";
const int8_t spikeText[] PROGMEM = "Spike  ";
const int8_t offThresholdText[] PROGMEM = "Off threshold:";
const int8_t onThresholdText[] PROGMEM = "On threshold:";
const int8_t spikeWidthText[] PROGMEM = "Spike width:";
const int8_t spikeHeightText[] PROGMEM = "Spike height:";
const int8_t spikeResetText[] PROGMEM = "Spike reset:";
const int8_t healthyText[] PROGMEM = "Healthy     ";
const int8_t tempFaultText[] PROGMEM = "Temp fault! ";
const int8_t fanText[] PROGMEM = "Fan ";
const int8_t faultText[] PROGMEM = " fault!";

uint8_t lastSatelliteData = 0;
uint8_t lastPressedButton = BUTTON_NONE;
uint8_t buttonIsPressed = false;
uint8_t secondDelay = 0;
uint8_t fanDelay = 0;
uint8_t tachometerDelay = 0;
uint8_t stuckDelay = 0;
uint8_t timeoutDelay = 0;
uint8_t minuteDelay = 0;

uint8_t hasTemperatureFault = false;
uint8_t currentTemperature = 0;
uint8_t offThreshold;
uint8_t onThreshold;
uint8_t spikeWidth;
uint8_t spikeHeight;
uint8_t spikeResetTime;
uint8_t runState = RUN_STATE_OFF;
uint8_t runningFanAmount = 0;
uint8_t stuckCounts[FAN_AMOUNT];
uint8_t stuckFan = 0;
uint8_t currentFault = FAULT_NONE;
uint8_t temperatureHistory[MAX_SPIKE_WIDTH];
uint8_t historyLength = 0;
uint8_t spikeCooldown = 0;

tunableScreen_t tunableScreens[TUNABLE_AMOUNT];
uint8_t currentScreen;
tunableScreen_t *currentTunable;
uint8_t isEditingTunable = false;
uint8_t editValue;
uint8_t heartbeat = 0;

uint8_t displayedTemperature;
uint8_t displayedRunState;
uint8_t displayedHeartbeat;
uint8_t displayedFault;

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
    if (fan2TachoPinRead()) {
        output |= 0x02;
    }
    if (fan3TachoPinRead()) {
        output |= 0x04;
    }
    if (fan4TachoPinRead()) {
        output |= 0x08;
    }
    if (fan5TachoPinRead()) {
        output |= 0x10;
    }
    if (fan6TachoPinRead()) {
        output |= 0x20;
    }
    return output;
}

uint8_t getPressedButton() {
    if (!button1PinRead()) {
        return BUTTON_PREV;
    } else if (!button2PinRead()) {
        return BUTTON_NEXT;
    } else if (!button3PinRead()) {
        return BUTTON_ENTER;
    } else {
        return BUTTON_NONE;
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
        stuckDelay = 1;
        if (timeoutDelay < MAX_TIMEOUT_DELAY) {
            timeoutDelay += 1;
        }
        heartbeat = 1 - heartbeat;
        if (minuteDelay < 60) {
            minuteDelay += 1;
        }
        secondDelay = 0;
    }
}

void updateTemperature() {
    uint16_t temperatureV = readTemperatureV();
    hasTemperatureFault = (temperatureV == 0);
    if (hasTemperatureFault) {
        currentTemperature = 0;
        return;
    }
    // At 25 degrees C, voltage = 750 mV = 153.6 ADC value
    // Increase of 1 degree C = 10 mV = 2.05 ADC value
    uint8_t temperatureC = (uint8_t)(25 + ((int16_t)temperatureV - 154) / 2);
    if (currentTemperature == 0) {
        currentTemperature = temperatureC;
    }
    if (temperatureC < currentTemperature - TEMPERATURE_MARGIN) {
        currentTemperature -= 1;
    }
    if (temperatureC > currentTemperature + TEMPERATURE_MARGIN) {
        currentTemperature += 1;
    }
}

void updateSpike() {
    if (hasTemperatureFault) {
        historyLength = 0;
        return;
    }
    if (minuteDelay < 60) {
        return;
    }
    minuteDelay = 0;
    if (spikeCooldown > 0) {
        spikeCooldown -= 1;
        return;
    }
    for (uint8_t index = MAX_SPIKE_WIDTH - 1; index > 0; index--) {
        temperatureHistory[index] = temperatureHistory[index - 1];
    }
    temperatureHistory[0] = currentTemperature;
    if (historyLength < MAX_SPIKE_WIDTH) {
        historyLength += 1;
    }
    if (spikeWidth >= historyLength) {
        return;
    }
    uint8_t refTemperature = temperatureHistory[spikeWidth];
    // Be careful of unsigned integers.
    if (currentTemperature > refTemperature
            && currentTemperature - refTemperature >= spikeHeight) {
        spikeCooldown = spikeResetTime;
        historyLength = 0;
    }
}

void updateFans() {
    if (hasTemperatureFault) {
        runState = RUN_STATE_OFF;
    } else if (spikeCooldown > 0) {
        runState = RUN_STATE_SPIKE;
    } else {
        if (currentTemperature <= offThreshold) {
            runState = RUN_STATE_OFF;
        }
        if (currentTemperature >= onThreshold) {
            runState = RUN_STATE_ON;
        }
    }
    if (fanDelay < 5) {
        return;
    }
    fanDelay = 0;
    if (runState == RUN_STATE_OFF) {
        if (runningFanAmount > 0) {
            runningFanAmount -= 1;
        }
    } else {
        if (runningFanAmount < FAN_AMOUNT) {
            runningFanAmount += 1;
        }
    }
    controlFans(runningFanAmount);
}

void updateTachometers() {
    
    // Only measure tachometers after all fans have been running for a little while.
    if (runningFanAmount < FAN_AMOUNT) {
        tachometerDelay = 0;
        return;
    }
    if (tachometerDelay < MAX_TACHOMETER_DELAY) {
        return;
    }
    
    // Detect which tachometers change during 100 ms.
    uint8_t changedTachometers = 0;
    uint8_t startTachometers = readTachometers();
    for (uint8_t count = 0; count < 20; count++) {
        sleepMilliseconds(5);
        uint8_t currentTachometers = readTachometers();
        changedTachometers |= currentTachometers ^ startTachometers;
        if (changedTachometers == (uint8_t)~(0xFF << FAN_AMOUNT)) {
            break;
        }
    }
    
    // Update stuck counts of tachometers.
    uint8_t shouldCountStuck = (stuckDelay > 0);
    stuckDelay = 0;
    for (uint8_t index = 0; index < FAN_AMOUNT; index++) {
        if (changedTachometers & (1 << index)) {
            stuckCounts[index] = 0;
        } else if (shouldCountStuck && stuckCounts[index] < MAX_STUCK_COUNT) {
            stuckCounts[index] += 1;
        }
    }
    
    // Determine if any fan is stuck.
    stuckFan = 0;
    for (uint8_t index = 0; index < FAN_AMOUNT; index++) {
        if (stuckCounts[index] >= MAX_STUCK_COUNT) {
            stuckFan = index + 1;
            break;
        }
    }
}

void updateFault() {
    if (hasTemperatureFault) {
        currentFault = FAULT_TEMPERATURE;
    } else if (stuckFan > 0) {
        currentFault = FAULT_FAN + stuckFan - 1;
    } else {
        currentFault = FAULT_NONE;
    }
}

// `text` must be a pointer in PROGMEM.
void displayText(uint8_t posX, uint8_t posY, const uint8_t *text) {
    setLcdCursorPos(posX, posY);
    uint8_t index = 0;
    while (true) {
        int8_t character = pgm_read_byte(text + index);
        if (character == 0) {
            break;
        }
        sendLcdCharacter(character);
        index += 1;
    }
}

uint8_t displayInt(uint8_t value) {
    uint8_t text[5];
    itoa(value, text, 10);
    uint8_t index = 0;
    while (index < 5) {
        int8_t character = text[index];
        if (character == 0) {
            break;
        }
        sendLcdCharacter(character);
        index += 1;
    }
    return index;
}

void displayTemperature(uint8_t posX, uint8_t posY, uint8_t temperature) {
    setLcdCursorPos(posX, posY);
    uint8_t offsetX;
    if (temperature == 0) {
        sendLcdCharacter('?');
        offsetX = 1;
    } else {
        offsetX = displayInt(temperature);
    }
    sendLcdCharacter(0xF2); // Degree symbol.
    sendLcdCharacter('C');
    offsetX += 2;
    while (offsetX < 5) {
        sendLcdCharacter(' ');
        offsetX += 1;
    }
}

void displayTime(uint8_t posX, uint8_t posY, uint8_t time) {
    setLcdCursorPos(posX, posY);
    uint8_t offsetX = displayInt(time);
    sendLcdCharacter('m');
    offsetX += 1;
    while (offsetX < 4) {
        sendLcdCharacter(' ');
        offsetX += 1;
    }
}

void displayTunable(uint8_t tunableType, uint8_t value) {
    if (tunableType == TUNABLE_TEMP) {
        displayTemperature(2, 1, value);
    } else if (tunableType == TUNABLE_TIME) {
        displayTime(2, 1, value);
    }
}

void displayCurrentTemp() {
    displayTemperature(0, 0, currentTemperature);
    displayedTemperature = currentTemperature;
}

void displayRunState() {
    const uint8_t *text = NULL;
    if (runState == RUN_STATE_OFF) {
        text = idleText;
    } else if (runState == RUN_STATE_ON) {
        text = runningText;
    } else if (runState == RUN_STATE_SPIKE) {
        text = spikeText;
    }
    if (text != NULL) {
        displayText(8, 0, text);
    }
    displayedRunState = runState;
}

void displayHeartbeat() {
    setLcdCursorPos(0, 1);
    // Overscore or underscore.
    sendLcdCharacter(heartbeat ? 0xFF : '_');
    displayedHeartbeat = heartbeat;
}

void displayFault() {
    if (currentFault == FAULT_NONE) {
        displayText(2, 1, healthyText);
    } else if (currentFault == FAULT_TEMPERATURE) {
        displayText(2, 1, tempFaultText);
    } else if (currentFault >= FAULT_FAN) {
        uint8_t fanIndex = currentFault - FAULT_FAN;
        displayText(2, 1, fanText);
        sendLcdCharacter('1' + fanIndex);
        displayText(7, 1, faultText);
    }
    displayedFault = currentFault;
}

void displayEditCursor() {
    setLcdCursorPos(0, 1);
    // Arrow or space.
    sendLcdCharacter(isEditingTunable ? 0x7E : ' ');
}

void showScreen(uint8_t screen) {
    currentScreen = screen;
    currentTunable = NULL;
    isEditingTunable = false;
    clearLcd();
    if (currentScreen == SCREEN_MAIN) {
        displayCurrentTemp();
        displayRunState();
        displayHeartbeat();
        displayFault();
    } else {
        currentTunable = tunableScreens + currentScreen - 1;
        displayText(0, 0, currentTunable->title);
        uint8_t value = *(currentTunable->valuePointer);
        displayTunable(currentTunable->tunableType, value);
    }
}

void checkTimeout() {
    if (currentScreen != SCREEN_MAIN && timeoutDelay >= MAX_TIMEOUT_DELAY) {
        showScreen(SCREEN_MAIN);
    }
}

void updateScreen() {
    if (currentScreen != SCREEN_MAIN) {
        return;
    }
    if (currentTemperature != displayedTemperature) {
        displayCurrentTemp();
    }
    if (runState != displayedRunState) {
        displayRunState();
    }
    if (heartbeat != displayedHeartbeat) {
        displayHeartbeat();
    }
    if (currentFault != displayedFault) {
        displayFault();
    }
}

uint8_t readEeprom(uint8_t address) {
    eeprom_busy_wait();
    return eeprom_read_byte((uint8_t *)(uint16_t)address);
}

void writeEeprom(uint8_t address, uint8_t value) {
    eeprom_busy_wait();
    eeprom_write_byte((uint8_t *)(uint16_t)address, value);
}

void saveThresholds() {
    writeEeprom(ADDRESS_OFF_THRESHOLD, offThreshold);
    writeEeprom(ADDRESS_ON_THRESHOLD, onThreshold);
}

void saveOffThreshold() {
    if (onThreshold <= offThreshold) {
        onThreshold = offThreshold + 1;
    }
    saveThresholds();
}

void saveOnThreshold() {
    if (offThreshold >= onThreshold) {
        offThreshold = onThreshold - 1;
    }
    saveThresholds();
}

void saveSpikeWidth() {
    writeEeprom(ADDRESS_SPIKE_WIDTH, spikeWidth);
}

void saveSpikeHeight() {
    writeEeprom(ADDRESS_SPIKE_HEIGHT, spikeHeight);
}

void saveSpikeReset() {
    writeEeprom(ADDRESS_SPIKE_RESET, spikeResetTime);
}

void initializeTunables() {
    tunableScreens[0] = (tunableScreen_t){
        offThresholdText,
        TUNABLE_TEMP,
        &offThreshold,
        10,
        90,
        &saveOffThreshold
    };
    tunableScreens[1] = (tunableScreen_t){
        onThresholdText,
        TUNABLE_TEMP,
        &onThreshold,
        10,
        90,
        &saveOnThreshold
    };
    tunableScreens[2] = (tunableScreen_t){
        spikeWidthText,
        TUNABLE_TIME,
        &spikeWidth,
        1,
        MAX_SPIKE_WIDTH,
        &saveSpikeWidth
    };
    tunableScreens[3] = (tunableScreen_t){
        spikeHeightText,
        TUNABLE_TEMP,
        &spikeHeight,
        1,
        90,
        &saveSpikeHeight
    };
    tunableScreens[4] = (tunableScreen_t){
        spikeResetText,
        TUNABLE_TIME,
        &spikeResetTime,
        1,
        10,
        &saveSpikeReset
    };
    offThreshold = readEeprom(ADDRESS_OFF_THRESHOLD);
    onThreshold = readEeprom(ADDRESS_ON_THRESHOLD);
    if (offThreshold == 0xFF || onThreshold == 0xFF) {
        offThreshold = 29;
        onThreshold = 32;
    }
    spikeWidth = readEeprom(ADDRESS_SPIKE_WIDTH);
    if (spikeWidth == 0xFF) {
        spikeWidth = 5;
    }
    spikeHeight = readEeprom(ADDRESS_SPIKE_HEIGHT);
    if (spikeHeight == 0xFF) {
        spikeHeight = 5;
    }
    spikeResetTime = readEeprom(ADDRESS_SPIKE_RESET);
    if (spikeResetTime == 0xFF) {
        spikeResetTime = 5;
    }
}

void handleButton() {
    if (lastPressedButton == BUTTON_NONE) {
        return;
    }
    uint8_t button = lastPressedButton;
    lastPressedButton = BUTTON_NONE;
    timeoutDelay = 0;
    if (isEditingTunable) {
        if (button == BUTTON_ENTER) {
            *(currentTunable->valuePointer) = editValue;
            (*(currentTunable->save))();
            isEditingTunable = false;
            displayEditCursor();
        } else {
            uint8_t lastValue = editValue;
            if (button == BUTTON_PREV) {
                if (editValue > currentTunable->minValue) {
                    editValue -= 1;
                }
            } else if (button == BUTTON_NEXT) {
                if (editValue < currentTunable->maxValue) {
                    editValue += 1;
                }
            }
            if (editValue != lastValue) {
                displayTunable(currentTunable->tunableType, editValue);
            }
        }
    } else {
        if (button == BUTTON_PREV) {
            uint8_t prevScreen = (currentScreen > 0) ? currentScreen - 1 : SCREEN_AMOUNT - 1;
            showScreen(prevScreen);
        } else if (button == BUTTON_NEXT) {
            uint8_t nextScreen = (currentScreen < SCREEN_AMOUNT - 1) ? currentScreen + 1 : 0;
            showScreen(nextScreen);
        } else if (button == BUTTON_ENTER) {
            if (currentTunable != NULL) {
                editValue = *(currentTunable->valuePointer);
                isEditingTunable = true;
                displayEditCursor();
            }
        }
    }
}

int main(void) {
    
    initializePinModes();
    initializeLcd();
    initializeTimer();
    initializeTunables();
    for (uint8_t index = 0; index < FAN_AMOUNT; index++) {
        stuckCounts[index] = 0;
    }
    showScreen(SCREEN_MAIN);
    
    while (true) {
        updateTemperature();
        updateSpike();
        updateFans();
        updateTachometers();
        updateFault();
        checkTimeout();
        updateScreen();
        handleButton();
    }
    
    return 0;
}


