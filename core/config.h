#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "lunkwill.h" //for config_t

/** \brief Starting libconfig to parse config*/
extern int load_config(config_t *config, char *config_file_name);

/** \brief Starting libconfig to parse config*/
extern int create_config(config_t *config, char *config_file_name);

#endif
