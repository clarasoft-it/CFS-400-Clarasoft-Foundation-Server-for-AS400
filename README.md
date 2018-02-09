# CFS-400-Clarasoft-Foundation-Server-for-AS400
TCP/IP Server and development framework for AS400

## What is CFS-400

CFS-400 is a TCP server platform for AS400. This platform enables ILE RPG developpers to create network services that can be used for modernizing user interfaces, enabling native web clients on existing ILE RPG applications and developping your own networking protocols.

## Building the TCP server and server handler

Copy the files on your AS400 in a source file named QCSRC (the CFSREG source needs to be copied in the QDDSSRC source file). Next, you will create the following modules (compilation commands follow):

* CFSAPI
* CLARAD
* CLARAH
* CLARAHS
* CSLIST
* CSSTR
* CSWSCK

From the AS400 command line, execute the following comamnds to create the above modules:

```bash
CRTCMOD MODULE(CFSAPI) SRCFILE(QCSRC) DBGVIEW(*ALL)  

CRTCMOD MODULE(CLARAD) SRCFILE(QCSRC) DBGVIEW(*ALL) 

CRTSQLCI OBJ(CLARAH) SRCFILE(QCSRC)
              SRCMBR(CLARAH) DBGVIEW(*SOURCE)   

CRTSQLCI OBJ(CLARAHS) SRCFILE(QCSRC)
              SRCMBR(CLARAHS) DBGVIEW(*SOURCE)   

CRTCMOD MODULE(CSLIST) SRCFILE(QCSRC) DBGVIEW(*ALL)     

CRTCMOD MODULE(CSSTR) SRCFILE(QCSRC) DBGVIEW(*ALL) 

CRTCMOD MODULE(CSWSCK) SRCFILE(QCSRC) DBGVIEW(*ALL)  
```

Next, you need to create a SRVPGM object that exports the CFS-400 toolkit functions (we will call it CFSAPI) by issuing the following command (alternatively, you can use the provided binding source cfsapi.bnd):

```bash
CRTSRVPGM SRVPGM(CFSAPI)
        MODULE(CFSAPI CSLIST  CSSTR CSWSCK) EXPORT(*ALL) 
```
        
Next, you must create the CFSREG file as follow:

```bash
CRTPF FILE(CFSREG) SRCMBR(CFSREG)
```

You will now create the the main TCP server (daemon) and the demonstration echo handler (respectively CLARAD and CLARAH) by issuing the following command:

```bash
CRTPGM PGM(CLARAD) MODULE(CLARAD) BNDSRVPGM(CFSAPI)

CRTPGM PGM(CLARAH) MODULE(CLARAH) BNDSRVPGM(CFSAPI)
```

## CFS-400 example ILE RPG service for use by network clients

To show how an ILE RPG program can be used to run as a handler, you will create the ILE RPG version of the websocket echo handler as follow: copy the ECHO.RPGLE and the ECHO.RPGH files from this repository to the QRPGLESRC source file on your system. You will then compile this source into a module by issuing the following command:

```bash
CRTRPGMOD MODULE(ECHOMOD) SRCFILE(QRPGLESRC) SRCMBR(ECHO.RPGLE) DBGVIEW(*ALL)  
```
Next, you will create a SRVPGM object from the above module (notice this uses the CFSAPI service program built above):

```bash
CRTSRVPGM SRVPGM(ECHOSRV)
        MODULE(ECHOMOD) BNDSRVPGM(CFSAPI) EXPORT(*ALL) 
```

For the CLARAH program to use the ILE RPG echo service, a record must be inserted into the CFSREG file: 

```bash
RGSRVNM: ECHO           
RGLIBNM: LIBNAME        
RGPRCHD: ECHOSRV        
RGPRCNM: ECHOHANDLER    
```

Where LIBNAME is the name of the library where the ECHOSRV service program resides. All other fields have to be as shown above. Now, to test this ILE RPG service, you can execute the CFS-400 daemon from the command line (in production, this command would be run in its own subsystem) and instruct it to use the ECHOSRV service by issuing the follwng command (this assumes that CLARAH is also in library LIBNAME although this is not mandatory: the service library does not have to match the library where the daemon resides):

```bash
call CLARAD                                                     
 parm('41101' '3' '4' '/QSYS.LIB/LIBANME.LIB/CLARAH.PGM' 'ECHO') 
```

The above command will run three handler jobs and the main daemon job:


```bash
Opt  Job         User        Type     -----Status-----  Function      
     CLARAH      DUSER       BATCHI   ACTIVE            PGM-CLARAH    
     CLARAH      DUSER       BATCHI   ACTIVE            PGM-CLARAH    
     CLARAH      DUSER       BATCHI   ACTIVE            PGM-CLARAH    
     DUSER       DUSER       INTER    ACTIVE            PGM-CLARAD    
```
     
There are 3 handlers running, waiting for client connections; if all 3 handlers are busy servicing connections, then a fourth handler will be executed to handle an additional connexion (that is the meaning of the '3' '4' parameters to the command). To test the handler, you can open a websocket test client from your web browser at http://www.websocket.org/echo.html:

In the "Location" text box, input your AS400 machine address:

```bash
ws://myAS400.mydomain.com:41101
```

Notice that the port number is the same as the one from the command line used to execute the CFS-400 daemon. Click on the "Connect" button. You should get a "CONNECTED" message in the "Log" text area. You can then write messages to send over to your ILE RPG service in the "Message" text box and by clicking on the "Send" button. Your ILE RPG service will echo back the data you sent in the "Log" text area. One of your CLARAH jobs is busy servicing this connection. The other two CLARAH jobs are available for other connections. When you click on the "DISCONNECT" button, the CLARAH job that was servicing your connection continues to execute but is no longer busy and is now available for another connection.

This example shows how easy it is to create web front ends with CFS-400. You are not limited to the Websocket protocol. CFS-400 provides all the functions needed to use basic TCP sockets (either secured with SSL or not) and create your own networking protocols or to create clients for any other networking protocol: this means you can create your own HTTP, FTP, SMTP, etc clients in ILE RPG (but this requires in-depth knowledge of those protocols however)!



     
 
 









