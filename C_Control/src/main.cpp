// Placeholder - replaced later. Builds only to check libraries and shared/ include path.
#include <M5Unified.h>
#include "topics.h"
#include "secrets.h"

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setRotation(1);
    M5.Display.setTextSize(2);
    M5.Display.println("C: Control");
    M5.Display.setTextSize(1);
    M5.Display.println(BASE_ACQ.c_str());
}

void loop() { M5.update(); }
