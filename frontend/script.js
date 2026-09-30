function showResult(message, success) {
  const result = document.getElementById('result');
  result.textContent = message;
  result.className = `result ${success ? 'success' : 'error'}`;
}

async function login(event) {
  event.preventDefault();

  const res = await fetch('/api/login', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      username: document.getElementById('login-username').value,
      password: document.getElementById('login-password').value
    })
  });

  const data = await res.json();
  showResult(data.message, res.ok);
  if (res.ok) {
    window.location.href = '/home.html';
  }
}

async function register(event) {
  event.preventDefault();

  const res = await fetch('/api/register', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      username: document.getElementById('register-username').value,
      password: document.getElementById('register-password').value,
      name: document.getElementById('register-name').value,
      phone_number: document.getElementById('register-phone').value
    })
  });

  const data = await res.json();
  showResult(data.message, res.ok);
  if (res.ok) {
    document.getElementById('register-form').reset();
    window.location.href = '/login.html?registered=1';
  }
}