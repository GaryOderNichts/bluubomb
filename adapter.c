#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "adapter.h"

#define HCI_TIMEOUT 1000

static char original_name[HCI_MAX_NAME_LENGTH];
static uint8_t original_class[3];
static uint8_t original_scan_enable;
static uint8_t original_iac[MAX_IAC_LAP][3];
static uint8_t original_iac_num;
static uint8_t original_simple_pairing_mode;

static const char * wiimote_name = "Nintendo RVL-CNT-01";
static const uint32_t wiimote_class = 0x002504;
static const uint8_t wiimote_iac[3] = { 0x00, 0x8B, 0x9E };

int hci_read_scan_enable(int dd, uint8_t * enabled, int to)
{
  struct {
    uint8_t status;
    uint8_t enabled;
  } __attribute__ ((packed)) rp;
  struct hci_request rq;

  memset(&rq, 0, sizeof(rq));
  rq.ogf    = OGF_HOST_CTL;
  rq.ocf    = OCF_READ_SCAN_ENABLE;
  rq.rparam = &rp;
  rq.rlen   = sizeof(rp);

  if (hci_send_req(dd, &rq, to) < 0)
  {
    return -1;
  }

  if (rp.status)
  {
    errno = EIO;
    return -1;
  }

  *enabled = rp.enabled;
  return 0;
}

int hci_write_scan_enable(int dd, uint8_t enabled, int to)
{
  struct {
    uint8_t enabled;
  } __attribute__ ((packed)) cp;
  struct {
    uint8_t status;
  } __attribute__ ((packed)) rp;
  struct hci_request rq;

  memset(&cp, 0, sizeof(cp));
  cp.enabled = enabled;

  memset(&rq, 0, sizeof(rq));
  rq.ogf    = OGF_HOST_CTL;
  rq.ocf    = OCF_WRITE_SCAN_ENABLE;
  rq.cparam = &cp;
  rq.clen   = sizeof(cp);
  rq.rparam = &rp;
  rq.rlen   = sizeof(rp);

  if (hci_send_req(dd, &rq, to) < 0)
  {
    return -1;
  }

  if (rp.status)
  {
    errno = EIO;
    return -1;
  }

  return 0;
}

int set_up_device_name(int dd)
{
  int ret;

  ret = hci_read_local_name(dd, HCI_MAX_NAME_LENGTH, original_name, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't read device name: %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  ret = hci_write_local_name(dd, wiimote_name, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't write device name: %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  return 0;
}

int restore_device_name(int dd)
{
  int ret;

  ret = hci_write_local_name(dd, original_name, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't restore device name: %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  return 0;
}

int set_up_device_class(int dd)
{
  int ret;

  ret = hci_read_class_of_dev(dd, original_class, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't read device class: %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  ret = hci_write_class_of_dev(dd, wiimote_class, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't write device class: %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  return 0;
}

int restore_device_class(int dd)
{
  int ret;

  uint32_t class_int = 0;
  class_int |= original_class[0];
  class_int |= original_class[1] << 8;
  class_int |= original_class[2] << 16;

  ret = hci_write_class_of_dev(dd, class_int, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't restore device class: %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  return 0;
}

int set_up_device_inquiry(int dd)
{
  int ret;

  ret = hci_read_scan_enable(dd, &original_scan_enable, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't read scan enable: %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  ret = hci_write_scan_enable(dd, SCAN_INQUIRY | SCAN_PAGE, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't write scan enable: %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  ret = hci_read_current_iac_lap(dd, &original_iac_num, (uint8_t *)original_iac, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't read iac: %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  ret = hci_write_current_iac_lap(dd, 1, (uint8_t *)wiimote_iac, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't write iac: %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  return 0;
}

int restore_device_inquiry(int dd)
{
  int ret;

  ret = hci_write_scan_enable(dd, original_scan_enable, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't restore scan enable: %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  ret = hci_write_current_iac_lap(dd, original_iac_num, (uint8_t *)original_iac, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't restore iac: %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  return 0;
}

int set_up_simple_pairing_mode(int dd)
{
  int ret;

  ret = hci_read_simple_pairing_mode(dd, &original_simple_pairing_mode, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't read simple pairing mode: %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  if (original_simple_pairing_mode != 0)
  {
    ret = hci_write_simple_pairing_mode(dd, 0, HCI_TIMEOUT);
    if (ret < 0)
    {
      fprintf(stderr, "Can't write simple pairing mode: %s (%d)\n", strerror(errno), errno);
      return -1;
    }
  }

  return 0;
}

int restore_simple_pairing_mode(int dd)
{
  int ret;

  if (original_simple_pairing_mode != 0)
  {
    ret = hci_write_simple_pairing_mode(dd, original_simple_pairing_mode, HCI_TIMEOUT);
    if (ret < 0)
    {
      fprintf(stderr, "Can't restore simple pairing mode: %s (%d)\n", strerror(errno), errno);
      return -1;
    }
  }

  return 0;
}

int set_up_device(char * dev_str)
{
  int device_id = 0, dd, ret;

  dd = hci_open_dev(device_id);
  if (dd < 0)
  {
    fprintf(stderr, "Can't open device hci%d: %s (%d)\n",
      device_id, strerror(errno), errno);
    return -1;
  }

  ret = set_up_device_name(dd);
  if (ret < 0)
  {
    printf("Failed to set device name\n");
    hci_close_dev(dd);
    return -1;
  }

  ret = set_up_device_class(dd);
  if (ret < 0)
  {
    printf("Failed to set device class\n");
    hci_close_dev(dd);
    return -1;
  }

  ret = set_up_device_inquiry(dd);
  if (ret < 0)
  {
    printf("Failed to set device inquiry settings\n");
    hci_close_dev(dd);
    return -1;
  }
  
  ret = set_up_simple_pairing_mode(dd);
  if (ret < 0)
  {
    printf("Failed to set simple pairing mode\n");
    printf("Warning: make sure secure simple pairing mode is disabled\n");
  }

  hci_close_dev(dd);
  return 0;
}

int restore_device()
{
  int device_id = 0, dd, ret;

  dd = hci_open_dev(device_id);
  if (dd < 0)
  {
    fprintf(stderr, "Can't open device hci%d: %s (%d)\n",
      device_id, strerror(errno), errno);
    return -1;
  }

  ret = restore_device_name(dd);
  if (ret < 0)
  {
    printf("Failed to restore device name\n");
    hci_close_dev(dd);
    return -1;
  }

  ret = restore_device_class(dd);
  if (ret < 0)
  {
    printf("Failed to restore device class\n");
    hci_close_dev(dd);
    return -1;
  }

  ret = restore_device_inquiry(dd);
  if (ret < 0)
  {
    printf("Failed to restore device inquiry settings\n");
    hci_close_dev(dd);
    return -1;
  }

  ret = restore_simple_pairing_mode(dd);
  if (ret < 0)
  {
    printf("Failed to restore simple pairing mode\n");
    hci_close_dev(dd);
    return -1;
  }

  hci_close_dev(dd);
  return 0;
}
