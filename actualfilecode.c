#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>

#define CURR 1
#define ALT 2

typedef struct buffer {
	int pagenumber;
	char buffer[512];
} buffer;

/*Explanation for buffer data type :
 * pagecount stores the number of buffer sized jumps required to be taken from start of file to file bytes currently stored in buffer
 * buffer actually stores the file bytes which are to be operated on	*/

typedef struct FILES {
	int fd, flag, length;
	char *readpointer, *writepointer;
	buffer currentbuffer, alternatebuffer;	
}FILES;

typedef struct fPos_t {
	int position;
	char key[29];
} fPos_t;


int ifeof(FILES *fp);

/* Explanation for FILES data type :
 * fd stores file descriptor for file as returned by open() function
 * flag indicated the mode of opening of file : r, w, a, r+, w+, a+ are respectively indicated by values 0, 1, 2, 3, 4, 5
 * length stores the length of the file at any point of time in the program
 * readpointer indicates the position of next readable byte in the buffer (current or alternate)
 * writepointer indicates the next writable position in the alternatebuffer
 * originalbuffer contains readpointer and writepointer in non-append mode of operation and only readpointer in some cases of append mode
 * writebuffer contains writepointer in append mode and readpointer in some cases of append mode	*/


void finit(FILES *, int);

FILES* ifopen(char *pathname, char *mode) {
	int fd, size;
	struct stat st;
	FILES *ret;
	ret = malloc(sizeof(FILES));
	if(!strcmp(mode, "r")) {
		ret->fd = open(pathname, O_RDONLY);
		stat(pathname, &st);
		ret->length = st.st_size;
		finit(ret, 0);
	}
	else if(!strcmp(mode, "w")) {
		ret->fd = open(pathname, O_WRONLY|O_CREAT|O_TRUNC, S_IWUSR);
		stat(pathname, &st);
		ret->length = st.st_size;
		finit(ret, 1);
	}
	else if(!strcmp(mode, "a")) {
		ret->fd = open(pathname, O_WRONLY|O_APPEND|O_CREAT, S_IWUSR);
		stat(pathname, &st);
		ret->length = st.st_size;
		finit(ret, 2);
	}
	else if(strcmp(mode, "r+") == 0) {
		ret->fd = open(pathname, O_RDWR);
		stat(pathname, &st);
		ret->length = st.st_size;
		finit(ret, 3);
	}

	else if(!strcmp(mode, "w+")) {
		ret->fd = open(pathname, O_WRONLY|O_TRUNC|O_CREAT, S_IRUSR|S_IWUSR);
		stat(pathname, &st);
		ret->length = st.st_size;
		finit(ret, 4);
	}
	else if(!strcmp(mode, "a+")) {
		ret->fd = open(pathname, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
		stat(pathname, &st);
		ret->length = st.st_size;
		finit(ret, 5);
	}
	return ret;	
}	

/*void finit(FILES *fp, int mode) {
	switch(mode) {
		case 0:	fp->writepointer = NULL;
			read(fp->fd, fp->currentbuffer.buffer, 512);
			fp->currentbuffer.pagenumber = 0;
			fp->readpointer = fp->currentbuffer.buffer;
			fp->flag = 0;
			break;

		case 1:	fp->readpointer = NULL;
			fp->currentbuffer.pagenumber = 0;
			fp->writepointer = fp->currentbuffer.buffer;
			fp->flag = 1;
			break;

		case 2: fp->readpointer = NULL;
			lseek(fp->fd, (fp->length / 512) * 512, SEEK_SET);
			read(fp->fd, fp->alternatebuffer.buffer, 512);
			lseek(fp->fd, 0, SEEK_SET);
			fp->writepointer = fp->alternatebuffer.buffer + fp->length % 512;
			fp->alternatebuffer.pagenumber = fp->length / 512;
			fp->flag = 2;
			break;

		case 3:	read(fp->fd, fp->currentbuffer.buffer, 512);
			fp->currentbuffer.pagenumber = 0;
			fp->readpointer = fp->currentbuffer.buffer;
			fp->flag = 5;
	}
}*/
void finit(FILES *fp, int mode) {
	read(fp->fd, fp->currentbuffer.buffer, 512);
	fp->currentbuffer.pagenumber = 0;
	fp->readpointer = fp->currentbuffer.buffer;
	fp->writepointer = fp->readpointer;
	fp->flag = mode;
}

int ifread(void *destination1, int record_size, int no_of_records, FILES *fp) {
	int readdifference, count, finalbytenumber, currentbytenumber, earliercount;
	char *destination = (char *) destination1;
	if(ifeof(fp))
		return 0; 
	//if((fp->currentbuffer.pagenumber * 512 + (fp->readpointer - fp->currentbuffer.buffer)) >= fp->length)
	//	return 0;
	
	currentbytenumber = fp->currentbuffer.pagenumber * 512 + (fp->readpointer - fp->currentbuffer.buffer);
	
	finalbytenumber = record_size * no_of_records + currentbytenumber;
	
	//Check whether read size requested exceeds the length of file

	if(finalbytenumber > fp->length)
		finalbytenumber = fp->length;
	

	//Set final number of bytes to be read
	readdifference = finalbytenumber - currentbytenumber;
	
	count = 0;
	while(count != readdifference) {
		destination[count] = *fp->readpointer;

		//If read pointer is in currentbuffer and buffer is full
		if(fp->readpointer - fp->currentbuffer.buffer == 511) {
			lseek(fp->fd, fp->currentbuffer.pagenumber * 511, SEEK_SET);
			write(fp->fd, fp->currentbuffer.buffer, 512);
			read(fp->fd, fp->currentbuffer.buffer, 512);
			fp->currentbuffer.pagenumber++;
			if(fp->flag == 3 || fp->flag == 4)
				fp->writepointer = fp->currentbuffer.buffer;
			fp->readpointer = fp->currentbuffer.buffer;
		}

		//If buffer is not full
		else {
			fp->readpointer++;
			fp->writepointer++;
		}
		count++;	
	}
		return no_of_records;	//Number of records read
}

int ifwrite(void *source1, int record_size, int no_of_records, FILES *fp) {
	int writedifference = 0, count, finalbytenumber = 0, currentbytenumber = 0, earliercount, bytecount, offset, readdifference;
	char *source = (char *) source1;
	
	//printf("%d\n", fp->currentbuffer.pagenumber);

	if((record_size * no_of_records) <= 0)
		return 0;

	
	if(fp->flag == 1 || fp->flag == 3 || fp->flag == 4) {
		currentbytenumber = fp->currentbuffer.pagenumber * 512 + (int)(fp->writepointer - fp->currentbuffer.buffer);
		finalbytenumber = record_size * no_of_records + currentbytenumber;
		if((currentbytenumber > fp->length)) {
			printf("other one called\n");
			count = 0;
			writedifference = currentbytenumber - fp->length - 1;
			if(fp->currentbuffer.pagenumber > fp->length / 512) {
				lseek(fp->fd, (fp->length / 512) * 512, SEEK_SET);
				read(fp->fd, fp->currentbuffer.buffer, 512);
				fp->currentbuffer.pagenumber = fp->length / 512;
				fp->readpointer = fp->currentbuffer.buffer + fp->length % 512;
			}
			while(count != writedifference) {
				*fp->readpointer = '\0';
				//Currentbuffer is exhausted
				if((fp->readpointer - fp->currentbuffer.buffer) == 511) {
					lseek(fp->fd, fp->currentbuffer.pagenumber * 512, SEEK_SET);
					write(fp->fd, fp->currentbuffer.buffer, 512);
					fp->currentbuffer.pagenumber++;
					fp->readpointer = fp->currentbuffer.buffer;
				}

				//Currentbuffer not yet exhauseted
				else {
					fp->readpointer++;
					fp->length++;
				}
				count++;
			}
		}
		fp->writepointer = fp->readpointer;
	      	count = 0;
	      	writedifference = finalbytenumber - currentbytenumber;
       		while(count != writedifference) {
	       		*(fp->writepointer) = *(source + count);	//byte by byte reading from source memory location
			//lseek sizeof(buffer) bytes before current location to overwrite existing data before moving forwa
			if(fp->writepointer - fp->currentbuffer.buffer == 511) {
				lseek(fp->fd, fp->currentbuffer.pagenumber * 512, SEEK_SET);
				write(fp->fd, fp->currentbuffer.buffer, 512);
				fp->currentbuffer.pagenumber++;
				lseek(fp->fd, fp->currentbuffer.pagenumber * 512, 512);
				read(fp->fd, fp->currentbuffer.buffer, 512);
				fp->writepointer = fp->currentbuffer.buffer;
				fp->readpointer = fp->currentbuffer.buffer;
			}
			
			//If currentbuffer is not exhausted
	       		else {
		        	fp->writepointer++;
			       	fp->readpointer++;
			}
	       		count++;
			if(currentbytenumber >= fp->length)
				fp->length++;
			currentbytenumber++;
       		}
}

	//If file is opened in either of append modes
	//Write pointer is always in alternatebuffer
	if(fp->flag == 2 || fp->flag == 5) {
		finalbytenumber = fp->length + record_size * no_of_records;
		count = 0;
		//Number of bytes to be written
		writedifference = finalbytenumber - fp->length;
		if(fp->currentbuffer.pagenumber != fp->length / 512) {
			lseek(fp->fd, (fp->length / 512) * 512, SEEK_SET);
			read(fp->fd, fp->currentbuffer.buffer, 512);
			fp->writepointer = fp->currentbuffer.buffer + fp->length % 512;
			fp->currentbuffer.pagenumber = fp->length / 512;
		}
		while(count < writedifference) {
			fp->writepointer = fp->currentbuffer.buffer + fp->length % 512;
			*fp->writepointer = source[count];	//Writing to buffer from source memory location

			//If alternate buffer is exhausted
			if((fp->writepointer - fp->currentbuffer.buffer) == 511) {

			       	//Replace required set of bytes in file by alternatebuffer
			       	lseek(fp->fd, fp->currentbuffer.pagenumber * 512, SEEK_SET);
			       	write(fp->fd, fp->currentbuffer.buffer, 512);
	
				//Alternatebuffer is now empty and anything written in it is going to be directly appended to file end
				fp->currentbuffer.pagenumber++;
				fp->writepointer = fp->currentbuffer.buffer;
			}
			else
				fp->writepointer++;
			fp->length++;	//Every write in append mode increases file length
			count++;
		}
	}
	lseek(fp->fd, fp->currentbuffer.pagenumber * 512, SEEK_SET);
	if(fp->currentbuffer.pagenumber == fp->length / 512) {
		write(fp->fd, fp->currentbuffer.buffer, fp->length % 512);
	}
	else {
		write(fp->fd, fp->currentbuffer.buffer, 512);
	}
	fp->readpointer = fp->writepointer;
	return writedifference;
}
	
int ifgetpos(FILES *fp, fPos_t *pos) {
	if(fp->flag != 2 && fp->flag != 5 && fp->currentbuffer.pagenumber * 512 + (fp->writepointer - fp->currentbuffer.buffer) > fp->length)
		pos->position = fp->currentbuffer.pagenumber * 512 + (fp->writepointer - fp->currentbuffer.buffer);
	else
		 pos->position = fp->currentbuffer.pagenumber * 512 + (fp->readpointer - fp->currentbuffer.buffer);
	strcpy(pos->key, "DSA is my favourite subject!");
	return 0;		 
}

int ifsetpos(FILES *fp, fPos_t *pos) {
	int destination;
	destination = pos->position;
	
	if(strcmp("DSA is my favourite subject!", pos->key))
		return 1;
			
	if(fp->flag != 2 && fp->flag != 5) {
		if(!((fp->currentbuffer.pagenumber * 512 <= destination) && ((fp->currentbuffer.pagenumber + 1) * 512) >= destination)) {
			if(destination <= (fp->length - 1)) {
				lseek(fp->fd, fp->currentbuffer.pagenumber * 512, SEEK_SET);
				if(fp->readpointer == fp->writepointer) {
					if(fp->length / 512 == fp->currentbuffer.pagenumber)
						write(fp->fd, fp->currentbuffer.buffer, fp->length % 512);
					else
						write(fp->fd, fp->currentbuffer.buffer, 512);
				}
				lseek(fp->fd, destination / 512, SEEK_SET);
				read(fp->fd, fp->currentbuffer.buffer, 512);
				fp->currentbuffer.pagenumber = destination / 512;
				fp->writepointer = fp->currentbuffer.buffer + destination % 512;
				fp->readpointer = fp->currentbuffer.buffer + destination % 512;
			}
			else {
				lseek(fp->fd, fp->currentbuffer.pagenumber * 512, SEEK_SET);
				if(fp->currentbuffer.pagenumber <= fp->length / 512) {
					if(fp->currentbuffer.pagenumber == fp->length / 512) 
						write(fp->fd, fp->currentbuffer.buffer, fp->length % 512);
					else
						write(fp->fd, fp->currentbuffer.buffer, 512);
				}
				fp->currentbuffer.pagenumber = destination / 512;
				fp->writepointer = fp->currentbuffer.buffer + destination % 512;
			}
		}
		else {
			if(destination > fp->length)
				fp->readpointer = fp->currentbuffer.buffer + fp->length;
			else
				fp->readpointer =  fp->currentbuffer.buffer + destination % 512;
			fp->writepointer = fp->currentbuffer.buffer + destination % 512;
		}
	}
	else {
		if((fp->currentbuffer.pagenumber * 512 <= destination) && (destination < (fp->currentbuffer.pagenumber * 512 + 512))) {
				fp->readpointer = fp->currentbuffer.buffer + destination % 512;
		}
		else if(destination < fp->length) {
			lseek(fp->fd, fp->currentbuffer.pagenumber * 512, SEEK_SET);
			if(fp->length / 512 == fp->currentbuffer.pagenumber)
				write(fp->fd, fp->currentbuffer.buffer, fp->length % 512);
			else 
				write(fp->fd, fp->currentbuffer.buffer, 512);
			lseek(fp->fd, (destination / 512) * 512, SEEK_SET);
			read(fp->fd, fp->currentbuffer.buffer, 512);
			fp->currentbuffer.pagenumber = destination / 512;
			fp->readpointer = fp->currentbuffer.buffer + destination % 512;
		}
		else {
			lseek(fp->fd, fp->currentbuffer.pagenumber * 512, SEEK_SET);
			if(fp->length / 512 == fp->currentbuffer.pagenumber)
				write(fp->fd, fp->currentbuffer.buffer, fp->length % 512);
			else
				write(fp->fd, fp->currentbuffer.buffer, 512);
			fp->currentbuffer.pagenumber = destination / 512;
			fp->readpointer = fp->currentbuffer.buffer + destination % 512;
		}
	}
	destination = fp->currentbuffer.pagenumber * 512 + (fp->readpointer - fp->currentbuffer.buffer);
	return 0;
}

int ifeof(FILES *fp) {
	if(fp->currentbuffer.pagenumber * 512 + (fp->readpointer - fp->currentbuffer.buffer) == fp->length)
		return 1;
	else 
		return 0;
}

long iftell(FILES *fp) {
		if(fp->flag != 2 && fp->flag != 5 && fp->currentbuffer.pagenumber * 512 + (fp->writepointer - fp->currentbuffer.buffer) > fp->length)
			return (long) fp->currentbuffer.pagenumber * 512 + (fp->writepointer - fp->currentbuffer.buffer);
		else
			return (long) fp->currentbuffer.pagenumber * 512 + (fp->readpointer - fp->currentbuffer.buffer);
}

int ifseek(FILES *fp, long offset1, int whence) {
	int destination, offset;
	offset = (int) offset1;
	if(whence == SEEK_CUR)
		destination = fp->currentbuffer.pagenumber * 512 + (fp->readpointer - fp->currentbuffer.buffer) + offset;
	if(whence == SEEK_SET)
		destination = offset;
	if(whence == SEEK_END)
		destination = fp->length + offset - 1;
	if(destination < 0)
		return 1;
	
	if(fp->flag != 2 && fp->flag != 5) {
		if(!((fp->currentbuffer.pagenumber * 512 <= destination) && ((fp->currentbuffer.pagenumber + 1) * 512) >= destination)) {
			if(destination <= (fp->length - 1)) {
				lseek(fp->fd, fp->currentbuffer.pagenumber * 512, SEEK_SET);
				if(fp->readpointer == fp->writepointer) {
					if(fp->length / 512 == fp->currentbuffer.pagenumber)
						write(fp->fd, fp->currentbuffer.buffer, fp->length % 512);
					else
						write(fp->fd, fp->currentbuffer.buffer, 512);
				}
				lseek(fp->fd, destination / 512, SEEK_SET);
				read(fp->fd, fp->currentbuffer.buffer, 512);
				fp->currentbuffer.pagenumber = destination / 512;
				fp->writepointer = fp->currentbuffer.buffer + destination % 512;
				fp->readpointer = fp->currentbuffer.buffer + destination % 512;
			}
			else {
				lseek(fp->fd, fp->currentbuffer.pagenumber * 512, SEEK_SET);
				if(fp->currentbuffer.pagenumber <= fp->length / 512) {
					if(fp->currentbuffer.pagenumber == fp->length / 512) 
						write(fp->fd, fp->currentbuffer.buffer, fp->length % 512);
					else
						write(fp->fd, fp->currentbuffer.buffer, 512);
				}
				fp->currentbuffer.pagenumber = destination / 512;
				fp->writepointer = fp->currentbuffer.buffer + destination % 512;
			}
		}
		else {
			if(destination > fp->length)
				fp->readpointer = fp->currentbuffer.buffer + fp->length;
			else
				fp->readpointer =  fp->currentbuffer.buffer + destination % 512;
			fp->writepointer = fp->currentbuffer.buffer + destination % 512;
		}
	}
	else {
		if((fp->currentbuffer.pagenumber * 512 <= destination) && (destination < (fp->currentbuffer.pagenumber * 512 + 512))) {
				fp->readpointer = fp->currentbuffer.buffer + destination % 512;
		}
		else if(destination < fp->length) {
			lseek(fp->fd, fp->currentbuffer.pagenumber * 512, SEEK_SET);
			if(fp->length / 512 == fp->currentbuffer.pagenumber)
				write(fp->fd, fp->currentbuffer.buffer, fp->length % 512);
			else 
				write(fp->fd, fp->currentbuffer.buffer, 512);
			lseek(fp->fd, (destination / 512) * 512, SEEK_SET);
			read(fp->fd, fp->currentbuffer.buffer, 512);
			fp->currentbuffer.pagenumber = destination / 512;
			fp->readpointer = fp->currentbuffer.buffer + destination % 512;
		}
		else {
			lseek(fp->fd, fp->currentbuffer.pagenumber * 512, SEEK_SET);
			if(fp->length / 512 == fp->currentbuffer.pagenumber)
				write(fp->fd, fp->currentbuffer.buffer, fp->length % 512);
			else
				write(fp->fd, fp->currentbuffer.buffer, 512);
			fp->currentbuffer.pagenumber = destination / 512;
			fp->readpointer = fp->currentbuffer.buffer + destination % 512;
		}
	}
	return 0;
}

int ifclose(FILES *fp) {
	close(fp->fd);
}	
