#ifndef __NIMPROTOCOL_H_
#define __NIMPROTOCOL_H_

#define UZMI          1
#define KOLIKO        2
#define KOLIKO_R      3
#define BOK           4
#define ODGOVOR       5
#define LOGIN         6

// ovo ispod koriste i klijent i server, pa moze biti tu...
#define OK      1
#define NIJEOK  0

int primiPoruku( int sock, int *vrstaPoruke, char **poruka );
int posaljiPoruku( int sock, int vrstaPoruke, const char *poruka );

#define error1( s ) { printf( s ); printf("budalo1");  exit( 0 ); }
#define error2( s1, s2 ) { printf( s1, s2 ); printf("budalo2");  exit( 0 ); }
#define myperror( s ) { perror( s ); printf("budalo3");  exit( 0 ); }

#endif