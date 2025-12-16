DROP TABLE IF EXISTS tickets;
DROP TABLE IF EXISTS schedules;
DROP TABLE IF EXISTS route_stops;
DROP TABLE IF EXISTS routes;
DROP TABLE IF EXISTS seats;
DROP TABLE IF EXISTS carriages;
DROP TABLE IF EXISTS trains;
DROP TABLE IF EXISTS stations;
DROP TABLE IF EXISTS sessions;
DROP TABLE IF EXISTS verification_codes;
DROP TABLE IF EXISTS audit_logs;
DROP TABLE IF EXISTS users;

CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    surname VARCHAR(100) NOT NULL,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    password_salt VARCHAR(32) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    is_verified BOOLEAN DEFAULT FALSE,
    last_login TIMESTAMP,
    failed_login_attempts INTEGER DEFAULT 0,
    locked_until TIMESTAMP,
    CONSTRAINT email_format CHECK (email ~* '^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}$')
);

CREATE TABLE audit_logs (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id) ON DELETE SET NULL,
    action VARCHAR(100) NOT NULL,
    ip_address VARCHAR(45),
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    details TEXT,
    success BOOLEAN DEFAULT TRUE
);

CREATE TABLE verification_codes (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    email VARCHAR(255) NOT NULL,
    code VARCHAR(6) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NOT NULL,
    is_used BOOLEAN DEFAULT FALSE
);

CREATE TABLE sessions (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    session_token VARCHAR(64) UNIQUE NOT NULL,
    ip_address VARCHAR(45),
    user_agent TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP NOT NULL,
    is_active BOOLEAN DEFAULT TRUE
);

CREATE TABLE stations (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    city VARCHAR(100) NOT NULL,
    code VARCHAR(10) UNIQUE NOT NULL,
    latitude DOUBLE PRECISION NOT NULL,
    longitude DOUBLE PRECISION NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE trains (
    id SERIAL PRIMARY KEY,
    train_number VARCHAR(20) UNIQUE NOT NULL,
    train_type VARCHAR(50) NOT NULL,
    total_seats INTEGER NOT NULL,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE routes (
    id SERIAL PRIMARY KEY,
    train_id INTEGER NOT NULL REFERENCES trains(id) ON DELETE CASCADE,
    route_name VARCHAR(255) NOT NULL,
    valid_from DATE NOT NULL,
    valid_to DATE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE route_stops (
    id SERIAL PRIMARY KEY,
    route_id INTEGER NOT NULL REFERENCES routes(id) ON DELETE CASCADE,
    station_id INTEGER NOT NULL REFERENCES stations(id) ON DELETE CASCADE,
    stop_order INTEGER NOT NULL,
    arrival_time TIME,
    departure_time TIME NOT NULL,
    stop_duration_minutes INTEGER DEFAULT 0,
    price_from_start DOUBLE PRECISION NOT NULL,
    UNIQUE(route_id, stop_order),
    UNIQUE(route_id, station_id)
);

CREATE TABLE carriages (
    id SERIAL PRIMARY KEY,
    train_id INTEGER NOT NULL REFERENCES trains(id) ON DELETE CASCADE,
    carriage_number INTEGER NOT NULL,
    carriage_type VARCHAR(50) NOT NULL,
    total_seats INTEGER NOT NULL,
    price_multiplier DOUBLE PRECISION DEFAULT 1.0,
    UNIQUE(train_id, carriage_number)
);

CREATE TABLE seats (
    id SERIAL PRIMARY KEY,
    carriage_id INTEGER NOT NULL REFERENCES carriages(id) ON DELETE CASCADE,
    seat_number INTEGER NOT NULL,
    seat_type VARCHAR(50) NOT NULL,
    is_available BOOLEAN DEFAULT TRUE,
    UNIQUE(carriage_id, seat_number)
);

CREATE TABLE schedules (
    id SERIAL PRIMARY KEY,
    route_id INTEGER NOT NULL REFERENCES routes(id) ON DELETE CASCADE,
    departure_date DATE NOT NULL,
    status VARCHAR(20) DEFAULT 'active',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(route_id, departure_date)
);

CREATE TABLE tickets (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    schedule_id INTEGER NOT NULL REFERENCES schedules(id) ON DELETE CASCADE,
    seat_id INTEGER NOT NULL REFERENCES seats(id) ON DELETE CASCADE,
    departure_station_id INTEGER NOT NULL REFERENCES stations(id),
    arrival_station_id INTEGER NOT NULL REFERENCES stations(id),
    ticket_number VARCHAR(50) UNIQUE NOT NULL,
    price DOUBLE PRECISION NOT NULL,
    status VARCHAR(20) DEFAULT 'booked',
    booked_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    paid_at TIMESTAMP,
    cancelled_at TIMESTAMP,
    passenger_name VARCHAR(200) NOT NULL,
    passenger_document VARCHAR(50) NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
CREATE INDEX IF NOT EXISTS idx_users_locked_until ON users(locked_until);
CREATE INDEX IF NOT EXISTS idx_audit_user ON audit_logs(user_id);
CREATE INDEX IF NOT EXISTS idx_audit_timestamp ON audit_logs(timestamp);
CREATE INDEX IF NOT EXISTS idx_sessions_token ON sessions(session_token);
CREATE INDEX IF NOT EXISTS idx_sessions_user ON sessions(user_id);
CREATE INDEX IF NOT EXISTS idx_sessions_expires ON sessions(expires_at);
CREATE INDEX IF NOT EXISTS idx_stations_code ON stations(code);
CREATE INDEX IF NOT EXISTS idx_trains_number ON trains(train_number);
CREATE INDEX IF NOT EXISTS idx_routes_train ON routes(train_id);
CREATE INDEX IF NOT EXISTS idx_route_stops_route ON route_stops(route_id);
CREATE INDEX IF NOT EXISTS idx_schedules_route_date ON schedules(route_id, departure_date);
CREATE INDEX IF NOT EXISTS idx_tickets_user ON tickets(user_id);
CREATE INDEX IF NOT EXISTS idx_tickets_schedule ON tickets(schedule_id);
CREATE INDEX IF NOT EXISTS idx_tickets_number ON tickets(ticket_number);
CREATE INDEX IF NOT EXISTS idx_tickets_status ON tickets(status);
CREATE INDEX IF NOT EXISTS idx_verification_codes_user ON verification_codes(user_id);
CREATE INDEX IF NOT EXISTS idx_verification_codes_code ON verification_codes(code);
CREATE INDEX IF NOT EXISTS idx_verification_codes_expires ON verification_codes(expires_at);

INSERT INTO stations (name, city, code, latitude, longitude) VALUES
('Ленинградский вокзал', 'Москва', 'MOS', 55.7761, 37.6553),
('Новосибирск-Главный', 'Новосибирск', 'NSK', 55.0345, 82.8988),
('Екатеринбург-Пасс.', 'Екатеринбург', 'EKB', 56.8584, 60.6057),
('Черемхово', 'Черемхово', 'CHM', 53.1537, 103.0722),
('Казань-Пасс.', 'Казань', 'KZN', 55.7894, 49.1032),
('Кемерово', 'Кемерово', 'KEM', 55.3370, 86.0691),
('Прокопьевск', 'Прокопьевск', 'PRK', 53.8833, 86.7333),
('Камень-на-Оби', 'Камень-на-Оби', 'KNO', 53.7833, 81.3500),
('Анапа', 'Анапа', 'ANA', 44.9298, 37.3486),
('Владивосток', 'Владивосток', 'VVO', 43.1116, 131.8755),
('Московский вокзал', 'Санкт-Петербург', 'SPB', 59.9294, 30.3619),
('Мурманск', 'Мурманск', 'MMK', 68.9707, 33.0749),
('Сочи', 'Сочи', 'AER', 43.5855, 39.7231),
('Гар-де-Л’Эст', 'Париж', 'PAR', 48.8768, 2.3591),
('Нижний Новгород', 'Нижний Новгород', 'NNO', 56.3287, 44.0020),
('Самара', 'Самара', 'SAM', 53.2001, 50.1500),
('Омск', 'Омск', 'OMS', 54.9885, 73.3242),
('Челябинск', 'Челябинск', 'CHE', 55.1644, 61.4368),
('Красноярск', 'Красноярск', 'KRA', 56.0153, 92.8932),
('Иркутск', 'Иркутск', 'IRK', 52.2978, 104.2964),
('Хабаровск', 'Хабаровск', 'KHV', 48.4827, 135.0838),
('Пермь', 'Пермь', 'PRM', 58.0105, 56.2502),
('Уфа', 'Уфа', 'UFA', 54.7388, 55.9721),
('Волгоград', 'Волгоград', 'VOG', 48.7194, 44.5018),
('Ростов-на-Дону', 'Ростов-на-Дону', 'ROV', 47.2357, 39.7015),
('Краснодар', 'Краснодар', 'KRR', 45.0355, 38.9753),
('Воронеж', 'Воронеж', 'VOR', 51.6605, 39.2005),
('Саратов', 'Саратов', 'RTW', 51.5406, 46.0086),
('Тюмень', 'Тюмень', 'TYU', 57.1522, 65.5272),
('Томск', 'Томск', 'TOM', 56.4977, 84.9744),
('Барнаул', 'Барнаул', 'BAX', 53.3606, 83.7636),
('Новокузнецк', 'Новокузнецк', 'NOK', 53.7557, 87.1099),
('Пенза', 'Пенза', 'PNZ', 53.1959, 45.0185),
('Ярославль', 'Ярославль', 'YAR', 57.6261, 39.8845),
('Владимир', 'Владимир', 'VLD', 56.1366, 40.3966),
('Тверь', 'Тверь', 'TVE', 56.8587, 35.9176),
('Вологда', 'Вологда', 'VLG', 59.2239, 39.8843),
('Архангельск', 'Архангельск', 'ARH', 64.5401, 40.5433),
('Киров', 'Киров', 'KIR', 58.6035, 49.6680),
('Ульяновск', 'Ульяновск', 'ULV', 54.3142, 48.4031);

INSERT INTO trains (train_number, train_type, total_seats, is_active) VALUES
('001M', 'Скоростной', 450, TRUE),
('054K', 'Фирменный', 420, TRUE),
('102A', 'Пассажирский', 540, TRUE),
('023Y', 'Международный', 360, TRUE),
('999S', 'Электричка', 300, TRUE),
('002А', 'Скоростной Сапсан', 600, TRUE),
('004А', 'Скоростной Сапсан', 600, TRUE),
('020А', 'Фирменный', 450, TRUE),
('030А', 'Пассажирский', 540, TRUE),
('016А', 'Скоростной', 480, TRUE),
('034К', 'Фирменный', 450, TRUE),
('048П', 'Пассажирский', 540, TRUE),
('092П', 'Пассажирский', 540, TRUE),
('003А', 'Скоростной Сапсан', 600, TRUE),
('005А', 'Скоростной Сапсан', 600, TRUE),
('021А', 'Фирменный', 450, TRUE),
('072У', 'Фирменный', 480, TRUE),
('084У', 'Пассажирский', 540, TRUE),
('096У', 'Пассажирский', 540, TRUE),
('110У', 'Пассажирский', 540, TRUE),
('056Н', 'Фирменный', 480, TRUE),
('070Н', 'Пассажирский', 540, TRUE),
('082Н', 'Пассажирский', 540, TRUE),
('094Н', 'Пассажирский', 540, TRUE),
('010В', 'Фирменный Россия', 540, TRUE),
('100В', 'Пассажирский', 600, TRUE),
('200В', 'Пассажирский', 600, TRUE),
('120Р', 'Пассажирский', 540, TRUE),
('130К', 'Пассажирский', 540, TRUE),
('140С', 'Пассажирский', 540, TRUE),
('150У', 'Пассажирский', 540, TRUE);

DO $$
DECLARE
    t_rec RECORD;
    c_num INT;
    s_num INT;
    curr_carriage_id INT;
BEGIN
    FOR t_rec IN SELECT id FROM trains LOOP
        FOR c_num IN 1..2 LOOP
            INSERT INTO carriages (train_id, carriage_number, carriage_type, total_seats, price_multiplier)
            VALUES (t_rec.id, c_num, 'Сидячий', 18, 1)
            RETURNING id INTO curr_carriage_id;

            FOR s_num IN 1..18 LOOP
                INSERT INTO seats (carriage_id, seat_number, seat_type, is_available)
                VALUES (curr_carriage_id, s_num, 
                        CASE WHEN s_num % 2 = 0 THEN 'Нижнее' ELSE 'Нижнее' END, 
                        TRUE);
            END LOOP;
        END LOOP;

        FOR c_num IN 3..7 LOOP
            INSERT INTO carriages (train_id, carriage_number, carriage_type, total_seats, price_multiplier)
            VALUES (t_rec.id, c_num, 'Купе', 36, 2.5)
            RETURNING id INTO curr_carriage_id;

            FOR s_num IN 1..36 LOOP
                INSERT INTO seats (carriage_id, seat_number, seat_type, is_available)
                VALUES (curr_carriage_id, s_num, 
                        CASE WHEN s_num % 2 = 0 THEN 'Верхнее' ELSE 'Нижнее' END, 
                        TRUE);
            END LOOP;
        END LOOP;

        FOR c_num IN 8..17 LOOP
            INSERT INTO carriages (train_id, carriage_number, carriage_type, total_seats, price_multiplier)
            VALUES (t_rec.id, c_num, 'Плацкарт', 54, 1.5)
            RETURNING id INTO curr_carriage_id;

            FOR s_num IN 1..54 LOOP
                INSERT INTO seats (carriage_id, seat_number, seat_type, is_available)
                VALUES (curr_carriage_id, s_num, 
                        CASE 
                            WHEN s_num > 36 THEN 
                        CASE 
                            WHEN s_num % 2 = 1 THEN 'Нижнее'
                            ELSE 'Верхнее'
                        END
                    WHEN s_num % 2 = 1 THEN 'Нижнее'
                    ELSE 'Верхнее'
                END, 
                TRUE);
            END LOOP;
        END LOOP;
    END LOOP;
END $$;

-- Трансибирский Экспресс: Москва -> Казань -> Екатеринбург -> Новосибирск -> Черемхово -> Владивосток
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='001M'), 'Трансибирский Экспресс', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE route_name='Трансибирский Экспресс')
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 1, NULL, '08:00:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Казань'), 2, '18:00:00', '18:30:00', 30, 2500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Екатеринбург'), 3, '06:00:00', '06:45:00', 45, 4500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Новосибирск'), 4, '16:00:00', '16:40:00', 40, 6500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Черемхово'), 5, '04:00:00', '04:15:00', 15, 8000),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Владивосток'), 6, '10:00:00', '10:15:00', 0, 12000);

-- Москва - Санкт-Петербург (поезд 002А)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='002А'), 'Москва - Санкт-Петербург', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='002А'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 1, NULL, '07:00:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Санкт-Петербург'), 2, '11:00:00', '11:15:00', 0, 3500);

-- Москва - Санкт-Петербург (поезд 004А)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='004А'), 'Москва - Санкт-Петербург', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='004А'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 1, NULL, '13:30:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Санкт-Петербург'), 2, '17:30:00', '17:45:00', 0, 3500);

-- Москва - Санкт-Петербург (поезд 020А)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='020А'), 'Москва - Санкт-Петербург', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='020А'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 1, NULL, '22:00:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Санкт-Петербург'), 2, '06:30:00', '06:45:00', 0, 2800);

-- Москва - Санкт-Петербург (поезд 030А)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='030А'), 'Москва - Санкт-Петербург', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='030А'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 1, NULL, '23:55:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Санкт-Петербург'), 2, '08:00:00', '08:15:00', 0, 2500);

-- Москва - Казань (поезд 016А)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='016А'), 'Москва - Казань', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='016А'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 1, NULL, '08:30:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Казань'), 2, '19:45:00', '20:00:00', 0, 2800);

-- Москва - Казань (поезд 034К)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='034К'), 'Москва - Казань', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='034К'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 1, NULL, '18:20:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Нижний Новгород'), 2, '22:30:00', '22:50:00', 20, 1200),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Казань'), 3, '05:30:00', '05:45:00', 0, 2500);

-- Москва - Казань (поезд 048П)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='048П'), 'Москва - Казань', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='048П'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 1, NULL, '21:40:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Казань'), 2, '09:15:00', '09:30:00', 0, 2200);

-- Москва - Казань (поезд 092П)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='092П'), 'Москва - Казань', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='092П'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 1, NULL, '23:30:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Казань'), 2, '11:00:00', '11:15:00', 0, 2100);

-- Санкт-Петербург - Москва (поезд 003А)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='003А'), 'Санкт-Петербург - Москва', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='003А'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Санкт-Петербург'), 1, NULL, '06:45:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 2, '10:45:00', '11:00:00', 0, 3500);

-- Санкт-Петербург - Москва (поезд 005А)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='005А'), 'Санкт-Петербург - Москва', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='005А'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Санкт-Петербург'), 1, NULL, '19:30:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 2, '23:30:00', '23:45:00', 0, 3500);

-- Санкт-Петербург - Москва (поезд 021А)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='021А'), 'Санкт-Петербург - Москва', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='021А'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Санкт-Петербург'), 1, NULL, '23:00:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 2, '07:00:00', '07:15:00', 0, 2800);

-- Екатеринбург - Москва (поезд 072У)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='072У'), 'Екатеринбург - Москва', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='072У'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Екатеринбург'), 1, NULL, '14:20:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Пермь'), 2, '18:45:00', '19:05:00', 20, 1500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Киров'), 3, '01:30:00', '01:50:00', 20, 2800),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 4, '14:00:00', '14:15:00', 0, 4500);

-- Екатеринбург - Москва (поезд 084У)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='084У'), 'Екатеринбург - Москва', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='084У'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Екатеринбург'), 1, NULL, '16:50:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Пермь'), 2, '21:15:00', '21:35:00', 20, 1500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 3, '16:30:00', '16:45:00', 0, 4200);

-- Екатеринбург - Москва (поезд 096У)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='096У'), 'Екатеринбург - Москва', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='096У'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Екатеринбург'), 1, NULL, '20:10:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 2, '19:40:00', '19:55:00', 0, 4000);

-- Екатеринбург - Москва (поезд 110У)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='110У'), 'Екатеринбург - Москва', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='110У'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Екатеринбург'), 1, NULL, '22:35:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Пермь'), 2, '03:00:00', '03:20:00', 20, 1500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Нижний Новгород'), 3, '15:10:00', '15:30:00', 20, 3200),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 4, '21:20:00', '21:35:00', 0, 4100);

-- Новосибирск - Москва (поезд 056Н)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='056Н'), 'Новосибирск - Москва', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='056Н'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Новосибирск'), 1, NULL, '10:15:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Омск'), 2, '18:30:00', '18:50:00', 20, 1800),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Тюмень'), 3, '05:20:00', '05:40:00', 20, 3200),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Екатеринбург'), 4, '12:30:00', '12:50:00', 20, 4500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Пермь'), 5, '17:15:00', '17:35:00', 20, 5500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 6, '13:45:00', '14:00:00', 0, 6500);

-- Новосибирск - Москва (поезд 070Н)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='070Н'), 'Новосибирск - Москва', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='070Н'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Новосибирск'), 1, NULL, '13:40:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Омск'), 2, '21:55:00', '22:15:00', 20, 1800),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Тюмень'), 3, '08:45:00', '09:05:00', 20, 3200),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 4, '16:20:00', '16:35:00', 0, 6200);

-- Новосибирск - Москва (поезд 082Н)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='082Н'), 'Новосибирск - Москва', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='082Н'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Новосибирск'), 1, NULL, '17:25:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Омск'), 2, '01:40:00', '02:00:00', 20, 1800),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Екатеринбург'), 3, '16:20:00', '16:40:00', 20, 4500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 4, '19:50:00', '20:05:00', 0, 6000);

-- Новосибирск - Москва (поезд 094Н)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='094Н'), 'Новосибирск - Москва', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='094Н'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Новосибирск'), 1, NULL, '20:50:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Барнаул'), 2, '00:30:00', '00:50:00', 20, 900),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Новокузнецк'), 3, '05:15:00', '05:35:00', 20, 1400),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Кемерово'), 4, '08:20:00', '08:40:00', 20, 1600),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Омск'), 5, '14:10:00', '14:30:00', 20, 2400),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 6, '23:15:00', '23:30:00', 0, 5900);

-- Москва - Владивосток (поезд 010В - Россия)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='010В'), 'Москва - Владивосток (Россия)', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='010В'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 1, NULL, '13:05:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Пермь'), 2, '10:25:00', '10:45:00', 20, 3000),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Екатеринбург'), 3, '15:30:00', '15:50:00', 20, 4500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Тюмень'), 4, '22:40:00', '23:00:00', 20, 5500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Омск'), 5, '09:15:00', '09:35:00', 20, 6500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Новосибирск'), 6, '17:30:00', '17:50:00', 20, 7500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Красноярск'), 7, '08:45:00', '09:15:00', 30, 9000),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Иркутск'), 8, '04:20:00', '04:50:00', 30, 10500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Хабаровск'), 9, '07:35:00', '08:05:00', 30, 13000),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Владивосток'), 10, '19:30:00', '19:45:00', 0, 14000);

-- Москва - Владивосток (поезд 100В)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='100В'), 'Москва - Владивосток', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='100В'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 1, NULL, '15:40:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Екатеринбург'), 2, '18:10:00', '18:30:00', 20, 4200),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Омск'), 3, '11:50:00', '12:10:00', 20, 6200),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Новосибирск'), 4, '20:05:00', '20:25:00', 20, 7200),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Красноярск'), 5, '11:20:00', '11:50:00', 30, 8800),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Иркутск'), 6, '06:55:00', '07:25:00', 30, 10200),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Хабаровск'), 7, '10:10:00', '10:40:00', 30, 12800),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Владивосток'), 8, '22:05:00', '22:20:00', 0, 13500);

-- Москва - Владивосток (поезд 200В)
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='200В'), 'Москва - Владивосток', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='200В'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 1, NULL, '22:20:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Казань'), 2, '08:30:00', '08:50:00', 20, 2500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Екатеринбург'), 3, '00:40:00', '01:00:00', 20, 4000),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Тюмень'), 4, '08:10:00', '08:30:00', 20, 5200),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Омск'), 5, '18:25:00', '18:45:00', 20, 6000),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Новосибирск'), 6, '02:40:00', '03:00:00', 20, 7000),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Красноярск'), 7, '18:15:00', '18:45:00', 30, 8600),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Иркутск'), 8, '13:50:00', '14:20:00', 30, 10000),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Хабаровск'), 9, '17:05:00', '17:35:00', 30, 12600),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Владивосток'), 10, '05:00:00', '05:15:00', 0, 13300);

-- Мурманск -> Санкт-Петербург -> Москва -> Сочи -> Анапа
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='054K'), 'Две Столицы -> Юг', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE route_name='Две Столицы -> Юг')
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Мурманск'), 1, NULL, '12:00:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Санкт-Петербург'), 2, '22:00:00', '23:00:00', 60, 3000),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 3, '06:00:00', '06:30:00', 30, 5000),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Сочи'), 4, '14:00:00', '14:20:00', 20, 8000),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Анапа'), 5, '18:00:00', '18:30:00', 0, 9000);

-- Новосибирск -> Камень-на-Оби -> Кемерово -> Прокопьевск
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='102A'), 'Сибирский Круиз', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE route_name='Сибирский Круиз')
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Новосибирск'), 1, NULL, '09:00:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Камень-на-Оби'), 2, '11:00:00', '11:10:00', 10, 800),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Кемерово'), 3, '15:00:00', '15:30:00', 30, 1500),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Прокопьевск'), 4, '18:00:00', '18:30:00', 0, 1900);

-- Казань -> Москва -> Париж
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='023Y'), 'Евро-Тур', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE route_name='Евро-Тур')
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Казань'), 1, NULL, '10:00:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 2, '19:00:00', '21:00:00', 120, 3000),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Париж'), 3, '08:00:00', '08:30:00', 0, 25000);

-- Москва - Ростов-на-Дону
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='120Р'), 'Москва - Ростов-на-Дону', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='120Р'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 1, NULL, '16:30:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Воронеж'), 2, '23:45:00', '00:05:00', 20, 1800),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Ростов-на-Дону'), 3, '10:20:00', '10:35:00', 0, 3200);

-- Москва - Краснодар
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='130К'), 'Москва - Краснодар', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='130К'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Москва'), 1, NULL, '19:15:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Воронеж'), 2, '02:30:00', '02:50:00', 20, 1800),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Ростов-на-Дону'), 3, '13:10:00', '13:30:00', 20, 3200),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Краснодар'), 4, '18:45:00', '19:00:00', 0, 4000);

-- Самара - Уфа
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='140С'), 'Самара - Уфа', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='140С'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Самара'), 1, NULL, '08:20:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Уфа'), 2, '16:40:00', '16:55:00', 0, 1500);

-- Ульяновск - Казань
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='150У'), 'Ульяновск - Казань', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE train_id = (SELECT id FROM trains WHERE train_number='150У'))
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Ульяновск'), 1, NULL, '07:00:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Казань'), 2, '11:30:00', '11:45:00', 0, 800);

-- Кузбасс-Пригород
INSERT INTO routes (train_id, route_name, valid_from, valid_to) VALUES
((SELECT id FROM trains WHERE train_number='999S'), 'Кузбасс-Пригород', CURRENT_DATE, CURRENT_DATE + INTERVAL '1 year');

WITH r AS (SELECT id FROM routes WHERE route_name='Кузбасс-Пригород')
INSERT INTO route_stops (route_id, station_id, stop_order, arrival_time, departure_time, stop_duration_minutes, price_from_start) VALUES
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Новокузнецк'), 1, NULL, '06:00:00', 0, 0),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Прокопьевск'), 2, '07:30:00', '07:45:00', 15, 200),
((SELECT id FROM r), (SELECT id FROM stations WHERE city='Кемерово'), 3, '09:00:00', '09:15:00', 0, 400);

DO $$
DECLARE
    route_rec RECORD;
    day_offset INT;
BEGIN
    FOR route_rec IN SELECT id FROM routes LOOP
        FOR day_offset IN 1..30 LOOP
            INSERT INTO schedules (route_id, departure_date, status)
            VALUES (route_rec.id, CURRENT_DATE + day_offset, 'active');
        END LOOP;
    END LOOP;
END $$;
