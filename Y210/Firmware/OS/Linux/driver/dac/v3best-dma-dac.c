#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>

//#define DEBUG
#ifdef DEBUG
#define dma_dbg(format, ...) \
	do { \
		printk(KERN_INFO format, ## __VA_ARGS__); \
	} while (0)
#else
#define dma_dbg(format, ...)
#endif

/*AXI DMA IPcore memory map address*/
#define AXI_DMA_BASE_RD 0x41220000

/*AXI DMA IPcore register offset*/
#define MM2S_HW
#ifdef MM2S_HW
#define MM2S_START   0x0
#define MM2S_SA      0x4
#define MM2S_LENGTH  0x8
#define MM2S_CYC     0xc

#define MM2S_DONE     0x0
#endif

#define MM2S_CHANNEL  1

#define IP_RESET  	 0
#define FIFO_IP_RESET	 1

#define DEVICE_NAME         "v3best-dma-rd"

#define V3BEST_YUNSDR
#ifdef V3BEST_YUNSDR
#define AXI_DMA_MM2S_IRQ    91
#else
#define AXI_DMA_MM2S_IRQ    89
#endif

#define  XDMA_MAGIC              'D'
#define  XDMA_IOCTL_RESET          _IO(XDMA_MAGIC, 1)
#define  XDMA_START_WAIT_COMPLETE        _IO(XDMA_MAGIC, 2)
#define  XDMA_IOCTL_CONFIG          _IO(XDMA_MAGIC, 3)
#define  XDMA_MEM_ALLOC          _IO(XDMA_MAGIC, 4)

#define MEM_MAX_SIZE (64*1024*1024)

static void __iomem *dma_rd_reg;
struct completion dma_completion;
struct mutex axi_dma_lock;
spinlock_t lock;

#define MEM_ALLOC     1
#define MEM_UNALLOC   0
struct axi_dma_mem {
	void *mem_base;
	dma_addr_t phy_base;
	ssize_t size;
	unsigned char mark;
}mem_info;

struct axi_dma_config_info {
	int channel;
	unsigned long mem_addr;
	unsigned long length;
	unsigned int  cycle;
	unsigned int  reset_type;
};

/**
 * @brief Setup AXI dma IPcore.
 *
 * @param[in] channel          0 S2MM_CHANNEL, 1 MM2S_CHANNEL
 * @param[in] length           Data length for once dma transmission
 * @param[in] cycle            Number of dma transmission 
 * @param[in] mem_addr         Physical DDR address of AXI dma IPCore
 * @param[in] reg_baseaddr     Virtual register address of AXI dma IPcore
 * @retval                     0 Success
 * @retval                     -1 Failure
 *
 */
int dma_init(int channel, unsigned long length, unsigned int cycle,
		unsigned long mem_addr, void __iomem *reg_baseaddr)
{
	if(channel == 1) {
		iowrite32(mem_addr, reg_baseaddr + MM2S_SA);
		iowrite32(length, reg_baseaddr + MM2S_LENGTH);
		iowrite32(cycle, reg_baseaddr + MM2S_CYC);
	}

	return 0;
}


/**
 * @brief start AXI dma IPcore.
 *
 * @param[in] channel          0 S2MM_CHANNEL, 1 MM2S_CHANNEL
 * @param[in] mode             Operating Mode of MM2S_CHANNEL  0 Auto, 1 Manual
 * @param[in] reg_baseaddr     Virtual register address of AXI dma IPcore
 * @retval                     0 Success
 * @retval                     -1 Failure
 *
 */
int dma_start(int channel, int mode, void __iomem *reg_baseaddr)
{
	if(channel == 1) {
		if(mode == 1)
			iowrite32(0x11, reg_baseaddr + MM2S_START);
		else
			iowrite32(0x13, reg_baseaddr + MM2S_START);
	}

	return 0;
}

/**
 * @brief reset AXI dma IPcore.
 *
 * @param[in] channel          0 S2MM_CHANNEL, 1 MM2S_CHANNEL
 * @param[in] length           Data length for once dma transmission
 * @param[in] reg_baseaddr     Virtual register address of AXI dma IPcore
 * @param[in] type             reset type, IP_RESET only reset IPCore, FIFO_IP_RESET reset IPcore&fifo
 * @retval                     0 Success
 * @retval                     -1 Failure
 *
 */
int dma_reset(int channel, void __iomem *reg_baseaddr, int type)
{
	if(channel == 1&&type == IP_RESET)
		iowrite32(0x10, reg_baseaddr + MM2S_START);

	if(channel == 1&&type == FIFO_IP_RESET)
		iowrite32(0x00, reg_baseaddr + MM2S_START);

	return 0;
}

static int axi_dma_open(struct inode *inode, struct file *filp)
{
	mem_info.mem_base = 0;
	mem_info.phy_base = 0;
	mem_info.size = 0;
	mem_info.mark = 0;
	return 0;
}

static int axi_dma_release(struct inode *inode, struct file *filp)
{
	if(mem_info.phy_base != 0) {
		printk(KERN_INFO"free cma memory\n");  
		dma_free_writecombine(NULL, mem_info.size, mem_info.mem_base,
				mem_info.phy_base);
	}
	return 0;
}

static long axi_dma_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	struct axi_dma_config_info *config_info = (struct axi_dma_config_info *)arg;
	switch (cmd) {
	case XDMA_IOCTL_RESET:
		if(config_info->channel == MM2S_CHANNEL){
			dma_reset(config_info->channel, dma_rd_reg, config_info->reset_type);
		}

		break;
	case XDMA_IOCTL_CONFIG:
		if(config_info->channel == MM2S_CHANNEL){
			//dma_init(config_info->channel, config_info->length, config_info->cycle,
			//       config_info->mem_addr, dma_rd_reg);
			break;
		}
		break;
	case XDMA_START_WAIT_COMPLETE:
		if(config_info->channel == MM2S_CHANNEL){
			dma_start(config_info->channel, 1, dma_rd_reg);
			wait_for_completion(&dma_completion);
		}
		break;
	case XDMA_MEM_ALLOC:
		if(config_info->length > 8 && config_info->length <= MEM_MAX_SIZE) {
			config_info->length = PAGE_ALIGN(config_info->length);
			mem_info.size = config_info->length;
			if(mem_info.size == 0) {
				printk(KERN_ERR"alloc memory failed\n");  
				return -ENOMEM;
			}
			mem_info.mem_base = dma_alloc_writecombine(NULL, mem_info.size, &(mem_info.phy_base), GFP_KERNEL);
			if(!mem_info.mem_base) {
				mem_info.phy_base = 0;
				printk(KERN_ERR"alloc memory failed\n");  
				return -ENOMEM;
			}
			config_info->mem_addr = (unsigned long)mem_info.phy_base;
			mem_info.mark = MEM_ALLOC;
		//	int i = 0;
		//	for(i = 0; i < 8192; i++)
		//		*((unsigned char *)mem_info.mem_base+i) = i%255;
		}
		break;
	default:
		return -EINVAL;
		break;
	}
	return 0;
}
static int axi_dma_mmap(struct file *filp, struct vm_area_struct *vma)
{
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if(remap_pfn_range(vma, vma->vm_start, mem_info.phy_base>>PAGE_SHIFT, vma->vm_end - vma->vm_start,
				vma->vm_page_prot))
		return -EAGAIN;
	vma->vm_flags &= ~VM_IO;   
	vma->vm_flags |=  (VM_DONTEXPAND | VM_DONTDUMP);  

	return 0;  
}


static struct file_operations axi_dma_fops = {
	.owner = THIS_MODULE,
	.open = axi_dma_open,
	.release = axi_dma_release,
	.mmap = axi_dma_mmap,
	.unlocked_ioctl = axi_dma_ioctl,
};

static struct miscdevice axi_dma_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &axi_dma_fops,
};

irqreturn_t axi_dma_intr_handler(int irq, void *dev_id)
{
	if(irq == AXI_DMA_MM2S_IRQ) {
		dma_dbg("MM2S_CHANNEL irq\n");
		complete(&dma_completion);
	}
	return IRQ_HANDLED;
}

static int __init axi_dma_init(void)
{
	int ret;

	dma_rd_reg = ioremap(AXI_DMA_BASE_RD, 64);

	/*DMA Interrupt enable */
	ret = request_irq(AXI_DMA_MM2S_IRQ, axi_dma_intr_handler, 
			IRQF_TRIGGER_RISING, DEVICE_NAME, NULL);
	if (ret < 0) {
		dma_dbg(KERN_ERR "unable to request IRQ%d : %d\n", AXI_DMA_MM2S_IRQ, ret);
		return ret;
	}

	init_completion(&dma_completion);
	mutex_init(&axi_dma_lock);
	spin_lock_init(&lock);
	ret = misc_register(&axi_dma_dev);
	return ret;
}

static void __exit axi_dma_exit(void)
{
	iounmap(dma_rd_reg);
	free_irq(AXI_DMA_MM2S_IRQ, NULL);
	misc_deregister(&axi_dma_dev);
	dma_dbg("AXI_DMA module exit.\n");
}

module_init(axi_dma_init);
module_exit(axi_dma_exit);

MODULE_DESCRIPTION("AXI DMA driver");
MODULE_VERSION("v1.0");
MODULE_LICENSE("GPL");
