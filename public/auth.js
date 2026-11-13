document.addEventListener("DOMContentLoaded", () => {

    const registerBtn = document.getElementById("registerBtn");
    const loginBtn = document.getElementById("loginBtn");

    if (registerBtn) {
        registerBtn.addEventListener("click", register);
    }

    if (loginBtn) {
        loginBtn.addEventListener("click", login);
    }
});

async function register() {

    console.log("REGISTER CLICKED");

    let username = document.getElementById("username").value;

    let password = document.getElementById("password").value;

   console.log("Sending request...");

    let response = await fetch("/api/register", {

        method: "POST",

        headers: {
            "Content-Type": "application/json"
        },

        body: JSON.stringify({
            username,
            password
        })
    });

   console.log("Response receieved");

    let text = await response.text();

   console.log("server says:", text);

    document.getElementById("message").innerHTML = text;
}

async function login() {

    console.log("LOGIN CLICKED");

    let username = document.getElementById("username").value;
    let password = document.getElementById("password").value;

    console.log("Sending login request...");

    let response = await fetch("/api/login", {
        method: "POST",
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify({ username, password })
    });

    console.log("Login response received");

    let text = await response.text();

    console.log("Server says:", text);

   if (text == "Login successful") {
	window.location.href = "/";
   }

    document.getElementById("message").innerHTML = text;
}
