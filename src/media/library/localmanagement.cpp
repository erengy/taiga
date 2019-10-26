
#include "media/anime.h"
#include "media/anime_item.h"
#include "media/anime_db.h"
#include "media/library/history.h"
#include "media/library/queue.h"
#include "track/scanner.h"
#include "track/media.h"
#include "track/play.h"
#include "track/episode.h"
#include "taiga/settings.h"
#include "track/recognition.h"
#include "base/log.h"
#include "ui/ui.h"

#include "localmanagement.h"
#include <deque>

namespace library {

  std::deque<std::pair<int, RemoveSettings>> purge_queue; // Episode removal processing queue
  // Lock variable for queue multi-thread handling
  win::CriticalSection removal_queue_section;

  /////////////////////////////////////////////////////////////////////
  // Methods for file purging
  /////////////////////////////////////////////////////////////////////
  
  void PurgeWatchedEpisodes(int anime_id, RemoveSettings remove_params, bool silent) {
    /*
    Removes episode files of given anime that are found in the library folders, according to
    given removal settings. Announces steps of removal if so is specified.
    */
    if (!silent)
      ui::ChangeStatusText(L"Identifying anime...");

    // Retrieve anime item
    anime::Item* item = anime::db.Find(anime_id);
    if (!item) { return; }
    else {
      if (library::queue.GetItemCount() > 0) {
        // needs to check history queue, in case database has not yet been updated
        auto episode_update_item = library::queue.FindItem(anime_id, QueueSearch::Episode);
        auto status_update_item = library::queue.FindItem(anime_id, QueueSearch::Status);
        if (episode_update_item)
          item->SetMyLastWatchedEpisode(*episode_update_item->episode);
        if (status_update_item)
          item->SetMyStatus(*status_update_item->status);
      }
    }

    PurgeWatchedEpisodes(item, remove_params, silent);
  }

  void PurgeWatchedEpisodes(anime::Item* item, RemoveSettings remove_params, bool silent) {
    /*
    Removes episode files of given anime item that are found in the library folders, according to
    given removal settings. Announces steps of removal if so is specified.
    */
    
    int eps_to_keep;
    bool after_completion;
    bool prompt;
    bool permanent_removal;
    bool series_completed = item->GetMyStatus(false) == anime::kCompleted;

    // read settings
    if (item->GetUseGlobalRemovalSetting()) { // use application-wide settings as default
      eps_to_keep = remove_params.eps_to_keep.value_or(Settings.GetInt(taiga::kLibrary_Management_KeepNum));
      after_completion = remove_params.remove_all_if_complete.value_or(Settings.GetBool(taiga::kLibrary_Management_DeleteAfterCompletion));
      prompt = remove_params.prompt_remove.value_or(Settings.GetBool(taiga::kLibrary_Management_PromptDelete));
      permanent_removal = remove_params.remove_permanent.value_or(Settings.GetBool(taiga::kLibrary_Management_DeletePermanently));
    }
    else { // use settings for specified anime as default
      eps_to_keep = remove_params.eps_to_keep.value_or(item->GetEpisodesToKeep());
      after_completion = remove_params.remove_all_if_complete.value_or(item->IsEpisodeRemovedWhenCompleted());
      prompt = remove_params.prompt_remove.value_or(item->IsPromptedAtEpisodeDelete());
      permanent_removal = remove_params.remove_permanent.value_or(item->IsEpisodesDeletedPermanently());
    }

    // calculate what episodes to remove
    std::vector<std::wstring> eps_to_remove;
    int episode_num_max = item->GetAvailableEpisodeCount();
    int last_to_remove = item->GetMyLastWatchedEpisode(true) - eps_to_keep;
    if (after_completion && series_completed) {
      last_to_remove = episode_num_max;
    }
    episode_num_max = std::min(episode_num_max, last_to_remove);

    // find episode files
    eps_to_remove = ReadEpisodePaths(*item, episode_num_max, true, silent);
    if (eps_to_remove.size() == 0)
      return;

    // prompt before deletion
    bool okay_to_delete = true;
    if (prompt && !silent)
      okay_to_delete = PromptDeletion(item, eps_to_remove, false);

    // delete files
    if (okay_to_delete)
      DeleteEpisodes(eps_to_remove, permanent_removal);
  }

  void DeleteEpisodes(std::vector<std::wstring> paths, bool permanent) {
    /*
      Deletes the given files if they exists.
      Permanently deletes files if so is specified, otherwise attempts to remove to recycle bin.
    */
    ui::ChangeStatusText(L"Purging files...");

    // Create and allocate SHFILEOPERATION object
    std::wstring ep_path; // file path buffer
    SHFILEOPSTRUCT delete_info;
    ZeroMemory(&delete_info, sizeof(delete_info));
    delete_info.wFunc = FO_DELETE;

    // Make deletion into recycle bin silent and only prompt user in case of error
    delete_info.fFlags = FOF_FILESONLY | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NORECURSION;
    if (!permanent)
      delete_info.fFlags |= FOF_ALLOWUNDO; // Try to remove to recycle bin

    int removal_num = 0;
    for (auto path : paths) {
      // Check if file exists
      if (GetFileAttributes(path.c_str()) == INVALID_FILE_ATTRIBUTES)
          continue;
      ep_path += path;
      // make path doubly nul terminated. Required by SHFILEOPERATION
      ep_path += L'\0';
      removal_num++;
    }
    if (removal_num > 0) {
      // perform deletion
      delete_info.pFrom = ep_path.c_str();
      SHFileOperation(&delete_info);
    }
  }

  bool PromptDeletion(anime::Item* item, std::vector<std::wstring> episode_paths, bool silent) {
    /*
      Prompts the user of episode file removal, detailing the anime and files to be removed.
      Also updates application status text if such is specified.
    */
    if (!silent)
      ui::ChangeStatusText(L"Asking for purge permission...");

    std::vector<std::wstring> ep_file_names(episode_paths.size());
    std::wstring path, base_name;
    for (int i = 0; i < episode_paths.size(); i++) {
      path = episode_paths[i];
      base_name = path.substr(path.find_last_of(L"/\\") + 1);
      ep_file_names[i] = base_name;
    }

    return ui::OnEpisodePurge(item, ep_file_names);
  }

  std::vector<std::wstring> ReadEpisodePaths(anime::Item item, int episode_num_max, bool scan, bool silent) {
    /*
      Help function for getting file paths. Performs library scan if specified, otherwise looks for file paths
      in metainfo already loaded.
    */
    if (!silent)
      ui::ChangeStatusText(L"Scanning local files...");

    // Identify watched episode files
    if (scan) // scan library
      ScanAvailableEpisodesQuick(item.GetId());

    if (!silent)
      ui::ChangeStatusText(L"Reading file info...");

    // Gather episode paths from loaded metainfo
    std::vector<std::wstring> episode_paths;
    for (int i = 1; i <= episode_num_max; i++) {
      if (item.IsEpisodeAvailable(i)) {
        std::wstring path_to_ep = item.GetEpisodePath(i);
        episode_paths.push_back(path_to_ep);
      }
    }
    return episode_paths;
  }

  void SchedulePurge(int anime_id, RemoveSettings settings) {
    /*
      Adds episode removal request to queue.
    */
    win::Lock lock(removal_queue_section); // allocate queue lock

    for (std::pair<int, RemoveSettings> entry : purge_queue) {
      if (entry.first == anime_id && entry.second == settings) // only one scheduled purge per anime and setting
        return;
    }
    purge_queue.push_front(std::pair<int, RemoveSettings>(anime_id, settings));
  }

  void SchedulePurge(int anime_id) {
    /*
      Schedules episode removal with default settings.
    */
    RemoveSettings nullsettings;
    SchedulePurge(anime_id, nullsettings);
  }

  void ProcessPurges() {
    /*
      Processes the episode removal queue.
    */
    win::Lock lock(removal_queue_section); // allocate lock for queue

    bool start_purge = false;
    int bound = purge_queue.size();
    for (int i = 0; i < bound; i++) {
      std::pair<int,RemoveSettings> entry_to_purge = purge_queue.back();
      purge_queue.pop_back();

      // check processing settings
      start_purge = true;
      if (entry_to_purge.second.wait_for_player) {
        // do not process entry if player is running and the anime of the entry is being played
        start_purge = !track::media_players.player_running();
        if (!start_purge) {
          start_purge = entry_to_purge.first != CurrentEpisode.anime_id;
        }
      }

      if (start_purge)
        PurgeWatchedEpisodes(entry_to_purge.first, entry_to_purge.second, false);
      else // delay entry processing
        purge_queue.push_front(entry_to_purge);
    }
  }
}