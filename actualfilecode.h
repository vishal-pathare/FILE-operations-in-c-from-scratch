#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<errno.h>


#define BUFF_SIZE 512

#define R 0			/*file is opened in r mode*/
#define W 1			/*file is opened in w mode*/
#define A 2			/*file is opened in a mode*/
#define RP 3			/*file is opened in r+ mode*/
#define WP 4			/*file is opened in w+ mode*/
#define AP 5			/*file is opened in a+ mode*/

/*Explanation for global list:
1. There will always be a global linked structure coexitsing with the header file.
2. The smallest subunit is fdnode which is mainly used to store the status of the file pointer and fd for identification.
3. Pathnode forms a linked structure, and stores the filename, where each node has a list called fdlist sprouting from it in which all nodes having fd's corresponding to that filename are stored.
4. Status flags are all set to 1 except the flag corresponding to the fd calling it to update their respective buffers after fseek is called.
5. Respective status flags are set to 0 after the buffer has been updated, and same thing is done when the file is opened in w or w+ mode by another fd.
*/
typedef struct fdnode {
	int fd;
	struct fdnode *nextnode;
	int status;
} fdnode;

typedef struct pathnode {
	char filename[100];
	struct pathnode *next;
	struct fdnode *fdlist;
} pathnode;

pathnode *head = NULL;


/*Explanation for buf data type :
 * pagecount stores the number of buf sized jumps required to be taken from start of file to file bytes currently stored in buf
 * buf actually stores the file bytes which are to be operated on */

typedef struct buf {
	int pageno;
	char buf[BUFF_SIZE];
} buf;

/* Explanation for FILES data type :
 * fd stores file descriptor for file as returned by open() function
 * flag indicated the mode of opening of file : r, w, a, r+, w+, a+ are respectively indicated by values R, W, A, RP, WP, AP
 * length stores the length of the file at any point of time in the program
 * readpointer indicates the position of next readable byte in the buf
 * writepointer indicates the position of the next writable byte
 * endfile flag indicates whether feof flag is set or not, ie. whether location beyond file length is accessed
 * pointer stores the location of the status node in global structure array, which is used to access and change the status variable in the list.
 * pathname stores the pathname of the file
*/
typedef struct FILES {
	int fd, flag, endflag, writeflag;
	unsigned long int length;
	char *readpointer, *writepointer, pathname[100];
	buf currbuf;
	fdnode *pointer;
}FILES;

typedef struct fPos_t {
	unsigned long int position;
	char key[29];
} fPos_t;

int ifeof(FILES *fp);

void finit(FILES *, char *);

/*addfd adds a new node in the global list when a new fp is initialized. A node might be
 added to the fdlist corresponding to the already existing pathname node or a new pathname
  node may be added and corresponding fd added to it, ie. fdlist*/
void addfd(FILES *fp) {
	char pathname[100];
	strcpy(pathname, fp->pathname);
	pathnode *path = (pathnode *)malloc(sizeof(pathnode));
	fdnode *node = (fdnode *)malloc(sizeof(fdnode));
	if(path == NULL || node == NULL){
		errno = ENOMEM;
		return;
	}
	node->fd = fp->fd;
	node->status = 0;
	fp->pointer = node;
	strcpy(path->filename, pathname);

	if(head == NULL) {
		path->next = NULL;
		node->nextnode = NULL;
		path->fdlist = node;
		head = path;
		return;
	}
	pathnode *temp1 = NULL;
	pathnode *temp2 = head;
	while((temp2 != NULL) && (strcmp(temp2->filename, pathname) != 0)) {
		temp1 = temp2;
		temp2 = temp2->next;
	}

	if(temp2 == NULL) {
		temp1->next = path;
		path->next = NULL;
		node->nextnode = NULL;
		path->fdlist = node;
		return;
	}
	else {
		fdnode *point = temp2->fdlist;
		temp2->fdlist = node;
		node->nextnode = point;
	}
}


/*removenode is called by fclose and removes the created node corresponding to the fd.*/
void removenode(FILES *fp) {
	pathnode *path = head;
	pathnode *pathprev = NULL;
	fdnode *node;
	fdnode *nodeprev = NULL;
	while(strcmp(path->filename, fp->pathname) != 0) {
		pathprev = path;       	
		path = path->next;
	}
	node = path->fdlist;
	while(node->fd != fp->fd) {
		nodeprev = node;
		node = node->nextnode;
	}
	if(nodeprev == NULL) 
		path->fdlist = node->nextnode;
	else
		nodeprev->nextnode = node->nextnode;
	free(node);
	if(path->fdlist == NULL) {
		if(pathprev == NULL) 
			head = path->next;
		else
			pathprev->next = path->next;
		free(path);
	}
}

/*setstatus sets the flags of all the fd's corresponding to their pathname to 1 except the fp calling it*/
void setstatus(FILES *fp) {
	pathnode *path = head;
	fdnode *node;
	while(strcmp(path->filename, fp->pathname)) 
		path = path->next;
	node = path->fdlist;
	while(node != NULL) {
		if(node->fd != fp->fd) 
			node->status = 1;
		node = node->nextnode;
	}
}	

/*ifopen opens the file and sets the fd value, and open works on the assumption that umask value is 0022*/					
FILES* ifopen(char *pathname, char *mode) {
	struct stat st;
	FILES *ret;
	ret = malloc(sizeof(FILES));
	if(ret == NULL) {
		errno = ENOMEM;
		return ret;
	}
	if(strcmp(mode, "r") == 0) {
		ret->fd = open(pathname, O_RDWR);
		stat(pathname, &st);
		ret->length = st.st_size;
		ret->flag = R;
		finit(ret, pathname);
	}
	else if(strcmp(mode, "w") == 0) {
		ret->fd = open(pathname, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		stat(pathname, &st);
		ret->length = st.st_size;
		ret->flag = W;
		finit(ret, pathname);
		setstatus(ret);
	}
	else if(strcmp(mode, "a") == 0) {
		ret->fd = open(pathname, O_RDWR|O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		stat(pathname, &st);
		ret->length = st.st_size;
		ret->flag = A;
		finit(ret, pathname);
	}
	else if(strcmp(mode, "r+") == 0 || strcmp(mode, "rb+") == 0 || strcmp(mode, "r+b") == 0){
		ret->fd = open(pathname, O_RDWR);
		stat(pathname, &st);
		ret->length = st.st_size;
		ret->flag = RP;
		finit(ret, pathname);
	}

	else if(strcmp(mode, "w+") == 0 || strcmp(mode, "wb+") == 0 || strcmp(mode, "w+b") == 0) {
		ret->fd = open(pathname, O_RDWR|O_TRUNC|O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		stat(pathname, &st);
		ret->length = st.st_size;
		ret->flag = WP;
		finit(ret, pathname);
		setstatus(ret);
	}
	else if(strcmp(mode, "a+") == 0 || strcmp(mode, "ab+") == 0 || strcmp(mode, "a+b")) {
		ret->fd = open(pathname, O_RDWR|O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		stat(pathname, &st);
		ret->length = st.st_size;
		ret->flag = AP;
		finit(ret, pathname);
	}
	else {
		errno = EINVAL;
	}
	return ret;	
}

/*finit sets the corresponding default valus to the flags and fills the buffer with data from the file according to the mode of opening*/
void finit(FILES *fp, char *pathname) {
	if(fp->flag != A && fp->flag != AP) {
		read(fp->fd, fp->currbuf.buf, BUFF_SIZE);
		fp->currbuf.pageno = 0;
		fp->readpointer = fp->currbuf.buf;
		fp->writepointer = fp->readpointer;
	}
	else {
		lseek(fp->fd, (fp->length / 512) * 512, SEEK_SET);
		read(fp->fd, fp->currbuf.buf, BUFF_SIZE);
		fp->currbuf.pageno = fp->length / 512;
		fp->readpointer = fp->writepointer = fp->currbuf.buf + fp->length % 512;
	}
	strcpy(fp->pathname, pathname);
	addfd(fp);
	fp->endflag = 0;
	fp->writeflag = 0;
}

/*read the byte as pointed as the readpointer and increment the readpointer after every succesful read. Return the number of successfully read records after encountering
 * an unsuccessful read. If current buf is exhausted then write the buf into the file and load the next BUFF_SIZE bytes into the buf, while incrementing the pageno.
 * In case of non-append modes, increment the writepointer along with the readpointer.
 * If the current position of the readpointer is beyond the file length then read is unsuccessful.
 * */ 
int ifread(void *destination1, int record_size, int no_of_records, FILES *fp) {
        unsigned long int finalbytenumber, currentbytenumber, readdifference, count;
	char *destination = (char *) destination1;
	if((fp->flag == W) || (fp->flag == A)) {
		errno = EBADF;
		return 0;
	}
	
	if(record_size <= 0)
		return 0;
		
	if(no_of_records <= 0)
		return 0;
	
	//if status is set then fill buffer	
	if(fp->pointer->status == 1) {
		lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
		read(fp->fd, fp->currbuf.buf, BUFF_SIZE);
		fp->pointer->status = 0;
		struct stat st;
		stat(fp->pathname, &st);
		fp->length = st.st_size;
	}
	
	
	currentbytenumber = fp->currbuf.pageno * BUFF_SIZE + (fp->readpointer - fp->currbuf.buf);
	
	finalbytenumber = record_size * no_of_records + currentbytenumber;
	
	if(currentbytenumber >= fp->length) {
		fp->endflag = 1;
		return 0;
	}
		
	else if((fp->currbuf.pageno * BUFF_SIZE + (fp->writepointer - fp->currbuf.buf) > fp->length) && fp->flag != A && fp->flag != AP) {
		fp->endflag = 1;
		return 0;
	}
	
	
	//Check whether read size requested exceeds the length of file

	if(finalbytenumber > fp->length)
		finalbytenumber = fp->length;
	

	//Set final number of bytes to be read
	readdifference = finalbytenumber - currentbytenumber;
	
	count = 0;
	if(fp->flag != AP)
		fp->writepointer = fp->readpointer;
	while(count != readdifference) {
		destination[count] = *fp->readpointer;

		//If read pointer is in currbuf and buf is full
		if(fp->readpointer - fp->currbuf.buf == BUFF_SIZE - 1) {
			lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
			write(fp->fd, fp->currbuf.buf, BUFF_SIZE);
			lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE + BUFF_SIZE, SEEK_SET);
			read(fp->fd, fp->currbuf.buf, BUFF_SIZE);
			fp->currbuf.pageno++;
			if(fp->flag == R || fp->flag == RP || fp->flag == WP)
				fp->writepointer = fp->currbuf.buf;
			fp->readpointer = fp->currbuf.buf;
		}

		//If buf is not full
		else {
			fp->readpointer++;
			if(fp->flag == RP || fp->flag == WP || fp->flag == R)
				fp->writepointer++;
		}
		count++;	
	}
		
	
	return readdifference / record_size;	//Number of records read
}

/*Write byte by byte from source to byte indicated by writepointer in the current buf.
 * In case the write pointer is beyond the file length then fill the bytes from file length to writepointer with '\0'.This is done with the help of readpointer.
 * In case of non-append modes move the readpointer along with the writepointer.
 * In case of append mode if the position pointed by the writepointer is not end of file then write the current buf into the file and load the buf containing the file length in to the buf and set the writepointer to the end of file and then start writing into the buf.
 * If the current buf is exhausted then write the current buf back into the file and then load the next BUFF_SIZE bytes into the current buf.
 * Return the number of records written.
 */
int ifwrite(void *source1, int record_size, int no_of_records, FILES *fp) {
	unsigned long int currentbytenumber = 0, writedifference = 0, finalbytenumber = 0, count;
	char *source = (char *) source1;
	
	if(fp->flag == R) {
		errno = EBADF;
		return 0;
	}

	if(fp->pointer->status == 1) {
		lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
		read(fp->fd, fp->currbuf.buf, BUFF_SIZE);
		fp->pointer->status = 0;
		struct stat st;
		stat(fp->pathname, &st);
		fp->length = st.st_size;
	}


	if(record_size <= 0 || no_of_records <= 0)
		return 0;

	
	if(fp->flag == W || fp->flag == RP || fp->flag == WP) {
		currentbytenumber = fp->currbuf.pageno * BUFF_SIZE + (fp->writepointer - fp->currbuf.buf);
		finalbytenumber = record_size * no_of_records + currentbytenumber;
		if((currentbytenumber > fp->length)) {
			count = 0;
			writedifference = currentbytenumber - fp->length;
			
			//if write location is not present in current buffer
			if(fp->currbuf.pageno > fp->length / BUFF_SIZE) {
				lseek(fp->fd, (fp->length / BUFF_SIZE) * BUFF_SIZE, SEEK_SET);
				read(fp->fd, fp->currbuf.buf, BUFF_SIZE);
				fp->currbuf.pageno = fp->length / BUFF_SIZE;
				fp->readpointer = fp->currbuf.buf + fp->length % BUFF_SIZE;
			}
			while(count != writedifference) {
				*fp->readpointer = '\0';
				//currbuf is exhausted
				if((fp->readpointer - fp->currbuf.buf) == BUFF_SIZE - 1) {
					lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
					write(fp->fd, fp->currbuf.buf, BUFF_SIZE);
					fp->currbuf.pageno++;
					fp->readpointer = fp->currbuf.buf;
				}

				//currbuf not yet exhauseted
				else {
					fp->readpointer++;
				}
				fp->length++;
				count++;
			}
		}
		fp->writepointer = fp->readpointer;
	      	count = 0;
	      	writedifference = finalbytenumber - currentbytenumber;
       		while(count != writedifference) {
	       		*(fp->writepointer) = *(source + count);	//byte by byte reading from source memory location
			if(fp->writepointer - fp->currbuf.buf == BUFF_SIZE - 1) {
				lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
				write(fp->fd, fp->currbuf.buf, BUFF_SIZE);
				fp->currbuf.pageno++;
				lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, BUFF_SIZE);
				read(fp->fd, fp->currbuf.buf, BUFF_SIZE);
				fp->writepointer = fp->currbuf.buf;
				fp->readpointer = fp->currbuf.buf;
			}
			
			//If currbuf is not exhausted
	       		else {
		        	fp->writepointer++;
			       	fp->readpointer++;
			}
	       		count++;
			if(currentbytenumber >= fp->length) {
				fp->length++;
			}
			currentbytenumber++;
       		}
	}

	//If file is opened in either of append modes
	//Write pointer is always in alternatebuf
	if(fp->flag == A || fp->flag == AP) {
		finalbytenumber = fp->length + record_size * no_of_records;
		count = 0;
		//Number of bytes to be written
		writedifference = finalbytenumber - fp->length;
		
		//If write location is not present in current buffer, ie. this is not the last buffer
		if(fp->currbuf.pageno != fp->length / BUFF_SIZE) {
			lseek(fp->fd, (fp->length / BUFF_SIZE) * BUFF_SIZE, SEEK_SET);
			read(fp->fd, fp->currbuf.buf, BUFF_SIZE);
			fp->writepointer = fp->currbuf.buf + fp->length % BUFF_SIZE;
			fp->currbuf.pageno = fp->length / BUFF_SIZE;
		}
		
		while(count < writedifference) {
			fp->writepointer = fp->currbuf.buf + fp->length % BUFF_SIZE;
			*fp->writepointer = source[count];	//Writing to buf from source memory location

			//If alternate buf is exhausted
			if((fp->writepointer - fp->currbuf.buf) == BUFF_SIZE - 1) {

			       	//Replace required set of bytes in file by alternatebuf
			       	lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
			       	write(fp->fd, fp->currbuf.buf, BUFF_SIZE);
	
				//Alternatebuf is now empty and anything written in it is going to be directly appended to file end
				fp->currbuf.pageno++;
				fp->writepointer = fp->currbuf.buf;
			}
			else
				fp->writepointer++;
			fp->length++;	//Every write in append mode increases file length
		
			count++;
		}
		fp->readpointer = fp->writepointer;
	}
	fp->writeflag = 1;
	return writedifference / record_size;
}

/*ifgetpos stores the current location of readpointer in the file or writepointer in case of non-append mode and it being ahead of file length
 * in to the memory location passed as argument. It also stores a key which can only be recognized by ifsetpos.
*/
int ifgetpos(FILES *fp, fPos_t *pos) {
	if(fp == NULL) {
		errno = EBADF;
		return -1;
	}
	if((fp->flag != A) && (fp->flag != AP) && ((fp->currbuf.pageno * BUFF_SIZE + (fp->writepointer - fp->currbuf.buf)) > fp->length))
		pos->position = fp->currbuf.pageno * BUFF_SIZE + (fp->writepointer - fp->currbuf.buf);
	else
		pos->position = fp->currbuf.pageno * BUFF_SIZE + (fp->readpointer - fp->currbuf.buf);
	strcpy(pos->key, "DSA is my favourite subject!");
	return 0;		 
}

/*ifsetpos sets the readpointer to the position which is passed as argument. 
 * In case of non-append modes readpointer and writepointer are both set to the position and old buf rewritten and new buf loaded if needed, if position is within file length
 *In case of non-append modes and position beyond the file length the readpointer is set to end of file and writepointer is set to desired location by setting in empty      buf and just setting the page number without loading new buf.
 In case of append modes if position is beyond file length then readpointer is set to that location.
 If position is within file length then current buf is written and buf containing desired location is loaded and readpointer is set to that location.
 */
int ifsetpos(FILES *fp, fPos_t *pos) {
	unsigned long int destination;
	destination = pos->position;
	
	if(fp == NULL) {
		errno = EBADF;
		return -1;
	}
	
	fp->endflag = 0;
	
	if(fp->flag != A && fp->flag != AP) {
		if((fp->currbuf.pageno * BUFF_SIZE <= destination) && ((fp->currbuf.pageno + 1) * BUFF_SIZE > destination)) {
			if(destination > fp->length)
				fp->readpointer = fp->currbuf.buf + fp->length;
			else
				fp->readpointer =  fp->currbuf.buf + destination % BUFF_SIZE;
			fp->writepointer = fp->currbuf.buf + destination % BUFF_SIZE;
		}
		else {
			if(destination <= (fp->length)) {
				lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
				if((fp->length / BUFF_SIZE) == fp->currbuf.pageno)
					write(fp->fd, fp->currbuf.buf, fp->length % BUFF_SIZE);
				else
					write(fp->fd, fp->currbuf.buf, BUFF_SIZE);
				lseek(fp->fd, (destination / BUFF_SIZE) * BUFF_SIZE, SEEK_SET);
				read(fp->fd, fp->currbuf.buf, BUFF_SIZE);
				fp->currbuf.pageno = destination / BUFF_SIZE;
				fp->writepointer = fp->currbuf.buf + destination % BUFF_SIZE;
				fp->readpointer = fp->currbuf.buf + destination % BUFF_SIZE;
			}
			else {
				lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
				if(fp->currbuf.pageno <= fp->length / BUFF_SIZE) {
					if(fp->currbuf.pageno == fp->length / BUFF_SIZE) 
						write(fp->fd, fp->currbuf.buf, fp->length % BUFF_SIZE);
					else
						write(fp->fd, fp->currbuf.buf, BUFF_SIZE);
				}
				fp->currbuf.pageno = destination / BUFF_SIZE;
				fp->writepointer = fp->currbuf.buf + destination % BUFF_SIZE;
			}
		}
	}
	else {
		if((fp->currbuf.pageno * BUFF_SIZE <= destination) && (destination < (fp->currbuf.pageno * BUFF_SIZE + BUFF_SIZE))) {
				fp->readpointer = fp->currbuf.buf + destination % BUFF_SIZE;
		}
		else if(destination < fp->length) {
			lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
			if(fp->length / BUFF_SIZE == fp->currbuf.pageno)
				write(fp->fd, fp->currbuf.buf, fp->length % BUFF_SIZE);
			else 
				write(fp->fd, fp->currbuf.buf, BUFF_SIZE);
			lseek(fp->fd, (destination / BUFF_SIZE) * BUFF_SIZE, SEEK_SET);
			read(fp->fd, fp->currbuf.buf, BUFF_SIZE);
			fp->currbuf.pageno = destination / BUFF_SIZE;
			fp->readpointer = fp->currbuf.buf + destination % BUFF_SIZE;
		}
		else {
			lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
			if(fp->length / BUFF_SIZE == fp->currbuf.pageno)
				write(fp->fd, fp->currbuf.buf, fp->length % BUFF_SIZE);
			else
				write(fp->fd, fp->currbuf.buf, BUFF_SIZE);
			fp->currbuf.pageno = destination / BUFF_SIZE;
			fp->readpointer = fp->currbuf.buf + destination % BUFF_SIZE;
		}
	}
	if(fp->flag != R && fp->writeflag == 1) {
		lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
		if(fp->currbuf.pageno == fp->length / BUFF_SIZE) {
			write(fp->fd, fp->currbuf.buf, fp->length % BUFF_SIZE);
		}
		else {
			write(fp->fd, fp->currbuf.buf, BUFF_SIZE);
		}
		setstatus(fp);
	}
	return 0;
}



/*Similar to fgetpos*/
unsigned long int iftell(FILES *fp) {
	if(fp == NULL) {
		errno = EBADF;
		return -1;
	}
	if((fp->flag != A) && (fp->flag != AP) && ((fp->currbuf.pageno * BUFF_SIZE + (fp->writepointer - fp->currbuf.buf)) > fp->length))
		return (unsigned long int) fp->currbuf.pageno * BUFF_SIZE + (fp->writepointer - fp->currbuf.buf);
	else
		return (unsigned long int) fp->currbuf.pageno * BUFF_SIZE + (fp->readpointer - fp->currbuf.buf);
}

/*Similar to fsetpos*/
int ifseek(FILES *fp, long int offset, int whence) {
	unsigned long int destination, currentbytenumber;
	
	if(fp->currbuf.pageno * BUFF_SIZE + (fp->writepointer - fp->currbuf.buf) > fp->length)
		currentbytenumber = fp->currbuf.pageno * BUFF_SIZE + (fp->writepointer - fp->currbuf.buf);
	else
		currentbytenumber = fp->currbuf.pageno * BUFF_SIZE + (fp->readpointer - fp->currbuf.buf);
	
	if(fp == NULL) {
		errno = EBADF;
		return -1;
	}
		
	if(whence == SEEK_CUR) {
		if(currentbytenumber + offset < 0)
			return -1;
		else
			destination = currentbytenumber + offset;
	}

	else if(whence == SEEK_SET) {
		if(offset < 0)
			return -1;
		else
			destination = offset;
	}
		
	else if(whence == SEEK_END) {
		if(fp->length + offset < 0)
			return -1;
		else
			destination = fp->length + offset;
	}
	else {
		errno = EINVAL;
		return -1;
	}
		
	fp->endflag = 0;
	
	if(fp->flag != A && fp->flag != AP) {
		if((fp->currbuf.pageno * BUFF_SIZE <= destination) && ((fp->currbuf.pageno + 1) * BUFF_SIZE > destination)) {
			if(destination > fp->length)
				fp->readpointer = fp->currbuf.buf + fp->length;
			else
				fp->readpointer =  fp->currbuf.buf + destination % BUFF_SIZE;
			fp->writepointer = fp->currbuf.buf + destination % BUFF_SIZE;
		}
		else {
			if(destination <= (fp->length)) {
				lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
				if((fp->length / BUFF_SIZE) == fp->currbuf.pageno)
					write(fp->fd, fp->currbuf.buf, fp->length % BUFF_SIZE);
				else
					write(fp->fd, fp->currbuf.buf, BUFF_SIZE);
				lseek(fp->fd, (destination / BUFF_SIZE) * BUFF_SIZE, SEEK_SET);
				read(fp->fd, fp->currbuf.buf, BUFF_SIZE);
				fp->currbuf.pageno = destination / BUFF_SIZE;
				fp->writepointer = fp->currbuf.buf + destination % BUFF_SIZE;
				fp->readpointer = fp->currbuf.buf + destination % BUFF_SIZE;
			}
			else {
				lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
				if(fp->currbuf.pageno <= fp->length / BUFF_SIZE) {
					if(fp->currbuf.pageno == fp->length / BUFF_SIZE) 
						write(fp->fd, fp->currbuf.buf, fp->length % BUFF_SIZE);
					else
						write(fp->fd, fp->currbuf.buf, BUFF_SIZE);
				}
				fp->currbuf.pageno = destination / BUFF_SIZE;
				fp->writepointer = fp->currbuf.buf + destination % BUFF_SIZE;
			}
		}
	}
	else {
		if((fp->currbuf.pageno * BUFF_SIZE <= destination) && (destination < (fp->currbuf.pageno * BUFF_SIZE + BUFF_SIZE))) {
				fp->readpointer = fp->currbuf.buf + destination % BUFF_SIZE;
		}
		else if(destination < fp->length) {
			lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
			if(fp->length / BUFF_SIZE == fp->currbuf.pageno)
				write(fp->fd, fp->currbuf.buf, fp->length % BUFF_SIZE);
			else 
				write(fp->fd, fp->currbuf.buf, BUFF_SIZE);
			lseek(fp->fd, (destination / BUFF_SIZE) * BUFF_SIZE, SEEK_SET);
			read(fp->fd, fp->currbuf.buf, BUFF_SIZE);
			fp->currbuf.pageno = destination / BUFF_SIZE;
			fp->readpointer = fp->currbuf.buf + destination % BUFF_SIZE;
		}
		else {
			lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
			if(fp->length / BUFF_SIZE == fp->currbuf.pageno)
				write(fp->fd, fp->currbuf.buf, fp->length % BUFF_SIZE);
			else
				write(fp->fd, fp->currbuf.buf, BUFF_SIZE);
			fp->currbuf.pageno = destination / BUFF_SIZE;
			fp->readpointer = fp->currbuf.buf + destination % BUFF_SIZE;
		}
	}
	if(fp->flag != R && fp->writeflag == 1) {
		lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
		if(fp->currbuf.pageno == fp->length / BUFF_SIZE) {
			write(fp->fd, fp->currbuf.buf, fp->length % BUFF_SIZE);
		}
		else {
			write(fp->fd, fp->currbuf.buf, BUFF_SIZE);
		}
		setstatus(fp);
	}
	return 0;
}

int ifeof(FILES *fp) {
	return fp->endflag;
}

void ifclose(FILES *fp) {
	if(fp->writeflag == 1) {
		lseek(fp->fd, fp->currbuf.pageno * BUFF_SIZE, SEEK_SET);
		if(fp->currbuf.pageno == fp->length / BUFF_SIZE) {
			write(fp->fd, fp->currbuf.buf, fp->length % BUFF_SIZE);
		}
		else {
			write(fp->fd, fp->currbuf.buf, BUFF_SIZE);
		}
	}
	setstatus(fp);
	removenode(fp);
	close(fp->fd);
	fp = NULL;
}	
