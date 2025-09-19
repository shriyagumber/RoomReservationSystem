# Distributed Conference Room Reservation System  

A multi-server client–server system implementing secure **room reservation and query services** with **TCP and UDP socket programming**. The system demonstrates authentication, encrypted communication, distributed data handling, and fault-tolerant message flow.  

---

## Features  
- **Authentication System**  
  - Encrypted credential-based login for members.  
  - Guest mode for query-only access.  

- **Client–Server Communication**  
  - **TCP** for client ↔ main server.  
  - **UDP** for main server ↔ backend servers.  
  - Concurrent client sessions supported.  

- **Room Management**  
  - Query availability by room, day, and time.  
  - Member-only reservations with dynamic updates.  
  - In-memory state management for performance.  

- **Advanced Querying**  
  - Query all available slots for a room.  
  - Query all available times for a room on a given day.  

---

## Technical Highlights  
- **Multi-server architecture** with main server, credential server, and distributed backend servers.  
- **Custom encryption scheme** for client-side credential security.  
- **Modular socket handling** for robust TCP/UDP communication.  
- **Role-based access control** ensuring integrity of reservation system.  
- **Dynamic port allocation** for scalable client connections.  

---

## Usage  
```bash
# Start servers in order
./serverM      # Main server
./serverC      # Credential server
./serverRTH    # Backend RTH
./serverEEB    # Backend EEB

# Start clients
./client
