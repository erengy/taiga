
#include "media/anime.h"
#include "media/anime_item.h"
#include "media/anime_db.h"
#include "media/library/history.h"
#include "media/library/queue.h"
#include "track/scanner.h"
#include "taiga/settings.h"
#include "ui/ui.h"

#include "localmanagement.h"
#include <deque>

namespace library {

  std::deque<std::pair<int, RemoveSettings>> purge_queue;
  // Lock variable for queue multi-thread handling
  win::CriticalSection removal_queue_section;

  // Methods for file purging
  void PurgeWatchedEpisodes(int anime_id, RemoveSettings remove_params, bool silent) {
    anime::Item item;

    if (!silent)
      ui::ChangeStatusText(L"Identifying anime...");

    // Retrieve anime item
    auto item_ptr = anime::db.Find(anime_id);
    if (!item_ptr) { return; }
    else {
      item = *item_ptr;
      if (library::queue.GetItemCount() > 0) {
        auto episode_update_item = library::queue.FindItem(anime_id, QueueSearch::Episode);
        auto status_update_item = library::queue.FindItem(anime_id, QueueSearch::Status);
        if (episode_update_item)
          item.SetMyLastWatchedEpisode(*episode_update_item->episode);
        if (status_update_item)
          item.SetMyStatus(*status_update_item->status);
      }
    }

    PurgeWatchedEpisodes(item, remove_params, silent);
  }

  void PurgeWatchedEpisodes(anime::Item item, RemoveSettings remove_params, bool silent) {
    // read settings
    int eps_to_keep;
    bool after_completion;
    bool prompt;
    bool permanent_removal;

    if (item.GetUseGlobalRemovalSetting()) {
      eps_to_keep = remove_params.eps_to_keep.value_or(Settings.GetInt(taiga::kLibrary_Management_KeepNum));
      after_completion = remove_params.remove_all_if_complete.value_or(Settings.GetBool(taiga::kLibrary_Management_DeleteAfterCompletion));
      prompt = remove_params.prompt_remove.value_or(Settings.GetBool(taiga::kLibrary_Management_PromptDelete));
      permanent_removal = remove_params.remove_permanent.value_or(Settings.GetBool(taiga::kLibrary_Management_DeletePermanently));
    }
    else {
      eps_to_keep = remove_params.eps_to_keep.value_or(item.GetEpisodesToKeep());
      after_completion = remove_params.remove_all_if_complete.value_or(item.IsEpisodeRemovedWhenCompleted());
      prompt = remove_params.prompt_remove.value_or(item.IsPromptedAtEpisodeDelete());
      permanent_removal = remove_params.remove_permanent.value_or(item.IsEpisodesDeletedPermanently());
    }

    bool series_completed = item.GetMyStatus(false) == anime::kCompleted;

    std::vector<std::wstring> eps_to_remove;
    int episode_num_max = item.GetAvailableEpisodeCount();
    int last_to_remove = item.GetMyLastWatchedEpisode(true) - eps_to_keep;
    if (after_completion && series_completed) {
      last_to_remove = episode_num_max;
    }
    episode_num_max = std::min(episode_num_max, last_to_remove);

    eps_to_remove = ReadEpisodePaths(item, episode_num_max, true, silent);
    if (eps_to_remove.size() == 0)
      return;

    bool okay_to_delete = true;
    // prompt before deletion
    if (prompt && eps_to_remove.size() > 0 && !silent)
      okay_to_delete = PromptDeletion(item, eps_to_remove, false);

    // delete
    if (okay_to_delete)
      DeleteEpisodes(eps_to_remove, permanent_removal);

    ui::ChangeStatusText(L"");
  }

  void DeleteEpisodes(std::vector<std::wstring> paths, bool permanent) {
    std::wstring ep_path;
    ui::ChangeStatusText(L"Purging files...");
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
      delete_info.pFrom = ep_path.c_str();
      SHFileOperation(&delete_info);
    }
  }

  bool PromptDeletion(anime::Item item, std::vector<std::wstring> episode_paths, bool silent) {
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
    if (!silent)
      ui::ChangeStatusText(L"Scanning local files...");
    // Identify watched episode files
    if (scan)
      ScanAvailableEpisodesQuick(item.GetId());

    if (!silent)
      ui::ChangeStatusText(L"Reading file info...");

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
    win::Lock lock(removal_queue_section);

    for (std::pair<int, RemoveSettings> entry : purge_queue) {
      if (entry.first == anime_id)
        return;
    }
    purge_queue.push_front(std::pair<int, RemoveSettings>(anime_id, settings));
  }

  void SchedulePurge(int anime_id) {
    RemoveSettings nullsettings;
    SchedulePurge(anime_id, nullsettings);
  }

  void ProcessPurges() {
    win::Lock lock(removal_queue_section);

    while (purge_queue.size() > 0) {
      std::pair<int,RemoveSettings> entry_to_purge = purge_queue.back();
      purge_queue.pop_back();
      PurgeWatchedEpisodes(entry_to_purge.first, entry_to_purge.second, false);
    }

    purge_queue.clear();
  }
}