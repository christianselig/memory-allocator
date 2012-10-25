/* File interface 
 * (c) Jack Lange, 2012
 */


#include <linux/fs.h>
#include <linux/namei.h>
#include <linux/version.h>
#include <linux/file.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/slab.h>



#define isprint(a) ((a >= ' ') && (a <= '~'))



int file_mkdir(const char * pathname, unsigned short perms, int recurse);

static int mkdir_recursive(const char * path, unsigned short perms) {
    char * tmp_str = NULL;
    char * dirname_ptr;
    char * tmp_iter;

    tmp_str = kmalloc(strlen(path) + 1, GFP_KERNEL);
    if (!tmp_str) { 
	printk(KERN_ERR "Cannot kmallocate in mkdir recursive\n");
	return -1;
    }

    memset(tmp_str, 0, strlen(path) + 1);
    strncpy(tmp_str, path, strlen(path));

    dirname_ptr = tmp_str;
    tmp_iter = tmp_str;

    // parse path string, call file_mkdir recursively.


    while (dirname_ptr != NULL) {
	int done = 0;

	while ((*tmp_iter != '/') && 
	       (*tmp_iter != '\0')) {

	    if ( (!isprint(*tmp_iter))) {
		printk(KERN_ERR "Invalid character in path name (%d)\n", *tmp_iter);
		kfree(tmp_str);
		return -1;
	    } else {
		tmp_iter++;
	    }
	}

	if (*tmp_iter == '/') {
	    *tmp_iter = '\0';
	} else {
	    done = 1;
	}

	// Ignore empty directories
	if ((tmp_iter - dirname_ptr) > 1) {
	    if (file_mkdir(tmp_str, perms, 0) != 0) {
		printk(KERN_ERR "Could not create directory (%s)\n", tmp_str);
		kfree(tmp_str);
		return -1;
	    }
	}

	if (done) {
	    break;
	} else {
	    *tmp_iter = '/';
	}
	
	tmp_iter++;

	dirname_ptr = tmp_iter;
    }
    
    kfree(tmp_str);

    return 0;
}

int file_mkdir(const char * pathname, unsigned short perms, int recurse) {
    /* Welcome to the jungle... */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,41)
    /* DO NOT REFERENCE THIS VARIABLE */
    /* It only exists to provide version compatibility */
    struct path tmp_path; 
#endif

    struct path * path_ptr = NULL;
    struct dentry * dentry;
    int ret = 0;



    if (recurse != 0) {
	return mkdir_recursive(pathname, perms);
    } 

    /* Before Linux 3.1 this was somewhat more difficult */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,41)
    {
	struct nameidata nd;

	// I'm not 100% sure about the version here, but it was around this time that the API changed
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,38) 
	ret = kern_path_parent(pathname, &nd);
#else 

	if (path_lookup(pathname, LOOKUP_DIRECTORY | LOOKUP_FOLLOW, &nd) == 0) {
	    return 0;
	}

	if (path_lookup(pathname, LOOKUP_PARENT | LOOKUP_FOLLOW, &nd) != 0) {
	    return -1;
	}
#endif

	if (ret != 0) {
	    printk(KERN_ERR "%s:%d - Error: kern_path_parent() returned error for (%s)\n", __FILE__, __LINE__, 
		   pathname);
	    return -1;
	}
	
	dentry = lookup_create(&nd, 1);
	path_ptr = &(nd.path);
    }
#else 
    {
	dentry = kern_path_create(AT_FDCWD, pathname, &tmp_path, 1);
	
	if (IS_ERR(dentry)) {
	    return 0;
	}
	
	path_ptr = &tmp_path;
    }
#endif    


    if (!IS_ERR(dentry)) {
	ret = vfs_mkdir(path_ptr->dentry->d_inode, dentry, perms);
    }

    mutex_unlock(&(path_ptr->dentry->d_inode->i_mutex));
    path_put(path_ptr);

    return ret;
}


struct file * file_open(const char * path, int mode) {
    struct file * file_ptr = NULL;	
    

    file_ptr = filp_open(path, mode | O_LARGEFILE, 0);
    
    if (IS_ERR(file_ptr)) {
	printk(KERN_ERR "Cannot open file: %s\n", path);
	return NULL;
    }

    return file_ptr;
}

int file_close(struct file * file_ptr) {

    filp_close(file_ptr, NULL);
    
    return 0;
}

unsigned long long file_size(struct file * file_ptr) {
    struct kstat s;
    int ret;
    
    ret = vfs_getattr(file_ptr->f_path.mnt, file_ptr->f_path.dentry, &s);

    if (ret != 0) {
	printk(KERN_ERR "Failed to fstat file\n");
	return -1;
    }

    return s.size;
}

unsigned long long file_read(struct file * file_ptr, void * buffer, unsigned long long length, unsigned long long offset){
    ssize_t ret;
    mm_segment_t old_fs;
	
    old_fs = get_fs();
    set_fs(get_ds());
	
    ret = vfs_read(file_ptr, buffer, length, &offset);
	
    set_fs(old_fs);
	
    if (ret <= 0) {
	printk(KERN_ERR "sys_read of %p for %lld bytes at offset %llu failed (ret=%ld)\n", file_ptr, length, offset, ret);
    }
	
    return ret;
}


unsigned long long file_write(struct file * file_ptr, void * buffer, unsigned long long length, unsigned long long offset) {
    mm_segment_t old_fs;
    ssize_t ret;

    old_fs = get_fs();
    set_fs(get_ds());

    ret = vfs_write(file_ptr, buffer, length, &offset);
	
    set_fs(old_fs);

 
    if (ret <= 0) {
	printk(KERN_ERR "sys_write for %llu bytes at offset %llu failed (ret=%ld)\n", length, offset, ret);
    }
	
    return ret;
}


