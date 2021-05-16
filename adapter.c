#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "adapter.h"
#include "bdaddr.h"

#define HCI_TIMEOUT 1000

static int bdaddr_was_set = 0;
static bdaddr_t original_bdaddr;
static char original_name[HCI_MAX_NAME_LENGTH];
static uint8_t original_class[3];
static uint8_t original_scan_enable;
static uint8_t original_iac[MAX_IAC_LAP][3];
static uint8_t original_iac_num;
static uint8_t original_simple_pairing_mode;

static bdaddr_t wiimote_baddr;
static const char * wiimote_name = "Nintendo RVL-CNT-01";
static const uint32_t wiimote_class = 0x002504;
static const uint8_t wiimote_iac[3] = { 0x00, 0x8B, 0x9E };

static const uint32_t nintendo_ouis[66] =
{
  0xECC40D, 0xE84ECE, 0xE0F6B5, 0xE0E751, 0xE00C7F, 0xDC68EB, 0xD86BF7, 0xD4F057,
  0xCCFB65, 0xCC9E00, 0xB8AE6E, 0xB88AEC, 0xB87826, 0xA4C0E1, 0xA45C27, 0xA438CC,
  0x9CE635, 0x98E8FA, 0x98B6E9, 0x98415C, 0x9458CB, 0x8CCDE8, 0x8C56C5, 0x7CBB8A,
  0x78A2A0, 0x7048F7, 0x64B5C6, 0x606BFF, 0x5C521E, 0x58BDA3, 0x582F40, 0x48A5E7,
  0x40F407, 0x40D28A, 0x34AF2C, 0x342FBD, 0x2C10C1, 0x182A7B, 0x0403D6, 0x002709,
  0x002659, 0x0025A0, 0x0024F3, 0x002444, 0x00241E, 0x0023CC, 0x002331, 0x0022D7,
  0x0022AA, 0x00224C, 0x0021BD, 0x002147, 0x001FC5, 0x001F32, 0x001EA9, 0x001E35,
  0x001DBC, 0x001CBE, 0x001BEA, 0x001B7A, 0x001AE9, 0x0019FD, 0x00191D, 0x0017AB,
  0x001656, 0x0009BF
};

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

int set_up_device_address(int dd, int device_id)
{
  int ret, i;
  uint32_t uap;
  struct hci_dev_info di;
  struct hci_version ver;

  ret = hci_devinfo(device_id, &di);
  if (ret < 0)
  {
    fprintf(stderr, "Can't get device info for hci%d: %s (%d)\n",
            device_id, strerror(errno), errno);
    return -1;
  }

  if (!bacmp(&di.bdaddr, BDADDR_ANY))
  {
    ret = hci_read_bd_addr(dd, &original_bdaddr, HCI_TIMEOUT);
    if (ret < 0)
    {
      fprintf(stderr, "Can't read address for hci%d: %s (%d)\n",
        device_id, strerror(errno), errno);
      return -1;
    }
  }
  else
  {
    bacpy(&original_bdaddr, &di.bdaddr);
  }

  ret = hci_read_local_version(dd, &ver, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't read version info for hci%d: %s (%d)\n",
      device_id, strerror(errno), errno);
    return -1;
  }

  //check if bdaddr already has a Nintendo OUI (e.g. it was manually set)
  uap = (original_bdaddr.b[5] << 16) | (original_bdaddr.b[4] << 8) |
    original_bdaddr.b[3];
  for (i = 0; i < sizeof(nintendo_ouis); i++)
  {
    if (nintendo_ouis[i] == uap)
    {
      return 0;
    }
  }

  bacpy(&wiimote_baddr, &original_bdaddr);
  wiimote_baddr.b[5] = (nintendo_ouis[65] >> 16) & 0xFF;
  wiimote_baddr.b[4] = (nintendo_ouis[65] >> 8) & 0xFF;
  wiimote_baddr.b[3] = (nintendo_ouis[65]) & 0xFF;

  ret = set_device_bdaddr(dd, &ver, &wiimote_baddr);
  if (ret < 0)
  {
    printf("Failed to set device address\n");
    printf("Device manufacturer: %s (%d)\n",
      bt_compidtostr(ver.manufacturer), ver.manufacturer);
    return -1;
  }

  bdaddr_was_set = 1;

  ret = ioctl(dd, HCIDEVDOWN, device_id);
  if (ret < 0)
  {
    fprintf(stderr, "Can't down device hci%d: %s (%d)\n",
      device_id, strerror(errno), errno);
  }

  ret = ioctl(dd, HCIDEVUP, device_id);
  if (ret < 0)
  {
    fprintf(stderr, "Can't init device hci%d: %s (%d)\n",
      device_id, strerror(errno), errno);
  }

  return 0;
}

int restore_device_address(int dd, int device_id)
{
  int ret;
  struct hci_version ver;

  if (bdaddr_was_set == 0)
  {
    return 0;
  }

  ret = hci_read_local_version(dd, &ver, 1000);
  if (ret < 0)
  {
    fprintf(stderr, "Can't read version info: %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  ret = set_device_bdaddr(dd, &ver, &original_bdaddr);
  if (ret < 0)
  {
    printf("Failed to restore device address\n");
    printf("Device manufacturer: %s (%d)\n",
      bt_compidtostr(ver.manufacturer), ver.manufacturer);
    return -1;
  }
  
  ret = ioctl(dd, HCIDEVDOWN, device_id);
  if (ret < 0)
  {
    fprintf(stderr, "Can't down device hci%d: %s (%d)\n",
      device_id, strerror(errno), errno);
  }

  ret = ioctl(dd, HCIDEVUP, device_id);
  if (ret < 0)
  {
    fprintf(stderr, "Can't init device hci%d: %s (%d)\n",
      device_id, strerror(errno), errno);
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

  ret = hci_write_simple_pairing_mode(dd, 0, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't write simple pairing mode: %s (%d)\n", strerror(errno), errno);
    return -1;
  }

  return 0;
}

int restore_simple_pairing_mode(int dd)
{
  int ret;

  ret = hci_write_simple_pairing_mode(dd, original_simple_pairing_mode, HCI_TIMEOUT);
  if (ret < 0)
  {
    fprintf(stderr, "Can't restore simple pairing mode: %s (%d)\n", strerror(errno), errno);
    return -1;
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

  ret = set_up_device_address(dd, device_id);
  if (ret < 0)
  {
    printf("Failed to set device address\n");
    printf("Warning: device address must have a Nintendo OUI\n");
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
    hci_close_dev(dd);
    return -1;
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

  ret = restore_device_address(dd, device_id);
  if (ret < 0)
  {
    printf("Failed to restore device address\n");
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

int power_off_host(const bdaddr_t * host_bdaddr)
{
  int ret, dd;
  struct hci_conn_info_req * cr;
  
  dd = hci_open_dev(hci_get_route((bdaddr_t *)host_bdaddr));
  if (dd < 0)
  {
    return dd;
  }
  
  cr = (struct hci_conn_info_req *)malloc(sizeof(struct hci_conn_info_req) + sizeof(struct hci_conn_info));
  bacpy(&cr->bdaddr, host_bdaddr);
  cr->type = ACL_LINK;

  ret = ioctl(dd, HCIGETCONNINFO, (unsigned long)cr);
  if (ret)
  {
    hci_close_dev(dd);
    free(cr);
    return ret;
  }
  
  ret = hci_disconnect(dd, cr->conn_info->handle, HCI_OE_POWER_OFF, HCI_TIMEOUT);
  hci_close_dev(dd);
  free(cr);

  if (ret)
  {    
    return ret;
  }

  return 0;
}
