#define TIMEZONE euCET // See NTP_Time.h tab for other "Zone references", UK, usMT etc

const wlanSSID wifiNetworks[] {
    {"PI4RAZ","PI4RAZ_Zoetermeer"},
    {"Loretz_Gast", "Lor_Steg_98"},
    {"Netwerk1", "Netwerk2"},
    {"Netwerk2", "Netwerk2"},
    {"FHanna", "Kleinsteg71"}
};

Settings settings = {
    '#',                //chkDigit
    "YourSSID",         //wifiSSID[25];
    "WiFiPassword!",    //wifiPass[25];
    "rotate.aprs.net",  //aprsIP
    14580,              //aprsPort
    "99999",            //aprsPassword    
    "APZRAZ",           //dest[8]
    0,                  //destSsid
    "PI4RAZ",           //call[8]
    7,                  //ssid
    14,                 //serverSsid
    600,                //aprsGatewayRefreshTime
    "73 de PI4RAZ",     //comment[16]
    '>',                //symbool
    "WIDE1",            //path1[8]
    1,                  //path1Ssid
    "WIDE2",            //path2[8]
    2,                  //path2Ssid
    30,                 //interval (in seconds, complete interval = interval*multiplier)
    10,                 //multiplier
    5,                  //power
    0,                  //height
    0,                  //gain
    0,                  //directivity
    500,                //preAmble  
    100,                //tail
    100,                //Brightness %
    51.903611,          //latitude
    6.461667,           //longitude
    1,                  //useAPR
    0,                  //preEmphase
    0,                  //highPass
    0,                  //lowPass
    0,                  //isDebug
    378,                //touchRotation calData[0]
    3473,               //touchRotation calData[1]
    271,                //touchRotation calData[2]
    3505,               //touchRotation calData[3]
    7,                  //touchRotation calData[4]
    1                   //doRotate
};
