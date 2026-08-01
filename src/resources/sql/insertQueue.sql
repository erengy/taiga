INSERT OR REPLACE INTO
  queue(
    anime_id,
    dirty,
    time,
    retry_count,
    last_error
  )
  VALUES(
    :anime_id,
    :dirty,
    :time,
    :retry_count,
    :last_error
  )
