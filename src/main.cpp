#include <Arduino.h>
#include <WiFiNINA.h>

#include "secret.h"

#define WHITE 12
#define RED 3
#define GREEN 2
#define BLUE 10

WiFiServer server(80);

void setup() {
    //Initialize serial and wait for port to open:
    Serial.begin(9600);
    while (!Serial) {
        ;  // wait for serial port to connect. Needed for native USB port only
    }

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
}

void loop() {
    WiFiClient client = server.available();

    if (client) {
        Serial.println("Client available");

        char buffer[256];
        int index = 0;
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                if (c == '\n') {
                    buffer[index] = '\0';

                    int val = atoi(buffer);

                    analogWrite(WHITE, val);

                    // reset buffer index
                    index = 0;
                } else {
                    // append char at end of buffer
                    buffer[index] = c;
                    index++;
                }
            }
        }
        Serial.println("Client disconnected");
        client.stop();
    }
}
