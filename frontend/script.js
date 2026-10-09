function showResult(message, success) {
  const result = document.getElementById('result');
  result.textContent = message;
  result.className = `result ${success ? 'success' : 'error'}`;
}

let administratorSession = false;

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
        <div class="vehicle-summary"><p class="vehicle-kicker">${escapeHtml(vehicle.category)}</p><h2>${escapeHtml(vehicle.brand)} ${escapeHtml(vehicle.model)}</h2>
        <p>${escapeHtml(vehicle.description)}</p></div>
        <dl class="vehicle-meta"><div><dt>Plate</dt><dd>${escapeHtml(vehicle.plate)}</dd></div>
        <div><dt>Status</dt><dd>${escapeHtml(vehicle.status)}</dd></div>
        <div><dt>Daily rate</dt><dd>$${Number(vehicle.daily_rate).toFixed(2)}</dd></div></dl>
        ${administratorSession ? `
          <div class="vehicle-actions">
            <button class="text-link button-link" type="button" onclick="editVehicle('${escapeHtml(vehicle.id)}')">Edit</button>
            <button class="text-link button-link danger-link" type="button" onclick="deleteVehicle('${escapeHtml(vehicle.id)}')">Delete</button>
          </div>
          <form class="vehicle-edit-form hidden" data-vehicle-id="${escapeHtml(vehicle.id)}">
            <label>License plate<input name="plate" value="${escapeHtml(vehicle.plate)}" required></label>
            <label>Brand<input name="brand" value="${escapeHtml(vehicle.brand)}" required></label>
            <label>Model<input name="model" value="${escapeHtml(vehicle.model)}" required></label>
            <label>Description<input name="description" value="${escapeHtml(vehicle.description)}" required></label>
            <label>Category<select name="category">
              <option value="StandardCar" ${vehicle.category === 'StandardCar' ? 'selected' : ''}>StandardCar</option>
              <option value="LuxuryCar" ${vehicle.category === 'LuxuryCar' ? 'selected' : ''}>LuxuryCar</option>
              <option value="SUV" ${vehicle.category === 'SUV' ? 'selected' : ''}>SUV</option>
            </select></label>
            <label>Status<select name="status">
              <option value="Available" ${vehicle.status === 'Available' ? 'selected' : ''}>Available</option>
              <option value="Rented" ${vehicle.status === 'Rented' ? 'selected' : ''}>Rented</option>
              <option value="Under_Maintenance" ${vehicle.status === 'Under_Maintenance' ? 'selected' : ''}>Under_Maintenance</option>
            </select></label>
            <label>Mileage<input name="mileage" type="number" min="0" step="0.1" value="${escapeHtml(vehicle.mileage)}" required></label>
            <label>Daily rate<input name="daily_rate" type="number" min="0" step="0.01" value="${escapeHtml(vehicle.daily_rate)}" required></label>
            <label>Security deposit<input name="security_deposit" type="number" min="0" step="0.01" value="${escapeHtml(vehicle.security_deposit)}" required></label>
            <label>Insurance rate<input name="insurance_rate" type="number" min="0" step="0.01" value="${escapeHtml(vehicle.insurance_rate)}" required></label>
            <div><button class="submit-button" type="submit">Save changes</button>
              <button class="text-link button-link" type="button" onclick="editVehicle('${escapeHtml(vehicle.id)}')">Cancel</button></div>
          </form>
        ` : ''}
      </article>
    `).join('');

    list.querySelectorAll('.vehicle-edit-form').forEach(form => {
      form.addEventListener('submit', updateVehicle);
    });
}

function editVehicle(vehicleId) {
  const form = document.querySelector(`.vehicle-edit-form[data-vehicle-id="${CSS.escape(vehicleId)}"]`);
  if (form) form.classList.toggle('hidden');
}

async function updateVehicle(event) {
  event.preventDefault();
  const form = event.target;
  const vehicleId = form.dataset.vehicleId;
  const result = document.getElementById('vehicle-result');

  try {
    const response = await fetch(`/api/vehicles/${encodeURIComponent(vehicleId)}`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(Object.fromEntries(new FormData(form)))
    });
    const data = await response.json();
    if (!response.ok) throw new Error(data.message);
    result.textContent = data.message;
    result.className = 'result success';
    await loadVehicles();
  } catch (error) {
    result.textContent = error.message;
    result.className = 'result error';
  }
}

async function deleteVehicle(vehicleId) {
  if (!window.confirm('Delete this vehicle? This action cannot be undone.')) return;
  const result = document.getElementById('vehicle-result');

  try {
    const response = await fetch(`/api/vehicles/${encodeURIComponent(vehicleId)}`, { method: 'DELETE' });
    const data = await response.json();
    if (!response.ok) throw new Error(data.message);
    result.textContent = data.message;
    result.className = 'result success';
    await loadVehicles();
  } catch (error) {
    result.textContent = error.message;
    result.className = 'result error';
  }
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
  const sessionResponse = await fetch('/api/session');
  const session = await sessionResponse.json();
  administratorSession = session.administrator;
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

const logoutButton = document.getElementById('logout-button');
if (logoutButton) {
  logoutButton.addEventListener('click', logout);
}

async function initializePage() {
  await initializeAdminControls();
  if (document.body.dataset.category) {
    await loadVehicles();
  }
}

initializePage();