# FILE-operations-in-c-from-scratch
The header file.h is used in C programming language to perform file I/O operations such as reading from file, writing to file etc<br>
Such similar operations can be performed by using the functions described below.<br>
<br>The user can call the following functions to perform file manipulation using the file pointer type FILES*:<br>
<ul>
  <li>FILES *ifopen(char *pathname, char *mode)</li>
  <li>void ifclose(FILES *fp)</li>
  <li>int ifread(void *mem, int no_of_records, int record_size, FILES *fp)</li>
  <li>int ifwrite(void *mem, int no_of_records, int record_size, FILES *fp)</li>
  <li>int ifgetpos(FILES *fp, fPos_t pos)</li>
  <li>int ifsetpos(FILES *fp, fPos_t pos)</li>
  <li>long int iftell(FILES *fp)</li>
  <li>int ifseek(FILES *fp, long offset, int whence)</li>
  <li>int ifeof(FILES *fp)</li>
</ul>
