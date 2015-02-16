Version 2.0
Update

new function poweroff
- setPowerOff
new functions  (WPA key management) and direct connection
- ConnectOpen(char *ssid);
- ConnectWPAwithPsw(char *ssid, char *psw);
- ConnectWPAandGetKey(char *ssid,char *psw, uint8_t key[32]);
- ConnectWPAwithKey(char *ssid,uint8_t key[32]); 

new function http
- sendRequestGET/POST with Host parameter
- getResponse with buffer
- getNextResponseBuffer to read all data  

Modified ResetMCW (still problems) 

bug corrected on sendRequestPOST (was GET instead of POST)

Version 2.2

MWIFI

- new function existSSID to verify if a Access Point is reachable
- new functions to use connection key in EEPROM
- new function for power saving
- modified error routine (some warning without reset)
- new flag ERRORLOG to show just error cause
- switched off led if power-down

HTTP

- new functions: sendRequestPUT and sendRequestDELETE
- no more sendRequest... with parameter HOST
- new general headers definition in sendRequest...
- new getResponseMessage to show http return code (just code)
- new getRequest with authentication protocol (Basic)
- internal functions for coding and encoding base64 format (for username-password)
- new function setUserAndPassword to code and store authentication key 
- new function respNoAuth if no authenticated
- more string in progmem to free ram space

Version 2.3

MWiFi 

- new function readDataLn with timeout
- EEPROM function included in directory utility

HTTP

- base64 function (code and decode) extracted from HTTPLib and now in 
  directory utility as new class BASE64
  
MAIL

- new class MAIL with simple functions for SMTP protocol (but not STL)  

Version 2.4

MWiFi

- better performance in receiver buffer drining
- new function pointer : errorHandle (to manage error in customer way)(def.=NULL) 

HTTP

- bugs (continuos allocation) correction in getParameter function 
- bug (incorrect resource detect) in getRequest with POST method 
- new function sendShortResponse for forms or AJAX better using
- new function sendResponse with page in modules (array of pieces o HTML code)
