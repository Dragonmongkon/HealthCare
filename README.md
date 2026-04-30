# 🏥 ระบบบันทึกและติดตามข้อมูลสุขภาพ (HealthCare Monitoring System)

# :yt:
https://youtube.com/shorts/tzSGTALhhWk

แอปพลิเคชันเว็บแบบ Full-stack สำหรับติดตามข้อมูลสุขภาพแบบเรียลไทม์ และบันทึกประวัติลงฐานข้อมูล โดยเชื่อมต่อกับ Firebase สำหรับข้อมูลสด และ PostgreSQL สำหรับการจัดเก็บข้อมูลถาวร

## 🌟 คุณสมบัติ (Features)

-   **Live Dashboard**: แสดงค่าสุขภาพแบบเรียลไทม์ (อุณหภูมิ, น้ำหนัก, ส่วนสูง, อัตราการเต้นของหัวใจ) ผ่าน Firebase
-   **Patient Registration**: บันทึกชื่อผู้ป่วยพร้อมกับข้อมูลสุขภาพปัจจุบันลงในฐานข้อมูล
-   **History Records**: เรียกดูประวัติการบันทึกข้อมูลสุขภาพทั้งหมดจากฐานข้อมูล PostgreSQL
-   **Responsive Design**: รองรับการแสดงผลบนคอมพิวเตอร์และมือถือ พร้อมดีไซน์ที่ทันสมัย (Modern UI)

## 🛠️ เทคโนโลยีที่ใช้ (Tech Stack)

-   **Frontend**: HTML5, CSS3 (Vanilla CSS), JavaScript (Vanilla JS)
-   **Backend**: Node.js, Express.js
-   **Database**: 
    -   **Firebase Realtime Database**: สำหรับรับข้อมูลสดจากเซนเซอร์ (เช่น ESP32)
    -   **PostgreSQL**: สำหรับจัดเก็บประวัติข้อมูลสุขภาพถาวร
-   **Icons**: Font Awesome 6

## 📂 โครงสร้างโฟลเดอร์

-   `web/`: ไฟล์ฝั่งหน้าเว็บ (HTML, CSS, JS)
-   `server.js`: เซิร์ฟเวอร์ Backend สำหรับจัดการฐานข้อมูล PostgreSQL
-   `schema.sql`: ไฟล์คำสั่ง SQL สำหรับสร้างตารางในฐานข้อมูล
-   `.env`: ไฟล์เก็บค่ากำหนดการเชื่อมต่อ (Credentials)

## 🚀 วิธีการติดตั้งและเริ่มใช้งาน

### 1. เตรียมฐานข้อมูล PostgreSQL
รันคำสั่งในไฟล์ `schema.sql` ในฐานข้อมูลของคุณเพื่อสร้างตาราง `health_records`

### 2. ติดตั้ง Dependencies
เปิด Terminal ในโฟลเดอร์โปรเจกต์แล้วรัน:
```bash
npm install
```

### 3. ตั้งค่าสภาพแวดล้อม (Environment Variables)
สร้างไฟล์ `.env` ใน Root Directory และกำหนดค่าดังนี้:
```env
PORT=3000
DB_USER=your_username
DB_HOST=localhost
DB_NAME=healthcare_db
DB_PASSWORD=your_password
DB_PORT=5432
```

### 4. ตั้งค่า Firebase
แก้ไขไฟล์ `web/script.js` และใส่ `firebaseConfig` ของคุณเองเพื่อให้ระบบ Live Dashboard ทำงานได้

### 5. รันโปรเจกต์
รันเซิร์ฟเวอร์ด้วยคำสั่ง:
```bash
npm start
```
จากนั้นเปิด Browser ไปที่ `http://localhost:3000` (หรือเปิด `web/index.html` โดยตรงหากรัน Backend แยกต่างหาก)

## 📝 ข้อมูลเพิ่มเติม
โปรเจกต์นี้ถูกออกแบบมาเพื่อทำงานร่วมกับอุปกรณ์ IoT (เช่น ESP32) ที่ส่งข้อมูลเข้า Firebase โดยตรง และใช้ Web Dashboard นี้เป็นหน้าจอควบคุมและบันทึกข้อมูลหลัก
