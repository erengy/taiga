
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

  std::deque<int> purge_queue;
  // Lock variable for queue multi-thread handling
  win::CriticalSection removal_queue_section;

  // Methods for file purging
  void PurgeWatchedEpisodes(int anime_id) {
    int episode_num_max;
    int last_to_remove;
    anime::Item item;

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

    ui::ChangeStatusText(L"Updating local info...");
    // Identify watched episode files
    ScanAvailableEpisodesQuick(anime_id);

    std::vector<std::pair<int, std::wstring>> eps_to_remove;
    episode_num_max = item.GetAvailableEpisodeCount();
    last_to_remove = item.GetMyLastWatchedEpisode(true) - Settings.GetInt(taiga::kLibrary_Management_KeepNum);
    if (Settings.GetBool(taiga::kLibrary_Management_DeleteAfterCompletion)
      && item.GetMyStatus(false) == anime::kCompleted) {
      last_to_remove = episode_num_max;
    }


    ui::ChangeStatusText(L"Searching local files...");
    for (int i = 1; i <= episode_num_max && i <= last_to_remove; i++) {
      if (item.IsEpisodeAvailable(i)) {
        std::wstring path_to_ep = item.GetEpisodePath(i);
        eps_to_remove.push_back(std::pair<int, std::wstring>(i, path_to_ep));
      }
    }

    if (eps_to_remove.size() <= 0)
      return;

    bool okay_to_delete = true;
    // prompt before deletion
    if (Settings.GetBool(taiga::kLibrary_Management_PromptDelete) && eps_to_remove.size() > 0) {
      ui::ChangeStatusText(L"Asking for purge permission...");
      std::vector<std::wstring> ep_file_names(eps_to_remove.size());
      std::wstring path, base_name;
      for (int i = 0; i < eps_to_remove.size(); i++) {
        path = eps_to_remove[i].second;
        base_name = path.substr(path.find_last_of(L"/\\") + 1);
        ep_file_names[i] = base_name;
      }
      okay_to_delete = ui::OnEpisodePurge(item, ep_file_names);
    }

    // delete
    if (okay_to_delete) {
      std::wstring ep_path;
      ui::ChangeStatusText(L"Purging files...");
      SHFILEOPSTRUCT delete_info;
      ZeroMemory(&delete_info, sizeof(delete_info));
      delete_info.wFunc = FO_DELETE;
      // Make deletion into recycle bin silent and only prompt user in case of error
      delete_info.fFlags = FOF_FILESONLY | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NORECURSION;
      if (!Settings.GetBool(taiga::kLibrary_Management_DeletePermanently))
        delete_info.fFlags |= FOF_ALLOWUNDO; // Try to remove to recycle bin
      int removal_num = 0;
      for (auto ep_pair : eps_to_remove) {
        if (Settings.GetBool(taiga::kLibrary_Management_PromptDelete)) {
          // Check if file still exists after waiting for approval
          if (GetFileAttributes(ep_pair.second.c_str()) == INVALID_FILE_ATTRIBUTES)
            continue;
        }
        ep_path += ep_pair.second;
        // make path doubly nul terminated. Required by SHFILEOPERATION
        ep_path += L'\0';
        removal_num++;
      }
      if (removal_num > 0) {
        delete_info.pFrom = ep_path.c_str();
        SHFileOperation(&delete_info);
      }
    }
    ui::ChangeStatusText(L"");
  }

  void SchedulePurge(int anime_id) {
    win::Lock lock(removal_queue_section);

    for (int i : purge_queue) {
      if (i == anime_id)
        return;
    }
    purge_queue.push_front(anime_id);
  }

  void ProcessPurges() {
    win::Lock lock(removal_queue_section);

    while (purge_queue.size() > 0) {
      int anime_to_purge = purge_queue.back();
      purge_queue.pop_back();
      PurgeWatchedEpisodes(anime_to_purge);
    }

    purge_queue.clear();
  }
}