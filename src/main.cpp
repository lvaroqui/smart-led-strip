#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiNINA.h>

#include "secret.h"

#define WHITE 12
#define RED 3
#define GREEN 2
#define BLUE 10

WiFiServer server(80);

char MAC_ADDRESS[32];

struct Color {
    Color() = default;
    Color(float r, float g, float b, float w) : r(r), g(g), b(b), w(w) {}

    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b && w == other.w;
    }
    bool operator!=(const Color& other) const { return !operator==(other); }

    Color operator+(const Color& other) const {
        return Color(r + other.r, g + other.g, b + other.b, w + other.w);
    }
    Color operator-(const Color& other) const {
        return Color(r - other.r, g - other.g, b - other.b, w - other.w);
    }
    Color operator*(const Color& other) const {
        return Color(r * other.r, g * other.g, b * other.b, w * other.w);
    }
    Color operator/(const Color& other) const {
        return Color(r / other.r, g / other.g, b / other.b, w / other.w);
    }

    Color operator+(float val) const {
        return Color(r + val, g + val, b + val, w + val);
    }
    Color operator-(float val) const {
        return Color(r - val, g - val, b - val, w - val);
    }
    Color operator*(float val) const {
        return Color(r * val, g * val, b * val, w * val);
    }
    Color operator/(float val) const {
        return Color(r / val, g / val, b / val, w / val);
    }

    float r = 0, g = 0, b = 0, w = 0;

    void apply() {
        auto convert = [=](float val) {
            return constrain(round(val * 4095.f), 0, 4095);
        };
        analogWrite(RED, convert(r));
        analogWrite(GREEN, convert(g));
        analogWrite(BLUE, convert(b));
        analogWrite(WHITE, convert(w));
    }
};

struct State {
    bool isOn = false;
    Color target;
    Color current;
    unsigned long transitionTime = 0;
} state;

void setup() {
    // check for the WiFi module:
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("Communication with WiFi module failed!");
        // don't continue
        while (true)
            ;
    }

    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
        Serial.println("Please upgrade the firmware");
    }

    int status = WL_IDLE_STATUS;  // the WiFi radio's status
    // attempt to connect to WiFi network:
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(SECRET_SSID);
        // Connect to WPA/WPA2 network:
        status = WiFi.begin(SECRET_SSID, SECRET_PASS);
        delay(1000);
    }

    // you're connected now, so print out the data:
    Serial.print("You're connected to the network");

    // print your board's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    byte mac[6];
    WiFi.macAddress(mac);
    sprintf(MAC_ADDRESS, "%02X:%02X:%02X:%02X:%02X:%02X", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
    Serial.print("MAC: ");
    Serial.println(MAC_ADDRESS);

    server.begin();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    analogWriteResolution(12);
}

enum class HttpMethod {
    GET,
    POST,
    UNKNOWN
};

const unsigned int LOOP_TIME = 10;  // ms

const Color off(0, 0, 0, 0);

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi lost, reseting arduino in 10 seconds");
        delay(10000);
        NVIC_SystemReset();
    }

    WiFiClient client = server.available();

    static unsigned int prev = 0;
    const unsigned long time = millis();
    const unsigned int diff = time - prev;

    if (client) {
        size_t lineNo = 0;
        HttpMethod method = HttpMethod::UNKNOWN;
        char route[20];

        // Parse Headers
        while (client.available()) {
            String line = client.readStringUntil('\r');
            client.read();  // Discard \n

            if (lineNo == 0) {
                if (line.startsWith("GET")) {
                    method = HttpMethod::GET;
                } else if (line.startsWith("POST")) {
                    method = HttpMethod::POST;
                }

                const char* routeBegin = strchr(line.c_str(), ' ') + 1;
                size_t i = 0;
                while (i < sizeof(route) && routeBegin[i] != '\0' && routeBegin[i] != ' ') {
                    route[i] = routeBegin[i];
                    i++;
                }
                route[i] = '\0';
            }

            if (line.length() == 0) {
                // End of headers
                break;
            }

            lineNo++;
        }

        if (method == HttpMethod::POST && strcmp(route, "/command") == 0) {
            StaticJsonDocument<1024> req;
            StaticJsonDocument<1024> res;
            deserializeJson(req, client);

            const char* method = req["method"];
            auto param = req["param"];

            if (strcmp(method, "set_rgbw") == 0) {
                if (param["time"]) {
                    state.transitionTime = param["time"].as<unsigned long>();
                } else {
                    state.transitionTime = 1000;
                }

                state.target.r = constrain(param["r"], 0.f, 1.f);
                state.target.g = constrain(param["g"], 0.f, 1.f);
                state.target.b = constrain(param["b"], 0.f, 1.f);
                state.target.w = constrain(param["w"], 0.f, 1.f);
            } else if (strcmp(method, "set_power") == 0) {
                state.isOn = param["value"];
                if (!state.isOn || state.transitionTime < 1000) {
                    state.transitionTime = 1000;
                }
            } else if (strcmp(method, "get_status") == 0) {
                res["power"] = state.isOn;
                res["r"] = (float)state.target.r;
                res["g"] = (float)state.target.g;
                res["b"] = (float)state.target.b;
                res["w"] = (float)state.target.w;
            } else if (strcmp(method, "get_info") == 0) {
                res["mac"] = MAC_ADDRESS;
            } else {
                client.println("HTTP/1.1 404 Not Found");
                client.println("Connection: close");
                client.println();
            }

            client.println("HTTP/1.1 200 OK");
            client.println("Connection: close");
            client.println();
            if (!res.isNull()) {
                serializeJson(res, client);
            }
        } else {
            client.println("HTTP/1.1 404 Not Found");
            client.println("Connection: close");
            client.println();
        }

        client.stop();
    }

    const Color target = state.isOn ? state.target : off;

    if (state.transitionTime > 0) {
        float ratio = (float)LOOP_TIME / (float)state.transitionTime;
        ratio = min(max(ratio, 0.f), 1.0f);
        state.current = state.current * (1.f - ratio) + target * ratio;
        state.current.apply();

        if (state.transitionTime > diff) {
            state.transitionTime -= diff;
        } else {
            state.transitionTime = 0;
        }
    }

    prev = time;

    delay(LOOP_TIME);
}
