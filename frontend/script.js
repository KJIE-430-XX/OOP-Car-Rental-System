function showResult(message, success) {
  const result = document.getElementById('result');
  result.textContent = message;
  result.className = `result ${success ? 'success' : 'error'}`;
}

let administratorSession = false;
let currentVehicles = [];
let activeReservationVehicleIds = new Set();

function escapeHtml(value) {
  return String(value).replace(/[&<>"']/g, character => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#039;'
  }[character]));
}

function updateLuxuryPricingFields(form) {
  const category = form.elements.category;
  const pricingFields = form.querySelector('[data-luxury-pricing-fields]');
  if (!category || !pricingFields) return;

  const isLuxury = category.value === 'LuxuryCar';
  pricingFields.classList.toggle('hidden', !isLuxury);
  pricingFields.querySelectorAll('input').forEach(input => {
    input.required = isLuxury;
    if (!isLuxury && !input.value) input.value = '0';
  });
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
    currentVehicles = vehicles;
    const list = document.getElementById('vehicle-list');
    if (!vehicles.length) {
      list.innerHTML = '';
      return;
    }
    list.innerHTML = vehicles.map(vehicle => `
      <article class="vehicle-item">
        <div class="vehicle-summary"><p class="vehicle-kicker">${escapeHtml(vehicle.category)}</p><h2>${escapeHtml(vehicle.brand)} ${escapeHtml(vehicle.model)}</h2>
        <p>${escapeHtml(vehicle.description)}</p></div>
        <dl class="vehicle-meta"><div><dt>Plate</dt><dd>${escapeHtml(vehicle.plate)}</dd></div>
        <div><dt>Status</dt><dd>${escapeHtml(vehicle.status)}</dd></div>
        <div><dt>Current mileage</dt><dd>${Number(vehicle.mileage).toFixed(1)} km</dd></div>
        <div><dt>Daily rate</dt><dd>$${Number(vehicle.daily_rate).toFixed(2)}</dd></div></dl>
        ${!administratorSession && vehicle.available
          && !activeReservationVehicleIds.has(String(vehicle.id)) ? `
          <div class="vehicle-actions">
            <button class="submit-button" type="button" onclick="bookVehicle('${escapeHtml(vehicle.id)}')">Book vehicle</button>
          </div>
        ` : ''}
        ${!administratorSession && activeReservationVehicleIds.has(String(vehicle.id)) ? `
          <div class="vehicle-actions">
            <span class="vehicle-rented-label">Rented</span>
          </div>
        ` : ''}
        ${administratorSession ? `
          <div class="vehicle-actions">
            <button class="text-link button-link" type="button" onclick="editVehicle('${escapeHtml(vehicle.id)}')">Edit</button>
            <button class="text-link button-link danger-link" type="button" onclick="deleteVehicle('${escapeHtml(vehicle.id)}')">Delete</button>
          </div>
          ${vehicle.status === 'Available' || vehicle.status === 'Under_Maintenance' ? `
            <div class="vehicle-maintenance-action">
              <button class="submit-button" type="button" onclick="toggleVehicleMaintenance('${escapeHtml(vehicle.id)}', '${escapeHtml(vehicle.status)}')">
                ${vehicle.status === 'Available' ? 'Mark under maintenance' : 'Mark available'}
              </button>
            </div>
          ` : ''}
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
            <div data-luxury-pricing-fields>
              <label>Security deposit<input name="security_deposit" type="number" min="0" step="0.01" value="${escapeHtml(vehicle.security_deposit)}" placeholder="e.g. 500.00"></label>
              <label>Insurance rate<input name="insurance_rate" type="number" min="0" step="0.01" value="${escapeHtml(vehicle.insurance_rate)}" placeholder="e.g. 20.00"></label>
            </div>
            <div><button class="submit-button" type="submit">Save changes</button>
              <button class="text-link button-link" type="button" onclick="editVehicle('${escapeHtml(vehicle.id)}')">Cancel</button></div>
          </form>
        ` : ''}
      </article>
    `).join('');

    list.querySelectorAll('.vehicle-edit-form').forEach(form => {
      updateLuxuryPricingFields(form);
      form.elements.category.addEventListener('change', () => updateLuxuryPricingFields(form));
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

async function toggleVehicleMaintenance(vehicleId, currentStatus) {
  const result = document.getElementById('vehicle-result');
  const action = currentStatus === 'Available'
    ? 'mark this vehicle under maintenance'
    : 'mark this vehicle available';
  if (!window.confirm(`Are you sure you want to ${action}?`)) return;

  try {
    const response = await fetch(
      `/api/vehicles/${encodeURIComponent(vehicleId)}/maintenance`,
      { method: 'POST' }
    );
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

async function bookVehicle(vehicleId) {
  const dates = await chooseBookingDates();
  if (!dates) return;

  try {
    const response = await fetch('/api/reservations', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        vehicle_id: vehicleId,
        start_date: dates.startDate,
        end_date: dates.endDate
      })
    });
    const data = await response.json();
    if (!response.ok) throw new Error(data.message);
    showReservationMessage(data.message, true);
    await loadVehicles();
    await loadReservations();
  } catch (error) {
    showBookingError(error.message);
  }
}

function chooseBookingDates() {
  return new Promise(resolve => {
    const currentDate = new Date();
    const today = [
      currentDate.getFullYear(),
      String(currentDate.getMonth() + 1).padStart(2, '0'),
      String(currentDate.getDate()).padStart(2, '0')
    ].join('-');
    const modal = document.createElement('div');
    modal.className = 'booking-modal';
    modal.innerHTML = `
      <form class="booking-dialog">
        <h2>Book vehicle</h2>
        <label>Start date
          <input name="start_date" type="date" min="${today}" required>
        </label>
        <label>End date
          <input name="end_date" type="date" min="${today}" required>
        </label>
        <p class="booking-dialog-error hidden"></p>
        <div class="booking-dialog-actions">
          <button class="submit-button" type="submit">Continue</button>
          <button class="text-link button-link" type="button" data-cancel>Cancel</button>
        </div>
      </form>
    `;
    document.body.appendChild(modal);

    const form = modal.querySelector('form');
    const startInput = form.elements.start_date;
    const endInput = form.elements.end_date;
    const error = modal.querySelector('.booking-dialog-error');

    startInput.addEventListener('change', () => {
      endInput.min = startInput.value || today;
      if (endInput.value && endInput.value < endInput.min) {
        endInput.value = '';
      }
    });

    const close = value => {
      modal.remove();
      resolve(value);
    };

    form.addEventListener('submit', event => {
      event.preventDefault();
      if (endInput.value < startInput.value) {
        error.textContent = 'End date must be on or after the start date.';
        error.classList.remove('hidden');
        return;
      }
      close({
        startDate: startInput.value,
        endDate: endInput.value
      });
    });
    modal.querySelector('[data-cancel]').addEventListener('click', () => close(null));
    startInput.focus();
  });
}

function showReservationMessage(message, success) {
  const result = document.getElementById('vehicle-result');
  if (result) {
    result.textContent = message;
    result.className = `result ${success ? 'success' : 'error'}`;
  }
}

function showBookingError(message) {
  const modal = document.createElement('div');
  modal.className = 'booking-modal';
  modal.innerHTML = `
    <section class="booking-dialog" role="alertdialog" aria-modal="true"
      aria-labelledby="booking-error-title">
      <h2 id="booking-error-title">Unable to book vehicle</h2>
      <p class="booking-dialog-error">${escapeHtml(message)}</p>
      <div class="booking-dialog-actions">
        <button class="submit-button" type="button" data-close>Close</button>
      </div>
    </section>
  `;
  document.body.appendChild(modal);

  const close = () => modal.remove();
  modal.querySelector('[data-close]').addEventListener('click', close);
  modal.addEventListener('click', event => {
    if (event.target === modal) close();
  });
  modal.querySelector('[data-close]').focus();
}

function chooseReturnedMileage(errorMessage = '') {
  return new Promise(resolve => {
    const modal = document.createElement('div');
    modal.className = 'booking-modal';
    modal.innerHTML = `
      <form class="booking-dialog">
        <h2>Check out vehicle</h2>
        <p class="booking-dialog-error${errorMessage ? '' : ' hidden'}">${errorMessage ? escapeHtml(errorMessage) : ''}</p>
        <label>Returned mileage
          <input name="returned_mileage" type="number" min="0" step="0.1" required>
        </label>
        <div class="booking-dialog-actions">
          <button class="submit-button" type="submit">Confirm checkout</button>
          <button class="text-link button-link" type="button" data-cancel>Cancel</button>
        </div>
      </form>
    `;
    document.body.appendChild(modal);

    const form = modal.querySelector('form');
    const close = value => {
      modal.remove();
      resolve(value);
    };

    form.addEventListener('submit', event => {
      event.preventDefault();
      close(Number(form.elements.returned_mileage.value));
    });
    modal.querySelector('[data-cancel]').addEventListener('click', () => close(null));
    form.elements.returned_mileage.focus();
  });
}

async function checkoutVehicle(reservationId) {
  let errorMessage = '';

  while (true) {
    const returnedMileage = await chooseReturnedMileage(errorMessage);
    if (returnedMileage === null) return false;

    const response = await fetch(`/api/reservations/${reservationId}/checkout`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ returned_mileage: returnedMileage })
    });
    const data = await response.json();
    if (response.ok) return true;

    if (response.status === 409) {
      errorMessage = data.message;
      continue;
    }

    throw new Error(data.message);
  }
}

function ensureReservationList() {
  let list = document.getElementById('reservation-list');
  if (!list) {
    list = document.createElement('section');
    list.id = 'reservation-list';
    list.className = 'reservation-list';
    document.querySelector('.category-page').appendChild(list);
  }
  return list;
}

async function loadReservations() {
  if (!document.body.dataset.category) return;

  try {
    const response = await fetch('/api/reservations');
    const data = await response.json();
    if (!response.ok) throw new Error(data.message);
    activeReservationVehicleIds = new Set(
      data.reservations
        .filter(reservation => (
          reservation.status === 'Confirmed' || reservation.status === 'Active'
            || reservation.status === 'Overdue'
        ))
        .map(reservation => String(reservation.vehicle_id))
    );
    displayVehicles(currentVehicles);
    const list = ensureReservationList();
    list.innerHTML = '<h2>My bookings</h2>' + (
      data.reservations.length
        ? data.reservations.map(reservation => `
          <article class="reservation-item">
            <a class="reservation-item-link" href="/reservation.html?id=${encodeURIComponent(reservation.id)}">
              <span>Vehicle #${escapeHtml(reservation.vehicle_id)}</span>
              <span>${escapeHtml(reservation.start_date)} to ${escapeHtml(reservation.end_date)}</span>
              <span>$${Number(reservation.total_fee).toFixed(2)}</span>
              <span>${escapeHtml(reservation.status)}</span>
            </a>
            ${reservation.status === 'Active' || reservation.status === 'Overdue'
              ? `<button class="text-link button-link" type="button" onclick="checkoutReservation(${reservation.id})">Check out</button>`
              : ''}
          </article>
        `).join('')
        : '<p>No bookings yet.</p>'
    );
  } catch (error) {
    showReservationMessage(error.message, false);
  }
}

async function loadReservationDetails() {
  const reservationId = new URLSearchParams(window.location.search).get('id');
  const details = document.getElementById('reservation-details');
  const result = document.getElementById('reservation-detail-result');
  if (!reservationId || !details) return;

  try {
    const response = await fetch(`/api/reservations/${encodeURIComponent(reservationId)}`);
    const reservation = await response.json();
    if (!response.ok) throw new Error(reservation.message);

    details.innerHTML = `
      <div class="reservation-detail-heading">
        <p class="vehicle-kicker">${escapeHtml(reservation.category)}</p>
        <h2>${escapeHtml(reservation.brand)} ${escapeHtml(reservation.model)}</h2>
        <p>${escapeHtml(reservation.description)}</p>
      </div>
      <dl class="reservation-detail-grid">
        <div><dt>Booking number</dt><dd>#${escapeHtml(reservation.id)}</dd></div>
        <div><dt>Vehicle ID</dt><dd>#${escapeHtml(reservation.vehicle_id)}</dd></div>
        <div><dt>License plate</dt><dd>${escapeHtml(reservation.plate)}</dd></div>
        <div><dt>Status</dt><dd>${escapeHtml(reservation.status)}</dd></div>
        <div><dt>Start date</dt><dd>${escapeHtml(reservation.start_date)}</dd></div>
        <div><dt>End date</dt><dd>${escapeHtml(reservation.end_date)}</dd></div>
        <div><dt>Daily rate</dt><dd>$${Number(reservation.daily_rate).toFixed(2)}</dd></div>
        <div><dt>Total fee</dt><dd>$${Number(reservation.total_fee).toFixed(2)}</dd></div>
        ${reservation.checkout_date
          ? `<div><dt>Checkout date</dt><dd>${escapeHtml(reservation.checkout_date)}</dd></div>`
          : ''}
      </dl>
      ${reservation.status === 'Active' || reservation.status === 'Overdue'
        ? `<button class="submit-button" type="button" onclick="checkoutReservationFromDetails(${reservation.id})">Check out vehicle</button>`
        : ''}
    `;
  } catch (error) {
    result.textContent = error.message;
    result.className = 'result error';
  }
}

async function checkoutReservationFromDetails(reservationId) {
  try {
    if (await checkoutVehicle(reservationId)) {
      window.location.reload();
    }
  } catch (error) {
    const result = document.getElementById('reservation-detail-result');
    result.textContent = error.message;
    result.className = 'result error';
  }
}

async function checkoutReservation(reservationId) {
  try {
    if (await checkoutVehicle(reservationId)) {
      showReservationMessage('Vehicle checked out successfully.', true);
      await loadVehicles();
      await loadReservations();
    }
  } catch (error) {
    showReservationMessage(error.message, false);
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
      if (fields.category !== 'LuxuryCar') {
        fields.security_deposit = '0';
        fields.insurance_rate = '0';
      }
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
        updateLuxuryPricingFields(form);
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
    updateLuxuryPricingFields(form);
    form.elements.category.addEventListener('change', () => updateLuxuryPricingFields(form));
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
    await loadReservations();
  }
  if (document.body.dataset.reservationPage !== undefined) {
    await loadReservationDetails();
  }
}

initializePage();