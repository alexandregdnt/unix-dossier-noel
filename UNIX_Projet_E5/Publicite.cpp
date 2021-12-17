#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "protocole.h" // contient la cle et la structure d'un message

int idQ, idShm;
int fd;
char* pShm;
MESSAGE message;

int main()
{
  // Armement des signaux

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la file de messages\n",getpid());
  if ((idQ = msgget(CLE, 0)) == -1)
  {
    perror("(PUBLICITE) Erreur de msgget");
    exit(1);
  }

  // Recuperation de l'identifiant de la mémoire partagée
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la mémoire partagée\n",getpid());
  if((idShm = shmget(CLE, 0, 0)) == -1)
  {
    perror("(PUBLICITE) Erreur de shmget");
    exit(1);
  }

  // Attachement à la mémoire partagée
  if((pShm = (char*)shmat(idShm,NULL,0)) == (char*)-1)
  {
    perror("(PUBLICITE) Erreur de shmat");
    exit(1);
  }

  // Ouverture du fichier de publicité
  if((fd = open("publicites.dat", O_RDONLY, 0644)) == -1)
  {
    perror("(PUBLICITE) Erreur d'ouverture du fichier publicites");
    exit(1);
  }

  while(1)
  {
  	PUBLICITE pub;
    // Lecture d'une publicité dans le fichier
    if(read(fd, &pub, sizeof(PUBLICITE)) != sizeof(PUBLICITE))
    {
      lseek(fd, 0, SEEK_SET);
    }

    // Ecriture en mémoire partagée
    strcpy(pShm, pub.texte);

    // Envoi d'une requete UPDATE_PUB au serveur
    message.type = 1;
    message.expediteur = getpid();
    message.requete = UPDATE_PUB;
    if(msgsnd(idQ, &message, sizeof(MESSAGE) - sizeof(long), 0) == -1)
    {
      msgctl(idQ, IPC_RMID, NULL);
      perror("(PUBLICITE) Erreur de msgsnd");
      exit(1);
    }

    sleep(pub.nbSecondes);
  }
}

