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



**jailhouse_ioctl**

用于控制输入命令的结果

```
static long jailhouse_ioctl(struct file *file, unsigned int ioctl,
			    unsigned long arg)
{
	long err;

	switch (ioctl) {
	case JAILHOUSE_ENABLE:
		err = jailhouse_cmd_enable(
			(struct jailhouse_system __user *)arg);
		break;
	case JAILHOUSE_DISABLE:
		err = jailhouse_cmd_disable();
		break;
	case JAILHOUSE_CELL_CREATE:
		err = jailhouse_cmd_cell_create(
			(struct jailhouse_cell_create __user *)arg);
		break;
	case JAILHOUSE_CELL_LOAD:
		err = jailhouse_cmd_cell_load(
			(struct jailhouse_cell_load __user *)arg);
		break;
	case JAILHOUSE_CELL_START:
		err = jailhouse_cmd_cell_start((const char __user *)arg);
		break;
	case JAILHOUSE_CELL_DESTROY:
		err = jailhouse_cmd_cell_destroy((const char __user *)arg);
		break;
	default:
		err = -EINVAL;
		break;
	}

	return err;
}
```

**JAILHOUSE_ENABLE**



**JAILHOUSE_DISABLE**



**JAILHOUSE_CELL_CREATE**

调用	jailhouse_cmd_cell_create，传入参数：struct jailhouse_cell_create



```
 int jailhouse_cmd_cell_create(struct jailhouse_cell_create __user *arg)
{
	struct jailhouse_cell_create cell_params;
	struct jailhouse_cell_desc *config;
	struct jailhouse_cell_id cell_id;
	void __user *user_config;
	struct cell *cell;
	unsigned int cpu;
	int err = 0;
	
	if (copy_from_user(&cell_params, arg, sizeof(cell_params)))
		return -EFAULT;

	config = kmalloc(cell_params.config_size, GFP_USER | __GFP_NOWARN);
	if (!config)
		return -ENOMEM;

	user_config = (void __user *)(unsigned long)cell_params.config_address;
	if (copy_from_user(config, user_config, cell_params.config_size)) {
		err = -EFAULT;
		goto kfree_config_out;
	}

	if (cell_params.config_size < sizeof(*config) ||
	    memcmp(config->signature, JAILHOUSE_CELL_DESC_SIGNATURE,
		   sizeof(config->signature)) != 0) {
		pr_err("jailhouse: Not a cell configuration\n");
		err = -EINVAL;
		goto kfree_config_out;
	}
	if (config->revision != JAILHOUSE_CONFIG_REVISION) {
		pr_err("jailhouse: Configuration revision mismatch\n");
		err = -EINVAL;
		goto kfree_config_out;
	}

	config->name[JAILHOUSE_CELL_NAME_MAXLEN] = 0;

	/* CONSOLE_ACTIVE implies CONSOLE_PERMITTED for non-root cells */
	if (CELL_FLAGS_VIRTUAL_CONSOLE_ACTIVE(config->flags))
		config->flags |= JAILHOUSE_CELL_VIRTUAL_CONSOLE_PERMITTED;

	if (mutex_lock_interruptible(&jailhouse_lock) != 0) {
		err = -EINTR;
		goto kfree_config_out;
	}

	if (!jailhouse_enabled) {
		err = -EINVAL;
		goto unlock_out;
	}

	cell_id.id = JAILHOUSE_CELL_ID_UNUSED;
	memcpy(cell_id.name, config->name, sizeof(cell_id.name));
	if (find_cell(&cell_id) != NULL) {
		err = -EEXIST;
		goto unlock_out;
	}

	cell = cell_create(config);
	if (IS_ERR(cell)) {
		err = PTR_ERR(cell);
		goto unlock_out;
	}

	config->id = cell->id;

	if (!cpumask_subset(&cell->cpus_assigned, &root_cell->cpus_assigned)) {
		err = -EBUSY;
		goto error_cell_delete;
	}

	/* Off-line each CPU assigned to the new cell and remove it from the
	 * root cell's set. */
	for_each_cpu(cpu, &cell->cpus_assigned) {
#ifdef CONFIG_X86
		if (cpu == 0) {
			/*
			 * On x86, Linux only parks CPU 0 when offlining it and
			 * expects to be able to get it back by sending an IPI.
			 * This is not support by Jailhouse wich destroys the
			 * CPU state across non-root assignments.
			 */
			pr_err("Cannot assign CPU 0 to other cells\n");
			err = -EINVAL;
			goto error_cpu_online;
		}
#endif
		if (cpu_online(cpu)) {
			err = cpu_down(cpu);
			if (err)
				goto error_cpu_online;
			cpumask_set_cpu(cpu, &offlined_cpus);
		}
		cpumask_clear_cpu(cpu, &root_cell->cpus_assigned);
	}

	jailhouse_pci_do_all_devices(cell, JAILHOUSE_PCI_TYPE_DEVICE,
	                             JAILHOUSE_PCI_ACTION_CLAIM);

	err = jailhouse_call_arg1(JAILHOUSE_HC_CELL_CREATE, __pa(config));
	if (err < 0)
		goto error_cpu_online;

	cell_register(cell);

	pr_info("Created Jailhouse cell \"%s\"\n", config->name);

unlock_out:
	mutex_unlock(&jailhouse_lock);

kfree_config_out:
	kfree(config);

	return err;

error_cpu_online:
	for_each_cpu(cpu, &cell->cpus_assigned) {
		if (!cpu_online(cpu) && cpu_up(cpu) == 0)
			cpumask_clear_cpu(cpu, &offlined_cpus);
		cpumask_set_cpu(cpu, &root_cell->cpus_assigned);
	}

error_cell_delete:
	cell_delete(cell);
	goto unlock_out;
}
```



**JAILHOUSE_CELL_LOAD**



**JAILHOUSE_CELL_START**



**JAILHOUSE_CELL_DESTROY**