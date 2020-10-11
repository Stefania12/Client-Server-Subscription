# Client-Server Subscription

## Subscriber
The subscriber is a TCP client that connects to the server. It can receive at any
time a command line input which represents a subscribe/unsubscribe command.


## Server
The server opens the TCP and UDP sockets and waits for:
* command line input
* connection request from TCP clients
* command from TCP clients
* notifications from UDP Clients

The server only allows clients with unique IDs.
If a TCP client sends a subscribe command for a topic it is already subscribed to,
the subscription type is overwritten.
Notifications from UDP clients are received and forwarded to TCP clients subscribed
to the specific topics. If SF value is 1 and the TCP clients is offline, the notification
is stored and forwarded when the client reconnects.


## Usage
### Client commands
* subscribe topic SF -> subscribe to a topic with the notification settings
* unsubscribe topic -> unsubscribe from topic
* exit -> close the client

Note: SF value 1 means that if the client is offline when a notification is received,
store and forward the notification when the client reconnects.

### Server commands
* exit -> closes the server