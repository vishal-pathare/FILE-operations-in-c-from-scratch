#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "file.h"

char longstring1[600];

void testread() {
	long int i, j;
	FILES *fp;
	fPos_t pos1;
	char str[20], cpy[20], c, longstring[600];
	fp = ifopen("read1.txt", "r");
	printf("File length is : %ld\n", fp->length);
	
	ifgetpos(fp, &pos1);
	
	//Checking read at first position	
	j = ifread(cpy, 1, 5, fp);
	printf("\nExpected output is 5 : %ld\n", j);
	
	//testing the three modes of fseek
  	ifseek(fp, 300, SEEK_SET);
	i = iftell(fp);
	printf("\nExpected output is 300: %ld\n", i);
	ifseek(fp, 500, SEEK_CUR);
	i = iftell(fp);
	printf("\nExpected output is 800: %ld\n", i);
	ifseek(fp, -10, SEEK_END);
	i = iftell(fp);
	printf("\nExpected output is %ld : %ld\n", fp->length - 10, i);
	ifseek(fp, 50, SEEK_CUR);
	i = iftell(fp);
	printf("\nExpected output is %ld : %ld\n", fp->length + 40, i);
	j = ifseek(fp, 10, 123445);
	printf("\nExpected output is -1 : %ld\n", j);
	j = ifseek(fp, -10, SEEK_SET);
	printf("\nExpected output is -1 : %ld\n", j); 
	
	//testing fread in single buffer
	ifseek(fp, 0, SEEK_SET);
	j = ifread(cpy, 1, 10, fp);
	printf("\nExpected output is 10 : %ld\n", j);
	
	//testing fread when multiple buffers are involved
	ifseek(fp, 500, SEEK_SET);
	j = ifread(longstring1, 12, 50, fp);
	i = iftell(fp);
	printf("\nExpected output is 1100, 50 : %ld, %ld\n", i, j);
	
	//testing fread when file terminates before required number of bytes are read
	ifseek(fp, -20, SEEK_END);
	j = ifread(longstring, 10, 3, fp);
	i = iftell(fp);
	printf("\nExpected output is %ld, 2 : %ld, %ld\n", fp->length, i, j);
	
	//fread should return 0 if invalid parameters
	i = (long int)ifread(str, -4, 6, fp);
	printf("\nExpected output is 0 : %ld\n", i); 
	
	//testing the feof condition of file
	ifseek(fp, -50, SEEK_END);
	i = 0;
	while(!ifeof(fp)) {
		ifread(&c, 1, 1, fp);
		i++;
	}
	printf("\nExpected output is 51 : %ld\n", i);
	
	//testing if fread fails if file pointer beyond file length
	ifseek(fp, 20, SEEK_END);
	j = ifread(&c, 1, 1, fp);
	printf("\nExpected output is 0 : %ld\n", j);
	
	//check if write is successful(it should fail)
	j = ifwrite(longstring, 1, 10, fp);
	i = iftell(fp);
	printf("\nExpected output is %ld, 0 : %ld, %ld\n", fp->length + 20, i, j);
	
	//Check if fsetpos sets position as indicated by fgetpos
	ifsetpos(fp, &pos1);
	i = iftell(fp);
	printf("\nExpected output is 0 : %ld", i);
	
	ifclose(fp);
}

void testwrite() {
	long int i, j;
	FILES *fp;
	char c, str[20] = "I am a boy";
	fPos_t pos1;
	
	fp = ifopen("readnew.txt", "w+");
	
	//checking whether file is truncated
	printf("\nLength of file is : %ld\n", fp->length);
	
	//writing in file for first time
	j = ifwrite(str, 1, 10, fp);
	i = iftell(fp);
	printf("\nExpected output is 10, 10 : %ld, %ld\n", i, j);
	
	//checking whether fread works in w mode
	ifseek(fp, -5, SEEK_END);
	i = iftell(fp);
	j = ifread(&c, 1, 1, fp);
	printf("Expected output is %ld, 1 : %ld, %ld\n", fp->length - 5, i, j);
	
	//checking whether writing is successful beyond file length
	ifgetpos(fp, &pos1);
	ifseek(fp, 5, SEEK_END);
	i = 10;
	while(i) {
		ifwrite(&c, 1, 1, fp);
		i--;
	}
	i = iftell(fp);
	printf("\nExpected output is %ld : %ld\n", fp->length, i);
	
	//testing if fwrite works when buffers need to be changed in between writing
	ifseek(fp, 500, SEEK_SET);
	j = ifwrite(longstring1, 1, 600, fp);
	i = iftell(fp);
	printf("\nExpected output is 1100, 600 : %ld, %ld\n", i, j);
	
	ifclose(fp);
}

void testappend() {
	long int i, j;
	FILES *fp;
	char c, str[20];
	
	fp = ifopen("append1.txt", "a");
	
	//checking whether file length is remaining unchanged in append mode
	printf("\nLength of the file is : %ld\n", fp->length);
	i = iftell(fp);
	printf("%ld\n", i);
	
	//checking whether writing is done successfully at end
	j = ifwrite(str, 3, 5, fp);
	i = iftell(fp);
	printf("\nExpected output is %ld, 5 : %ld, %ld\n", fp->length, i, j);
	
	//checking whether writing is done only at the end and also if buffers are to be changed
	ifseek(fp, 1, SEEK_SET);
	i = iftell(fp);
	printf("\nExpected output is  1 : %ld\n", i);
	j = ifwrite(longstring1, 1, 600, fp);
	i = iftell(fp);
	printf("\nExpected output is %ld, 600 : %ld, %ld\n", fp->length, i, j);
	
	
	//testing the modes of fseek in append mode
	ifseek(fp, 300, SEEK_SET);
	i = iftell(fp);
	printf("\nExpected output is 300 : %ld\n", i);
	ifseek(fp, 500, SEEK_CUR);
	i = iftell(fp);
	printf("\nExpected output is 800 : %ld\n", i);
	ifseek(fp, -10, SEEK_END);
	i = iftell(fp);
	printf("\nExpected output is %ld : %ld\n", fp->length - 10, i);
	ifseek(fp, 50, SEEK_CUR);
	i = iftell(fp);
	printf("\nExpected output is %ld : %ld\n", fp->length + 40, i);
	ifseek(fp, 100, SEEK_END);
	i = iftell(fp);
	printf("\nExpected output is %ld : %ld\n", fp->length + 100, i);
	
	//testing if fread fails
	j = ifread(&c, 1, 1, fp);
	printf("\nExpected output is 0 : %ld\n", j);
	ifclose(fp);
}
		
void testreadplus() {
	long int i, j;
	char longstring[600];
	FILES *fp = ifopen("read1.txt", "r+");
	
	//check the original length of file
	printf("\nLength of file is : %ld\n", fp->length);
	
	//check if read works if buffers are to be changed
	ifseek(fp, 500, SEEK_SET);
	j = ifread(longstring, 1, 600, fp);
	i = iftell(fp);
	printf("\nExpected output is 1100, 600 : %ld, %ld\n", i, j);
	
	//check if write works if buffers are to be changed
	ifseek(fp, 1000, SEEK_SET);
	j = ifwrite(longstring, 1, 600, fp);
	i = iftell(fp);
	printf("\nExpected output is 1600, 600 : %ld, %ld\n", i, j);
	
	//check if write works if written beyond current file length
	ifseek(fp, 600, SEEK_END);
	j = ifwrite(longstring, 1, 600, fp);
	i = iftell(fp);
	printf("\nExpected output is %ld, 600 : %ld, %ld\n", fp->length, i, j);
	
	//check if read is performed at file length
	ifseek(fp, 511, SEEK_SET);
	j = ifread(longstring, 1, 12, fp);
	i = iftell(fp);
	printf("\nExpected output is 523, 12 : %ld, %ld\n", i, j);

	ifclose(fp);
}

void testwriteplus() {
	long int i, j;
	char str[20] = "I am a girl";
	FILES *fp = ifopen("readnew.txt", "w+");
	
	//check if file is truncated
	printf("\nLength of file is : %ld\n", fp->length);
	
	//check if fwrite is working over single buffer
	j = ifwrite(str, 1, 10, fp);
	i = iftell(fp);
	printf("\nExpected output is 10, 10 : %ld, %ld\n", i, j);
	
	//check if fwrite is working over multiple buffers
	j = ifwrite(fp, 1, 600, fp);
	i = iftell(fp);
	printf("\nExpected output is 610, 600 : %ld, %ld\n", i, j);
	
	ifclose(fp);
}

void testappendplus() {
	long int i, j;
	char longstring[600], str[20] = "I am a girl", c;
	FILES *fp = ifopen("append1.txt", "a+");

	//check if file length is correct
	printf("\nLength of file is : %ld\n", fp->length);

	//check if reading works successfully in single buffer at any point in file
	ifseek(fp, 50, SEEK_SET);
	j = ifread(str, 1, 10, fp);
	i = iftell(fp);
	printf("\nExpected output is 60, 10 : %ld, %ld\n", i, j);

	//check if reading works successfully in multiple buffers at any point in file
	ifseek(fp, 500, SEEK_SET);
	j = ifread(str, 1, 10, fp);
	i = iftell(fp);
	printf("\nExpected output is 510, 500 : %ld, %ld\n", i, j);

	//check if writing works successfully in single buffer at end
	j = ifwrite(str, 1, 10, fp);
	i = iftell(fp);
	printf("\nExpected output is %ld, 10 : %ld, %ld\n", fp->length, i, j);

	//check if writing works successfully in multiple buffers at end
	j = ifwrite(longstring, 1, 600, fp);
	i = iftell(fp);
	printf("\nExpected output is %ld, 600 : %ld, %ld\n", fp->length, i, j);

	//check if writing works successfully in same buffer
	ifseek(fp, -10 ,SEEK_END);
	j = ifwrite(str, 1, 5, fp);
	i = iftell(fp);
	printf("\nExpected output is %ld, 5 : %ld, %ld\n", fp->length, i, j);

	//check if writing is done at end even if byte location is beyond file length
	ifseek(fp, 50, SEEK_END);
	j = ifwrite(str, 5, 1, fp);
	i = iftell(fp);
	printf("\nExpected output is %ld, 1 : %ld, %ld\n", fp->length, i, j);

	//check if writing is done at end even if byte location is beyond file length and beyond current buffer
	ifseek(fp, 600, SEEK_END);
	j = ifwrite(str, 5, 1, fp);
	i = iftell(fp);
	printf("\nExpected output is %ld, 1 : %ld, %ld\n",fp->length, i, j);
	
	//check if fread fails if done right after fwrite
	j = ifread(str, 1, 10, fp);
	printf("\nExpected output is 0 : %ld", j);

	//check if foef works
	ifseek(fp, -5, SEEK_END);
	i = 0;
	while(!ifeof(fp)) {
		j = ifread(&c, 1, 1, fp);
		i++;
	}
	printf("\nExpected output is 6, 0 : %ld, %ld\n", i, j);

	ifclose(fp);
}

void testmultipleread() {
	FILES *fp1, *fp2;
	long int i, j;
	fp1 = ifopen("read2.txt", "r");
	fp2 = ifopen("read2.txt", "r");
	
	//fseek works independently for both files
	ifseek(fp1, 500, SEEK_SET);
	ifseek(fp2, 1000, SEEK_SET);
	i = iftell(fp1);
	j = iftell(fp2);
	printf("\nExpected output is 500, 1000 : %ld, %ld\n", i, j);
}

void testmultiplewriteplus() {
	FILES *fp1, *fp2;
	long int i, j;
	char str[20] = "I am a boy", c;
	fp1 = ifopen("readnew.txt", "w+");

	//we make changes in buffer of fp1 but do not call fseek so no changes in file
	fp2 = ifopen("readnew.txt", "w+");
	ifwrite(str, 1, 10, fp1);
	i = 0;
	while(!ifeof(fp2)) {
		ifread(&c, 1, 1, fp2);
		i++;
	}
	printf("\nExpected output is 1 : %ld\n", i);

	//if we do call fseek then changes are made in file as well as in buffers of any already open fp's 
	ifseek(fp1, 0, SEEK_SET);
	ifseek(fp2, 0, SEEK_SET);
	i = 0;
	while(!ifeof(fp2)) {
		j = ifread(&c, 1, 1, fp2);
		i++;
	}
	printf("\nExpected output is 11, 0 : %ld, %ld\n", i, j);
	ifclose(fp1);
	ifclose(fp2);
}

void testmultipleappend() {
	FILES *fp1, *fp2;
	long int i;
	char str[20] = "I am a boy";
	fp1 = ifopen("append1.txt", "a");
	fp2 = ifopen("append1.txt", "a");
	
	printf("\nLength of file is : %ld", fp1->length);
	
	//We write 10 bytes in fp1 and 10 bytes in fp2 and show that the latter are written after previously written 10 bytes
	
	ifwrite(str, 1, 10, fp1);
	i = iftell(fp1);
	printf("\nExpected output is %ld : %ld\n", fp1->length, i);
	ifseek(fp1, 0, SEEK_SET);
	ifwrite(str, 1, 10, fp2);
	i = iftell(fp2);
	printf("\nExpected output is %ld : %ld\n", fp2->length, i);
	ifclose(fp1);
	ifclose(fp2);
}

void testmultiplereadplus() {
	FILES *fp1, *fp2;
	long int i;
	char str[20] = "I am a boy", str2[20], str3[20];
	fp1 = ifopen("read1.txt", "r+");

	fp2 = ifopen("read1.txt", "r+");
	//We show that buffer is not replaced in original file and other fp's opening it unless fseek is called
	ifwrite(str, 1, 10, fp1);
	ifread(str2, 1, 10, fp2);
	str2[10] = '\0';
	if(!strcmp(str, str2))
		printf("\nUnsuccessful\n");
	else
		printf("\nSuccessful\n");
	i = iftell(fp2);
	printf("\nExpected output is 10 : %ld\n", i);
	ifseek(fp1, 0, SEEK_SET);
	ifseek(fp2, 0, SEEK_SET);
	ifread(str3, 1, 10, fp2);
	str3[10] = '\0';
	printf("\nExpected output is 'I am a boy' : %s\n", str3);
	i = iftell(fp2);
	printf("\nExpected output is 10 : %ld\n", i);
	ifclose(fp1);
	ifclose(fp2);
}

void testmultipleappendplus() {
	FILES *fp1 ,*fp2;
	fp1 = ifopen("read1.txt", "a+");
	fp2 = ifopen("read1.txt", "a+");
	long int i;
	char str[11] = "I am a boy", c;

	//We check if write in one fp is detected by other fp and vice versa along with write taking place only at end of file
	printf("\nLength of file is %ld", fp1->length);
	
	ifwrite(str, 1, 10, fp1);
	i = 0;
	ifseek(fp2, 0, SEEK_SET);
	while(!ifeof(fp2)) {
		ifread(&c, 1, 1, fp2);
		i++;
	}
	printf("\nExpected output is %ld : %ld\n", fp1->length - 9, i);
	ifseek(fp1, 0, SEEK_CUR);
	ifseek(fp2, 0, SEEK_SET);
	i = 0;
	while(!ifeof(fp2)) {
		ifread(&c, 1, 1, fp2);
		i++;
	}
	printf("\nExpected output is %ld : %ld\n", fp1->length + 1, i);
	ifwrite(str, 1, 10, fp2);
	i = 0;
	while(!ifeof(fp1)) {
		ifread(&c, 1, 1, fp1);
		i++;
	}
	printf("\nExpected output is 1 : %ld\n", i);
	ifseek(fp2, 0, SEEK_SET);
	ifseek(fp1, 0, SEEK_CUR);
	i = 0;
	while(!ifeof(fp1)) {
		ifread(&c, 1, 1, fp1);
		i++;
	}
	printf("\nExpected output is 11 : %ld\n", i);
	ifclose(fp1);
	ifclose(fp2);
}	

void testcombinedmode() {
	//we check whether change in one mode is reflected in file opened in another mode after fseek is called
	FILES *fp1, *fp2;
	long int i;
	char str[20] = "I am a boy", c;
	fp1 = ifopen("append1.txt", "a+");
	fp2 = ifopen("apppend1.txt", "r+");
	printf("\nLength of file is : %ld", fp1->length);
	ifwrite(str, 1, 10, fp1);
	ifseek(fp1, 0, SEEK_CUR);
	i = 0;
	while(!ifeof(fp2)) {
		ifread(&c, 1, 1, fp2);
		i++;
	}
	printf("\nExpected output is %ld : %ld\n", fp2->length + 1, i);
	ifclose(fp1);
	ifclose(fp2);
}
int main(int argc, char *argv[]) {
	
	printf("\n----------r mode----------\n");
	testread();	
	printf("\n----------r mode over----------\n");

	printf("\n----------w mode----------\n");
	testwrite();
	printf("\n----------w mode over ----------\n");

	printf("\n----------a mode ----------\n");
	testappend();
	printf("\n----------a mode over----------\n");

	printf("\n----------r+ mode----------\n");
	testreadplus();
	printf("\n----------r+ mode over----------\n");

	printf("\n----------w+ mode----------\n");
	testwriteplus();
	printf("\n----------w+ mode over----------\n");

	printf("\n----------a+ mode----------\n");
	testappendplus();
	printf("\n----------a+ mode over----------\n");

	printf("\n----------multiple read----------\n");
	testmultipleread();
	printf("\n----------muliple read over----------\n");

	printf("\n----------multiple writeplus----------\n");
	testmultiplewriteplus();
	printf("\n----------multiple writeplus over----------\n");

	printf("\n----------multiple append----------\n");
	testmultipleappend();
	printf("\n----------multiple append over----------\n");

	printf("\n----------multiple readplus----------\n");
	testmultiplereadplus();
	printf("\n----------multiple readplus over----------\n");

	printf("\n----------multiple appendplus----------\n");
	testmultipleappendplus();
	printf("\n----------multiple appendplus over----------\n");

	printf("\n----------combined modes----------\n");
	testcombinedmode();
	printf("\n----------combined modes over----------\n\n");
	
	printf("\n------------------------------------THANK YOU----------------------------\n");
	
	return 0;
}

