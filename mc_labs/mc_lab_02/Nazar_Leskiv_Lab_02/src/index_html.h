#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; margin: 0px auto; padding-top: 30px; }
    .button {
      padding: 10px 20px;
      font-size: 24px;
      text-align: center;
      outline: none;
      color: #fff;
      background-color: #2f4468;
      border: none;
      border-radius: 5px;
      box-shadow: 0 6px #999;
      cursor: pointer;
    }
    .button:hover { background-color: #1f2e45; }
    .button:active {
      background-color: #1f2e45;
      box-shadow: 0 4px #666;
      transform: translateY(2px);
    }
    .led { width: 70px; height: 70px; margin: 10px; display: inline-block; border-radius: 50%; }
    #led1 { background-color: grey; }
    #led2 { background-color: grey; }
    #led3 { background-color: grey; }
  </style>
</head>
<body>
<h1>ESP8266 LED Control</h1>
<button class="button" id="ledButton">LED PUSHBUTTON</button>
<button class="button" id="clown">LED FRIEND</button>
<div>
  <div class="led" id="led1"></div>
  <div class="led" id="led2"></div>
  <div class="led" id="led3"></div>
</div>
<script>
  function updateLEDs() {
      fetch('/ledState')
        .then(response => response.text())
        .then(state => {
          document.getElementById('led1').style.backgroundColor = (state == 0) ? 'green' : 'grey';
          document.getElementById('led2').style.backgroundColor = (state == 1) ? 'yellow' : 'grey';
          document.getElementById('led3').style.backgroundColor = (state == 2) ? 'red' : 'grey';
        });
    }
    setInterval(updateLEDs, 500);
    function sendRequest(url) {
      fetch(url);
    }

  document.getElementById("ledButton").addEventListener("mousedown", () => {
    fetch('/buttonPress');
  });

  document.getElementById("ledButton").addEventListener("mouseup", () => {
    fetch('/buttonRelease');
  });

  let timer, wasHeld;

  const clownButton = document.getElementById("clown");
  clownButton.addEventListener("mousedown", hold);
  clownButton.addEventListener("mouseup", release);

  function hold() {
    wasHeld = false;
    timer = setTimeout(() => {
      wasHeld = true;
      fetch("/friendButtonPress");
    }, 500);
  }

  function release() {
    if (wasHeld) {
      fetch("/friendButtonRelease");
    };
    clearTimeout(timer);
    wasHeld = false;
  }
</script>
</body>
</html>
)rawliteral";

#endif // INDEX_HTML_H