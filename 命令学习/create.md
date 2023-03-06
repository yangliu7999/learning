This process starts in a Linux cell with the ioctl (JAILHOUSE_CELL_CREATE) that cause
jailhouse_cell_create() function call in the Linux kernel.

这个过程在Linux单元中开始，通过ioctl (JAILHOUSE_CELL_CREATE)系统调用在Linux内核中引起jailhouse_cell_create函数调用。



ioctl 是设备驱动程序中设备控制接口函数，一个字符设备驱动通常会实现设备打开、关闭、读、写等功能，在一些需要细分的情境下，如果需要扩展新的功能，通常以增设 ioctl() 命令的方式实现。



\#define JAILHOUSE_CELL_CREATE    _IOW(0, 2, struct jailhouse_cell_create)

***_IOW***宏，用于创建设备上写入数据的命令，其余内容与 *_IOR* 相同。通常，使用该命令时，*ioctl()* 的 *arg* 变量值指定设备驱动程序上写入数据时的缓存*(*结构体*)*地址。



jailhouse_cell_create

```
struct jailhouse_cell_create {
	__u64 config_address;
	__u32 config_size;
	__u32 padding;
};
```

流程：

**1.misc_open**

**2.jailhouse_console_open**

**3.jailhouse_ioctl**

**4.jailhouse_cmd_cell_create** 

**5.find_cell**

**6.cell_create**

**7.jailhouse_pci_cell_setup**

**8.jailhouse_sysfs_cell_create**

**9.jailhouse_pci_do_all_devices**

**10.jailhouse_sysfs_cell_register**

**11.jailhouse_console_release**