/* File Interface 
 *  (c) Jack Lange, 2012
 */

#ifndef __FILE_IO_H__
#define __FILE_IO_H__

int file_mkdir(const char * pathname, unsigned short perms, int recurse);


/* MODE: 
   O_RDWR
   O_RDONLY
   O_WRONLY
   O_CREAT
   
*/
struct file * file_open(const char * path, int mode);
int file_close(struct file * file_ptr);

unsigned long long file_size(struct file * file_ptr);


unsigned long long file_read(struct file * file_ptr, void * buffer, 
			     unsigned long long length, 
			     unsigned long long offset);

unsigned long long file_write(struct file * file_ptr, void * buffer, 
			      unsigned long long length, 
			      unsigned long long offset);

#endif
