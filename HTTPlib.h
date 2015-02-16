/*
  Copyright (c) 2014 Daniele Denaro.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
*/


/******************************************************************************/
/*HTTP library.
* This library has the purpose of drastically reducing work when you are in WEB 
* based programming activity. WEB communication is based on http protocol. 
* This protocol is, essentially based on a couple of messages : a request and 
* consequent response.
* This library comes with MWiFi library but can be easy adapted to other socket
* software. Just provide the new version of readData and write data functions, 
* try to use readLine function (or make another version) and define a buffer 
* linebuff present in MWiFi library.
* As server:
*  - getRequest()
*  - sendResponse()
*  - sendDynResponse()
* As client
*  - sendRequest()
*  - getResponse() 
* Suppose we want to be a WEB server.
* First of all, we connect Arduino with the net. Then we act as a server socket 
* and listen for incoming link. When link is accepted, we can start to listen 
* for incoming request message. 
* The function getRequest() do noting until a request message arrives.
* When request message arrives the function decodes header and detects the 
* resource (URL) required. The resource names are automatically mapped with 
* functions we have prepared. 
* This map is done preparing an array of typedef WEBRES and provide this as 
* argument of getRequest().
* WEBRES type is a struct of resource-name,function-name coupled.
* So, if the name of resource (URL) required, matches one resource-name in the 
* array, corresponding function is activated. In other case, a Resource not 
* found message is automatically send as response.
* Function activated has access to a possible query string, and can decode 
* parameters and its values.
* Function coupled with resource name must send a response.
* Response can be a html static page stored in PROGMEM space of Arduino, 
* or a dynamic page stored in PROGMEM. 
* Static page is a html text sent exactly like it. Dynamic page contains one or 
* more tags @. Each tag is substitutes by a string in an array in sequential 
* order.
* This array can be made at the moment of response, and has to be provided to 
* the function sendDynResponse() as argument.
* 
* Example of link resource-routine:
* // we prepare links resouce-name->routine 
* WEBRES rs[7]={{"/index",pindex},{"/Analog",panalog},{"/RDigital",rdigital},
     {"/WDigital",wdigital},{"/wdig",wdig},{"/Pwm",pwmpage},{"/pwmset",pwmset}}; 
*
* void loop()                        // Arduino loop
* {
*   WIFI.getRequest(csocket,7,rs);  // if request arrives for resource 
*                                   // "http://...../Analog
*                                   // function panalog() is activated 
* }
* 
* Example of dynamic response:
* prog_char pageAnalog[] PROGMEM=    // html page containing a tag @
* "<html>.........."
* "....A1 value = @....."
* "......</hml>";

* void panalog(char *query)// call back routine activated by request ("/Analog")
* {
*    // if need function can retrieve parameters and its value from string query
*    // using svpar=getParameter(query,querylength,parameter-name)
*    // prepare dynamic response making strings to substitute tag @
*    char sval[5];                     //buffer to hold analogic value of pin A1
*    sprintf(sval,"%d",anlogRead(1));  //put integer value in sval as string
*    char *tags[1];                    //make an array of only one string 
*    tags[0]=sval;                     //put string value into the array
*    //provide array (end length) to response function
*    sendResponse(socket,pageAnalog,1,tags);
* }                               // function substitutes tag and send html page
*
* If Arduino acts as client, it has to send request (mode GET or mode POST)
* with parameters and to listen for response.
* sendRequestGET() function has a string resource as argument. This query string
* is made by a resource-name+query-string. Example: /page1?par1=100&par2=60
* Query string can by added to resource name manually or using the 
* addParameter() function. Add parameter function decode automatically special 
* characters and blank (see query format on the WEB).
* sendRequestPOST() function has separate arguments for resurce name and data.
* In this case data is a buffer contains any kind of format. Web applications
* can retrieve these data using request input stream. 
* getResponse() function can return data sent in response message (as null 
* termited string), but the max length is LINEBUFFLEN (see MWiFi.h) and data in
* excess are lost.
* If response is error (for example 404 Not found) getResponse() returns RESPERR
* string ("NOPAGE" at present).
* If getResponse() can't receive response returns NULL
*
* Version 2.4
* Author Daniele Denaro
*/
/******************************************************************************/


#ifndef HTTPlib_h
#define HTTPlib_h

#include <MWiFi.h>
#include <utility/BASE64.h>

#define MAXC 64             //chunk packet length 

#define RESPERR "NOPAGE"    //response error message when page not found(client)

#define URILEN 15           //resource name buffer length (see struct res) 
#define QUERYLEN 64         //query buffer length (see struct res))


// WEBRES typedef
typedef struct rsc
	{                                   
		char *name;                       //URI to join
		void (*fun)(char *querystring);   //calback function name joined
	} WEBRES;



class HTTP : public MWiFi
{
public:

/************************ Main get/send functions *****************************/   
//for server activity

/* 
*  Manages request actviating correct call back function using rs[]
*  Or send a 404 Not Found message
*  Returns the resource name request.
*  N.B. Query string is provided to call back function as argument by this fun. 
*  Request can be GET or POST
*/
	char* getRequest(int socket,int nres,WEBRES rs[]);

/*
*  Second version with authentication (basic process).
*  key is one of NAUTHKEY stored keys.
*  Key is stored using setUserAndPassword function
*/
  char* getRequest(int socket,int nres,WEBRES rs[],char *key);
	
/*
*  Sends page stored as prog_char array in PROGMEM  
*/	
	void sendResponse(int socket,prog_char *page);

/*
* Version for pages made by multiple modules (to optimize prog space)
* amodule: array of PROGMEM module; nm: its dimension
*/
  void sendResponse(int sk,prog_char* amodule[],uint8_t nm);
	
/*
*  It sends dynamically assembled page substituting tag @ with strings found in
*  array param[] (array of string: array of char array i.e. array of pointer to
*  char array)
*  npar is the dimension of this array 
*/	
	void sendDynResponse(int sk,prog_char *page,int npar,char *param[]);

/*
* Version for pages made by multiple modules (to optimize prog space)
* amodule: array of PROGMEM module; nm: its dimension
*/
  void sendDynResponse(int sk,prog_char* amodule[],uint8_t nm,int npar,char *param[]);

/*
* Sends short data as response. Typically used for forms or ajax answering.
*/
  void sendShortResponse(int sk,char *data);

/*
*  It sets the autentication key for controlled access.
*  The key is the link of usename+':'+password, and is encoded using base64 format.
*  So, the key about 1.3 longer than the original. Don't use username and password 
*  too long because the Arduino ram lack.
*  It's possible to define up to NAUTHKEY keys.
*/
  char* codeWebKey(char user[],char psw[]);	
	
// for client activity

/* Sends a request of a resource(URI) GET or POST

/*
*  For GET method:
*  Resource can be : resourcename+querystring Ex: "/index?A1=122&A2=34"
*  Resource can by prepared using addParameter function (see later)
*/
	void sendRequestGET(int socket,char* resource);
	
/*
*  As previous function. But with possibility of setting headers of http packet.
*  For example to set Host propriety.Useful for sites with one address and 
*  multiple hosts. Heach header is a null terminated string like: "xxxx: xxxxxxx"
*  nh is the headers[] dimension (sizeoff)
*/	
  void sendRequestGET(int sk,char* headers[],int nh,char* resource);
  
/*  
*  For POST method:
*  Resource doesn't contain query string (just the URI) 
*  data can contain any string. In this case values can by formatted as any 
*  kind of sequence (XML,JSON, sequence of values etc.). Obviously you can
*  also use query string format. Web application can retrieve data using input
*  stream (not getParameters). 
*/  	
  void sendRequestPOST(int sk,char* resource,char* data);
  
/*
*  As previous function. But with possibility of setting headers of http packet.
*  For example to set Host propriety.Useful for sites with one address and 
*  multiple hosts. Heach header is a null terminated string like: "xxxx: xxxxxxx"
*  nh is the headers[] dimension (sizeoff)
*/  
  void sendRequestPOST(int sk,char* headers[],int nh,char* resource,char* data);

/*
*  Sends a request of a resource(URI) using PUT method:
*  Like POST method. 
*/
  void sendRequestPUT(int sk,char* resource,char* data);

/*
*  With headers setting possibility.
*/
  void sendRequestPUT(int sk,char* headers[],int nh,char* resource,char* data);  

/*
*  Sends a request of a resource(URI) using DELETE method
*/

  void sendRequestDELETE(int sk,char* resource);

/*
*  As previous function. But with possibility of setting headers of http packet.
*/
  void sendRequestDELETE(int sk,char* headers[],int nh,char* resource);    	



/*
*  Returns pointer to predefined buffer linebuff (see MWiFi) containing 
*  data sent. If data sents exceed LINEBUFFLEN data are cutted. (linebuff is 
*  always a null terminated string)
*/	
	char* getResponse(int socket);
	
/*
* With timeout (millisec)
*/   
  char* getResponse(int sk,int timeout);   //timeout in millisec

/*
*   This function lets you receive long response.
*   Returns -1 if response not arrived or -2 if response error.
*   Else returns number of bytes loaded into the rbuff.
*   If this number is equal to rbufflen it means there are more bytes to read
*   and you have to use getNextResponseBuffer() function until it returns 0.
*   Be carefull! You must read all bytes to clean MCW buffer.
*/
   unsigned int getResponse(int sk,unsigned char rbuff[],int rbufflen);
   
/*
* With timeout (millisec)
*/   
   unsigned int getResponse(int sk,uint8_t rbuff[],int rbufflen,int timeout);

/*
*   Function for further bytes reading (and cleaning MCW receive buffer).
*   Returns number of bytes loaded into the rbuff; or 0 if no more bytes
*   available:
*/
   unsigned int getNextResponseBuffer(int sk,unsigned char rbuff[],int rbufflen);

/*
*   Get response message code. It can be called after  getResponse functions.
*/
   char* getResponseMessage();

// utilities
	
/*********** Routines for retrieving parameters from query_string. ************/
/************* Or to format parameters to add to query_string. ****************/
/* 
* Returns value (string) giving name and query_string received (with its length); 
* or NULL if name not found.
* Function actually tokenize query string, so doesn't allocate space
* Query_string can start with ? or not.
*/
  char* getParameter(char *query, char *name);

/*
* Returns encoded resource+query string 
* if value==NULL name is considered as resource name and initialize query string 
* Any following function call add (coded) parameter to query string
* Be careful! querylen is the sizeof buffer query (not strlen)
*/
  void addParameter(char *query, int querylen,char *name, char *value);

  void respOKempty(int sk); // just an ACK
  
	void respERR(int sk);  // usually not used by user
	void respNOK(int sk);  // usually not used by user
  void respNoAuth(int sk);	
	
/******************************/
// buffer for resource name and query string. Returned by getRequest function	
// usually not used directly
	struct res
	{
	  char name[URILEN];
	  char query[QUERYLEN];
	  int qlen; 
  }Resource;
  

private:
  
  BASE64 B64;
/* functions called by previous principal get/send functions  */
	void reqGET(int socket,char *endl,int nres,WEBRES rs[],char *key);
	void reqPOST(int socket,char *endl,int nres,WEBRES rs[],char *key);
	void activateRes(int sk,int nres,WEBRES rs[]);
	void startLongResponse(int sk);
	void sendChunkResponse(int sk,prog_char data[],int ldata,char* param);
	void endLongResponse(int sk);
	char* checkHeader(char *buff, prog_char *header);
	bool checkUserPsw(char *param,char *userpsw);

/* functions used by previous principal parameter functions */
  char* setParameter(char *name, char *value);
  char* unencode (char *src, int len);
  char* encode(char *src,char *dest, int len);
  char* getVal(char* name,char* ini,char* end);
  
};
#endif





