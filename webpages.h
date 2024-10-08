const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML>
  <html>
  <head>
    <title>APRS WiFi Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <!-- <meta http-equiv="refresh" content="1">  -->
    <link rel="icon" href="data:,">
    <link rel="stylesheet" type="text/css" href="style.css">
    <script src="https://maps.googleapis.com/maps/api/js?key=%GoogleMapKey%" async defer></script>
  </head>
  <body>
  <div class="freqinfo" style="border:solid; border-radius: 1em; border-color: black; background-image: linear-gradient(blue, black); text-align:center">
    <div class="topnav">
      <h1>APRS WiFi Server</h1>
      <h6>%APRSINFO%, AGE:<span id="AGEINFO">%AGEINFO%</span><br></h6>
      <h2> </h2>     
    </div>

    <div class="row">
      <div style="line-height:10px" class="col-md-6 text-center">
        <h1 style="text-align:center;color:yellow"><span>GPS</span></h1>
        <h2 style="text-align:center;color:yellow"><span id="LATINFO">%LATINFO%</span>N, <span id="LONINFO">%LONINFO%</span>E</h2>
        <h2 style="text-align:center;color:red">Speed:<span id="SPEEDINFO">%SPEEDINFO%</span> KM/H</h6>
        <br>
      </div>
    </div>
    <hr>
    <div class="row">
      <div style="margin: auto; width:80%; height:600px" id="map">
    </div>    

  </div>
  <hr>
  <div class="topnav" style="background-color: lightblue; ">
    <table class="fwidth">
      <tbody>
        <tr>
          <td style="text-align:left">
            <h6><small>Copyright (c) Robert de Kok, PA2RDK</small></h6>
          </td>
          <td style="text-align:right">
            <a href="/settings"><button>Settings</button></a>
            <a href="/reboot"><button>Reboot</button></a>
          </td>
        </tr>
      </tbody>
    </table>
  </div>

  <script>
  	var map = undefined;
    var mapInitizialized = false;
    var marker = undefined;
    var path = [];

    function initMap(lat, lng){
      var mapOptions = {
        center: new google.maps.LatLng(lat, lng),
        zoom: 14,
			  mapTypeId: 'roadmap'
      };
      map = new google.maps.Map(document.getElementById("map"), mapOptions);
    }
	
    function moveToLocation(lat, lng){
      var center = new google.maps.LatLng(lat, lng);
		  map.panTo(center);
      
      if (typeof marker !== 'undefined'){
        path[0] = new google.maps.LatLng(marker.getPosition().lat(), marker.getPosition().lng());
        path[1] = new google.maps.LatLng(lat, lng);
        marker.setMap(null);
      }

		  addMarker(lat, lng);
      drawLine();	
	  }

    function addMarker(lat, lng){
        marker = new google.maps.Marker({
        position: new google.maps.LatLng(lat, lng),
        map: map,
        icon: "https://www.rjdekok.nl/icons8-car-48.png",
        title: "My APRS location"
    	});		
    }

    function drawLine(){
        var line = new google.maps.Polyline({
        path: path,
        strokeColor: "#FF0000",
        strokeOpacity: 1.0,
        strokeWeight: 1,
        geodesic: true,
        map: map
      });
    } 

    if (!!window.EventSource) {
      var source = new EventSource('/events');
      source.addEventListener('GPSINFO', function(e) {
        console.log("GPSINFO", e.data);
        document.getElementById("GPSINFO").innerHTML = e.data;
      }, false);

      source.addEventListener('LATINFO', function(e) {
        console.log("LATINFO", e.data);
        document.getElementById("LATINFO").innerHTML = e.data;
      }, false);

      source.addEventListener('LONINFO', function(e) {
        console.log("LONINFO", e.data);
        document.getElementById("LONINFO").innerHTML = e.data;
      }, false);

      source.addEventListener('SPEEDINFO', function(e) {
        console.log("SPEEDINFO", e.data);
        document.getElementById("SPEEDINFO").innerHTML = e.data;
      }, false);

      source.addEventListener('AGEINFO', function(e) {
        console.log("AGEINFO", e.data);
        document.getElementById("AGEINFO").innerHTML = e.data;
      }, false);

      source.addEventListener('DRAWMAP', function(e) {
        console.log("DRAWMAP", e.data);
        const location = e.data.split(",");
        if (!mapInitizialized){
          mapInitizialized = true;
          initMap(location[0],location[1]);
        }
        moveToLocation(location[0],location[1]);
      }, false);

      source.addEventListener('INITMAP', function(e) {
        alert("InitMap" + e.data);
        console.log("DRAWMAP", e.data);
        const location = e.data.split(",");
        initMap(location[0],location[1]);
      }, false);
    }
  </script>    

  </body>
  </html>
)rawliteral";


const char warning_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>APRS WiFi Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <!-- <meta http-equiv="refresh" content="1">  -->
  <link rel="icon" href="data:,">
  <link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>
  <div class="topnav">
    <h1>APRS WiFi Server</h1><br>
    <h2>Not allowed from external!</h2>
  </div>
  <hr>
</body>

<script>
  setTimeout(function(){
   window.location.href = "/"
  }, 5000);
</script>
</html>
)rawliteral";

const char settings_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML>
  <html>
  <head>
    <title>APRS WiFi Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <!-- <meta http-equiv="refresh" content="1">  -->
    <link rel="icon" href="data:,">
    <link rel="stylesheet" type="text/css" href="style.css">
  </head>
  <body>
  <div class="freqinfo" style="border:solid; border-radius: 1em; border-color: black; background-image: linear-gradient(blue, black); text-align:center">
    <div class="topnav">
      <h1>APRS WiFi Server</h1>
    </div>
    <div class="freqinfo">
      <h4 style="text-align:center;color:yellow;margin-top: 1.5em;margin-bottom: 1.5em;"><span>Settings</span></h4>
    </div>
  </div>
  <hr>
  <div style="border:solid; border-radius: 1em; border-color: black; background-image: linear-gradient(black, blue); text-align:center">
  <div class="divinfo">
    <form action="/store" method="get">

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              WiFi SSID: 
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="wifiSSID" value="%wifiSSID%">
            </td>
          </tr>
        </table>
      </div>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              WiFi Password:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="password" name="wifiPass" value="%wifiPass%">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              APRS IP:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="aprsIP" value="%aprsIP%">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              APRS Port:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="aprsPort" value="%aprsPort%" min="0" max="99999">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              APRS Password:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="aprsPassword" value="%aprsPassword%">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              APRS Server SSID:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="serverSsid" value="%serverSsid%" min="0" max="99">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              APRS Server refresh interval:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="aprsGatewayRefreshTime" value="%aprsGatewayRefreshTime%" min="0" max="86400">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Call:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="call" value="%call%">
              <input type="number" name="ssid" value="%ssid%" min="0" max="99">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Symbol:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="symbool" value="%symbool%">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Dest:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="dest" value="%dest%">
              <input type="number" name="destSsid" value="%destSsid%" min="0" max="99">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Path1:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="path1" value="%path1%">
              <input type="number" name="path1Ssid" value="%path1Ssid%" min="0" max="99">
            </td>
          </tr>
        </table>
      </div>
      <br>     

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Path2:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="path2" value="%path2%">
              <input type="number" name="path2Ssid" value="%path2Ssid%" min="0" max="99">
            </td>
          </tr>
        </table>
      </div>
      <br> 

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Comment:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="comment" value="%comment%">
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Interval:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="interval" value="%interval%" min="0" max="255">
              &nbsp;Multiplier:
              <input type="number" name="multiplier" value="%multiplier%" min="0" max="255">
            </td>
          </tr>
        </table>
      </div>
      <br> 

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Lat:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="lat" value="%lat%" min="-90" max="90" step="any">
            </td>
          </tr>
        </table>
      </div>
      <br> 

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Lon:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="number" name="lon" value="%lon%" min="-180" max="180" step="any">
            </td>
          </tr>
        </table>
      </div>
      <br> 

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Google Maps API Key:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="text" name="GoogleMapKey" value="%GoogleMapKey%">
            </td>
          </tr>
        </table>
      </div>
      <br>      

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Rotate screen:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="checkbox" name="doRotate" value="doRotate" %doRotate%>
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Rotate touch:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="checkbox" name="rotateTouch" value="rotateTouch" %rotateTouch%>
            </td>
          </tr>
        </table>
      </div>
      <br>

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="hwidth" style="text-align:right;font-size: medium;">
              Debugmode:
            </td>
            <td class="hwidth" style="text-align:left;font-size: medium;">
              <input type="checkbox" name="isDebug" value="isDebug" %isDebug%>
            </td>
          </tr>
        </table>
      </div>
      <br>      

      <div class="divinfo">
        <table class="fwidth">
          <tr>
            <td class="fwidth" style="text-align:center;font-size: medium;">
              <input style="font-size: medium;"" type="submit" value="Submit">
            </td>
          </tr>
        </table>
      </div>
      
    </form><br>
  </div>
  </div>
  <hr>

  <div class="topnav" style="background-color: lightblue; ">
    <table class="fwidth">
        <tr>
          <td class="hwidth" style="text-align:left">
            <h6><small>Copyright (c) Robert de Kok, PA2RDK</small></h6>
          </td>
          <td style="text-align:right">
            <a href="/"><button>Main</button></a>
            <a href="/reboot"><button>Reboot</button></a>
          </td>
        </tr>
    </table>
  </div>

  <script>
    if(typeof window.history.pushState == 'function') {
      if (window.location.href.lastIndexOf('/command')>10 || window.location.href.lastIndexOf('/reboot')>10 || window.location.href.lastIndexOf('/store')>10){
        console.log(window.location.href);
        window.location.href =  window.location.href.substring(0,window.location.href.lastIndexOf('/'))
      }
    }
    </script>

  </body>
  </html>  
)rawliteral";

const char css_html[] PROGMEM = R"rawliteral(
html {
    font-family: Arial; 
    display: inline-block; 
    text-align: center;
    background-color: gray; 
}
p { 
    font-size: 1.2rem;
}
body {  
    margin: 0;
}
.hwidth {
    width: 50%;
}

.fwidth {
    width: 100%;
}

.kader {
    position: absolute; 
    left: 20%; 
    width: 80%;
}

.freqdisp {
    border: none;
    color: yellow;
    font-size: xx-large;
    text-align: center;
    background: transparent;
}

.topnav { 
    overflow: hidden; 
    background-color: blue; 
    color: white; 
    font-size: 1rem; 
    line-height: 0%;
}
.divinfo { 	
    overflow: hidden;  
    color: white; 
    font-size: 0.7rem; 
    line-height: 0%;
}
.freqinfo { 	
    overflow: hidden; 
    color: yellow; 
    font-size: 1rem; 
    line-height: 0%;
}
.content { 
    padding: 20px; 
}
.card { 
    background-color: lightblue; 
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); 
    font-size: 0.7rem; 
}
.cards { 
    max-width: 1000px; 
    margin: 0 auto; 
    display: grid; 
    grid-gap: 1rem; 
    grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); 
}
.reading { 
    font-size: 1.4rem;  
}
h6 { 
  font-size: 0.7em;
  margin-top: 0.3em;
  margin-bottom: 0.3em;
  margin-left: 0;
  margin-right: 0;
  font-weight: bold;
}
h4 { 
  margin-top: 0.4em;
  margin-bottom: 0.6em;
  margin-left: 0;
  margin-right: 0;
}
h1 { 
  font-size:32px;
  margin-top: 0.7em;
  margin-bottom: 0.7em;
  margin-left: 0;
  margin-right: 0;
})rawliteral";
