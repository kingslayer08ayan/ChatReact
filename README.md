# ChatReact 💬⚡

ChatReact is a **real-time, full-stack chat application** built with **C++ (backend server), React (frontend)**, and a **Node.js bridge**.  
It supports **multiple clients chatting simultaneously in global mode ,groups and privately**, and an **epoll-based asynchronous event loop** for high concurrency.  

---

## 🚀 Features & Functionalities

### 🔹 Core Functionalities
- **Multi-user real-time chat** – multiple clients can join and chat simultaneously.  
- **Asynchronous server** – backend uses **epoll** for high-performance concurrency.  
- **Bridge communication** – `bridge.js` connects the backend (C++) with the frontend (React).  
- **Interactive React UI** – clean, responsive chat interface built with React + Tailwind.  
- **End-to-End integration** – backend, frontend, and bridge are wired together seamlessly.  

### 🔹 Chat Commands (from `/help`)
- `/help` → Show available commands  
- `/msg <user> <message>` → Send private message  
- `/list` → List all online users  
- `/whoami` → Show current username  
- `/creategroup <group_name>` → Create a new group (admin role granted to creator)  
- `/addmember <group_name> <username>` → Add user to a group (admin only)  
- `/kickmember <group_name> <username>` → Remove user from group (admin only)  
- `/listgroups` → Show all groups & members you belong to  
- `/gmsg <group_name> <message>` → Send message to group members  
- `/sendfile <user> <filename> <filesize>` → Send a file to a user  
- `/quit` → Disconnect from server  

---

## 🛠️ Tech Stack

- **Backend:** C++17 (epoll-based asynchronous TCP server/client)  
- **Frontend:** React (Vite, TailwindCSS)  
- **Bridge:** Node.js (WebSocket / TCP bridge)  
- **Build System:** CMake  
- **Version Control:** Git  

---

## 📂 Project Structure

```
ChatReact/
├── backend/                         # C++ Epoll-based Chat Server & Client
│   ├── include/                     # Header files
│   │   ├── server.hpp
│   │   ├── client.hpp
│   │   ├── message.hpp
│   │   ├── utils.hpp
│   │   └── config.hpp
│   ├── src/                         # Source files
│   │   ├── server.cpp
│   │   ├── client.cpp
│   │   ├── message.cpp
│   │   ├── utils.cpp
│   │   └── main.cpp
│   ├── build/                       # Compiled binaries (ignored in git)
│   ├── CMakeLists.txt
│   └── tests/                       # Unit tests for backend
│
├── frontend/                        # React + Vite Frontend
│   ├── public/
│   ├── src/
│   │   ├── components/              # Reusable components
│   │   │   ├── ChatWindow.jsx
│   │   │   ├── Sidebar.jsx
│   │   │   ├── MessageInput.jsx
│   │   │   ├── LoginForm.jsx
│   │   │   └── UserList.jsx
│   │   ├── pages/                   # Pages
│   │   │   ├── LoginPage.jsx
│   │   │   └── ChatPage.jsx
│   │   ├── hooks/                   # Custom hooks
│   │   │   └── useWebSocket.js
│   │   ├── context/                 # Context providers (UserContext, ChatContext)
│   │   ├── services/                # API/WebSocket service layer
│   │   │   └── wsService.js
│   │   ├── styles/                  # Tailwind configs/custom CSS
│   │   ├── App.jsx
│   │   └── main.jsx
│   ├── package.json
│   └── vite.config.js
│
├── docs/                            # Documentation (design, API specs, diagrams, usage)
├── scripts/                         # Deployment scripts (Dockerfile, CI/CD pipelines)
├── bridge.js                        # Node.js WebSocket <-> TCP bridge
├── package.json                     
├── .gitignore
└── README.md
```

---

## ⚙️ Setup Instructions

### 1️⃣ Backend (C++ server)
```bash
cd backend
mkdir build && cd build
cmake ..
make
./chat_server
```

(Optional: Run standalone client)
```bash
./chat_client
```

### 2️⃣ Bridge (Node.js)
```bash
npm install
node bridge.js
```

- WebSocket server: `ws://localhost:3001`  
- TCP backend: `127.0.0.1:12345`  

### 3️⃣ Frontend (React)
```bash
cd frontend
npm install
npm run dev
```

Open 👉 [http://localhost:5173](http://localhost:5173)

---

## 🎥 Demo

![Demo](demo.gif)

---

## 🤝 Contributing

1. Fork the repository  
2. Create your feature branch (`git checkout -b feature/xyz`)  
3. Commit your changes (`git commit -m "Add xyz feature"`)  
4. Push to the branch (`git push origin feature/xyz`)  
5. Open a Pull Request  

---

## 📜 License

This project is licensed under the MIT License.  

