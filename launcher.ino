const uint8_t ENABLE_LED_PIN = 2;
const uint8_t ARMED_LED_PIN = 3;
const uint8_t BTN_1_LED_PIN = 4;
const uint8_t BTN_2_LED_PIN = 5;
const uint8_t BTN_3_LED_PIN = 6;
const uint8_t BTN_4_LED_PIN = 7;
const uint8_t BTN_5_LED_PIN = 8;
const uint8_t BTN_6_LED_PIN = 9;
const uint8_t BTN_7_LED_PIN = 10;
const uint8_t BTN_8_LED_PIN = 11;
const uint8_t ENABLE_PIN = 22;
const uint8_t ARMED_PIN = 23;
const uint8_t BTN_1_PIN = 24;
const uint8_t BTN_2_PIN = 25;
const uint8_t BTN_3_PIN = 26;
const uint8_t BTN_4_PIN = 27;
const uint8_t BTN_5_PIN = 28;
const uint8_t BTN_6_PIN = 29;
const uint8_t BTN_7_PIN = 30;
const uint8_t BTN_8_PIN = 31;
const uint8_t RELAY_1_PIN = 32;
const uint8_t RELAY_2_PIN = 33;
const uint8_t RELAY_3_PIN = 34;
const uint8_t RELAY_4_PIN = 35;
const uint8_t RELAY_5_PIN = 36;
const uint8_t RELAY_6_PIN = 37;
const uint8_t RELAY_7_PIN = 38;
const uint8_t RELAY_8_PIN = 39;
const uint8_t NUM_OF_BUTTONS = 8;
const uint8_t DIM = 20;
const uint8_t BURN_BLINK_MILLIS = 100;
const uint8_t MAX_PWM = 255;
const uint8_t LIGHTSHOW_MILLIS_PER_LIGHT = 100;
const uint16_t RELAY_BURN_MILLIS = 3000;

struct Light {
    uint8_t ledPin = 0;
    void on(bool on) {
        setValue(on ? MAX_PWM : 0);
    };
    void setValue(int value) {
        if (value < 0) value = 0;
        if (value > MAX_PWM) value = MAX_PWM;
        analogWrite(ledPin, value);
    };
    Light() {};
    Light(int ledPinNumber) {
        ledPin = ledPinNumber;
        pinMode(ledPin, OUTPUT);
    };
};

struct Switch {
    uint8_t switchPin = 0;
    uint8_t ledPin = 0;
    Light light;
    bool isClosed() {
        return digitalRead(switchPin) == LOW;
    };
    bool isOpen() {
        return !isClosed();
    }
    Switch() {};
    Switch(int switchPinNumber, int ledPinNumber) {
        switchPin = switchPinNumber;
        pinMode(switchPin, INPUT_PULLUP);
        light = Light(ledPinNumber);
    };
};

struct LaunchButton {
    Switch launchSwitch;
    bool fired = false;
    bool burning = false;
    bool burnBlinkToggle = true;
    unsigned long burnBlinkSwitchMillis = 0;
    unsigned long firedAtMillis = 0;
    int relayPin = 0;
    LaunchButton() {};
    LaunchButton(int switchPinNumber, int ledPinNumber, int relayPinNumber) {
        relayPin = relayPinNumber;
        launchSwitch = Switch(switchPinNumber, ledPinNumber);
        pinMode(relayPin, OUTPUT);
    };
    void processInput(bool isEnabled, bool isArmed, unsigned long now) {
        if (burning && (now - firedAtMillis) >= RELAY_BURN_MILLIS) {
            digitalWrite(relayPin, LOW);
            launchSwitch.light.on(false);
            burning = false;
            return;
        }
        
        if (burning) {
            launchSwitch.light.on(burnBlinkToggle);
            if ((now - burnBlinkSwitchMillis) >= BURN_BLINK_MILLIS) {
                burnBlinkSwitchMillis = now;
                burnBlinkToggle = !burnBlinkToggle;
            }
            return;
        }

        if (!isEnabled || fired) {
            launchSwitch.light.on(false);
            return;
        }

        if (isArmed && launchSwitch.isClosed()) {
          firedAtMillis = now;
          burnBlinkSwitchMillis = now;
          fired = true;
          burning = true;
          digitalWrite(relayPin, HIGH);
          return;
        }
        
        if (isArmed && launchSwitch.isOpen()) {
            launchSwitch.light.on(true);
            return;
        }
        
        if (launchSwitch.isClosed()) {
            launchSwitch.light.on(true);
            return;
        }

        launchSwitch.light.setValue(DIM);
    }
};

struct Lightshow {
    unsigned long lightshowLastSwitchMillis = 0;
    uint8_t lightshowLightIndex = 0;

    void display(LaunchButton *buttons, unsigned long now) {
        if (lightshowLastSwitchMillis == 0) {
            lightshowLastSwitchMillis = now;
        }
   
        if (now - lightshowLastSwitchMillis >= LIGHTSHOW_MILLIS_PER_LIGHT) {
            lightshowLastSwitchMillis = now;
            lightshowLightIndex++;
            if (lightshowLightIndex > NUM_OF_BUTTONS) {
                lightshowLightIndex = 0;
            }
        }

        for (int i = 0; i < NUM_OF_BUTTONS; i++) {
            if (i == lightshowLightIndex) {
                buttons[i].launchSwitch.light.setValue(DIM);
            } else {
                buttons[i].launchSwitch.light.on(false);
            }
        }
    }
};

Switch enableSwitch, armedSwitch;
LaunchButton buttons[NUM_OF_BUTTONS];
Lightshow lightshow;

void setup()
{
    Serial.begin(115200);
	enableSwitch = Switch(ENABLE_PIN, ENABLE_LED_PIN);
    armedSwitch = Switch(ARMED_PIN, ARMED_LED_PIN);
    buttons[0] = LaunchButton(BTN_1_PIN, BTN_1_LED_PIN, RELAY_1_PIN);
    buttons[1] = LaunchButton(BTN_2_PIN, BTN_2_LED_PIN, RELAY_2_PIN);
    buttons[2] = LaunchButton(BTN_3_PIN, BTN_3_LED_PIN, RELAY_3_PIN);
    buttons[3] = LaunchButton(BTN_4_PIN, BTN_4_LED_PIN, RELAY_4_PIN);
    buttons[4] = LaunchButton(BTN_5_PIN, BTN_5_LED_PIN, RELAY_5_PIN);
    buttons[5] = LaunchButton(BTN_6_PIN, BTN_6_LED_PIN, RELAY_6_PIN);
    buttons[6] = LaunchButton(BTN_7_PIN, BTN_7_LED_PIN, RELAY_7_PIN);
    buttons[7] = LaunchButton(BTN_8_PIN, BTN_8_LED_PIN, RELAY_8_PIN);
}

void loop()
{
    unsigned long now = millis();

    bool isEnabled = enableSwitch.isClosed();
    bool isArmed = armedSwitch.isClosed();

    enableSwitch.light.on(isEnabled);
    armedSwitch.light.on(isArmed);

    if (!isEnabled && !isArmed) {
        lightshow.display(buttons, now);
    }

    for (int i = 0; i < NUM_OF_BUTTONS; i++) {
        buttons[i].processInput(isEnabled, isArmed, now); 
    }
}