/*
    nimServer.c
    Author: Matej Crnković
    parametri komandne linije:
    1. port na kojeg se spajamo
    2. veličina prve hrpe
    3. veličina druge hrpe
    4. veličina treće hrpe

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

#include <pthread.h>

#include "nimProtocol.h"

int obradiLOGIN( int sock, const char *ime, const char *poruka );
int obradiUZMI( int sock, const char *name,char *poruka );
int obradiKOLIKO( int sock, const char *name, char *poruka );

#define MAXDRETVI 10 // ukupno najviše 2 parova može igrati
#define MAXARTIKALA     3 //koliko hrpa mozemo imati

typedef struct
{
	int commSocket;
	int indexDretve;
} obradiKlijenta__parametar;

typedef struct 
{
    int first_heap;
    int second_heap;
    int third_heap;
    char first_player[10];
    char second_player[10];
    int whosTurn ; //0 za prvog, 1 za drugog
} gameOfNim;

int broj_korisnika = 0;
int broj_trenutnih_igara = 0;
char korisnici[MAXDRETVI][10];
int aktivneDretve[MAXDRETVI] = { 0 };
gameOfNim allGames[MAXDRETVI/2];
obradiKlijenta__parametar parametarDretve[MAXDRETVI]; //cuvamo strukture parametara dretvi
pthread_mutex_t lokot_aktivneDretve = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lokot_filesystem = PTHREAD_MUTEX_INITIALIZER;

int brojArtikala = 3, kolicinaArtikla[MAXARTIKALA];
const char* imeArtikla[3] = {"prva", "druga", "treca"};
int first_heap, second_heap, third_heap;

void krajKomunikacije( void *parametar, const char *ime )
{
	obradiKlijenta__parametar *param = (obradiKlijenta__parametar *) parametar;
	int commSocket  = param->commSocket;
	int indexDretve = param->indexDretve;

	printf( "Kraj komunikacije [dretva=%d, ime=%s]... \n", indexDretve, ime );

	pthread_mutex_lock( &lokot_aktivneDretve );
	aktivneDretve[ indexDretve ] = 2;
	pthread_mutex_unlock( &lokot_aktivneDretve );

	close( commSocket );
}

void *obradiKlijenta( void *parametar )
{

    obradiKlijenta__parametar *param = (obradiKlijenta__parametar *) parametar;
	int commSocket  = param->commSocket;
	int vrstaPoruke;
	char *poruka;

	// prvo trazi login
	if( primiPoruku( commSocket, &vrstaPoruke, &poruka ) != OK )
	{
        
		krajKomunikacije( parametar, "" );
		return NULL;
	}
    
	if( vrstaPoruke != LOGIN || strlen( poruka ) > 8 )
	{
		krajKomunikacije( parametar, "" );
		return NULL;
	}

	char imeKlijenta[9];
	strcpy( imeKlijenta, poruka );
	if( obradiLOGIN( commSocket, imeKlijenta, poruka ) != OK )
	{
		krajKomunikacije( parametar, imeKlijenta );
		return NULL;
	}
    
	free( poruka );

	int gotovo = 0;
	
	while( !gotovo )
	{
		if( primiPoruku( commSocket, &vrstaPoruke, &poruka ) != OK )
		{
			krajKomunikacije( parametar, imeKlijenta );
			gotovo = 1;
			continue;
		}
		
		switch( vrstaPoruke )
		{
			case UZMI: if( obradiUZMI( commSocket, imeKlijenta, poruka ) != OK )
				{
                    krajKomunikacije( parametar, imeKlijenta ); gotovo = 1;
				}
				break;
			case KOLIKO: if( obradiKOLIKO( commSocket, imeKlijenta, poruka ) != OK )
				{
                    krajKomunikacije( parametar, imeKlijenta ); gotovo = 1;
				}
				break; 
			case BOK: krajKomunikacije( parametar, imeKlijenta ); gotovo = 1; break;
			default: krajKomunikacije( parametar, imeKlijenta ); gotovo = 1; break;
		}
		
		free( poruka );
	}

    return NULL;
}

int main( int argc, char **argv )
{
	if( argc != 5 )
		error2( "Upotreba: %s port velicina_prve_hrpe velicina_druge_hrpe velicina_trece_hrpe\n", argv[0] );
		
	int port; sscanf( argv[1], "%d", &port );
    sscanf( argv[2], "%d", &first_heap );
    sscanf( argv[3], "%d", &second_heap );
    sscanf( argv[4], "%d", &third_heap );
    kolicinaArtikla[0] = first_heap;
    kolicinaArtikla[1] = second_heap;
    kolicinaArtikla[2] = third_heap;
    printf("na hrpama %d %d %d\n", first_heap, second_heap, third_heap);
	
	// socket...
    int listenerSocket = socket( PF_INET, SOCK_STREAM, 0 );
	if( listenerSocket == -1 )
		myperror( "socket" );
		
	// bind...
	struct sockaddr_in mojaAdresa;

	mojaAdresa.sin_family = AF_INET;
	mojaAdresa.sin_port = htons( port );
	mojaAdresa.sin_addr.s_addr = INADDR_ANY;
	memset( mojaAdresa.sin_zero, '\0', 8 );
	
	if( bind( listenerSocket,
		      (struct sockaddr *) &mojaAdresa,
		      sizeof( mojaAdresa ) ) == -1 )
		myperror( "bind" );
		
	// listen
	if( listen( listenerSocket, 10 ) == -1 )
		myperror( "listen" );
	//FLAG sta je ovo 4
    pthread_t dretve[10];
	// accept
	while( 1 )
	{
        struct sockaddr_in klijentAdresa;
		unsigned int lenAddr = sizeof( klijentAdresa );

		int commSocket = accept( listenerSocket,
			                     (struct sockaddr *) &klijentAdresa,
			                     &lenAddr );

		if( commSocket == -1 )
			myperror( "accept" );

		char *dekadskiIP = inet_ntoa( klijentAdresa.sin_addr );
		printf( "Prihvatio konekciju od %s [socket=%d]\n", dekadskiIP, commSocket );
		
        pthread_mutex_lock( &lokot_aktivneDretve );
		int i, indexNeaktivne = -1;
		for( i = 0; i < MAXDRETVI; ++i ) //ovo pronalazi zapravo zadnju s kojom se moze raditi
			if( aktivneDretve[i] == 0 )
                {
				indexNeaktivne = i;
                }
			else if( aktivneDretve[i] == 2 )
			{
				pthread_join( dretve[i], NULL );
				aktivneDretve[i] = 0;
				indexNeaktivne = i;
			}

		if( indexNeaktivne == -1 )
		{
			close( commSocket ); // nemam vise dretvi...
			printf( "--> ipak odbijam konekciju jer nemam vise dretvi.\n" );
		}
		else
		{
		    aktivneDretve[indexNeaktivne] = 1;
			parametarDretve[indexNeaktivne].commSocket = commSocket;
			parametarDretve[indexNeaktivne].indexDretve = indexNeaktivne;
			printf( "--> koristim dretvu broj %d.\n", indexNeaktivne);
            int bla = -1;
			bla = pthread_create(
				&dretve[indexNeaktivne], NULL,
				obradiKlijenta, &parametarDretve[indexNeaktivne] );

		}
		pthread_mutex_unlock( &lokot_aktivneDretve );
	}

	return 0;
}


int obradiLOGIN( int sock, const char *ime, const char *poruka )
{
	// postoji li vec korisnik sa imenom ime?
	// samo 1 dretva smije u neko vrijeme prckati po file-ovima!
	pthread_mutex_lock( &lokot_filesystem );
    int i = 0;
    int nasli = 0;
    for(i = 0; i < broj_korisnika; i++){
        if(strcmp(korisnici[i], ime) == 0){
            nasli = 1;
            pthread_mutex_unlock( &lokot_filesystem );
            return NIJEOK;
        }
    }
    if(nasli == 0 && (broj_korisnika >= MAXDRETVI))
        {
            pthread_mutex_unlock( &lokot_filesystem );
            return NIJEOK;
        }
    strcpy(korisnici[broj_korisnika], ime);
    broj_korisnika++;
    //tu cemo popuniti igru
    if(broj_korisnika % 2 == 1)
    {
        int currentGameInd = (broj_korisnika -1)/2;
        allGames[currentGameInd].first_heap = first_heap;
        allGames[currentGameInd].second_heap = second_heap;
        allGames[currentGameInd].third_heap = third_heap;
        strcpy(allGames[currentGameInd].first_player, ime);
        broj_trenutnih_igara ++;
    }
    else
    {
        strcpy(allGames[(broj_korisnika/2)-1].second_player, ime);
    }


    pthread_mutex_unlock( &lokot_filesystem );
	return OK;
    
}

int obradiUZMI( int sock, const char *name,char *poruka )
{
    pthread_mutex_lock( &lokot_filesystem );
	int koliko;
	char hrpa[100];
	
	if( sscanf( poruka, "%s %d\n", hrpa, &koliko ) != 2 || koliko <= 0 )
	{
		posaljiPoruku( sock, ODGOVOR, "Pogresan oblik naredbe UZMI" );
        pthread_mutex_unlock( &lokot_filesystem );
		return NIJEOK;
	}
    int j = 0;
    for (j = 0; j < broj_trenutnih_igara; j++){
        if(strcmp(allGames[j].first_player, name) ==0 ||
            strcmp(allGames[j].second_player, name) ==0 )
            break;
    }
	
	int i, postoji = 0;
	for( i = 0; i < brojArtikala; ++i )
		if( strcmp( imeArtikla[i], hrpa ) == 0 )
        switch(i)
        {
            case 0:{
                if( allGames[j].first_heap < koliko )
                    {
                    posaljiPoruku( sock, ODGOVOR, "Ne mozes uzeti toliko s ove hrpe"  );
                    pthread_mutex_unlock( &lokot_filesystem );
                    return NIJEOK;
                    }
			    else
                    {
                        posaljiPoruku( sock, ODGOVOR, "OK" );
                        allGames[j].first_heap -= koliko;
                    } 
                break;}
            case 1:{
                if( allGames[j].second_heap < koliko )
                    {
                    posaljiPoruku( sock, ODGOVOR, "Ne mozes uzeti toliko s ove hrpe"  );
                    pthread_mutex_unlock( &lokot_filesystem );
                    return NIJEOK;
                    }
			    else
                    {
                        posaljiPoruku( sock, ODGOVOR, "OK" );
                        allGames[j].second_heap -= koliko;
                    } 
                break;}
            case 2:{
                if( allGames[j].third_heap < koliko )
                    {
                    posaljiPoruku( sock, ODGOVOR, "Ne mozes uzeti toliko s ove hrpe"  );
                    pthread_mutex_unlock( &lokot_filesystem );
                    return NIJEOK;
                    }
			    else
                    {
                        posaljiPoruku( sock, ODGOVOR, "OK" );
                        allGames[j].third_heap -= koliko;
                    } 
                break;}
            default : {posaljiPoruku( sock, ODGOVOR, "Ta hrpa ne postoji" );
            pthread_mutex_unlock( &lokot_filesystem );
            return NIJEOK;}
        }
    pthread_mutex_unlock( &lokot_filesystem );
    return OK;
    
}

int obradiKOLIKO( int sock, const char *name, char *poruka )
{
    pthread_mutex_lock( &lokot_filesystem );
    int i = 0;
    for (i = 0; i < broj_trenutnih_igara; i++){
        if(strcmp(allGames[i].first_player,name ) ==0 ||
            strcmp(allGames[i].second_player,name) ==0 )
            break; // sigurno se nekad mora naci
    }
    char novaPoruka[200];
    char *ime = poruka;
    posaljiPoruku( sock, ODGOVOR, "OK" );
    sprintf( novaPoruka, "%s %d",ime, allGames[i].first_heap);
	posaljiPoruku( sock, KOLIKO_R, novaPoruka );
    posaljiPoruku( sock, ODGOVOR, "OK" );
    sprintf( novaPoruka, "%s %d",ime, allGames[i].second_heap);
	posaljiPoruku( sock, KOLIKO_R, novaPoruka );
    posaljiPoruku( sock, ODGOVOR, "OK" );
    sprintf( novaPoruka, "%s %d",ime, allGames[i].third_heap);
	posaljiPoruku( sock, KOLIKO_R, novaPoruka );
    pthread_mutex_unlock( &lokot_filesystem );

    return OK;
	
}