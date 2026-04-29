const express = require('express');
const { Pool } = require('pg');
const cors = require('cors');
const path = require('path');
require('dotenv').config();

const app = express();
const port = process.env.PORT || 3000;

// Middleware
app.use(cors());
app.use(express.json());
app.use(express.static(path.join(__dirname, 'web')));

// PostgreSQL Connection
const pool = new Pool({
    user: process.env.DB_USER || 'postgres',
    host: process.env.DB_HOST || 'localhost',
    database: process.env.DB_NAME || 'healthcare_db',
    password: process.env.DB_PASSWORD || 'password',
    port: process.env.DB_PORT || 5432,
});

// Test DB Connection
pool.query('SELECT NOW()', (err, res) => {
    if (err) {
        console.error('❌ Database connection error:', err.stack);
    } else {
        console.log('✅ PostgreSQL Connected at:', res.rows[0].now);
    }
});

// Route: Save a record
app.post('/api/save-record', async (req, res) => {
    const { patient_name, weight, height, temp, bpm } = req.body;

    if (!patient_name) {
        return res.status(400).json({ error: 'Patient name is required' });
    }

    try {
        const query = `
            INSERT INTO health_records (patient_name, weight, height, temp, bpm)
            VALUES ($1, $2, $3, $4, $5)
            RETURNING *;
        `;
        const values = [patient_name, weight, height, temp, bpm];
        const result = await pool.query(query, values);
        
        res.status(201).json({
            message: 'Record saved successfully',
            data: result.rows[0]
        });
    } catch (err) {
        console.error(err);
        res.status(500).json({ error: 'Database error while saving record' });
    }
});

// Route: Get all records
app.get('/api/records', async (req, res) => {
    try {
        const result = await pool.query('SELECT * FROM health_records ORDER BY recorded_at DESC');
        res.json(result.rows);
    } catch (err) {
        console.error(err);
        res.status(500).json({ error: 'Database error while fetching records' });
    }
});

app.listen(port, () => {
    console.log(`🚀 Backend server running at http://localhost:${port}`);
});
