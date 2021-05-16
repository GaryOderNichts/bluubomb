#ifndef ADAPTER_H
#define ADAPTER_H

int set_up_device(char * dev_str);
int restore_device();
int power_off_host(const bdaddr_t * host_bdaddr);

#endif /* ADAPTER_H */
