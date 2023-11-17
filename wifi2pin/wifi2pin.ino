/*
 * Connects to Smartphone Hotspot per Wifi, then sets up a server the app ESPController can connect to.
 * 
 * LED D0 (Lower LED):  Is On when the server is waiting for a new connection
 * LED D4 (Upper LED): Is On when the server sets up the Wifi Connection and the Server and when the engine is on
 * 
 */
#include <ESP8266WiFi.h>
#include <string.h>
#include "config.h" 

#define NUM_PINS 25

int mode[NUM_PINS];

enum _state {
	CONNECTED, DISCONNECTED
} state;


//#define DEBUG         //Comment this line if the server should not print any debug messages

#define ACK 6 //acknowledge byte, sent every second so the client knows the server didnt time out
#define VERSION "1.0"

#define SERVER_PORT 1337


#define LED_BLINK_RATE 3 

class LEDManager {
	int pin;
	bool isOn;
	int lastCycleSwitched;

	LEDManager(pin) :
		pin(pin), isOn(false), lastCycleSwitched(0) {
		pinMode(pin, OUTPUT);
	}

	void on() {
		isOn = true;
		digitalWrite(this->pin, 1);
	}
	void off() {
		isOn = true;
		digitalWrite(this->pin, 1);
	}

	void toggle() {
		if (isOn)
			off();
		else
			on();
	}

	void blink(int cycleNr, int cycleRate) {
		// TODO
	}

}

#define PIN_LED D0 //Lower LED, on when LOW (default:LOW)
#define PIN_LED2 D4 //Upper LED, on when LOW(default:LOW)

void flush(WiFiClient client);
void findClient();


bool isLedOn = false, isLed2On = false;
WiFiServer server(SERVER_PORT);
WiFiClient client;
int loopsSinceACK = 0;

void setup() {
	// init pins
	pinMode(PIN_LED, OUTPUT);
	pinMode(PIN_LED2, OUTPUT);
	//TODO use LEDManager instead
  	for (int i = 0; i < NUM_PINS; i++)
    	mode[i] = -1;

	//delay(10);
#ifdef DEBUG  
    Serial.begin(19200);
#endif

  #if ACCESSPOINT
      IPAddress local_IP(192, 168, 0, 1);
      IPAddress gateway(192, 168, 9, 4); //TODO what is the gateway?
      IPAddress subnet(255, 255, 255, 0);

      #ifdef DEBUG
        Serial.print("Setting soft-AP configuration ... ");
      #endif
      bool ret = WiFi.softAPConfig(local_IP, gateway, subnet);
      #ifdef DEBUG
        Serial.println(ret ? "Ready" : "Failed!");
        Serial.print("Setting soft-AP.. ");
      #endif
      //delay(20);
      ret = WiFi.softAP(SSID, PASSWORD, 1, false);
    
      #ifdef DEBUG
        Serial.println(ret ? "Ready" : "Failed!");
        Serial.print("Soft-AP IP address = ");
        Serial.println(WiFi.softAPIP());
      #endif
  #else
        #ifdef DEBUG
            Serial.print("Connecting to ");
            Serial.print(ssid);
        #endif
        
        WiFi.begin(SSID, PASSWORD);
        delay(20);
        while (WiFi.status() != WL_CONNECTED) {
            delay(200);
            #ifdef DEBUG
                Serial.print(".");
            #endif
        }
        #ifdef DEBUG
            Serial.println("\nWiFi connected");
        #endif
        #ifdef DEBUG
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
        #endif
    #endif
    delay(20);
    // Start the server
    server.setNoDelay(true);
    server.begin();
  
    #ifdef DEBUG
        Serial.println("Server started");
    #endif

    digitalWrite(PIN_LED2, HIGH);

	state = DISCONNECTED;
}


void loop() {
	switch (state) {
		case DISCONNECTED:
			checkClientAvailable();
			if (client.connected()) {
				state = CONNECTED;
				#ifdef DEBUG
					Serial.print("Connected to Client at ");
					Serial.println(client.remoteIP());
				#endif
				// TODO make LED on
			}
			//TODO make LED blink
			return;
		case CONNECTED:
			// TODO check if client timed out 
    		loopsSinceACK++;
			if (loopsSinceACK > 2) { //client timed out
				#ifdef DEBUG
					Serial.println("Client timed out.");
				#endif
				client.stop();
				client = WiFiClient();
				return;
			}
			if (!client.connected() ) {
				state = DISCONNECTED;
				#ifdef DEBUG
					Serial.println("Client disconnected.");
				#endif
				// TODO make all outputs off?
				return;
			}
			// TODO check if another client wants to connect (server.available)
			// and reject the client; or connect, send error message and disconnect
    		// as the program allows only one Client to be connected at the same time
			//    WiFiClient::stopAllExcept(&client);
			// TODO send ack periodically
    		//client.write(ACK);
			break;
	}

	// this part is only called for state connected

    delay(2);

	// process client messages
	if (client.available()) {           
		char action = client.read(); //first char says left or right motor

		//Test if it was just an ACKnowledge byte sent to confirm the connection didnt timeout
		// TODO just ignore ACK and instead count cycles until client.available?
		// this would mean client/server can avoid ACK if another piece of data is sent
		if (action == ACK) {
			loopsSinceACK = 0;
		} else {
			// TODO find right functions
			int pin = client.parseInt();
			String value = client.readStringUntil('\n');
			value.trim();
			value.toLowerCase();

			switch(action) {
			case 'r':
				if (mode[pin] == 'r' || mode[pin] == 'p') {
					client.printf("IN [Pin %d]: %2d\n", pin, digitalRead(pin));
				} else {
					client.printf("[ERROR] Pin %2d is not set to input\n", pin);
				}
				break;
			case 'w':
				if (mode[pin] == 'w') {
					// TODO value checking? PWM?
					int outVal = atoi(value.c_str());
					digitalWrite(pin, outVal);
					client.printf("OUT [Pin %2d]: %d\n", pin, outVal);
				} else {
					client.printf("[ERROR] Pin %d is not set to output\n", pin);
				}
				break;
			case 'p':
				if (value.startsWith("in")) {
					pinMode(pin, INPUT);
					client.printf("PINMODE [Pin %d]: input\n", pin);
					mode[pin] = 'r';
				} else if (value.startsWith("out")) {
					pinMode(pin, OUTPUT);
					client.printf("PINMODE [Pin %d]: output\n", pin);
					mode[pin] = 'w';
				} else if (value.endsWith("pullup")) {
					pinMode(pin, INPUT_PULLUP);
					client.printf("PINMODE [Pin %d]: input_pullup\n", pin);
					mode[pin] = 'p';
				} else {
					client.printf("[ERROR] Pin mode %s invalid\n", value.c_str());
				}
				break;
			default:
				Serial.printf("[ERROR] Command invalid\n");
			}
			// TODO necessary?
			flush(client); 
		}

	}
    
}


void checkClientAvailable() {
	client = server.available();
	if (client) {
    	client.printf("ESP_SERVER %s\n", VERSION);
		bool clientApproved = false;
		for (int i = 0; i < 200; ++i) { //Wait 1 Second for Message By client
			delay(5);
			if (client.available()) {
				String response = client.readStringUntil('\n');
				Serial.println(response);
				if (response.equals(String("ESP_CLIENT ") + VERSION)) {
					clientApproved = true;
				}
				break;
			}
		}
		if (!clientApproved) {
			client.printf("Client does not match server, requires ESP_CLIENT %s\n", VERSION);
			// reset client
			client.stop();
			client = WifiClient();
		} else {
			return true;
		}
	}
	return false;
}


void flush(WiFiClient client) {
    char c;
    do {
        c = client.read();
    } while (c != '\n' && c != -1);
}
