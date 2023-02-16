#pragma once

#include <iostream>
#include <optional>
#include <string>

#include "gtkmm/box.h"
#include "gtkmm/label.h"

extern "C" {
#include <playerctl/playerctl.h>
}

#include "ALabel.hpp"
#include "util/sleeper_thread.hpp"

namespace waybar::modules::mpris {

class Mpris : public AModule {
 public:
  Mpris(const std::string&, const Json::Value&);
  ~Mpris();
  auto update() -> void;
  bool handleToggle(GdkEventButton* const&);

 private:
  static auto onPlayerNameAppeared(PlayerctlPlayerManager*, PlayerctlPlayerName*, gpointer) -> void;
  static auto onPlayerNameVanished(PlayerctlPlayerManager*, PlayerctlPlayerName*, gpointer) -> void;
  static auto onPlayerPlay(PlayerctlPlayer*, gpointer) -> void;
  static auto onPlayerPause(PlayerctlPlayer*, gpointer) -> void;
  static auto onPlayerStop(PlayerctlPlayer*, gpointer) -> void;
  static auto onPlayerMetadata(PlayerctlPlayer*, GVariant*, gpointer) -> void;

  struct PlayerInfo {
    std::string name;
    PlayerctlPlaybackStatus status;
    std::string status_string;

    std::optional<std::string> artist;
    std::optional<std::string> album;
    std::optional<std::string> title;
    std::optional<std::string> length;  // as HH:MM:SS
  };

  auto getPlayerInfo() -> std::optional<PlayerInfo>;
  auto getIcon(const Json::Value&, const std::string&) -> std::string;
  auto getArtistStr(const PlayerInfo&, bool) -> std::string;
  auto getAlbumStr(const PlayerInfo&, bool) -> std::string;
  auto getTitleStr(const PlayerInfo&, bool) -> std::string;
  auto getLengthStr(const PlayerInfo&, bool) -> std::string;
  auto getDynamicStr(const PlayerInfo&, bool) -> std::string;

  Gtk::Box box_;
  Gtk::Label label_;

  // config
  std::string format_;
  std::string format_playing_;
  std::string format_paused_;
  std::string format_stopped_;

  std::string tooltip_;
  std::string tooltip_playing_;
  std::string tooltip_paused_;
  std::string tooltip_stopped_;

  int artist_len_;
  int album_len_;
  int title_len_;
  int dynamic_len_;
  std::vector<std::string> dynamic_prio_;
  bool tooltip_len_limits_;

  std::chrono::seconds interval_;
  std::string player_;
  std::vector<std::string> ignored_players_;

  PlayerctlPlayerManager* manager;
  PlayerctlPlayer* player;
  std::string lastStatus;
  std::string lastPlayer;

  util::SleeperThread thread_;
};

}  // namespace waybar::modules::mpris
