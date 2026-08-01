CREATE TABLE IF NOT EXISTS queue(
  id INTEGER PRIMARY KEY,
  enabled INTEGER NOT NULL DEFAULT 1,
  anime_id INTEGER NOT NULL,
  mode INTEGER NOT NULL,
  time INTEGER NOT NULL,
  episode INTEGER,
  score INTEGER,
  status INTEGER,
  date_started TEXT,
  date_completed TEXT,
  rewatching INTEGER,
  rewatched_times INTEGER,
  notes TEXT,
  FOREIGN KEY (anime_id) REFERENCES anime (id)
);
