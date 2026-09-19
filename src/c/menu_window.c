#include "ui.h"
#include "game.h"
#include "gfx.h"
#include "item_art.h"

// ============================================================
// メニュー（町にいるときとダンジョンにいるときで項目が変わる）
// ============================================================
typedef enum {
  CMD_DUNGEONS,
  CMD_SHOP,
  CMD_STATUS,
  CMD_STASH,
  CMD_SMITH,
  CMD_IDENTIFY,
  CMD_AGENT,
  CMD_CODEX,
  CMD_SETTINGS,
  CMD_PORTAL,
  CMD_WALK_BACK,
} Command;

#define MAX_COMMANDS 10

static Window *s_window;
static MenuLayer *s_menu;

static int build_commands(Command *out) {
  int n = 0;
  if (game_run_mode() == RUN_NONE) {
    out[n++] = CMD_DUNGEONS;
    out[n++] = CMD_SHOP;
    out[n++] = CMD_STATUS;
    out[n++] = CMD_STASH;
    out[n++] = CMD_SMITH;
    if (game_unidentified_count() > 0) out[n++] = CMD_IDENTIFY;
    if (game_drop_exists()) out[n++] = CMD_AGENT;
    out[n++] = CMD_CODEX;
    out[n++] = CMD_SETTINGS;
  } else {
    out[n++] = CMD_PORTAL;
    if (game_run_mode() == RUN_EXPLORE) out[n++] = CMD_WALK_BACK;
    out[n++] = CMD_STATUS;
    out[n++] = CMD_CODEX;
    out[n++] = CMD_SETTINGS;
  }
  return n;
}

static uint16_t get_num_rows(MenuLayer *menu, uint16_t section, void *data) {
  Command cmds[MAX_COMMANDS];
  return build_commands(cmds);
}

static int16_t get_cell_height(MenuLayer *menu, MenuIndex *index, void *data) {
  return ROW_H;
}

static void draw_row(GContext *ctx, const Layer *cell, MenuIndex *index, void *data) {
  Command cmds[MAX_COMMANDS];
  int n = build_commands(cmds);
  if (index->row >= n) return;
  static char sub[32];
  RowSpec row = { .icon = -1 };
  switch (cmds[index->row]) {
    case CMD_DUNGEONS:
      row.icon = UI_ICON_DUNGEONS;
      row.title = "Dungeons";
      if (game_saved_steps() > 0) {
        snprintf(sub, sizeof(sub), "Saved %ld steps", (long)game_saved_steps());
        row.sub = sub;
      } else {
        row.sub = "Go exploring";
      }
      break;
    case CMD_SHOP:
      row.icon = UI_ICON_SHOP;
      row.title = "Shop";
      row.sub = "Buy & sell";
      break;
    case CMD_STATUS:
      row.icon = UI_ICON_STATUS;
      row.title = "Status";
      row.sub = "Gear & bag";
      break;
    case CMD_STASH:
      row.icon = UI_ICON_STASH;
      row.title = "Stash";
      snprintf(sub, sizeof(sub), "%d/%d stored", game_stash_count(), STASH_SIZE);
      row.sub = sub;
      break;
    case CMD_SMITH:
      row.icon = UI_ICON_SMITH;
      row.title = "Blacksmith";
      row.sub = "Upgrade to +10";
      break;
    case CMD_CODEX:
      row.icon = UI_ICON_CODEX;
      row.title = "Codex";
      if (game_rumour_count() > 0) {
        snprintf(sub, sizeof(sub), "%d/%d, %d hunt%s", game_codex_found_count(), game_codex_size(),
                 game_rumour_count(), game_rumour_count() > 1 ? "s" : "");
      } else {
        snprintf(sub, sizeof(sub), "%d/%d found", game_codex_found_count(), game_codex_size());
      }
      row.sub = sub;
      break;
    case CMD_IDENTIFY:
      row.icon = UI_ICON_APPRAISER;
      row.title = "Appraiser";
      snprintf(sub, sizeof(sub), "%d unidentified", game_unidentified_count());
      row.sub = sub;
      break;
    case CMD_AGENT:
      row.icon = UI_ICON_LOST_GEAR;
      row.title = "Lost Gear";
      snprintf(sub, sizeof(sub), "%ldh left", (long)(game_drop_seconds_left() / 3600));
      row.sub = sub;
      row.warn_sub = true;
      break;
    case CMD_SETTINGS:
      row.icon = UI_ICON_SETTINGS;
      row.title = "Settings";
      row.sub = "Auto return";
      break;
    case CMD_PORTAL:
      row.icon = UI_ICON_PORTAL;
      row.title = "Portal";
      snprintf(sub, sizeof(sub), "Scrolls x%d", game_portals());
      row.sub = sub;
      row.dim = game_portals() == 0;
      break;
    case CMD_WALK_BACK:
      row.icon = UI_ICON_WALK_BACK;
      row.title = "Walk Back";
      row.sub = "Head to town";
      break;
  }
  gfx_draw_row(ctx, cell, &row);
}

static void select_click(MenuLayer *menu, MenuIndex *index, void *data) {
  Command cmds[MAX_COMMANDS];
  int n = build_commands(cmds);
  if (index->row >= n) return;
  switch (cmds[index->row]) {
    case CMD_DUNGEONS:
      dungeon_window_push();
      break;
    case CMD_SHOP:
      shop_window_push();
      break;
    case CMD_STATUS:
      status_window_push();
      break;
    case CMD_STASH:
      stash_window_push();
      break;
    case CMD_SMITH:
      smith_window_push();
      break;
    case CMD_CODEX:
      codex_window_push();
      break;
    case CMD_IDENTIFY:
      identify_window_push();
      break;
    case CMD_AGENT:
      agent_window_push();
      break;
    case CMD_SETTINGS:
      settings_window_push();
      break;
    case CMD_PORTAL:
      if (game_use_portal()) {
        ui_back_to_scene();
      } else {
        vibes_short_pulse();
      }
      break;
    case CMD_WALK_BACK:
      game_walk_back();
      ui_back_to_scene();
      break;
  }
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_menu = menu_layer_create(layer_get_bounds(root));
  menu_layer_set_callbacks(s_menu, NULL, (MenuLayerCallbacks){
    .get_num_rows = get_num_rows,
    .get_cell_height = get_cell_height,
    .draw_row = draw_row,
    .select_click = select_click,
  });
  gfx_setup_menu(s_menu, window);
  layer_add_child(root, menu_layer_get_layer(s_menu));
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu);
  s_menu = NULL;
  window_destroy(window);
  s_window = NULL;
}

void menu_window_refresh(void) {
  if (s_menu) {
    menu_layer_reload_data(s_menu);
    menu_layer_set_selected_index(s_menu, MenuIndex(0, 0), MenuRowAlignCenter, false);
  }
}

void menu_window_push(void) {
  if (s_window) return;
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
}
