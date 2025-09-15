#ifndef CONFIG_DEF_H
#define CONFIG_DEF_H

// Server configuration
static char *default_port = "6667";
static char *server_name = "sis";
static char *server_info = "Simple IRC Server";
static int max_clients = 64;
static int max_channels = 16;

// Message length limits  
static int max_msg_len = 512;
static int max_nick_len = 32;
static int max_chan_len = 32;

#endif
