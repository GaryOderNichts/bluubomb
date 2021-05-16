#ifndef BDADDR_H
#define BDADDR_H

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

int set_device_bdaddr(int dd, const struct hci_version * ver, const bdaddr_t * bdaddr);

#endif /* BDADDR_H */
