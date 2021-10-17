#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiNINA.h>

#include "secret.h"

#define WHITE 12
#define RED 3
#define GREEN 2
#define BLUE 10

WiFiServer server(80);

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

    server.begin();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
}

struct Color {
    uint8_t r = 0, g = 0, b = 0, w = 0;

    void apply() {
        analogWrite(RED, r);
        analogWrite(GREEN, g);
        analogWrite(BLUE, b);
        analogWrite(WHITE, w);
    }
};

Color current;

enum class HttpMethod {
    GET,
    POST,
    UNKNOWN
};

void loop() {
    WiFiClient client = server.available();

    if (client) {
        Serial.println("Client available");

        size_t lineNo = 0;
        HttpMethod method = HttpMethod::UNKNOWN;

        // Parse Headers
        while (client.available()) {
            String line = client.readStringUntil('\r');
            client.read();  // Discard \n
            Serial.println(line);

            if (lineNo == 0) {
                if (line.startsWith("GET")) {
                    method = HttpMethod::GET;
                } else if (line.startsWith("POST")) {
                    method = HttpMethod::POST;
                }
            }

            if (line.length() == 0) {
                // End of headers
                break;
            }
        }

        if (method == HttpMethod::GET) {
            DynamicJsonDocument doc(1024);

            doc["r"] = current.r;
            doc["g"] = current.g;
            doc["b"] = current.b;
            doc["w"] = current.w;

            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: application/json");
            client.println("Connection: close");
            client.println();
            serializeJson(doc, client);
        } else if (method == HttpMethod::POST) {
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, client);

            current.r = doc["r"];
            current.g = doc["g"];
            current.b = doc["b"];
            current.w = doc["w"];

            current.apply();

            client.println("HTTP/1.1 200 OK");
            client.println("Connection: close");
            client.println();
        } else {
            client.println("HTTP/1.1 404 Not Found");
            client.println("Connection: close");
            client.println();
        }

        client.stop();
    }
}
