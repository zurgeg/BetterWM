#ifndef DEVICE_SETUP_H
#define DEVICE_SETUP_H

int set_up_device(char * dev_str);
int restore_device();
int power_off_host(const bdaddr_t * host_bdaddr);

#endif /* DEVICE_SETUP_H */
