CREATE TABLE IF NOT EXISTS anime_list(
  id INTEGER PRIMARY KEY,
  media_id INTEGER NOT NULL,
  progress INTEGER,
  date_start TEXT,
  date_end TEXT,
  score INTEGER,
  status INTEGER,
  private INTEGER,
  rewatched_times INTEGER,
  rewatching INTEGER,
  rewatching_ep INTEGER,
  notes TEXT,
  last_updated TEXT,
  pending_delete INTEGER NOT NULL DEFAULT 0,
  FOREIGN KEY (media_id) REFERENCES media (id)
);
