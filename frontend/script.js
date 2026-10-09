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

async function logout() {
  const response = await fetch('/api/logout', { method: 'POST' });
  if (response.ok) {
    window.location.href = '/login.html';
    return;
  }

  const data = await response.json();
  const result = document.getElementById('result') || document.getElementById('vehicle-result');
  if (result) {
    result.textContent = data.message || 'Unable to log out.';
    result.className = 'result error';
  }
}

function displayVehicles(vehicles) {
    const list = document.getElementById('vehicle-list');
    if (!vehicles.length) {
      list.innerHTML = '';
      return;
    }
    const escapeHtml = value => String(value).replace(/[&<>"']/g, character => ({
      '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#039;'
    }[character]));
    list.innerHTML = vehicles.map(vehicle => `
      <article class="vehicle-item">
        <div><p class="vehicle-kicker">${escapeHtml(vehicle.category)}</p><h2>${escapeHtml(vehicle.brand)} ${escapeHtml(vehicle.model)}</h2>
        <p>${escapeHtml(vehicle.description)}</p></div>
        <dl><div><dt>Plate</dt><dd>${escapeHtml(vehicle.plate)}</dd></div>
        <div><dt>Status</dt><dd>${escapeHtml(vehicle.status)}</dd></div>
        <div><dt>Daily rate</dt><dd>$${Number(vehicle.daily_rate).toFixed(2)}</dd></div></dl>
      </article>
    `).join('');
}

async function loadVehicles() {
    const category = document.body.dataset.category;
    if (!category) return;
    const result = document.getElementById('vehicle-result');

    try {
      // Different category different page
      const response = await fetch(`/api/vehicles?category=${encodeURIComponent(category)}`);
      const data = await response.json();
      if (!response.ok) throw new Error(data.message);
      displayVehicles(data.vehicles);
    } catch (error) {
      result.textContent = error.message;
      result.className = 'result error';
    }

}

async function addVehicle(event) {
    event.preventDefault();
    const form = event.target;
    const result = document.getElementById('vehicle-result') || document.getElementById('result');
    try {
      const fields = Object.fromEntries(new FormData(form));
      const response = await fetch('/api/vehicles', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(fields)
      });
      const responseText = await response.text();
      let data;
      try {
        data = JSON.parse(responseText);
      } catch {
        throw new Error('The server returned an invalid response. Please restart the application and try again.');
      }
      result.textContent = data.message;
      result.className = `result ${response.ok ? 'success' : 'error'}`;
      if (response.ok) {
        form.reset();
        form.classList.add('hidden');
      }
    } catch (error) {
      result.textContent = error.message;
      result.className = 'result error';
    }
}

async function initializeAdminControls() {
  const toggle = document.getElementById('add-vehicle-toggle');
  const form = document.getElementById('add-vehicle-form');
  const isAddVehiclePage = Boolean(form);
  if (!toggle && !form) return;

  const sessionResponse = await fetch('/api/session');
  const session = await sessionResponse.json();
  if (!session.administrator) {
    if (toggle) toggle.remove();
    if (form) form.remove();
    if (isAddVehiclePage) {
      window.location.href = '/home.html';
    }
    return;
  }

  if (toggle) {
    toggle.classList.remove('hidden');
    toggle.addEventListener('click', () => {
      window.location.href = '/add_vehicle.html';
    });
  }
  if (form) {
    form.classList.remove('hidden');
    form.addEventListener('submit', addVehicle);
  }
}

if (document.body.dataset.category) {
  loadVehicles();
}

const logoutButton = document.getElementById('logout-button');
if (logoutButton) {
  logoutButton.addEventListener('click', logout);
}

initializeAdminControls();