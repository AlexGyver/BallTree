#include <Arduino.h>
#include <FastLED.h>
#include <WiFiConnector.h>

#include "config.h"
#include "gridGen.h"

constexpr int countBalls(int side, int res = 1) {
    return --side ? countBalls(side, res + side * 6) : res;
}
constexpr int BALLS_AMOUNT = countBalls(HEX_SIZE);

// motors
#define plan_t int16_t
#include <MultiStepPlanner.h>
#include <MultiStepperSPI.h>
MultiStepperSPI<1000000ul> motors(BALLS_AMOUNT, DRIVER_CS);
MultiStepPlanner balls(motors);

// settings
#include <GyverDBFile.h>
#include <LittleFS.h>
GyverDBFile db(&LittleFS, "/data.db");
#include <SettingsESP.h>
SettingsESP sett(PROJECT_NAME, &db);

DoubleArray<uint8_t> matrix, segs;
bool stoppped = false;

DB_KEYS(
    kk,
    wifi_ssid,
    wifi_pass,
    wifi_save,
    max_h,
    max_speed,
    fig_begin,
    fig_end,
    sin_step,
    noise_step,
    noise_shift,
    noise_ampli

);

enum Tabs : uint8_t {
    Spiral,
    Pyramid,
    Sin,
    Noise,
    All,
    Single,
    Config,
} tab;

// ============== BUILD ==============
void build(sets::Builder& b) {
    // TABS
    if (b.Tabs("Спираль;Пирамида;Синус;Шум;Все;Один;Настройки", (uint8_t*)&tab)) {
        b.reload();
        balls.stop();
        stoppped = true;
        return;
    }

    switch (tab) {
        case Tabs::All: {
            {
                sets::Row r(b);
                b.LabelNum(H(height), "Высота", balls.getPos(0));
                if (b.Button("Сброс")) balls.reset();
            }
            {
                sets::Row r(b);
                if (b.ButtonHold("Вниз")) b.build.pressed() ? balls.runSpeed(1) : balls.stop();
                if (b.ButtonHold("Вверх")) b.build.pressed() ? balls.runSpeed(0) : balls.stop();
            }
        } break;

        case Tabs::Single: {
            static uint8_t ball;
            b.Slider("Ball", 0, BALLS_AMOUNT, 1, "", &ball);
            {
                sets::Row r(b);
                if (b.ButtonHold("Вниз")) b.build.pressed() ? balls.runSpeed(ball, 1) : balls.stop();
                if (b.ButtonHold("Вверх")) b.build.pressed() ? balls.runSpeed(ball, 0) : balls.stop();
            }
        } break;

        case Tabs::Config: {
            {
                sets::Group g(b, "Система");
                b.Input(kk::max_h, "Макс. высота");
            }
            {
                sets::Group g(b, "WiFi");
                b.Pass(kk::wifi_ssid, "SSID");
                b.Pass(kk::wifi_pass, "Pass", "");

                if (b.Button(kk::wifi_save, "Подключить")) {
                    WiFiConnector.connect(db[kk::wifi_ssid], db[kk::wifi_pass]);
                }
            }
        } break;

        case Tabs::Spiral: {
            if (b.Slider2(kk::fig_begin, kk::fig_end, "Диапазон", 0, db[kk::max_h])) {
                uint16_t h = db[kk::fig_begin];
                uint16_t step = (db[kk::fig_end].toInt() - db[kk::fig_begin].toInt()) / BALLS_AMOUNT;

                for (uint16_t s = 0; s < segs.length(); s++) {
                    for (uint16_t i = 0; i < segs.length(s); i++) {
                        balls.setTarget(segs[s][i], h);
                        h += step;
                    }
                }
                balls.runTarget();
            }
        } break;

        case Tabs::Pyramid: {
            if (b.Slider2(kk::fig_begin, kk::fig_end, "Диапазон", 0, db[kk::max_h])) {
                uint16_t h = db[kk::fig_begin];
                uint16_t step = (db[kk::fig_end].toInt() - db[kk::fig_begin].toInt()) / HEX_SIZE;

                for (uint16_t s = 0; s < segs.length(); s++) {
                    for (uint16_t i = 0; i < segs.length(s); i++) {
                        balls.setTarget(segs[s][i], h);
                    }
                    h += step;
                }
                balls.runTarget();
            }
        } break;

        case Tabs::Sin: {
            b.Slider(kk::sin_step, "Шаг", 0, db[kk::max_h].toInt() / 2);
            if (b.Button("Start")) stoppped = false;
        } break;

        case Tabs::Noise: {
            b.Slider(kk::noise_step, "Шаг (скорость)", 10, 30);
            b.Slider(kk::noise_shift, "Сдвиг XY", 0, 30);
            b.Slider(kk::noise_ampli, "Амплитуда", 0, db[kk::max_h].toInt() / 2);
            if (b.Button("Start")) stoppped = false;
        } break;

        default: break;
    }

    // общее
    if (b.Slider(kk::max_speed, "Скорость", 0, 300, 1, "")) {
        balls.setSpeed(db[kk::max_speed]);
    }
    if (b.Button("STOP", sets::Colors::Red)) {
        motors.disable();
        balls.stop();
        stoppped = true;
    }
}

void update(sets::Updater& u) {
    u.update(H(height), balls.getPos(0));
}

void setup() {
    Serial.begin(115200);
    SPI.begin();
    LittleFS.begin();
    db.begin();

    db.init(kk::wifi_ssid, "");
    db.init(kk::wifi_pass, "");
    db.init(kk::max_h, 1500);
    db.init(kk::max_speed, 100);
    db.init(kk::fig_begin, 0);
    db.init(kk::fig_end, 1000);
    db.init(kk::sin_step, 50);
    db.init(kk::noise_step, 50);
    db.init(kk::noise_shift, 50);
    db.init(kk::noise_ampli, 50);

    sett.begin();
    sett.onBuild(build);
    sett.onUpdate(update);

    WiFiConnector.onConnect([]() {
        Serial.print("Connected: ");
        Serial.println(WiFi.localIP());
    });
    WiFiConnector.onError([]() {
        Serial.print("Error. Start AP: ");
        Serial.println(WiFi.softAPIP());
    });

    WiFiConnector.setName(PROJECT_NAME);
    WiFiConnector.connect(db[kk::wifi_ssid], db[kk::wifi_pass]);

    balls.setSpeed(db[kk::max_speed]);
    balls.useRealDistance(false);
    balls.useShift(false);
    gridGen(matrix, segs, HEX_SIZE);
}

void loop() {
    sett.tick();
    WiFiConnector.tick();
    balls.tick();

    if (!balls.running() && !stoppped) {
        static uint16_t counter;

        switch (tab) {
            case Tabs::Sin: {
                uint16_t maxh = db[kk::max_h];
                uint16_t step = db[kk::sin_step];

                for (uint16_t s = 0; s < segs.length(); s++) {
                    uint16_t h = maxh / 2 + step * sin(counter / 30.0 + s * 3.14 / segs.length());
                    for (uint16_t i = 0; i < segs.length(s); i++) {
                        balls.setTarget(segs[s][i], h);
                    }
                }
                balls.runTarget();
                counter++;
            } break;

            case Tabs::Noise: {
                uint16_t maxh = db[kk::max_h];
                uint16_t shift = db[kk::noise_shift];
                int16_t ampli = db[kk::noise_ampli];

                for (uint16_t y = 0; y < matrix.length(); y++) {
                    for (uint16_t x = 0; x < matrix.length(y); x++) {
                        if (!matrix[y][x]) continue;
                        uint16_t h = maxh / 2 + ampli * (inoise8(counter, x * shift, y * shift) - 128) / 128;
                        balls.setTarget(matrix[y][x] - 1, h);
                    }
                }
                balls.runTarget();
                counter += db[kk::noise_step].toInt();
            } break;

            default: break;
        }
    }
}