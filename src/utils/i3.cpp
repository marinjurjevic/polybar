#include "utils/i3.hpp"

#include <xcb/xcb.h>

#include <i3ipc++/ipc.hpp>

#include "common.hpp"
#include "settings.hpp"
#include "utils/socket.hpp"
#include "utils/string.hpp"
#include "x11/connection.hpp"
#include "x11/ewmh.hpp"
#include "x11/icccm.hpp"

#include "modules/meta/base.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>  // for getenv

POLYBAR_NS

static logger m_logger(polybar::loglevel::INFO);

namespace i3_util {

  void print_workspaces(const vector<shared_ptr<i3_util::workspace_t>>& workspaces) {
    std::string ws_list;

    for (const auto& ws : workspaces) {
      if (!ws_list.empty()) {
        ws_list += ", ";
      }
      ws_list += ws->name;
    }

    m_logger.info("Workspaces: %s", ws_list);
  }

  int get_current_group_id() {
    const char* home = std::getenv("HOME");
    if (!home) return 0;

    std::ifstream f(std::string(home) + "/.cache/i3/current_group");
    int group_id = 0;
    f >> group_id;
    return group_id;
  }

  /**
   * Get all workspaces for given output
   */
  vector<shared_ptr<workspace_t>> workspaces(const connection_t& conn, const string& output, const bool show_urgent) {
    vector<shared_ptr<workspace_t>> result;
    auto group_id = get_current_group_id();

    for (auto&& ws : conn.get_workspaces()) {
      if ( (output.empty() || ws->output == output || (show_urgent && ws->urgent))
        && (ws->num > group_id && ws->num <= (group_id + 10))) {
        result.emplace_back(forward<decltype(ws)>(ws));
      }
    }
    return result;
  }

  /**
   * Get currently focused workspace
   */
  shared_ptr<workspace_t> focused_workspace(const connection_t& conn) {
    for (auto&& ws : conn.get_workspaces()) {
      if (ws->focused) {
        return ws;
      }
    }
    return nullptr;
  }

  /**
   * Get main root window
   */
  xcb_window_t root_window(connection& conn) {
    auto children = conn.query_tree(conn.root()).children();
    const auto wm_name = [&](xcb_connection_t* conn, xcb_window_t win) -> string {
      string title;
      if (!(title = ewmh_util::get_wm_name(win)).empty()) {
        return title;
      } else if (!(title = icccm_util::get_wm_name(conn, win)).empty()) {
        return title;
      } else {
        return "";
      }
    };

    for (auto it = children.begin(); it != children.end(); it++) {
      if (wm_name(conn, *it) == "i3") {
        return *it;
      }
    }

    return XCB_NONE;
  }

  /**
   * Returns window against which to restack.
   */
  restack_util::params get_restack_params(connection& conn) {
    return {root_window(conn), XCB_STACK_MODE_ABOVE};
  }
} // namespace i3_util

POLYBAR_NS_END
