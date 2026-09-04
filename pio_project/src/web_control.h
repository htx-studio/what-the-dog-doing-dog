#ifndef WEB_CONTROL_H
#define WEB_CONTROL_H

#include <Arduino.h>
#include <WebServer.h>
#include "config.h"

class WebControlClass {
public:
    bool begin();
    void loop();

private:
    enum class ManualMotion : uint8_t {
        STOP = 0,
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
    };

    WebServer server_{80};
    String apName_;
    ManualMotion manualMotion_ = ManualMotion::STOP;
    uint8_t manualPwm_ = WEB_MANUAL_DEFAULT_PWM;
    uint32_t lastManualCommandMs_ = 0;
    bool started_ = false;

    void handleRoot();
    void handleStatus();
    void handleMotion();
    void handleCruise();
    void handleCruiseTurn();
    void sendResult(bool ok, const char* error = nullptr, int statusCode = 200);
    void stopManualMotion();
    void applyManualMotion(ManualMotion motion);
    const char* manualMotionName() const;
};

extern WebControlClass WebControl;

#endif
