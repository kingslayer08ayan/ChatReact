import { useEffect, useRef, useState } from "react";

export default function useWebSocket(url) {
  const socketRef = useRef(null);
  const [messages, setMessages] = useState([]);
  const [connected, setConnected] = useState(false);
  const [incomingFile, setIncomingFile] = useState(null);

  useEffect(() => {
    socketRef.current = new WebSocket(url);

    socketRef.current.onopen = () => {
      console.log("✅ Connected to WebSocket server");
      setConnected(true);
    };

    socketRef.current.onmessage = (event) => {
      // Check if it's binary data (file content)
      if (event.data instanceof ArrayBuffer || event.data instanceof Blob) {
        handleFileData(event.data);
        return;
      }

      const msg = event.data;
      console.log("📨 Received message:", msg);

      // Check if it's a file incoming notification
      const fileIncomingMatch = msg.match(/\[File incoming\] (.+) from (.+) \((\d+) bytes\)/);
      if (fileIncomingMatch) {
        const [, filename, sender, size] = fileIncomingMatch;
        setIncomingFile({
          filename,
          sender,
          size: parseInt(size),
          data: new Uint8Array(0), // Initialize empty array for file data
          receivedBytes: 0
        });
        
        // Add the notification message to chat
        setMessages((prev) => [...prev, {
          type: 'file-notification',
          text: msg,
          timestamp: new Date().toISOString()
        }]);
        return;
      }

      // Check if it's a file received confirmation
      if (msg.includes("received successfully")) {
        setMessages((prev) => [...prev, {
          type: 'file-success',
          text: msg,
          timestamp: new Date().toISOString()
        }]);
        return;
      }

      // Regular text message
      setMessages((prev) => [...prev, {
        type: 'text',
        text: msg,
        timestamp: new Date().toISOString()
      }]);
    };

    socketRef.current.onclose = () => {
      console.log("❌ Disconnected from WebSocket server");
      setConnected(false);
    };

    return () => {
      socketRef.current?.close();
    };
  }, [url]);

  const handleFileData = (data) => {
    if (!incomingFile) {
      console.warn("⚠️ Received file data but no incoming file expected");
      return;
    }

    // Convert data to Uint8Array if needed
    let dataArray;
    if (data instanceof ArrayBuffer) {
      dataArray = new Uint8Array(data);
    } else if (data instanceof Blob) {
      // Handle blob data (convert to array buffer)
      const reader = new FileReader();
      reader.onload = (e) => {
        const arrayBuffer = e.target.result;
        const uint8Array = new Uint8Array(arrayBuffer);
        appendFileData(uint8Array);
      };
      reader.readAsArrayBuffer(data);
      return;
    } else {
      console.error("❌ Unexpected file data type:", typeof data);
      return;
    }

    appendFileData(dataArray);
  };

  const appendFileData = (dataArray) => {
    setIncomingFile(prev => {
      if (!prev) return null;

      // Combine existing data with new data
      const newData = new Uint8Array(prev.data.length + dataArray.length);
      newData.set(prev.data);
      newData.set(dataArray, prev.data.length);

      const newReceivedBytes = prev.receivedBytes + dataArray.length;

      // Check if file is complete
      if (newReceivedBytes >= prev.size) {
        console.log("✅ File received completely:", prev.filename);
        
        // Trigger download
        downloadFile(newData, prev.filename);
        
        // Add completion message
        setMessages(prevMessages => [...prevMessages, {
          type: 'file-complete',
          text: `File '${prev.filename}' downloaded successfully (${prev.size} bytes)`,
          timestamp: new Date().toISOString()
        }]);

        // Clear incoming file state
        return null;
      }

      return {
        ...prev,
        data: newData,
        receivedBytes: newReceivedBytes
      };
    });
  };

  const downloadFile = (data, filename) => {
    const blob = new Blob([data]);
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `received_${filename}`;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  };

  const sendMessage = (msg) => {
    if (socketRef.current && connected) {
      socketRef.current.send(msg);
    }
  };

  // Enhanced file sending function - supports both File objects and local paths
  const sendFile = async (target, fileOrPath) => {
    if (!socketRef.current || !connected) {
      console.error("❌ WebSocket not connected");
      return false;
    }

    try {
      let fileName, fileSize, fileData;

      // Check if it's a string (file path) or File object
      if (typeof fileOrPath === 'string') {
        // Handle local file path
        const filePath = fileOrPath;
        fileName = filePath.split('/').pop().split('\\').pop(); // Extract filename from path
        
        try {
          // Try to read file using File System Access API (modern browsers)
          if (window.fs && window.fs.readFile) {
            // This is for environments that support file system access
            const fileBuffer = await window.fs.readFile(filePath);
            fileData = fileBuffer;
            fileSize = fileBuffer.length;
          } else {
            // For regular web browsers, we need to use file input
            throw new Error("Direct file system access not available. Please use file input.");
          }
        } catch (error) {
          console.error("❌ Could not read file from path:", error.message);
          return false;
        }
      } else if (fileOrPath instanceof File) {
        // Handle File object (from file input)
        fileName = fileOrPath.name;
        fileSize = fileOrPath.size;
        fileData = await fileOrPath.arrayBuffer();
      } else {
        console.error("❌ Invalid file parameter. Expected File object or string path.");
        return false;
      }

      // Send file header
      const header = `/sendfile ${target} ${fileName} ${fileSize}`;
      console.log("📤 Sending file header:", header);
      socketRef.current.send(header);

      // Send file data
      console.log("📤 Sending file data:", fileSize, "bytes");
      socketRef.current.send(fileData);

      return true;
    } catch (error) {
      console.error("❌ Error sending file:", error);
      return false;
    }
  };

  // Text-based file send command (for manual commands like "/sendfile Bitan /path/to/file")
  const sendFileCommand = (command) => {
    // For server-side file reading, just send the command as-is
    // The enhanced server will handle file path parsing and reading
    if (socketRef.current && connected) {
      console.log("📤 Sending file command:", command);
      socketRef.current.send(command);
      return true;
    }
    return false;
  };

  // Direct file path sending (server-side reading)
  const sendFileByPath = (target, filePath) => {
    const command = `/sendfile ${target} ${filePath}`;
    return sendFileCommand(command);
  };

  return { 
    messages, 
    sendMessage, 
    sendFile,
    sendFileCommand,
    connected, 
    incomingFile 
  };
}