/*
   Eclipse Paho MQTT-JS Utility
   by Elliot Williams for Hackaday article

   Hacked up by:  Mark McDermott
   On:            5/29/23
   Reason:        EE445L Lab 4E
   
*/

// ---------------------------------------
// Global variables
//

var client      = null;
var hour        = "";
var minute      = "";
var second      = "";
var ampm        = "";
var mil_time    = "";

// These are configs	
var hostname        = "192.168.0.143";
var port            = "9001";
var eid             = "mcdermot"
var clientId        = "mqtt_ee445l_" + eid;


// Subscribe (from board_2_webpage)

var hour_bd =   "mcdermot/b2w/hour"; 
var min_bd  =   "mcdermot/b2w/min";
var sec_bd  =   "mcdermot/b2w/sec"; 
var mil_bd  =   "mcdermot/b2w/mil"   

// Publish (from webpage_2_board) 
 
var w2b   =  "mcdermot/w2b";

// -----------------------------------------------------------------------
// This is called after the webpage is completely loaded
// It is the main entry point into the JS code

function connect(){
	// Set up the client
	client = new Paho.MQTT.Client(hostname, Number(port), clientId);
	
    console.info('Connecting to Server: Hostname: ', hostname, 
			    '. Port: ', port, '. Client ID: ', clientId);
    

	// set callback handlers
	client.onConnectionLost = onConnectionLost;
	client.onMessageArrived = onMessageArrived;

	// see client class docs for all the options
	var options = {
		onSuccess: onConnect, // after connected, subscribes
		onFailure: onFail     // useful for logging / debugging
	};
	// connect the client
	client.connect(options);
	console.info('Connecting...');
}


function onConnect(context) {
	console.log("Client Connected");
    
    // And subscribe to our topics	-- both with the same callback function
	options = {qos:0, onSuccess:function(context){ console.log("subscribed"); } }

	client.subscribe(hour_bd,   options);
	client.subscribe(min_bd,    options);
	client.subscribe(sec_bd,    options);
    client.subscribe(mil_bd,    options);

}

function onFail(context) {
	console.log("Failed to connect");
}

function onConnectionLost(responseObject) {
	if (responseObject.errorCode !== 0) {
		console.log("Connection Lost: " + responseObject.errorMessage);
		window.alert("Someone else took my websocket!\nRefresh to take it back.");
	}
}

// Here are the two main actions that drive the webpage:
//  handling incoming messages and the toggle button.

// Updates the webpage elements with new data, and 
//  tracks the display LED status as well, 
//  in case multiple clients are toggling it.
// function onMessageArrived(message) {
// 	console.log(message.destinationName, message.payloadString);

// 	// Update element depending on which topic's data came in
// 	if (message.destinationName == hour_bd){ 
// 		var hour_heading = document.getElementById("HOUR_DISPLAY");
// 		hour_heading.innerHTML = message.payloadString;
// 	} 
    
//     else if (message.destinationName == min_bd) {
// 		var min_heading = document.getElementById("MIN_DISPLAY");
// 		min_heading.innerHTML = "MINUTE: " + message.payloadString + "%";
// 	} 
    
//     else if (message.destinationName == status_topic) {
// 		var status_heading = document.getElementById("led_status");
// 		status_heading.innerHTML = "LED Status: " + message.payloadString;
// 		// Accomodates one or two byte "on" commands.  Anything else is off.
// 		if (message.payloadString == "on" || message.payloadString == "o"){
// 			led_is_on = true;
// 		} else {
// 			led_is_on = false;
// 		}
// 	}
// }

function onMessageArrived(message) {
	console.log(message.destinationName, message.payloadString);

	// Update element depending on which topic's data came in
	if (message.destinationName == hour_bd){ 
		hour = message.payloadString;
        hour = update(hour);
        console.log("Hour: ", hour);
	} 
    
    else if (message.destinationName == min_bd) {
		minute = message.payloadString;
        minute = update(minute);
        console.log("Minute: ", minute);
	} 
    
    else if (message.destinationName == sec_bd) {
		second = message.payloadString;
        second = update(second);
        console.log("Second: ", second);
	} 
    
    else if (message.destinationName == mil_bd) {
		mil_time = message.payloadString;
        console.log("MIL-TIME: ", mil_time);
	} 

}
// -----------------------------------------------------------------------
// Provides the button logic that toggles the AM/PM mode
// Triggered by pressing the HTML button "AM/PM"
//
// function toggle_am_pm() {

//     // --------------------------------------------------------------------
// 	// Send message to board
// 	//
//     var payload = "0";
// 	message = new Paho.MQTT.Message(payload);
// 	message.destinationName = w2b;
// 	message.retained = true;
// 	client.send(message);
// 	console.info('sending: ', payload);
// }

// -----------------------------------------------------------------------
// Provides the button logic that toggles the miltary mode
// Triggered by pressing the HTML button "12/24"
//
function toggle_mil_time(){
	var payload = "1";
	message = new Paho.MQTT.Message(payload);
	message.destinationName = w2b;
	message.retained = true;
	client.send(message);
	console.info('sending: ', payload);
}

// -----------------------------------------------------------------------
// Provides the button logic that increments the HOUR
// Triggered by pressing the HTML button "HOUR++"
//
function inc_hour(){
    // --------------------------------------------------------------------
	// Send message to board
	//
	var payload = "2";
    message = new Paho.MQTT.Message(payload);
	message.destinationName = w2b;
	message.retained = true;
	client.send(message);
	console.info('sending: ', payload);
}

// -----------------------------------------------------------------------
// Provides the button logic that decrements the HOUR
// Triggered by pressing the HTML button "HOUR--"
//
function dec_hour(){
    // --------------------------------------------------------------------
	// Send message to board
	//
	var payload = "3";
    message = new Paho.MQTT.Message(payload);
	message.destinationName = w2b;
	message.retained = true;
	client.send(message);
	console.info('sending: ', payload);
}

// -----------------------------------------------------------------------
// Provides the button logic that increments the MIN
// Triggered by pressing the HTML button "MIN++"
//
function inc_min(){
    // --------------------------------------------------------------------
	// Send message to board
	//
	var payload = "4";
    message = new Paho.MQTT.Message(payload);
	message.destinationName = w2b;
	message.retained = true;
	client.send(message);
	console.info('sending: ', payload);
}

// -----------------------------------------------------------------------
// Provides the button logic that decrements the MIN
// Triggered by pressing the HTML button "MIN--"
//
function dec_min(){
    // --------------------------------------------------------------------
	// Send message to board
	//
	var payload = "5";
    message = new Paho.MQTT.Message(payload);
	message.destinationName = w2b;
	message.retained = true;
	client.send(message);
	console.info('sending: ', payload);
}

// -----------------------------------------------------------------------
// Provides the button logic that increments the SEC
// Triggered by pressing the HTML button "SEC++"
//
function inc_sec(){
    // --------------------------------------------------------------------
	// Send message to board
	//
	var payload = "6";
    message = new Paho.MQTT.Message(payload);
	message.destinationName = w2b;
	message.retained = true;
	client.send(message);
	console.info('sending: ', payload);
}

// -----------------------------------------------------------------------
// Provides the button logic that decrements the SEC
// Triggered by pressing the HTML button "SEC--"
//
function dec_sec(){
    // --------------------------------------------------------------------
	// Send message to board
	//
	var payload = "7";
    message = new Paho.MQTT.Message(payload);
	message.destinationName = w2b;
	message.retained = true;
	client.send(message);
	console.info('sending: ', payload);
}

function Board_Time() {

    // Creating object of the Date class
    ;
  
    // Variable to store AM / PM
    
    var period = "";
  
    // Assigning AM / PM according to current hour
    if ((hour <= 11) && (mil_time == 0))
    {
        period = "AM";
    } 
    else if (hour >= 13 && (mil_time == 0))
    {
        period = "PM";
        hour = hour - 12;
    }
    else if (mil_time == 1)
    {
        period ="";
    }
    
  
    // Adding time elements to the div
    document.getElementById("board-clock").innerText = hour + " : " + minute + " : " + second + " " + period;
  
    // Set Timer to 1 sec (1000 ms)
    setTimeout(Board_Time, 1000);
  }

  function update(t) {
    if (t < 10) {
      return "0" + t;
    }
    else {
      return t;
    }
  }
  
  Board_Time();