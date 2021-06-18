// Initialise websockets
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

// Canvas objects and context objects for those canvases 
var pcanvas=document.getElementById("pcanvas");
var pctx=pcanvas.getContext("2d");
var rcanvas=document.getElementById("rcanvas");
var rctx=rcanvas.getContext("2d");

// Image objects
var imageSide=document.createElement("img");
var imageBack=document.createElement("img");

// Calibration and calculation objects
var rollDeviation = 0;
var pitchDeviation = 0; 
var wheelbase = 2000;
var drawbar = 4000;
var calibration;
var rollValue = "0";
var pitchValue = "0";
var heightAdjust = "0";
var jockeyAdjust = "0";

// On page load resize canvas objects to fit cards in grid
imageSide.onload=function(){
    cardDim = document.getElementById("pitchCard").clientWidth;
    pcanvas.width = cardDim-3;
    pcanvas.height = cardDim-3;
    pctx.drawImage(imageSide,0,0,pcanvas.width,pcanvas.height);
    cardDim = document.getElementById("rollCard").clientWidth;
    rcanvas.width = cardDim-3;
    rcanvas.height = cardDim-3;
    rctx.drawImage(imageBack,0,0,rcanvas.width,rcanvas.height);
}

// Load images for roll and pich view -
// square images 512px x 512px with transparent background work best
imageSide.src="van_side.png";
imageSide.class="image1";
imageBack.src="van_back.png";
imageBack.class="image1";

// Add listener for page load 
window.addEventListener('load', onload);

// Intialise websocket on page load
function onload(event) {
    initWebSocket();
}

// Function requests update by sending message
// "getValues"
function getValues(){
    websocket.send("getValues");
}

// Set up websocket events
function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

// This actually does most of the work, when websocket is established
// this function wil request an update every 300 msecs, will only stop
// when page is refreshed or closed.
function onOpen(event) {
    console.log('Connection opened');
    let timerID = setInterval(getValues, 300 );   
} 

// Websocket close event
function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

// Validate form data before sending to server
function wsSubmitForm() {
    // Display warning message - because we will zero the angles to their current settings if 
    // box checked else just change the other values if not
    if (document.forms["calibration"]["zeroAngles"].checked)  {
        confirmationMessage = "Warning: Before clicking OK, ensure device is firmly mounted to a" +
            " known level surface.  Displayed angles will be zeroed to their current values.";
    } else {
        confirmationMessage = "Click OK to change roll and pitch zones, drawbar and wheelbase lengths." +
            " Displayed angles will not be zeroed.";
    }
    if (confirm(confirmationMessage)) {
        // build JSON string of deviation, wheelbase and zero angle flag  
        calibration = '{"rollDeviation":"' + document.forms["calibration"]["rollDeviation"].value +
            '","pitchDeviation":"' + document.forms["calibration"]["pitchDeviation"].value +
            '","wheelbase":"' +  document.forms["calibration"]["wheelbase"].value + 
            '","drawbar":"' + document.forms["calibration"]["drawbar"].value +
            '","zeroAngles":"' + document.forms["calibration"]["zeroAngles"].checked + '"}';
        // send to server
        websocket.send(calibration);
        document.getElementById("zero").checked = false;
    } else {
        // We clicked Cancel
        alert("No calibration done");
        return;
    }
}

// Generic message event as we only ever expect one type of
// message i.e. a JSON string in the format:
// {"rollValue":"nn.n","pitchValue":"nn.n","rollDeviation":"nn.n","pitchDeviation":"nn.n","wheelbase":"nnn"}
function onMessage(event) {
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    // Extract angles from JSON and write angles
    // note we write 0.0 for angles between -0.1 and 0.1 because they display with 
    // one decimal place - avoids value -0.0 displaying
    if ((parseFloat(myObj.rollValue) > -0.1) && (parseFloat(myObj.rollValue) < 0.1)) {
        rollValue = "Roll Angle: 0.0º";
    } else {
        rollValue = "Roll Angle: " + myObj.rollValue + "º";
    }
    if ((parseFloat(myObj.pitchValue) > -0.1) && (parseFloat(myObj.pitchValue) < 0.1)) {
        pitchValue = "Pitch Angle: 0.0º";
    } else {
        pitchValue = "Pitch Angle: " + myObj.pitchValue + "º";
    }
    // Draw rotated image - roll is reversed as we are looking from back
    drawPitchRotated(parseFloat(myObj.pitchValue));
    drawRollRotated(-parseFloat(myObj.rollValue));
    // Populate form values if they have changed
    // extract previous calibration data from JSON
    // also update them on the form
    if (wheelbase !== myObj.wheelbase) {
        wheelbase = myObj.wheelbase;
        document.getElementById("wbase").value = wheelbase;
    }
    if (drawbar !== myObj.drawbar) {
        drawbar = myObj.drawbar;
        document.getElementById("dbar").value = drawbar;
    }
    if (rollDeviation !== myObj.rollDeviation) {
        rollDeviation = myObj.rollDeviation;
        document.getElementById("rollD").value = rollDeviation;
    }
    if (pitchDeviation !== myObj.pitchDeviation) {
        pitchDeviation = myObj.pitchDeviation;
        document.getElementById("pitchD").value = pitchDeviation;
    }
    // Height adjustment calc
    x = wheelbase * Math.sin(myObj.rollValue*Math.PI/180);
    if (x < 0) {
        heightAdjust =  "Raise right " +  Math.round(Math.abs(x)) + " cm";
    } else {
        heightAdjust = "Raise left " +  Math.round(Math.abs(x)) + " cm";
    }
    // Jockey wheel height adjustment calc
    x = drawbar * Math.sin(myObj.pitchValue*Math.PI/180);
    if (x < 0) {
        jockeyAdjust = "Raise " +  Math.round(Math.abs(x)) + " cm";
    } else {
        jockeyAdjust = "Lower " +  Math.round(Math.abs(x)) + " cm";
    }
}

// Draw image on canvas for pitch (side view)
function drawPitchRotated(degrees){
    // Clear canvas, set canvas backgroud if we are "level".  
    // Deviation values are saved in SPIFFS and passed from ESP to client
    pctx.textAlign="center";
    pctx.clearRect(0,0,pcanvas.width,pcanvas.height);
    if ((degrees >= -pitchDeviation) && (degrees <= pitchDeviation)) {
      pctx.fillStyle="green";
    } else {
      pctx.fillStyle="ivory";
    }
    // Values written to canvas before we rotate
    pctx.fillRect(0,0,pcanvas.width,pcanvas.height);
    pctx.font = pcanvas.width/16 + "px Arial";
    pctx.fillStyle="darkblue";
    // Angle
    pctx.fillText(pitchValue,pcanvas.width/2,pcanvas.height*0.1);
    // Height adjustment
    pctx.fillText(jockeyAdjust,pcanvas.width/2,pcanvas.height*0.95);
    pctx.save();
    // Set origin of canvas to centre of image because rotate 
    // method rotates around origin, normally top left
    pctx.translate(pcanvas.width/2,pcanvas.height/2);
    // Rotate canvas by angle in radians
    pctx.rotate(degrees*Math.PI/180);
    // Draw image on canvas, starting at top left of canvas
    // If we used 0,0 as draw coordinates it would draw from
    // centre (the new origin we set above)
    pctx.drawImage(imageSide,-pcanvas.width/2,-pcanvas.height/2,pcanvas.width,pcanvas.height);
    pctx.restore();
}

// Draw image on convas for roll (back view)
// All other comments as per pitch function above
function drawRollRotated(degrees){
    rctx.textAlign="center";
    rctx.clearRect(0,0,rcanvas.width,rcanvas.height);
    if ((degrees >= -rollDeviation) && (degrees <= rollDeviation)) {
      rctx.fillStyle="green";
    } else {
      rctx.fillStyle="ivory";
    }
    rctx.fillRect(0,0,rcanvas.width,rcanvas.height);
    rctx.font = rcanvas.width/16 + "px Arial";
    rctx.fillStyle="darkblue";
    rctx.fillText(rollValue,rcanvas.width/2,rcanvas.height*0.1);
    rctx.fillText(heightAdjust,rcanvas.width/2,rcanvas.height*0.95);
    rctx.save();
    rctx.translate(rcanvas.width/2,rcanvas.height/2);
    rctx.rotate(degrees*Math.PI/180);
    rctx.drawImage(imageBack,-rcanvas.width/2,-rcanvas.height/2,rcanvas.width,rcanvas.height);
    rctx.restore();
}
