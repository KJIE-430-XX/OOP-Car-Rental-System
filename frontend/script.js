function showResult(message, success) {
  const result = document.getElementById('result');
  result.textContent = message;
  result.className = `result ${success ? 'success' : 'error'}`;
}

let administratorSession = false;
let currentCustomerId = 0;
let currentVehicles = [];
let activeReservationVehicleIds = new Set();
const damageChecklistSections = [
  {
    title: 'Front Exterior',
    components: [
      'Front bumper', 'Front grille', 'Front bonnet / hood', 'Left headlight',
      'Right headlight', 'Front windshield', 'Front number plate'
    ]
  },
  {
    title: 'Sides and Doors',
    components: [
      'Front-left door', 'Front-right door', 'Rear-left door', 'Rear-right door',
      'Left side mirror', 'Right side mirror', 'Left side panel / fender',
      'Right side panel / fender', 'Side skirts', 'Door handles and locks',
      'Windows and window mechanisms', 'Side indicators and other lights'
    ]
  },
  {
    title: 'Rear Exterior',
    components: [
      'Rear bumper', 'Left taillight', 'Right taillight', 'Boot / trunk / tailgate',
      'Rear windshield', 'Rear number plate'
    ]
  },
  {
    title: 'Wheels and Tyres',
    components: [
      'Front-left tyre and rim', 'Front-right tyre and rim',
      'Rear-left tyre and rim', 'Rear-right tyre and rim', 'Spare tyre (if provided)'
    ]
  },
  {
    title: 'Interior',
    components: [
      'Dashboard', 'Steering wheel', 'Front seats', 'Rear seats', 'Seat belts',
      'Centre console', 'Interior door panels', 'Floor mats and carpet',
      'Roof lining', 'Boot interior'
    ]
  },
  {
    title: 'Mechanical and Other Components',
    components: [
      'Engine warning lights', 'Engine oil / fluid leakage',
      'Visible engine compartment damage', 'Air conditioning', 'Windscreen wipers'
    ]
  }
];

function escapeHtml(value) {
  return String(value).replace(/[&<>"']/g, character => ({
    '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#039;'
  }[character]));
}

function reservationStatusClass(status) {
  const statusClasses = {
    Completed: 'reservation-status-completed',
    Overdue: 'reservation-status-overdue',
    Cancelled: 'reservation-status-cancelled',
    Active: 'reservation-status-active',
    Confirmed: 'reservation-status-confirmed',
    'Awaiting Payment': 'reservation-status-awaiting-payment'
  };

  return statusClasses[status] || '';
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
      <article class="vehicle-item" data-vehicle-id="${escapeHtml(vehicle.id)}">
        <div class="vehicle-summary"><p class="vehicle-kicker">${escapeHtml(vehicle.category)}</p><h2>${escapeHtml(vehicle.brand)} ${escapeHtml(vehicle.model)}</h2>
        <p>${escapeHtml(vehicle.description)}</p></div>
        <dl class="vehicle-meta"><div><dt>Plate</dt><dd>${escapeHtml(vehicle.plate)}</dd></div>
        <div><dt>Status</dt><dd style="color: ${vehicle.status === 'Under_Maintenance' ? '#F59E0B' : ''}">${escapeHtml(vehicle.status === 'Under_Maintenance' ? 'Under Maintenance' : vehicle.status)}</dd></div>
        <div><dt>Current mileage</dt><dd>${Number(vehicle.mileage).toFixed(1)} km</dd></div>
        <div><dt>Daily rate</dt><dd>RM ${Number(vehicle.daily_rate).toFixed(2)}</dd></div></dl>
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
            <button class="text-link button-link" id="edit-vehicle" type="button" onclick="editVehicle('${escapeHtml(vehicle.id)}')">Edit</button>
            <button class="text-link button-link danger-link" type="button" onclick="deleteVehicle('${escapeHtml(vehicle.id)}')">Delete</button>
            <button class="text-link button-link" type="button" id="view-maintenance" onclick="viewMaintenanceHistory('${escapeHtml(vehicle.id)}')">View maintenance history</button>
          </div>
          ${vehicle.status === 'Available' || vehicle.status === 'Under_Maintenance' ? `
            <div class="vehicle-maintenance-action">
              <button class="submit-button" type="button" id="mark-under-maintenance" onclick="toggleVehicleMaintenance('${escapeHtml(vehicle.id)}', '${escapeHtml(vehicle.status)}')">
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
            <label>Daily rate (RM)<input name="daily_rate" type="number" min="0" step="0.01" value="${escapeHtml(vehicle.daily_rate)}" required></label>
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
    const responseText = await response.text();
    let data;
    try {
      data = JSON.parse(responseText);
    } catch {
      throw new Error(
        response.ok
          ? 'The server returned an invalid response. Please restart the application and try again.'
          : `Unable to update vehicle (${response.status}).`
      );
    }
    if (!response.ok) throw new Error(data.message || 'Unable to update vehicle.');
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
  const prompt = currentStatus === 'Available'
    ? 'Enter maintenance notes:'
    : 'Enter maintenance completion notes (optional):';
  const notes = window.prompt(prompt, '');
  if (notes === null) return;

  try {
    const response = await fetch(
      `/api/vehicles/${encodeURIComponent(vehicleId)}/maintenance`,
      {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ notes })
      }
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

async function viewMaintenanceHistory(vehicleId) {
  const result = document.getElementById('vehicle-result');
  try {
    const response = await fetch(
      `/api/vehicles/${encodeURIComponent(vehicleId)}/maintenance-records`
    );
    const data = await response.json();
    if (!response.ok) throw new Error(data.message);
    const vehicleItem = document.querySelector(
      `.vehicle-item[data-vehicle-id="${CSS.escape(String(vehicleId))}"]`
    );
    if (!vehicleItem) throw new Error('Unable to find the selected vehicle.');
    const existingHistory = vehicleItem.querySelector('.maintenance-history');
    if (existingHistory) existingHistory.remove();
    if (!data.records.length) {
      const history = document.createElement('section');
      history.className = 'maintenance-history';
      history.innerHTML = `
        <div class="maintenance-history-heading">
          <h3>Maintenance history</h3>
          <button class="text-link button-link" type="button" data-close-maintenance>
            Close
          </button>
        </div>
        <p class="maintenance-empty">No maintenance records found.</p>
      `;
      vehicleItem.append(history);
      history.querySelector('[data-close-maintenance]').addEventListener('click', () => {
        history.remove();
      });
      result.textContent = '';
      result.className = 'result success';
      return;
    }

    const history = document.createElement('section');
    history.id = 'maintenance-history';
    history.className = 'maintenance-history';
    history.innerHTML = `
      <div class="maintenance-history-heading">
        <h3>Maintenance history</h3>
        <button class="text-link button-link" type="button" data-close-maintenance>
          Close
        </button>
      </div>
      <div class="reservation-table-wrap">
        <table class="reservation-table">
          <thead>
            <tr>
              <th>ID</th>
              <th>Started</th>
              <th>Ended</th>
              <th>Started by</th>
              <th>Notes</th>
              <th>Completion notes</th>
            </tr>
          </thead>
          <tbody>
            ${data.records.map(record => `
              <tr>
                <td>${escapeHtml(record.id)}</td>
                <td>${escapeHtml(record.started_at)}</td>
                <td>${escapeHtml(record.ended_at || 'Ongoing')}</td>
                <td>${escapeHtml(record.started_by_username)}</td>
                <td>${escapeHtml(record.notes || 'No notes.')}</td>
                <td>${escapeHtml(record.completion_notes || '—')}</td>
              </tr>
            `).join('')}
          </tbody>
        </table>
      </div>
    `;
    vehicleItem.append(history);
    history.querySelector('[data-close-maintenance]').addEventListener('click', () => {
      history.remove();
    });
    result.textContent = '';
    result.className = 'result success';
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
  const result = document.getElementById('vehicle-result')
    || document.getElementById('reservation-detail-result');
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

function reservationRow(reservation, adminView) {
  return `
    <tr>
      ${adminView ? `<td>#${escapeHtml(reservation.id)}</td>` : ''}
      ${adminView ? `<td><strong>${escapeHtml(reservation.customer_name)}</strong><small>${escapeHtml(reservation.customer_phone || 'No phone number')}</small></td>` : ''}
      <td>
        <a class="reservation-vehicle-link" href="/reservation.html?id=${encodeURIComponent(reservation.id)}">
          ${escapeHtml(reservation.vehicle_name || `Vehicle #${reservation.vehicle_id}`)}
        </a>
      </td>
      <td><span class="reservation-date">${escapeHtml(reservation.start_date)} <span aria-hidden="true">TO</span> ${escapeHtml(reservation.end_date)}</span></td>
      <td>
        <strong>RM ${Number(reservation.total_fee).toFixed(2)}</strong>
        ${Number(reservation.overdue_fee) > 0 ? `<small>Overdue: $${Number(reservation.overdue_fee).toFixed(2)}</small>` : ''}
      </td>
      <td><span class="reservation-status ${reservationStatusClass(reservation.status)}">${escapeHtml(reservation.status)}</span></td>
      <td class="reservation-actions">
        <a class="reservation-view-link" href="/reservation.html?id=${encodeURIComponent(reservation.id)}">View</a>
        ${!administratorSession && (reservation.status === 'Active' || reservation.status === 'Overdue')
          ? `<button class="button-link" type="button" onclick="checkoutReservation(${reservation.id})">Check out</button>` : ''}
        ${!administratorSession && reservation.status === 'Awaiting Payment'
          ? `<button class="button-link" type="button" onclick="payDamageFee(${reservation.id})">Pay</button>` : ''}
        ${administratorSession && reservation.status === 'Pending Inspection'
          ? `<button class="button-link" type="button" onclick="inspectReservation(${reservation.id})">Inspect</button>` : ''}
        ${(administratorSession || reservation.customer_id === currentCustomerId)
          && reservation.status === 'Confirmed'
          ? `<button class="button-link danger-link" type="button" onclick="cancelReservation(${reservation.id})">Cancel</button>` : ''}
      </td>
    </tr>
  `;
}

function reservationSection(title, reservations, adminView) {
  if (!reservations.length) return '';
  return `
    <section class="reservation-status-section">
      <h3>${title}</h3>
      <div class="reservation-table-wrap">
        <table class="reservation-table">
          <thead><tr>
            ${adminView ? '<th>Reservation ID</th>' : ''}
            ${adminView ? '<th>Customer</th>' : ''}
            <th>Vehicle</th><th>Rental period</th><th>Total</th><th>Status</th><th><span class="sr-only">Actions</span></th>
          </tr></thead>
          <tbody>${reservations.map(reservation => reservationRow(reservation, adminView)).join('')}</tbody>
        </table>
      </div>
    </section>
  `;
}

async function cancelReservation(reservationId) {
  if (!window.confirm('Cancel this booking?')) return;

  try {
    const response = await fetch(`/api/reservations/${reservationId}`, {
      method: 'DELETE'
    });
    const responseText = await response.text();
    let data;
    try {
      data = JSON.parse(responseText);
    } catch {
      throw new Error(
        response.ok
          ? 'The server returned an invalid cancellation response.'
          : responseText || `Unable to cancel booking (${response.status}).`
      );
    }
    if (!response.ok) throw new Error(data.message);
    if (document.body.dataset.reservationPage !== undefined) {
      window.location.href = '/home.html';
      return;
    }
    await loadVehicles();
    await loadReservations();
    showReservationMessage(data.message, true);
  } catch (error) {
    showReservationMessage(error.message, false);
  }
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
    const adminView = administratorSession;
    const reservationContent = adminView
      ? [
          reservationSection(
            'Pending inspection',
            data.reservations.filter(reservation => reservation.status === 'Pending Inspection'),
            true
          ),
          reservationSection(
            'Active or Confirmed',
            data.reservations.filter(reservation => (
              reservation.status === 'Confirmed' || reservation.status === 'Active'
            )),
            true
          ),
          reservationSection(
            'Completed or Overdue',
            data.reservations.filter(reservation => (
              reservation.status === 'Completed' || reservation.status === 'Overdue'
                || reservation.status === 'Awaiting Payment'
            )),
            true
          ),
          reservationSection(
            'Cancelled',
            data.reservations.filter(reservation => reservation.status === 'Cancelled'),
            true
          )
        ].join('')
      : reservationSection('My bookings', data.reservations, false);
    list.innerHTML = `
      <div class="reservation-list-heading">
        <div>
          <p class="eyebrow">BOOKINGS LIST</p>
          <h2>${adminView ? 'All reservations' : 'My bookings'}</h2>
        </div>
        <span class="reservation-count">${data.reservations.length} booking${data.reservations.length === 1 ? '' : 's'}</span>
      </div>
      ${data.reservations.length ? reservationContent : '<p class="reservation-empty">No bookings yet.</p>'}
    `;
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
        <div><dt>Daily rate</dt><dd>RM ${Number(reservation.daily_rate).toFixed(2)}</dd></div>
        <div><dt>Total fee</dt><dd>RM ${Number(reservation.total_fee).toFixed(2)}</dd></div>
        ${reservation.checkout_date
          ? `<div><dt>Checkout date</dt><dd>${escapeHtml(reservation.checkout_date)}</dd></div>`
          : ''}
      </dl>
      ${reservation.status === 'Active' || reservation.status === 'Overdue'
        ? `<button class="submit-button" type="button" onclick="checkoutReservationFromDetails(${reservation.id})">Check out vehicle</button>`
        : ''}
      ${reservation.status === 'Confirmed'
        && (administratorSession || reservation.customer_id === currentCustomerId)
        ? `<button class="submit-button danger-button" type="button" onclick="cancelReservation(${reservation.id})">Cancel booking</button>`
        : ''}
      ${reservation.status === 'Pending Inspection' && administratorSession
        ? `<form class="damage-checklist" onsubmit="submitInspection(event, ${reservation.id})">
            <h3>Vehicle damage checklist</h3>
            <p>Select every component with damage. Fees are calculated automatically.</p>
            <div class="checklist-sections">${damageChecklistSections.map(section => `
              <section class="checklist-section">
                <h4>${escapeHtml(section.title)}</h4>
                <div class="checklist-grid">${section.components.map(component => `
                  <label><input type="checkbox" name="damage" value="${escapeHtml(component)}"> ${escapeHtml(component)}</label>
                `).join('')}</div>
              </section>
            `).join('')}</div>
            <button class="submit-button" type="submit">Submit damage report</button>
          </form>`
        : ''}
      ${Number(reservation.overdue_fee) > 0
        ? `<section class="damage-report overdue-report">
            <h3>Overdue fee</h3>
            <p>Your vehicle was returned after the rental end date.</p>
            <p>Overdue days: <strong>${Number(reservation.overdue_days)} day${Number(reservation.overdue_days) === 1 ? '' : 's'}</strong></p>
            <p>Calculation: <strong>RM ${Number(reservation.overdue_daily_fee).toFixed(2)} x ${Number(reservation.overdue_days)} days = RM ${Number(reservation.overdue_fee).toFixed(2)}</strong></p>
            <p class="damage-total">Overdue fee: <strong>RM ${Number(reservation.overdue_fee).toFixed(2)}</strong></p>
          </section>`
        : ''}
      ${reservation.status === 'Awaiting Payment' && reservation.damage_items
        ? `<section class="damage-report">
            <h3>Damage report</h3>
            <ul>${reservation.damage_items.map(item => `
              <li>${escapeHtml(item.component)} <strong>RM ${Number(item.fee).toFixed(2)}</strong></li>
            `).join('')}</ul>
            <p class="damage-total">Damage fee: <strong>RM ${Number(reservation.damage_fee).toFixed(2)}</strong></p>
            ${!administratorSession && reservation.customer_id === currentCustomerId
              ? `<button class="submit-button" type="button" onclick="payDamageFee(${reservation.id})">Pay and Checkout</button>`
              : ''}
          </section>`
        : ''}
      ${reservation.status === 'Completed' && reservation.damage_paid
        ? `<section class="damage-report"><h3>Paid Damage Report</h3>
            ${reservation.damage_items && reservation.damage_items.length
              ? `<ul>${reservation.damage_items.map(item => `<li>${escapeHtml(item.component)} <strong>RM ${Number(item.fee).toFixed(2)}</strong></li>`).join('')}</ul>`
              : '<p>No damage recorded.</p>'}
            <p class="damage-total">Damage fee paid: <strong>RM ${Number(reservation.damage_fee).toFixed(2)}</strong></p>
          </section>`
        : ''}
    `;
  } catch (error) {
    result.textContent = error.message;
    result.className = 'result error';
  }

}

async function inspectReservation(reservationId) {
  window.location.href = `/reservation.html?id=${encodeURIComponent(reservationId)}`;
}

async function submitInspection(event, reservationId) {
  event.preventDefault();
  const damages = [...event.target.querySelectorAll('input[name="damage"]:checked')]
    .map(input => input.value);
  try {
    const response = await fetch(`/api/reservations/${reservationId}/inspection`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ damages })
    });
    const data = await response.json();
    if (!response.ok) throw new Error(data.message);
    window.location.reload();
  } catch (error) {
    const result = document.getElementById('reservation-detail-result');
    result.textContent = error.message;
    result.className = 'result error';
  }
}

async function payDamageFee(reservationId) {
  if (!window.confirm('Confirm the dummy payment for this damage fee and overdue fee?')) return;
  try {
    const response = await fetch(`/api/reservations/${reservationId}/damage-payment`, { method: 'POST' });
    const data = await response.json();
    if (!response.ok) throw new Error(data.message);
    window.location.reload();
  } catch (error) {
    const result = document.getElementById('reservation-detail-result');
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
      showReservationMessage('Return submitted. The vehicle is pending inspection.', true);
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
    if (!response.ok) {
      window.alert(data.message || 'This vehicle cannot be deleted.');
      throw new Error(data.message || 'This vehicle cannot be deleted.');
    }
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
  currentCustomerId = Number(session.customer_id) || 0;
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