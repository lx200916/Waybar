#include <fmt/core.h>

#include <optional>
#include <sstream>
#include <string>

#include "modules/mpris/mpris.hpp"

extern "C" {
#include <playerctl/playerctl.h>
}

#include <spdlog/spdlog.h>

namespace waybar::modules::mpris {

const std::string DEFAULT_FORMAT = "{player} ({status}): {dynamic}";

Mpris::Mpris(const std::string& id, const Json::Value& config)
    : AModule(config, "mpris", id),
      box_(Gtk::ORIENTATION_HORIZONTAL, 0),
      label_(),
      format_(DEFAULT_FORMAT),
      tooltip_(DEFAULT_FORMAT),
      artist_len_(-1),
      album_len_(-1),
      title_len_(-1),
      dynamic_len_(-1),
      dynamic_prio_({"title", "length", "position", "artist", "album"}),
      truncate_hours_(true),
      tooltip_len_limits_(false),
      // this character is used in Gnome so it's fine to use it here
      ellipsis_(u8"\u2026"),
      interval_(0),
      player_("playerctld"),
      manager(),
      player() {
  box_.pack_start(label_);
  box_.set_name(name_);
  event_box_.add(box_);
  event_box_.signal_button_press_event().connect(sigc::mem_fun(*this, &Mpris::handleToggle));

  if (config_["format"].isString()) {
    format_ = config_["format"].asString();
  }
  if (config_["format-playing"].isString()) {
    format_playing_ = config_["format-playing"].asString();
  }
  if (config_["format-paused"].isString()) {
    format_paused_ = config_["format-paused"].asString();
  }
  if (config_["format-stopped"].isString()) {
    format_stopped_ = config_["format-stopped"].asString();
  }
  if (config_["ellipsis"].isString()) {
    ellipsis_ = config_["ellipsis"].asString();
  }
  if (tooltipEnabled()) {
    if (config_["tooltip-format"].isString()) {
      tooltip_ = config_["tooltip-format"].asString();
    }
    if (config_["tooltip-format-playing"].isString()) {
      tooltip_playing_ = config_["tooltip-format-playing"].asString();
    }
    if (config_["tooltip-format-paused"].isString()) {
      tooltip_paused_ = config_["tooltip-format-paused"].asString();
    }
    if (config_["tooltip-format-stopped"].isString()) {
      tooltip_stopped_ = config_["tooltip-format-stopped"].asString();
    }
    if (config_["enable-tooltip-len-limits"].isBool()) {
      tooltip_len_limits_ = config["enable-tooltip-len-limits"].asBool();
    }
  }

  if (config["artist-len"].isUInt()) {
    artist_len_ = config["artist-len"].asUInt();
  }
  if (config["album-len"].isUInt()) {
    album_len_ = config["album-len"].asUInt();
  }
  if (config["title-len"].isUInt()) {
    title_len_ = config["title-len"].asUInt();
  }
  if (config["dynamic-len"].isUInt()) {
    dynamic_len_ = config["dynamic-len"].asUInt();
  }
  if (config_["dynamic-priority"].isArray()) {
    dynamic_prio_.clear();
    for (auto it = config_["dynamic-priority"].begin(); it != config_["dynamic-priority"].end();
         ++it) {
      if (it->isString()) {
        dynamic_prio_.push_back(it->asString());
      }
    }
  }

  if (config_["truncate-hours"].isBool()) {
    truncate_hours_ = config["truncate-hours"].asBool();
  }
  if (config_["interval"].isUInt()) {
    interval_ = std::chrono::seconds(config_["interval"].asUInt());
  }
  if (config_["player"].isString()) {
    player_ = config_["player"].asString();
  }
  if (config_["ignored-players"].isArray()) {
    for (auto it = config_["ignored-players"].begin(); it != config_["ignored-players"].end();
         ++it) {
      if (it->isString()) {
        ignored_players_.push_back(it->asString());
      }
    }
  }

  GError* error = nullptr;
  manager = playerctl_player_manager_new(&error);
  if (error) {
    throw std::runtime_error(fmt::format("unable to create MPRIS client: {}", error->message));
  }

  g_object_connect(manager, "signal::name-appeared", G_CALLBACK(onPlayerNameAppeared), this, NULL);
  g_object_connect(manager, "signal::name-vanished", G_CALLBACK(onPlayerNameVanished), this, NULL);

  if (player_ == "playerctld") {
    // use playerctld proxy
    PlayerctlPlayerName name = {
        .instance = (gchar*)player_.c_str(),
        .source = PLAYERCTL_SOURCE_DBUS_SESSION,
    };
    player = playerctl_player_new_from_name(&name, &error);

  } else {
    GList* players = playerctl_list_players(&error);
    if (error) {
      auto e = fmt::format("unable to list players: {}", error->message);
      g_error_free(error);
      throw std::runtime_error(e);
    }

    for (auto p = players; p != NULL; p = p->next) {
      auto pn = static_cast<PlayerctlPlayerName*>(p->data);
      if (strcmp(pn->name, player_.c_str()) == 0) {
        player = playerctl_player_new_from_name(pn, &error);
        break;
      }
    }
  }

  if (error) {
    throw std::runtime_error(
        fmt::format("unable to connect to player {}: {}", player_, error->message));
  }

  if (player) {
    g_object_connect(player, "signal::play", G_CALLBACK(onPlayerPlay), this, "signal::pause",
                     G_CALLBACK(onPlayerPause), this, "signal::stop", G_CALLBACK(onPlayerStop),
                     this, "signal::stop", G_CALLBACK(onPlayerStop), this, "signal::metadata",
                     G_CALLBACK(onPlayerMetadata), this, NULL);
  }

  // allow setting an interval count that triggers periodic refreshes
  if (interval_.count() > 0) {
    thread_ = [this] {
      dp.emit();
      thread_.sleep_for(interval_);
    };
  }

  // trigger initial update
  dp.emit();
}

Mpris::~Mpris() {
  if (manager != NULL) g_object_unref(manager);
  if (player != NULL) g_object_unref(player);
}

auto Mpris::getIcon(const Json::Value& icons, const std::string& key) -> std::string {
  if (icons.isObject()) {
    if (icons[key].isString()) {
      return icons[key].asString();
    } else if (icons["default"].isString()) {
      return icons["default"].asString();
    }
  }
  return "";
}

#include "wide_check.h"

// Japanese characters, emoji, etc are twice as long in mono fonts
auto utf8_char_wide(const std::string& str, size_t pos) -> bool {
  uint32_t c = (unsigned char)str[pos];
  if (c <= 127) return false;
  else if ((c & 0xE0) == 0xC0) {
    if (pos + 1 >= str.length()) return false;
    c = (str[pos + 1] & 0x3F) | ((c & 0x1F) << 6);
  } else if ((c & 0xF0) == 0xE0) {
    if (pos + 2 >= str.length()) return false;
    c = (str[pos + 2] & 0x3F) | ((str[pos + 1] & 0x3F) << 6) | ((c & 0x1F) << 12);
  } else if ((c & 0xF8) == 0xF0) {
    if (pos + 3 >= str.length()) return false;
    c = (str[pos + 3] & 0x3F) | ((str[pos + 2] & 0x3F) << 6) | ((str[pos + 1] & 0x3F) << 12) | ((c & 0x1F) << 18);
  }
  return char_is_wide(c);
}

auto utf8_length(const std::string& str) -> size_t {
  size_t i = 0, len = 0;
  for (i = len = 0; i < str.length(); i++) {
    unsigned char c = (unsigned char)str[i];
    len += utf8_char_wide(str, i) ? 2 : 1;
    if (c <= 127)
      continue;
    else if ((c & 0xE0) == 0xC0)
      i += 1;
    else if ((c & 0xF0) == 0xE0)
      i += 2;
    else if ((c & 0xF8) == 0xF0)
      i += 3;
    else
      return str.length();  // invalid utf8
  }
  return len;
}

std::string utf8_truncate(const std::string& str, size_t len) {
  if (len == 0) {
    return std::string();
  }
  if (len == std::string::npos) {
    return str;
  }
  size_t pos = std::string::npos;
  size_t i = 0, u = 0;
  for (i = u = 0; i < str.length(); i++) {
    if (u <= len) {
      pos = i;
    } else {
      break;
    }
    u += utf8_char_wide(str, i) ? 2 : 1;
    unsigned char c = (unsigned char)str[i];
    if (c <= 127)
      continue;
    else if ((c & 0xE0) == 0xC0)
      i += 1;
    else if ((c & 0xF0) == 0xE0)
      i += 2;
    else if ((c & 0xF8) == 0xF0)
      i += 3;
    else
      return str.substr(0, len);  // invalid utf8
  }
  if (u <= len) {
    pos = i;
  }
  return str.substr(0, pos);
}

auto truncate(const std::string& s, const std::string& ellipsis, size_t max_len) -> std::string {
  if (max_len == 0) return std::string();
  size_t len = utf8_length(s);
  if (len > max_len) {
    size_t ellipsis_len = utf8_length(ellipsis);
    if (max_len >= ellipsis_len) {
      return utf8_truncate(s, max_len - ellipsis_len) + ellipsis;
    } else {
      return std::string();
    }
  }
  return s;
}

auto Mpris::getArtistStr(const PlayerInfo& info, bool truncated) -> std::string {
  std::string artist = info.artist.value_or(std::string());
  return (truncated && artist_len_ >= 0) ? truncate(artist, ellipsis_, artist_len_) : artist;
}

auto Mpris::getAlbumStr(const PlayerInfo& info, bool truncated) -> std::string {
  std::string album = info.album.value_or(std::string());
  return (truncated && album_len_ >= 0) ? truncate(album, ellipsis_, album_len_) : album;
}

auto Mpris::getTitleStr(const PlayerInfo& info, bool truncated) -> std::string {
  std::string title = info.title.value_or(std::string());
  return (truncated && title_len_ >= 0) ? truncate(title, ellipsis_, title_len_) : title;
}

auto Mpris::getLengthStr(const PlayerInfo& info, bool truncated) -> std::string {
  if (info.length.has_value()) {
    std::string length = info.length.value();
    if (truncated && length.substr(0, 3) == "00:") {
      return length.substr(3);
    }
    return length;
  }
  return std::string();
}

auto Mpris::getPositionStr(const PlayerInfo& info, bool truncated) -> std::string {
  if (info.position.has_value()) {
    std::string position = info.position.value();
    if (truncated && position.substr(0, 3) == "00:") {
      return position.substr(3);
    }
    return position;
  }
  return std::string();
}

auto Mpris::getDynamicStr(const PlayerInfo& info, bool truncated, bool html) -> std::string {
  std::string artist = getArtistStr(info, truncated);
  std::string album = getAlbumStr(info, truncated);
  std::string title = getTitleStr(info, truncated);
  std::string length = getLengthStr(info, truncated && truncate_hours_);
  // keep position format same as length format
  std::string position = getPositionStr(info, truncated && truncate_hours_ && length.length() < 6);

  size_t artistLen = utf8_length(artist);
  size_t albumLen = utf8_length(album);
  size_t titleLen = utf8_length(title);
  size_t lengthLen = length.length();
  size_t posLen = position.length();

  bool showArtist = artistLen != 0;
  bool showAlbum = albumLen != 0;
  bool showTitle = titleLen != 0;
  bool showLength = lengthLen != 0;
  bool showPos = posLen != 0;

  if (truncated && dynamic_len_ >= 0) {
    size_t dynamicLen = dynamic_len_;
    if (artistLen != 0) {
      artistLen += 3;
    }
    if (albumLen != 0) {
      albumLen += 3;
    }
    if (lengthLen != 0) {
      lengthLen += 3;
    }
    if (posLen != 0) {
      posLen += 3;
    }

    size_t totalLen = 0;

    for (auto it = dynamic_prio_.begin(); it != dynamic_prio_.end(); ++it) {
      if (*it == "artist") {
        if (totalLen + artistLen > dynamicLen) {
          showArtist = false;
        } else {
          totalLen += artistLen;
        }
      } else if (*it == "album") {
        if (totalLen + albumLen > dynamicLen) {
          showAlbum = false;
        } else {
          totalLen += albumLen;
        }
      } else if (*it == "title") {
        if (totalLen + titleLen > dynamicLen) {
          showTitle = false;
        } else {
          totalLen += titleLen;
        }
      } else if (*it == "length") {
        if (totalLen + lengthLen > dynamicLen) {
          showLength = false;
        } else {
          totalLen += lengthLen;
          posLen = std::max((size_t)2, posLen) - 2;
        }
      } else if (*it == "position") {
        if (totalLen + posLen > dynamicLen) {
          showPos = false;
        } else {
          totalLen += posLen;
          lengthLen = std::max((size_t)2, lengthLen) - 2;
        }
      }
    }
  }

  std::stringstream dynamic;
  if (showArtist) dynamic << artist << " - ";
  if (showAlbum) dynamic << album << " - ";
  if (showTitle) dynamic << title;
  if (showLength || showPos) {
    dynamic << " ";
    if (html) {
      dynamic << "<small>";
    }
    dynamic << '[';
    if (showPos) {
      dynamic << position;
      if (showLength) {
        dynamic << '/';
      }
    }
    if (showLength) {
      dynamic << length;
    }
    dynamic << ']';
    if (html) {
      dynamic << "</small>";
    }
  }
  return dynamic.str();
}

auto Mpris::onPlayerNameAppeared(PlayerctlPlayerManager* manager, PlayerctlPlayerName* player_name,
                                 gpointer data) -> void {
  Mpris* mpris = static_cast<Mpris*>(data);
  if (!mpris) return;

  spdlog::debug("mpris: name-appeared callback: {}", player_name->name);

  if (std::string(player_name->name) != mpris->player_) {
    return;
  }

  GError* error = nullptr;
  mpris->player = playerctl_player_new_from_name(player_name, &error);
  g_object_connect(mpris->player, "signal::play", G_CALLBACK(onPlayerPlay), mpris, "signal::pause",
                   G_CALLBACK(onPlayerPause), mpris, "signal::stop", G_CALLBACK(onPlayerStop),
                   mpris, "signal::stop", G_CALLBACK(onPlayerStop), mpris, "signal::metadata",
                   G_CALLBACK(onPlayerMetadata), mpris, NULL);

  mpris->dp.emit();
}

auto Mpris::onPlayerNameVanished(PlayerctlPlayerManager* manager, PlayerctlPlayerName* player_name,
                                 gpointer data) -> void {
  Mpris* mpris = static_cast<Mpris*>(data);
  if (!mpris) return;

  spdlog::debug("mpris: player-vanished callback: {}", player_name->name);

  if (std::string(player_name->name) == mpris->player_) {
    mpris->player = nullptr;
    mpris->dp.emit();
  }
}

auto Mpris::onPlayerPlay(PlayerctlPlayer* player, gpointer data) -> void {
  Mpris* mpris = static_cast<Mpris*>(data);
  if (!mpris) return;

  spdlog::debug("mpris: player-play callback");
  // update widget
  mpris->dp.emit();
}

auto Mpris::onPlayerPause(PlayerctlPlayer* player, gpointer data) -> void {
  Mpris* mpris = static_cast<Mpris*>(data);
  if (!mpris) return;

  spdlog::debug("mpris: player-pause callback");
  // update widget
  mpris->dp.emit();
}

auto Mpris::onPlayerStop(PlayerctlPlayer* player, gpointer data) -> void {
  Mpris* mpris = static_cast<Mpris*>(data);
  if (!mpris) return;

  spdlog::debug("mpris: player-stop callback");

  // hide widget
  mpris->event_box_.set_visible(false);
  // update widget
  mpris->dp.emit();
}

auto Mpris::onPlayerMetadata(PlayerctlPlayer* player, GVariant* metadata, gpointer data) -> void {
  Mpris* mpris = static_cast<Mpris*>(data);
  if (!mpris) return;

  spdlog::debug("mpris: player-metadata callback");
  // update widget
  mpris->dp.emit();
}

auto Mpris::getPlayerInfo() -> std::optional<PlayerInfo> {
  if (!player) {
    return std::nullopt;
  }

  GError* error = nullptr;

  char* player_status = nullptr;
  auto player_playback_status = PLAYERCTL_PLAYBACK_STATUS_STOPPED;
  g_object_get(player, "status", &player_status, "playback-status", &player_playback_status, NULL);

  std::string player_name = player_;
  if (player_name == "playerctld") {
    GList* players = playerctl_list_players(&error);
    if (error) {
      auto e = fmt::format("unable to list players: {}", error->message);
      g_error_free(error);
      throw std::runtime_error(e);
    }
    // > get the list of players [..] in order of activity
    // https://github.com/altdesktop/playerctl/blob/b19a71cb9dba635df68d271bd2b3f6a99336a223/playerctl/playerctl-common.c#L248-L249
    players = g_list_first(players);
    if (players) player_name = static_cast<PlayerctlPlayerName*>(players->data)->name;
  }

  if (std::any_of(ignored_players_.begin(), ignored_players_.end(),
                  [&](const std::string& pn) { return player_name == pn; })) {
    spdlog::warn("mpris[{}]: ignoring player update", player_name);
    return std::nullopt;
  }

  // make status lowercase
  player_status[0] = std::tolower(player_status[0]);

  PlayerInfo info = {
      .name = player_name,
      .status = player_playback_status,
      .status_string = player_status,
  };

  if (auto artist_ = playerctl_player_get_artist(player, &error)) {
    spdlog::debug("mpris[{}]: artist = {}", info.name, artist_);
    info.artist = Glib::Markup::escape_text(artist_);
    g_free(artist_);
  }
  if (error) goto errorexit;

  if (auto album_ = playerctl_player_get_album(player, &error)) {
    spdlog::debug("mpris[{}]: album = {}", info.name, album_);
    info.album = Glib::Markup::escape_text(album_);
    g_free(album_);
  }
  if (error) goto errorexit;

  if (auto title_ = playerctl_player_get_title(player, &error)) {
    spdlog::debug("mpris[{}]: title = {}", info.name, title_);
    info.title = Glib::Markup::escape_text(title_);
    g_free(title_);
  }
  if (error) goto errorexit;

  if (auto length_ = playerctl_player_print_metadata_prop(player, "mpris:length", &error)) {
    spdlog::debug("mpris[{}]: mpris:length = {}", info.name, length_);
    std::chrono::microseconds len = std::chrono::microseconds(std::strtol(length_, nullptr, 10));
    auto len_h = std::chrono::duration_cast<std::chrono::hours>(len);
    auto len_m = std::chrono::duration_cast<std::chrono::minutes>(len - len_h);
    auto len_s = std::chrono::duration_cast<std::chrono::seconds>(len - len_m);
    info.length = fmt::format("{:02}:{:02}:{:02}", len_h.count(), len_m.count(), len_s.count());
    g_free(length_);
  }
  if (error) goto errorexit;

  {
    auto position_ = playerctl_player_get_position(player, &error);
    if (error) {
      // it's fine to have an error here because not all players report a position
      g_error_free(error);
      error = nullptr;
    } else {
      spdlog::debug("mpris[{}]: position = {}", info.name, position_);
      std::chrono::microseconds len = std::chrono::microseconds(position_);
      auto len_h = std::chrono::duration_cast<std::chrono::hours>(len);
      auto len_m = std::chrono::duration_cast<std::chrono::minutes>(len - len_h);
      auto len_s = std::chrono::duration_cast<std::chrono::seconds>(len - len_m);
      info.position = fmt::format("{:02}:{:02}:{:02}", len_h.count(), len_m.count(), len_s.count());
    }
  }

  return info;

errorexit:
  spdlog::error("mpris[{}]: {}", info.name, error->message);
  g_error_free(error);
  return std::nullopt;
}

bool Mpris::handleToggle(GdkEventButton* const& e) {
  GError* error = nullptr;

  auto info = getPlayerInfo();
  if (!info) return false;

  if (e->type == GdkEventType::GDK_BUTTON_PRESS) {
    switch (e->button) {
      case 1:  // left-click
        if (config_["on-click"].isString()) {
          return AModule::handleToggle(e);
        }
        playerctl_player_play_pause(player, &error);
        break;
      case 2:  // middle-click
        if (config_["on-middle-click"].isString()) {
          return AModule::handleToggle(e);
        }
        playerctl_player_previous(player, &error);
        break;
      case 3:  // right-click
        if (config_["on-right-click"].isString()) {
          return AModule::handleToggle(e);
        }
        playerctl_player_next(player, &error);
        break;
    }
  }
  if (error) {
    spdlog::error("mpris[{}]: error running builtin on-click action: {}", (*info).name,
                  error->message);
    g_error_free(error);
    return false;
  }
  return true;
}

auto Mpris::update() -> void {
  auto opt = getPlayerInfo();
  if (!opt) {
    event_box_.set_visible(false);
    AModule::update();
    return;
  }
  auto info = *opt;

  if (info.status == PLAYERCTL_PLAYBACK_STATUS_STOPPED) {
    spdlog::debug("mpris[{}]: player stopped, skipping update", info.name);
    return;
  }

  spdlog::debug("mpris[{}]: running update", info.name);

  // set css class for player status
  if (!lastStatus.empty() && box_.get_style_context()->has_class(lastStatus)) {
    box_.get_style_context()->remove_class(lastStatus);
  }
  if (!box_.get_style_context()->has_class(info.status_string)) {
    box_.get_style_context()->add_class(info.status_string);
  }
  lastStatus = info.status_string;

  // set css class for player name
  if (!lastPlayer.empty() && box_.get_style_context()->has_class(lastPlayer)) {
    box_.get_style_context()->remove_class(lastPlayer);
  }
  if (!box_.get_style_context()->has_class(info.name)) {
    box_.get_style_context()->add_class(info.name);
  }
  lastPlayer = info.name;

  auto formatstr = format_;
  auto tooltipstr = tooltip_;
  switch (info.status) {
    case PLAYERCTL_PLAYBACK_STATUS_PLAYING:
      if (!format_playing_.empty()) formatstr = format_playing_;
      if (!tooltip_playing_.empty()) tooltipstr = tooltip_playing_;
      break;
    case PLAYERCTL_PLAYBACK_STATUS_PAUSED:
      if (!format_paused_.empty()) formatstr = format_paused_;
      if (!tooltip_paused_.empty()) tooltipstr = tooltip_paused_;
      break;
    case PLAYERCTL_PLAYBACK_STATUS_STOPPED:
      if (!format_stopped_.empty()) formatstr = format_stopped_;
      if (!tooltip_stopped_.empty()) tooltipstr = tooltip_stopped_;
      break;
  }

  std::string length = getLengthStr(info, truncate_hours_);
  std::string tooltipLength =
      (tooltip_len_limits_ || length.length() > 5) ? length : getLengthStr(info, false);
  // keep position format same as length format
  std::string position = getPositionStr(info, truncate_hours_ && length.length() < 6);
  std::string tooltipPosition =
      (tooltip_len_limits_ || position.length() > 5) ? position : getPositionStr(info, false);

  try {
    auto label_format = fmt::format(
        fmt::runtime(formatstr), fmt::arg("player", info.name),
        fmt::arg("status", info.status_string), fmt::arg("artist", getArtistStr(info, true)),
        fmt::arg("title", getTitleStr(info, true)), fmt::arg("album", getAlbumStr(info, true)),
        fmt::arg("length", length), fmt::arg("position", position),
        fmt::arg("dynamic", getDynamicStr(info, true, true)),
        fmt::arg("player_icon", getIcon(config_["player-icons"], info.name)),
        fmt::arg("status_icon", getIcon(config_["status-icons"], info.status_string)));

    label_.set_markup(label_format);
  } catch (fmt::format_error const& e) {
    spdlog::warn("mpris: format error: {}", e.what());
  }

  if (tooltipEnabled()) {
    try {
      auto tooltip_text = fmt::format(
          fmt::runtime(tooltipstr), fmt::arg("player", info.name),
          fmt::arg("status", info.status_string),
          fmt::arg("artist", getArtistStr(info, tooltip_len_limits_)),
          fmt::arg("title", getTitleStr(info, tooltip_len_limits_)),
          fmt::arg("album", getAlbumStr(info, tooltip_len_limits_)),
          fmt::arg("length", tooltipLength), fmt::arg("position", tooltipPosition),
          fmt::arg("dynamic", getDynamicStr(info, tooltip_len_limits_, false)),
          fmt::arg("player_icon", getIcon(config_["player-icons"], info.name)),
          fmt::arg("status_icon", getIcon(config_["status-icons"], info.status_string)));

      label_.set_tooltip_text(tooltip_text);
    } catch (fmt::format_error const& e) {
      spdlog::warn("mpris: format error (tooltip): {}", e.what());
    }
  }

  event_box_.set_visible(true);
  // call parent update
  AModule::update();
}

}  // namespace waybar::modules::mpris
