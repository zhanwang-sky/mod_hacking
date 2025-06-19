//
//  mod_hacking.c
//  mod_hacking
//
//  Created by zhanwang-sky on 2025/3/25.
//

#include <switch.h>

#define HACK_CONFIG_FILE "hacking.conf"
#define HACK_EVENT_WR_ACT "hacking::wr_act"
#define HACK_EVENT_RD_ACT "hacking::rd_act"

SWITCH_MODULE_LOAD_FUNCTION(mod_hacking_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_hacking_shutdown);
SWITCH_MODULE_DEFINITION(mod_hacking, mod_hacking_load, mod_hacking_shutdown, NULL);

typedef struct hacking_session {
  switch_core_session_t* session;
  switch_media_bug_t* bug;
  uint32_t sample_rate;
  uint64_t rd_samples;
  uint64_t wr_samples;
  switch_bool_t wr_act_fired;
  switch_bool_t rd_act_fired;
} hacking_session_t;

static struct {
  char* foo;
  int bar;
} hacking_globals;

static switch_xml_config_item_t hacking_instructions[] = {
  SWITCH_CONFIG_ITEM_STRING_STRDUP("foo", CONFIG_REQUIRED, &hacking_globals.foo, "", "", "foo"),
  SWITCH_CONFIG_ITEM("bar", SWITCH_CONFIG_INT, CONFIG_RELOADABLE, &hacking_globals.bar, (void *) 0, NULL,
                     "", "[optional] bar"),
  SWITCH_CONFIG_ITEM_END()
};

static switch_bool_t
hacking_fire_event(const char* uuid, const char* event_name) {
  switch_event_t* event = NULL;

  if (switch_event_create_subclass(&event, SWITCH_EVENT_CUSTOM, event_name) != SWITCH_STATUS_SUCCESS) {
    return SWITCH_FALSE;
  }
  switch_event_add_header(event, SWITCH_STACK_BOTTOM, "Session-UUID", "%s", uuid);

  DUMP_EVENT(event);

  switch_event_fire(&event);

  return SWITCH_TRUE;
}

static switch_bool_t
hacking_cb(switch_media_bug_t* bug, void* data, switch_abc_type_t type) {
  switch_core_session_t* session = switch_core_media_bug_get_session(bug);
  hacking_session_t* s = (hacking_session_t*) data;
  switch_codec_t* read_codec = NULL;
  switch_frame_t* frame = NULL;
  uint32_t sample_rate = 0;
  switch_bool_t ret = SWITCH_TRUE;

  switch (type) {
    case SWITCH_ABC_TYPE_INIT:
      do {
        s->session = session;
        s->bug = bug;

        read_codec = switch_core_session_get_read_codec(session);
        if (!read_codec || !read_codec->implementation) {
          switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR,
                            "hacking_cb[init]: read codec not set\n");
          ret = SWITCH_FALSE;
          break;
        }

        sample_rate = read_codec->implementation->samples_per_second;
        if (sample_rate < 8000 || sample_rate > 48000) {
          switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR,
                            "hacking_cb[init]: unsupported sample_rate: %u\n", sample_rate);
          ret = SWITCH_FALSE;
          break;
        }
        s->sample_rate = sample_rate;

        switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO,
                          "hacking_cb[init]: hacking_session started, sample_rate=%u\n",
                          sample_rate);

      } while (0);

      if (ret != SWITCH_TRUE) {
        // XXX TODO: do clean
      }

      break; // case SWITCH_ABC_TYPE_INIT

    case SWITCH_ABC_TYPE_WRITE_REPLACE:
      frame = switch_core_media_bug_get_write_replace_frame(bug);
      if (frame) {
        s->wr_samples += frame->samples;
        if (!s->wr_act_fired && (s->wr_samples > s->sample_rate)) {
          char* uuid = switch_core_session_get_uuid(session);
          s->wr_act_fired = hacking_fire_event(uuid, HACK_EVENT_WR_ACT);
        }
      }
      break;

    case SWITCH_ABC_TYPE_READ_REPLACE:
      frame = switch_core_media_bug_get_read_replace_frame(bug);
      if (frame) {
        s->rd_samples += frame->samples;
        if (!s->rd_act_fired && (s->rd_samples > s->sample_rate)) {
          char* uuid = switch_core_session_get_uuid(session);
          s->rd_act_fired = hacking_fire_event(uuid, HACK_EVENT_RD_ACT);
        }
      }
      break;

    case SWITCH_ABC_TYPE_CLOSE:
      // XXX TODO: do clean
      switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO,
                        "hacking_cb[close]: hacking_session ended, "
                        "total %lu samples read, %lu samples write\n",
                        s->rd_samples, s->wr_samples);
      break;

    default:
      break;
  }

  return ret;
}

// static void hacking_app_func(switch_core_session_t *session, const char *data)
SWITCH_STANDARD_APP(hacking_app_func) {
  switch_channel_t* channel = NULL;
  hacking_session_t* s = NULL;
  switch_media_bug_t* bug = NULL;

  switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO,
                    "app_func: data={%s}\n", data);

  channel = switch_core_session_get_channel(session);
  if (!channel) {
    switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR,
                      "app_func: fail to get media channel\n");
    return;
  }

  if (switch_channel_get_private(channel, "__hacking_session__") != NULL) {
    switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR,
                      "app_func: media bug already attached\n");
    return;
  }

  if (!(s = switch_core_session_alloc(session, sizeof(hacking_session_t)))) {
    switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR,
                      "app_func: fail to alloc hacking_session\n");
    return;
  }

  if (switch_core_media_bug_add(session, "hacking", NULL, hacking_cb, s, 0,
                                SMBF_WRITE_REPLACE | SMBF_READ_REPLACE,
                                &bug) != SWITCH_STATUS_SUCCESS) {
    switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR,
                      "app_func: fail to add media bug!\n");
    return;
  }

  if (switch_channel_set_private(channel, "__hacking_session__", s) != SWITCH_STATUS_SUCCESS) {
    switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_ERROR,
                      "app_func: fail to set channel private\n");
    // XXX TODO: remove media bug
    return;
  }

  switch_log_printf(SWITCH_CHANNEL_SESSION_LOG(session), SWITCH_LOG_INFO,
                    "app_func: media bug added\n");
}

// switch_status_t mod_hacking_load(switch_loadable_module_interface_t **module_interface, switch_memory_pool_t *pool)
SWITCH_MODULE_LOAD_FUNCTION(mod_hacking_load) {
  switch_application_interface_t* app_interface = NULL;

  // load config
  if (switch_xml_config_parse_module_settings(HACK_CONFIG_FILE, SWITCH_FALSE, hacking_instructions) != SWITCH_STATUS_SUCCESS) {
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "fail to parse config file '%s'\n", HACK_CONFIG_FILE);
    return SWITCH_STATUS_FALSE;
  }

  // connect my internal structure to the blank pointer passed to me
  *module_interface = switch_loadable_module_create_module_interface(pool, modname);

  SWITCH_ADD_APP(app_interface,
                 "hacking", "do some hacking with media data", "hacking - do some hacking with media data",
                 hacking_app_func, "rtmp://balabala.com/foo/bar", SAF_MEDIA_TAP);

  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "%s loaded\n", modname);

  return SWITCH_STATUS_SUCCESS;
}

// switch_status_t mod_hacking_shutdown(void)
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_hacking_shutdown) {
  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "shutting down %s...\n", modname);
  // XXX TODO: cleanup
  return SWITCH_STATUS_SUCCESS;
}
