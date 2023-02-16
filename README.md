# QRServer
Server and Client code for a server that Decodes QR Codes sent by concurrent clients.  It returns the decoded URL

Compile both files with "make all".

Run the server with "./QRServer" and the client afterwards with "./connect".

QRServer supports options:
PORT 2000-3000
RATE [number requests] [number seconds]
MAX_USERS [number of users]
TIME_OUT [number of seconds]

connect supports option:
PORT 2000-3000
