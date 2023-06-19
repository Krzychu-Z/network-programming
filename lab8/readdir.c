#include        <dirent.h>
#include        <errno.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include <unistd.h>



void listujPliki( const char * nazwa_sciezki ) {
    struct dirent * plik;
    DIR * sciezka;
   
    if(( sciezka = opendir( nazwa_sciezki ) ) ) {
        while(( plik = readdir( sciezka ) ) )
             puts( plik->d_name );
       
        closedir( sciezka );
    }
    else
         printf( "! wywo³uj¹c funkcjê opendir(%s) pojawi³ siê b³¹d otwarcia strumienia dla danej œcie¿ki, mo¿e nie istnieje, lub podano œcie¿kê pust¹\n", nazwa_sciezki );
   
}

int
main(int argc, char **argv)
{

	if (argc != 2){
		fprintf(stderr, "usage: %s <dir_path> \n", argv[0]);
		return 1;
	}
	chdir("/");
	listujPliki(argv[1]);
    chroot("/tmp");
    listujPliki("/");
    listujPliki("..");
     listujPliki(".");
    //listujPliki("../etc");
}
