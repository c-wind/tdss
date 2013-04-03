#ifndef __DATA_FILE_H__
#define __DATA_FILE_H__



int ds_open_file(file_handle_t **fh, char *name);

int ds_close_file(file_handle_t *fh);

int disk_status(char *path, data_server_info_t *dsi);

#endif

