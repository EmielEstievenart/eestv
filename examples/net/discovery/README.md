# UDP Discovery Examples

This directory contains examples demonstrating the UDP discovery mechanism with a server and client.

## Building

From the build directory:
```bash
cmake --build . --target discovery_server_example discovery_client_example
```

## Running the Examples

### Server

Start the discovery server (in one terminal):
```bash
./discovery_server_example [port]
```

Default port: `12345`

Example:
```bash
./discovery_server_example 12345
```

The server registers several example services:
- `database` - Returns "127.0.0.1:5432"
- `api` - Returns "127.0.0.1:8080"
- `web` - Returns "127.0.0.1:3000"
- `time` - Returns current timestamp

### Client

Search for a service (in another terminal):
```bash
./discovery_client_example [service_name] [port] [timeout_ms]
```

Default service: `database`  
Default port: `12345`  
Default timeout: `2000` ms

Examples:
```bash
# Search for the database service
./discovery_client_example database

# Search for the api service on port 12345 with 3 second timeout
./discovery_client_example api 12345 3000

# Search for the time service
./discovery_client_example time

# Search for a non-existent service (will timeout)
./discovery_client_example nonexistent
```

## How It Works

1. **Server**: 
   - Listens on all network interfaces on the specified UDP port
   - Maintains a registry of discoverable services
   - When it receives a discovery request with a service identifier, it responds with the service's information

2. **Client**:
   - Broadcasts discovery requests to the network on the specified UDP port
   - Listens for responses from discovery servers
   - Calls a callback function when a matching service is found
   - Automatically retries if no response is received within the timeout period

## Use Cases

- **Service Discovery**: Find services on the local network without hardcoding IP addresses
- **Dynamic Configuration**: Services can announce their endpoints dynamically
- **Load Balancing**: Multiple servers can respond to the same service identifier
- **Testing**: Easy to test distributed systems on a single machine using localhost
