# STUN

Session Traversal Utilities for NAT (STUN).

---

## About

The STUN-server (Session Traversal Utilities for NAT) uses by default the UDP protocol for data transfering, and listens on port: 3478. It is written in C++, and implements basic, core functionality of a STUN-server following the RFC 5389 document.

See: https://tools.ietf.org/html/rfc5389

### Background

The STUN-server was initially written for a volountary project in the course 'IDATT2104 Nettverksprogrammering' at NTNU Trondheim.

---

## TODO

List of functionality to be implemented.

---

#### TCP support

Implement support for TCP data transfer.

---

#### TLS support

Implement support for TLS data transfer.

---

#### Multi-threading

Use multi-threading when establishing a connection over UDP/TCP & TLS.

---

#### Credential mechanism

Add username and password for Short-Term Credential Mechanism.

See: https://tools.ietf.org/html/rfc5389#page-22

---

#### Processing error response

Create an ERROR-CODE attribute, and handle the processing of an error response.

See: https://tools.ietf.org/html/rfc5389#page-20

---

#### Fingerprint mechanism

Optional mechanism for STUN that aids in distinguishing STUN messages from packets or other protocols.

See: https://tools.ietf.org/html/rfc5389#page-20

---

#### DNS discovery

Allow a client to use DNS to determine the IP address and port of the server.

See: https://tools.ietf.org/html/rfc5389#page-21

---

#### Refactor & improve code

Refactor code into multiple files, and improve code quality and readability.

---

## Dependencies

#### Docker / Docker Compose

Used to run the STUN-server in an isolated container.

See: https://www.docker.com/

See: https://docs.docker.com/compose/

#### Easylogging++

Used for logging the STUN-server.

See: https://github.com/easylogging/

---

## Installation

The project requires the following to be installed locally on your computer ([Dependencies](#dependencies)):

* Docker

* Docker Compose

Clone the repository.

Change directory to the root folder of the project:

`cd stun`

Start the STUN-server:

`docker-compose up --build`

---

## Testing

To test the STUN-server, it is **recommended** to use a third-party STUN-client, e.g. STUNTMAN.

See: http://www.stunprotocol.org/