Java programs using.

To run these programs you must have Java installed on your computer.
You can instal Java from http://www.java.com/  

If you have Java, you can start programs directly clicking on the name or using 
command : java -jar namefile

SocketClient.jar
Is a GUI program that can open a link with a computer using the IP address and 
the port number written on reserved fields in the window. You can use this 
program as a remote console.
The program window has two panels. 
The upper panel is the receiver panel. Everything sent from Arduino is written 
on this panel.
The lower panel receives input from keyboard. A record is sent only when a 
return key is pressed. Record sent is also echoed on upper panel.
You can also recalls previous records inserted using arrow keys.
Connection starts when you press "Connect" button. Clear button blanks panel.

SocketLog.jar
Is a GUI program that receives records from Arduino and saves them on file.
This program listen on port number that you can choose using "Port" menu on 
program window (default 5000).
Using "File" menu you can choose file where saving data (default no file; just 
showing on panel).
Using "Record" menu you can decide to echo records on panel or not; and you can 
decide if to add timestamp on file, on panel or nothing.