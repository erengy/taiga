UPDATE queue SET
  enabled = :enabled,
  anime_id = :anime_id,
  mode = :mode,
  time = :time,
  episode = :episode,
  score = :score,
  status = :status,
  date_started = :date_started,
  date_completed = :date_completed,
  rewatching = :rewatching,
  rewatched_times = :rewatched_times,
  notes = :notes
WHERE
  id = :id
