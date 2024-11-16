#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/device.h>

#define GPIO_PIN 531 // GPIO pin number 19
#define DEVICE_NAME "btnintp"

static unsigned int irq_number;
static int irq_counter = 0;
static dev_t btnintp_devnum;
static struct cdev btnintp_cdev;
static struct class *btnintp_class = NULL;
static struct device *btnintp_device = NULL;

static DECLARE_WAIT_QUEUE_HEAD(btnintp_wait_queue);
static int btnintp_available = 1;
static int event_counter = 0;

/* ISR function for handling GPIO interrupts */
static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    event_counter++;
    irq_counter++;
    wake_up_interruptible(&btnintp_wait_queue);
    printk(KERN_INFO "btnintp: Interrupt occurred! Count: %d\n", irq_counter);
    return IRQ_HANDLED;
}

static int btnintp_open(struct inode *inode, struct file *file)
{
    if (!btnintp_available) {
        return -EBUSY;
    }
    btnintp_available = 0;
    printk(KERN_INFO "btnintp: Device opened\n");
    return 0;
}

static int btnintp_release(struct inode *inode, struct file *file)
{
    btnintp_available = 1;
    printk(KERN_INFO "btnintp: Device closed\n");
    return 0;
}

static unsigned int btnintp_poll(struct file *file, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    poll_wait(file, &btnintp_wait_queue, wait);

    if (event_counter > 0) {
        mask |= POLLIN | POLLRDNORM;
        event_counter = 0;
    }
    
    return mask;
}

static const struct file_operations btnintp_fops = {
    .owner = THIS_MODULE,
    .open = btnintp_open,
    .release = btnintp_release,
    .poll = btnintp_poll,
};

static int __init gpio_irq_init(void)
{
    int ret;

    printk(KERN_INFO "btnintp: Initializing the GPIO Interrupt driver...\n");

    // Allocate a device number
    ret = alloc_chrdev_region(&btnintp_devnum, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "btnintp: Failed to allocate device number\n");
        return ret;
    }

    // Create device class
    btnintp_class = class_create(DEVICE_NAME);
    if (IS_ERR(btnintp_class)) {
        ret = PTR_ERR(btnintp_class);
        goto fail_class_create;
    }

    // Create device node
    btnintp_device = device_create(btnintp_class, NULL, btnintp_devnum, 
                                  NULL, DEVICE_NAME);
    if (IS_ERR(btnintp_device)) {
        ret = PTR_ERR(btnintp_device);
        goto fail_device_create;
    }

    // Initialize and add the character device
    cdev_init(&btnintp_cdev, &btnintp_fops);
    btnintp_cdev.owner = THIS_MODULE;
    ret = cdev_add(&btnintp_cdev, btnintp_devnum, 1);
    if (ret < 0) {
        printk(KERN_ERR "btnintp: Failed to add character device\n");
        goto fail_cdev_add;
    }

    // GPIO setup
    if (!gpio_is_valid(GPIO_PIN)) {
        ret = -ENODEV;
        printk(KERN_ERR "btnintp: Invalid GPIO pin %d\n", GPIO_PIN);
        goto fail_gpio_valid;
    }

    ret = gpio_request(GPIO_PIN, "btn-gpio");
    if (ret) {
        printk(KERN_ERR "btnintp: Failed to request GPIO %d\n", GPIO_PIN);
        goto fail_gpio_valid;
    }

    ret = gpio_direction_input(GPIO_PIN);
    if (ret) {
        printk(KERN_ERR "btnintp: Failed to set GPIO direction\n");
        goto fail_gpio_direction;
    }

    // Setup interrupt
    irq_number = gpio_to_irq(GPIO_PIN);
    if (irq_number < 0) {
        ret = irq_number;
        printk(KERN_ERR "btnintp: Failed to map GPIO to IRQ\n");
        goto fail_gpio_direction;
    }

    ret = request_irq(irq_number, (irq_handler_t)gpio_irq_handler,
                     IRQF_TRIGGER_RISING, "gpio_irq_handler", NULL);
    if (ret) {
        printk(KERN_ERR "btnintp: Failed to request IRQ\n");
        goto fail_gpio_direction;
    }

    printk(KERN_INFO "btnintp: Successfully initialized with IRQ: %d\n", 
           irq_number);
    return 0;

// Error handling
fail_gpio_direction:
    gpio_free(GPIO_PIN);
fail_gpio_valid:
    cdev_del(&btnintp_cdev);
fail_cdev_add:
    device_destroy(btnintp_class, btnintp_devnum);
fail_device_create:
    class_destroy(btnintp_class);
fail_class_create:
    unregister_chrdev_region(btnintp_devnum, 1);
    return ret;
}

static void __exit gpio_irq_exit(void)
{
    free_irq(irq_number, NULL);
    gpio_free(GPIO_PIN);
    device_destroy(btnintp_class, btnintp_devnum);
    cdev_del(&btnintp_cdev);
    class_destroy(btnintp_class);
    unregister_chrdev_region(btnintp_devnum, 1);
    printk(KERN_INFO "btnintp: Successfully unloaded\n");
}

module_init(gpio_irq_init);
module_exit(gpio_irq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pravin Raghul S");
MODULE_DESCRIPTION("A Basic Interrupt Kernel Driver on Raspberry Pi 3B+");