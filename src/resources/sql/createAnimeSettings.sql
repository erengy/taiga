CREATE TABLE IF NOT EXISTS anime_settings(
  id INTEGER PRIMARY KEY,
  display_title TEXT NOT NULL DEFAULT '',
  synonyms TEXT NOT NULL DEFAULT '',
  FOREIGN KEY (id) REFERENCES anime (id)
);
