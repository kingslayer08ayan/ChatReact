import React, { useState } from "react";

const MessageInput = ({ onSend }) => {
  const [text, setText] = useState("");

  const handleSend = () => {
    if (text.trim()) {
      onSend(text);
      setText("");
    }
  };

  return (
    <div style={{ marginTop: "10px" }}>
      <input
        type="text"
        value={text}
        onChange={(e) => setText(e.target.value)}
        style={{ padding: "8px", width: "80%" }}
        placeholder="Enter your name..."
      />
      <button onClick={handleSend} style={{ padding: "8px" }}>Send</button>
    </div>
  );
};

export default MessageInput;
