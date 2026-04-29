CREATE TABLE IF NOT EXISTS health_records (
    id SERIAL PRIMARY KEY,
    patient_name VARCHAR(255) NOT NULL,
    weight DECIMAL(5, 2),
    height DECIMAL(5, 2),
    temp DECIMAL(4, 1),
    bpm INTEGER,
    recorded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);-- Create the database
CREATE DATABASE healthcare_db;

-- Connect to the new database
\c healthcare_db

-- Create the tables
CREATE TABLE IF NOT EXISTS health_records (
    id SERIAL PRIMARY KEY,
    patient_name VARCHAR(255) NOT NULL,
    weight DECIMAL(5, 2),
    height DECIMAL(5, 2),
    temp DECIMAL(4, 1),
    bpm INTEGER,
    recorded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Verify
\dt
