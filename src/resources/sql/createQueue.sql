CREATE TABLE IF NOT EXISTS queue(
  anime_id INTEGER PRIMARY KEY,
  dirty INTEGER NOT NULL,
  time INTEGER NOT NULL,
  retry_count INTEGER NOT NULL DEFAULT 0,
  last_error TEXT,
  FOREIGN KEY (anime_id) REFERENCES anime (id)
);
