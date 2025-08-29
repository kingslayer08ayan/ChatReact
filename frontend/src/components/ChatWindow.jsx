import React from "react";

const ChatWindow = ({ messages }) => {
  return (
    <div className="chat-window" style={{ padding: "10px", border: "1px solid #ccc", height: "300px", overflowY: "scroll" }}>
      {messages.map((msg, i) => (
        <div key={i} style={{ margin: "5px 0" }}>
          {msg}
        </div>
      ))}
    </div>
  );
};

export default ChatWindow;
