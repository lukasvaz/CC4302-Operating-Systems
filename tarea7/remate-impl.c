/* Necessary includes for device drivers */
#include <linux/init.h>
/* #include <linux/config.h> */
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/uaccess.h> /* copy_from/to_user */

#include "kmutex.h"

MODULE_LICENSE("Dual BSD/GPL");

/* Declaration of remate.c functions */
int remate_open(struct inode *inode, struct file *filp);
int remate_release(struct inode *inode, struct file *filp);
ssize_t remate_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t remate_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void remate_exit(void);
int remate_init(void);

/* Structure that declares the usual file */
/* access functions */
struct file_operations remate_fops = {
  read: remate_read,
  write: remate_write,
  open: remate_open,
  release: remate_release
};

/* Declaration of the init and exit functions */
module_init(remate_init);
module_exit(remate_exit);

/*** El driver para lecturas sincronas *************************************/

#define TRUE 1
#define FALSE 0

/* Global variables of the driver */

int remate_major = 61;     /* Major number */



/* Buffer to store data */
#define MAX_SIZE 8192

static char *remate_buffer;
static ssize_t curr_size;

static size_t max_offer;

static int readers_waiting;
static int writing;
static int reading;

/* El mutex y la condicion para remate */
static KMutex mutex;
static KCondition cond;

int remate_init(void) {
  /* Registering device */
  int rc = register_chrdev(remate_major, "remate", &remate_fops);
  if (rc < 0) {
    printk(
      "<1>remate: cannot obtain major number %d\n", remate_major);
    return rc;
  }
  readers_waiting=0;
  curr_size= 0;
  max_offer=0;
  m_init(&mutex);
  c_init(&cond);

  /* Allocating vigia_buffer */
  remate_buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
  if (remate_buffer==NULL) {
    remate_exit();
    return -ENOMEM;
  }

  printk("<1>Inserting remate module\n");
  return 0;
}

void remate_exit(void) {
  /* Freeing the major number */
  unregister_chrdev(remate_major, "remate");

  /* Freeing buffer remate */
  if (remate_buffer) {
    kfree(remate_buffer);
  }

  printk("<1>Removing remate module\n");
  
}

int remate_open(struct inode *inode, struct file *filp) {
  int rc= 0;
  m_lock(&mutex);
  //si se lee  y no  hay ofertas espera
  if (filp->f_mode & FMODE_READ){
    readers_waiting++;
    while(max_offer==0){
       if (c_wait(&cond, &mutex)) {
        readers_waiting--;
        reading=FALSE;
        rc= -EINTR;
        goto epilog;
      }
    }
    reading=TRUE;
    printk("open in read mode  reading:%d",reading);
  }
  else if  (filp->f_mode & FMODE_WRITE){
    writing=TRUE;
  }
 epilog:
  m_unlock(&mutex);
  return rc;
}

int remate_release(struct inode *inode, struct file *filp) {
  m_lock(&mutex);
  
  if (filp->f_mode & FMODE_WRITE) {
    printk("<1>release for writing\n");
  }

  if (filp->f_mode & FMODE_READ) {
    printk("<1>release read , pendient %d\n", readers_waiting);
    max_offer=0;

  }
  m_unlock(&mutex);
  return 0;

}

ssize_t remate_read(struct file *filp, char *buf,
                    size_t count, loff_t *f_pos) {
  ssize_t rc;
  m_lock(&mutex);
  // si no se ha ofertado nada espera
  while (max_offer==0) {
      printk("<1>max offer =0  entro al while\n");
    if (c_wait(&cond, &mutex)) {
      printk("<1>read interrupted\n");
      rc= -EINTR;
      goto epilog;
    }
  }
  //hay oferta maxima leo y retorno
  if (count > curr_size-*f_pos) {
    count= curr_size-*f_pos;
  }
  printk("<1>read %d bytes at %d\n", (int)count, (int)*f_pos);

  /* Transfiriendo datos hacia el espacio del usuario */
  if (copy_to_user(buf, remate_buffer+*f_pos, count)!=0) {
    /* el valor de buf es una direccion invalida */
    rc= -EFAULT;
    goto epilog;
  }
  *f_pos+= count;
  rc= count;
  //activo a max offer
  c_broadcast(&cond);

  epilog:
    printk("<1>return in read\n");
    m_unlock(&mutex);
    return rc;
}

ssize_t remate_write( struct file *filp, const char *buf,
                      size_t count, loff_t *f_pos) {
  ssize_t rc;
  loff_t last;
  m_lock(&mutex);
  printk("<1>count %d",(int)count);
  last= *f_pos + count;
 //si es oferta maxima
 if (count>max_offer){
    max_offer=count;
    printk("<1>nueva oferta maxima %d",(int)max_offer);
    if (last>MAX_SIZE) {
      count -= last-MAX_SIZE;
    }
    printk("<1>write %d bytes at %d\n", (int)count, (int)*f_pos);
    if (copy_from_user(remate_buffer+*f_pos, buf, count)!=0) {
    /* el valor de buf es una direccion invalida */
    return -EFAULT;
    }
    *f_pos += count;
    curr_size= *f_pos;
    rc= count;
    c_broadcast(&cond);
    printk("activo antiguo max\n");
 }
 else{
  printk("no es oferta max " );
  goto epilog;
 }
  //si es  max_offer espera
 while(max_offer== count && !reading){
  printk("es max_offer, entro al while reading %d\n", reading);
  if (c_wait(&cond, &mutex)) {
      printk("<1>write interrupted por teclado\n");
      *f_pos -= count;
      curr_size= *f_pos;
      rc= -EINTR;
      max_offer=0;
      goto epilog;
    }
 }
  //cuando es despertado  si es max_offer 
 if(max_offer== count){
    printk("offer =0  reading false in write" );
    reading=FALSE;
    readers_waiting--;
    m_unlock(&mutex);
    return count;
 }
  epilog:
  printk("epilog:decreasing f pos in write %d",(int)count);
  m_unlock(&mutex);
  return -ECANCELED;
}

