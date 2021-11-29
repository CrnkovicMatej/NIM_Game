/*
    nimClient.c
    Author: Matej Crnković
    parametri komandne linije:
        1. korisničko ime
        2. ip-adresa na koeju se spajamo
        3. port na kojeg se spajamo
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "nimProtocol.h"

void obradiLOGIN( int sock, const char *ime );
void obradiUZMI( int sock );
void obradiKOLIKO( int sock );
void obradiBOK( int sock );

int main( int argc, char **argv )
{
	if( argc != 4 )
		error2( "Upotreba: %s ime IP-adresa port\n", argv[0] );
    char mojeIme[9];
	strcpy( mojeIme, argv[1] ); mojeIme[8] = '\0'; // ne dam da ime ima >8 znakova!
	char dekadskiIP[20]; 
    strcpy( dekadskiIP, argv[2] );
	int port;          
    sscanf( argv[3], "%d", &port );

	// socket...
	int mojSocket = socket( PF_INET, SOCK_STREAM, 0 );
	if( mojSocket == -1 )
		myperror( "socket" );
		
	// connect...
	struct sockaddr_in adresaServera;
	
	adresaServera.sin_family = AF_INET;
	adresaServera.sin_port = htons( port );

	if( inet_aton( dekadskiIP, &adresaServera.sin_addr ) == 0 )
		error2( "%s nije dobra adresa!\n", dekadskiIP );
	memset( adresaServera.sin_zero, '\0', 8 );
    
	if( connect( mojSocket,
		        (struct sockaddr *) &adresaServera,
		        sizeof( adresaServera ) ) == -1 )
		myperror( "connect" );

	obradiLOGIN( mojSocket, mojeIme );	
	// ispisi menu
	int gotovo = 0;
	while( !gotovo )
	{
		printf( "\n\nOdaberi opciju...\n"
				"   1. uzimanje sibica s neke od hrpa\n"
				"   2. ispis kolicine sibica na hrpama\n"
				"   3. izlaz iz programa\n"
				"   \n: " );
				
		int opcija;
		scanf( "%d", &opcija );
		
		switch( opcija )
		{
			case 1: obradiUZMI( mojSocket ); break;
			case 2: obradiKOLIKO( mojSocket ); break;
			case 3: obradiBOK( mojSocket ); gotovo = 1; break;
			default: printf( "Pogresna opcija...\n" ); break;
		}
	}

	//close( mojSocket );

	return 0;
}

void obradiUZMI( int sock )
{
	char imeArtikla[50];
	int kolicinaArtikla;

	printf( "S koje hrpe zelis uzeti (upute: prva , druga ili treca): " );
	scanf( "%s", imeArtikla );

	printf( "Koliko koliko sibica zelis uzeti: " );
	scanf( "%d", &kolicinaArtikla );

	char poruka[100];
	sprintf( poruka, "%s %d", imeArtikla, kolicinaArtikla );

	posaljiPoruku( sock, UZMI, poruka );

	int vrstaPoruke; char *odgovor;
	if( primiPoruku( sock, &vrstaPoruke, &odgovor ) != OK )
		error1( "Doslo je do pogreske u komunikaciji sa serverom...\n" );

	if( vrstaPoruke != ODGOVOR )
		error1( "Doslo je do pogreske u komunikaciji sa serverom (nije poslao ODGOVOR)...\n" );

	if( strcmp( odgovor, "OK" ) != 0 )
		printf( "Greska: %s\n", odgovor );
	else
	    printf( "OK\n" );
	    
	free( odgovor );
}

void obradiKOLIKO( int sock )
{
	/* char imeArtikla[50];
	int kolicinaArtikla;

	printf( "Za koju hrpu zelis saznati koliko ima sibica (prvu, drugu ili trecu): " );
	scanf( "%s", imeArtikla ); */

	posaljiPoruku( sock, KOLIKO, "Na hrpama ima" );

	int vrstaPoruke;
	char *odgovor;
    int i = 0;
    for(i = 0 ; i<3 ; i++){
        char garbage[10];
        char garbage2[10];
        char garbage3[10];
        int a_new = 0;
	    if( primiPoruku( sock, &vrstaPoruke, &odgovor ) != OK )
		    error1( "Doslo je do pogreske u komunikaciji sa serverom...\n" );

	    if( vrstaPoruke != ODGOVOR )
		    error1( "Doslo je do pogreske u komunikaciji sa serverom (nije poslao ODGOVOR)...\n" );

	    if( strcmp( odgovor, "OK" ) != 0 )
	    {
		    printf( "Greska: %s\n", odgovor );
		    free( odgovor );
		    return;
	    }
	
	    free( odgovor );
	
	    if( primiPoruku( sock, &vrstaPoruke, &odgovor ) != OK )
		    error1( "Doslo je do pogreske u komunikaciji sa serverom...\n" );

	    if( vrstaPoruke != KOLIKO_R )
		    error1( "Doslo je do pogreske u komunikaciji sa serverom (nije poslao KOLIKO_R)...\n" );
        sscanf( odgovor, "%s %s %s %d", garbage, garbage2, garbage3, &a_new);
        printf( "Ima jos %d sibica na hrpi %d.\n", a_new, i+1);
        
        free( odgovor );
    }
}

void obradiLOGIN( int sock, const char *ime )
{   
	if( posaljiPoruku( sock, LOGIN, ime ) == NIJEOK )
		error1( "Pogreska u LOGIN...izlazim.\n" );
}

void obradiBOK( int sock )
{
	if(posaljiPoruku( sock, BOK, "" ) == NIJEOK)
        myperror("Pogreska u BOK...izlazim.\n");
    
    close(sock);

	/* int vrstaPoruke;
	char *odgovor;
	if( primiPoruku( sock, &vrstaPoruke, &odgovor ) != OK )
		error1( "Doslo je do pogreske u komunikaciji sa serverom...\n" );

	if( vrstaPoruke != ODGOVOR )
		error1( "Doslo je do pogreske u komunikaciji sa serverom (nije poslao ODGOVOR)...\n" );

	if( strcmp( odgovor, "OK" ) != 0 )
		printf( "Greska: %s\n", odgovor );
	else
	    printf( "OK\n" );

    close (sock); */
}