
#include "MFRC522/MFRC522.h"
// This is library is available in the online build library.


#define SS_PIN A2
#define RST_PIN D2
#define GRN_PIN D5
#define YEL_PIN D4
#define RED_PIN D3

String new_card = String(""); //used for Card UID
String scan_actor = ""; // Used for the actor in the xAPI statement
String scan_verb = ""; // used to build the verb for the xAPI statement
String scan_obj = ""; //used for the object in the xAPI statement
String scan_stmt = ""; // xAPI statement to be sent
String result = ""; // the resulting statement ID, or error, from the LRS

// I broke up the three parts of the statement to make working with this easier.
// You do not have to do this.


TCPClient lrs; // Crete TCP connection to Learning Record Store
byte server[] = {192, 168, 1, 1}; // local LRS IP address

MFRC522 mfrc522(SS_PIN, RST_PIN);	// Create MFRC522 instance.



void setup() {
    pinMode(GRN_PIN, OUTPUT); //set the pin for digital output to turn the LED on/off
    pinMode(YEL_PIN, OUTPUT); //set the pin for digital output to turn the LED on/off
    pinMode(RED_PIN, OUTPUT); //set the pin for digital output to turn the LED on/off
	Serial.begin(9600);	// Initialize serial communications with the PC for debugging
	mfrc522.setSPIConfig(); // intialize the miFare card scanner

	mfrc522.PCD_Init();	// Init MFRC522 card
	Serial.println("Scan a MIFARE Classic PICC to demonstrate Value Blocks.");
}

void loop() {
    digitalWrite(GRN_PIN, LOW); // turn off the led
    digitalWrite(YEL_PIN, LOW); // turn off the led
    digitalWrite(RED_PIN, LOW); // turn off the led


	// Look for new cards
	if ( ! mfrc522.PICC_IsNewCardPresent()) {
		return;
	}

	// Select one of the cards
	if ( ! mfrc522.PICC_ReadCardSerial()) {
		return;
	}

    new_card = String("");
    result = String("");

    // This loop will build the new_card string with the UID of the MiFare card
	for (byte i = 0; i < mfrc522.uid.size; i++) {
        new_card.concat(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
		new_card.concat(String(char(mfrc522.uid.uidByte[i]),HEX));
	}

   	digitalWrite(YEL_PIN, HIGH); // Turn on the LED to signal that the scanner is "thinking"


// Build the xAPI statement
// Object will be built using card UID
        scan_obj = "\"object\":{\"id\":\"http://omnesnet.com/cards/";
        scan_obj += new_card.toUpperCase();
        scan_obj += "\",\"definition\":{\"name\":{\"en-US\":\"";
        scan_obj += new_card.toUpperCase();
        scan_obj += "\"},\"description\":{\"en-US\":\"";
        scan_obj += new_card.toUpperCase();
        scan_obj += "\"}},\"objectType\":\"Activity\"}";
        Serial.println(scan_obj);

// Verb is "scanned"
        scan_verb = "\"verb\":{\"id\":\"http://omnesnet.com/xapi/scanned\",\"display\":{\"en-US\": \"scanned\"}}";
        Serial.println(scan_verb);

// Actor is the door at which the card was scanned
        scan_actor = "\"actor\":{\"mbox\":\"mailto:front_door@omnesnet.com\",\"objectType\":\"Agent\"}";
        Serial.println(scan_actor);

// Put the parts together into the statement
// There is no timestamp because I don't feel like building it.
        scan_stmt = "{";
        scan_stmt += scan_actor;
        scan_stmt += ",";
        scan_stmt += scan_verb;
        scan_stmt += ",";
        scan_stmt += scan_obj;
        scan_stmt += "}";
        Serial.println(scan_stmt);
        Serial.println(scan_stmt.length());


        Serial.println("connecting...");
        if (lrs.connect(server, 80)) {  // Open the connection to the server
            Serial.println("connected");
            lrs.println("POST /data/xAPI/statements HTTP/1.1");
            lrs.println("Host: 192.168.1.1"); // you must send the server addy as part of the POST
            lrs.println("Content-Type: text/xml");
            lrs.println("X-Experience-API-Version: 1.0.1"); // set the version of xAPI used
            lrs.println("Authorization: Basic Y2Q3MjFlODc4MjM1MDhmYzY5N2M4OTg5NzU1YzAyMzEzNDlhYjZhNTozNDQzYmVmMDFhYjIzZjUxZjRlMGU5MDQ1MWU1MDk4MDdjZTlmNzg1"); // you need to encrypt the password yourself
            lrs.println("Cache-Control: no-cache");
            lrs.print("Content-Length: ");
            lrs.println(scan_stmt.length());  // Character count of the payload.  In this case, just the xAPI statement.
                                              // You must send this so the system knows how much data to expect in the statement.
                                              // You must add this manually!

            lrs.println();   // honestly not sure why this has to be here.  I guess the system needs to take a REST.  Get it... cause it's a RESTFul... api...
            lrs.println(scan_stmt); // Send the statement built above
            Serial.println("Statment sent");
 //           delay (12000);  // wait until the LRS has time to process the request.  On a raspberry pi Zero, this is about 12 seconds or so.
            while (lrs.available()) {  // collect the result, which should only be the statement ID
                char c = lrs.read();
                Serial.print(c);  // print out the statement ID/errors for diagnostics.
            }
            Serial.println();
            lrs.stop(); // close down the communication with the server.

// Send query to see if this card is allowed. It doesn't matter if the card HAD been approved in the past.
// We want to know what the last statement said.  So we'll sent the following parameters:
// Agent: admin@omnesnet.com
// activity: http://omnesnet.com/card/new_card.toUpperCase() - search for THAT card
// limit 1 - only show the last statement to meet the above


           if (lrs.connect(server, 80)) {  // Open the connection to the server
                Serial.println("connected");
                lrs.print("GET /data/xAPI/statements?format=exact&agent=%7B%22mbox%22%3A%22mailto%3Aadmin%40omnesnet.com%22%7D&limit=1&activity=http://omnesnet.com/cards/");
                lrs.print(new_card.toUpperCase());
                lrs.println(" HTTP/1.1");  //these three lines build the actual query, including the card ID
                lrs.println("Host: 192.168.1.1"); // you must send the server addy as part of the GET
                lrs.println("SOAPaction: xyz");
                lrs.println("Content-Type: text/xml");
                lrs.println("X-Experience-API-Version: 1.0.1"); // specify what version of xAPI is being used
                lrs.println("Authorization: Basic Y2Q3MjFlODc4MjM1MDhmYzY5N2M4OTg5NzU1YzAyMzEzNDlhYjZhNTozNDQzYmVmMDFhYjIzZjUxZjRlMGU5MDQ1MWU1MDk4MDdjZTlmNzg1"); // you need to encrypt the password yourself
                lrs.println("Cache-Control: no-cache");
                lrs.println(); // again with the blank line to close out the GET
                Serial.println("Query Sent");
                delay(5000); // delay 5 seconds so the LRS has time to run the query and respond in full
                while (lrs.available()) { // listen for the resulting statement
                    char c = lrs.read();
                    result += c; // build the string Result with the statement and header information
                    Serial.print(c);
                }
                Serial.println();
            lrs.stop(); // Close down the communication with the server
            }
        }
        else { // If you couldn't open communication with the server, show an error
                Serial.println("connection failed");
        }


// Look at the resulting statement from the query.
// If the statement contains the word "approved"
// then let the card through.  If not, denied.

    if (result.indexOf("approved") > 0 ){
        Serial.print("Found it: ");
        Serial.println(result.indexOf("approved"));
        digitalWrite(YEL_PIN, LOW); // Turn off the Yellow LED
        digitalWrite(GRN_PIN, HIGH); // Turn on the Green LED to indicate success
        delay(5000); // give the person five seconds to notice the light/result
    }
    else {
        Serial.println("DENIED!!");
        digitalWrite(YEL_PIN, LOW); // Turn off the Yellow LED
        digitalWrite(RED_PIN, HIGH); // Turn on the Red LED to indicate the card was not accepted
        delay(5000); // give the person five seconds to notice the light/result
    }

 //put a border on it to separate the results
	Serial.println("=======================================");
	Serial.println();
	Serial.println();

//	}

} // FIN loop
