#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "device_setup.h"
#include "bdaddr.h"

#define HCI_TIMEOUT 1000

static bdaddr_t original_bdaddr;
static char original_name[HCI_MAX_NAME_LENGTH];
static uint8_t original_class[3];
static uint8_t original_scan_enable;
static uint8_t original_iac[MAX_IAC_LAP][3];
static uint8_t original_iac_num;

static bdaddr_t wiimote_baddr;
static const char * wiimote_name = "Nintendo RVL-CNT-01";
static const uint32_t wiimote_class = 0x002504;
static const uint8_t wiimote_iac[3] = { 0x00, 0x8B, 0x9E };

//a few nintendo OUIs that can be used (there are many)
static const bdaddr_t nintendo_ouis[3] =
{
  { { 0x00, 0x00, 0x00, 0xBF, 0x09, 0x00 } },
  { { 0x00, 0x00, 0x00, 0x56, 0x16, 0x00 } },
  { { 0x00, 0x00, 0x00, 0xEA, 0x1B, 0x00 } }
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
  int ret;
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

  bacpy(&wiimote_baddr, &original_bdaddr);
  wiimote_baddr.b[5] = nintendo_ouis[0].b[5];
  wiimote_baddr.b[4] = nintendo_ouis[0].b[4];
  wiimote_baddr.b[3] = nintendo_ouis[0].b[3];

  ret = set_device_bdaddr(dd, &ver, &wiimote_baddr);
  if (ret < 0)
  {
    printf("Failed to set device address\n");
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

int restore_device_address(int dd, int device_id)
{
  int ret;
  struct hci_version ver;

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
    hci_close_dev(dd);
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
    hci_close_dev(dd);
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
  
  hci_close_dev(dd);
  return 0;
}