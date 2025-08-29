import React from "react";

const UserList = ({ users }) => {
  return (
    <div style={{ borderRight: "1px solid #ccc", padding: "10px", width: "150px" }}>
      <h4>Users</h4>
      <ul>
        {users.map((user, i) => (
          <li key={i}>{user}</li>
        ))}
      </ul>
    </div>
  );
};

export default UserList;
