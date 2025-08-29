const WebSocket = require("ws");
const net = require("net");

const WS_PORT = 3001;       // frontend <-> bridge
const TCP_PORT = 12345;      // bridge <-> C++ backend
const TCP_HOST = "127.0.0.1";

const wss = new WebSocket.Server({ port: WS_PORT });
console.log(`✅ WebSocket bridge running on ws://localhost:${WS_PORT}`);

wss.on("connection", (ws) => {
  console.log("🌐 New Web client connected");

  // connect to C++ backend
  const tcpClient = net.createConnection({ host: TCP_HOST, port: TCP_PORT }, () => {
    console.log("🔌 Connected to C++ backend");
  });

  // relay backend → frontend
  tcpClient.on("data", (data) => {
    ws.send(data.toString());
  });

  // relay frontend → backend
  ws.on("message", (message) => {
    tcpClient.write(message.toString() + "\n");
  });

  ws.on("close", () => {
    tcpClient.end();
  });
});
