// Firebase Configuration
const firebaseConfig = {
    apiKey: "AIzaSyDunuaEB9WxsQyLRHoH1ro-dYzb9g9nuek",
    databaseURL: "https://healthcare-3fc3b-default-rtdb.asia-southeast1.firebasedatabase.app"
};

// Backend Configuration
const API_URL = 'http://localhost:3000/api';

// Initialize Firebase
firebase.initializeApp(firebaseConfig);
const database = firebase.database();
const healthcareRef = database.ref('/healthcare/latest');

// Global State for current data
let currentData = {
    weight: 0,
    height: 0,
    temp: 0,
    bpm: 0
};

// DOM Elements
const elements = {
    temp: document.getElementById('temp-val'),
    weight: document.getElementById('weight-val'),
    height: document.getElementById('height-val'),
    bpm: document.getElementById('bpm-val'),
    tempTime: document.getElementById('temp-time'),
    weightTime: document.getElementById('weight-time'),
    heightTime: document.getElementById('height-time'),
    bpmTime: document.getElementById('bpm-time'),
    statusDot: document.getElementById('connection-status'),
    statusText: document.getElementById('status-text'),
    patientName: document.getElementById('patient-name'),
    saveBtn: document.getElementById('save-data-btn'),
    historyBody: document.getElementById('history-body'),
    liveSection: document.getElementById('live-section'),
    historySection: document.getElementById('history-section'),
    btnLive: document.getElementById('btn-live'),
    btnHistory: document.getElementById('btn-history'),
    refreshHistory: document.getElementById('refresh-history'),
    noData: document.getElementById('no-data'),
    toast: document.getElementById('toast')
};

// --- NAVIGATION ---
elements.btnLive.addEventListener('click', () => {
    elements.liveSection.style.display = 'block';
    elements.historySection.style.display = 'none';
    elements.btnLive.classList.add('active');
    elements.btnHistory.classList.remove('active');
});

elements.btnHistory.addEventListener('click', () => {
    elements.liveSection.style.display = 'none';
    elements.historySection.style.display = 'block';
    elements.btnHistory.classList.add('active');
    elements.btnLive.classList.remove('active');
    fetchHistory();
});

// --- UI HELPERS ---
function showToast(message, type = 'success') {
    elements.toast.innerText = message;
    elements.toast.style.borderLeftColor = type === 'success' ? '#10b981' : '#f43f5e';
    elements.toast.classList.add('show');
    setTimeout(() => elements.toast.classList.remove('show'), 3000);
}

function animateValue(element, newValue) {
    const startValue = parseFloat(element.innerText) || 0;
    if (startValue === parseFloat(newValue)) return;
    element.classList.add('updating');
    element.innerText = newValue;
    setTimeout(() => element.classList.remove('updating'), 500);
}

// --- CORE LOGIC ---

// Update UI from Firebase
function updateDisplay(data) {
    if (!data) return;
    const { temp, weight, height, bpm, timestamp } = data;
    const timeString = timestamp ? new Date(timestamp).toLocaleTimeString() : 'Just now';

    // Update global state for saving
    currentData = { weight, height, temp, bpm };

    if (temp !== undefined) {
        animateValue(elements.temp, temp.toFixed(1));
        elements.tempTime.innerText = `Update: ${timeString}`;
    }
    if (weight !== undefined) {
        animateValue(elements.weight, weight.toFixed(2));
        elements.weightTime.innerText = `Update: ${timeString}`;
    }
    if (height !== undefined) {
        animateValue(elements.height, Math.round(height));
        elements.heightTime.innerText = `Update: ${timeString}`;
    }
    if (bpm !== undefined) {
        animateValue(elements.bpm, bpm);
        elements.bpmTime.innerText = `Update: ${timeString}`;
    }

    elements.statusDot.style.background = '#10b981';
    elements.statusDot.style.boxShadow = '0 0 10px #10b981';
    elements.statusText.innerText = 'System Live';
}

// Save data to PostgreSQL
elements.saveBtn.addEventListener('click', async () => {
    const name = elements.patientName.value.trim();
    if (!name) {
        showToast('Please enter patient name!', 'error');
        elements.patientName.focus();
        return;
    }

    elements.saveBtn.disabled = true;
    elements.saveBtn.innerHTML = '<i class="fa-solid fa-spinner fa-spin"></i> Saving...';

    try {
        const response = await fetch(`${API_URL}/save-record`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                patient_name: name,
                ...currentData
            })
        });

        if (response.ok) {
            showToast(`Data for ${name} saved successfully!`);
            elements.patientName.value = '';
        } else {
            throw new Error('Failed to save');
        }
    } catch (error) {
        console.error(error);
        showToast('Connection error to database!', 'error');
    } finally {
        elements.saveBtn.disabled = false;
        elements.saveBtn.innerHTML = '<i class="fa-solid fa-cloud-arrow-up"></i> Save to Database';
    }
});

// Fetch data from PostgreSQL
async function fetchHistory() {
    elements.historyBody.innerHTML = '';
    elements.noData.style.display = 'none';

    try {
        const response = await fetch(`${API_URL}/records`);
        const data = await response.json();

        if (data.length === 0) {
            elements.noData.style.display = 'block';
            return;
        }

        data.forEach(record => {
            const row = document.createElement('tr');
            const date = new Date(record.recorded_at).toLocaleString();
            row.innerHTML = `
                <td>${date}</td>
                <td><strong>${record.patient_name}</strong></td>
                <td>${record.weight}</td>
                <td>${record.height}</td>
                <td>${record.temp}</td>
                <td>${record.bpm}</td>
            `;
            elements.historyBody.appendChild(row);
        });
    } catch (error) {
        console.error(error);
        showToast('Could not load history!', 'error');
    }
}

elements.refreshHistory.addEventListener('click', fetchHistory);

// --- LISTENERS ---
healthcareRef.on('value', (snapshot) => {
    updateDisplay(snapshot.val());
}, (error) => {
    console.error("Firebase Error:", error);
    elements.statusDot.style.background = '#f43f5e';
    elements.statusText.innerText = 'Sync Error';
});

database.ref(".info/connected").on("value", (snap) => {
    if (snap.val() === false) {
        elements.statusDot.style.background = '#94a3b8';
        elements.statusText.innerText = 'Offline';
    }
});
